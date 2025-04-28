#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <random>
#include <unistd.h>     // fork
#include <sys/wait.h>   // wait
#include <ctime>

using namespace std;

struct Carta {
    string nombre;
    int valor;
};

vector<Carta> mazo;
vector<int> puntos(5, 0); // Crupier + 4 jugadores

void inicializarMazo() {
    mazo.clear();
    vector<string> palos = {"♠", "♥", "♦", "♣"};
    for (auto palo : palos) {
        for (int i = 2; i <= 10; ++i) {
            mazo.push_back({to_string(i) + palo, i});
        }
        mazo.push_back({"J" + palo, 10});
        mazo.push_back({"Q" + palo, 10});
        mazo.push_back({"K" + palo, 10});
        mazo.push_back({"A" + palo, 11});
    }
    shuffle(mazo.begin(), mazo.end(), default_random_engine(time(0)));
}

Carta robarCarta() {
    Carta c = mazo.back();
    mazo.pop_back();
    return c;
}

int valorMano(vector<Carta>& mano) {
    int total = 0;
    int ases = 0;
    for (auto& carta : mano) {
        total += carta.valor;
        if (carta.nombre[0] == 'A') ases++;
    }
    while (total > 21 && ases > 0) {
        total -= 10;
        ases--;
    }
    return total;
}

bool turnoJugador(vector<Carta>& mano, int jugador_id) {
    while (true) {
        int total = valorMano(mano);
        if (total > 21) {
            cout << "Jugador " << jugador_id << " se pasa con " << total << " puntos.\n";
            return false;
        }
        if (total <= 11) {
            mano.push_back(robarCarta());
            cout << "Jugador " << jugador_id << " pide carta: " << mano.back().nombre << endl;
        } else if (total <= 18) {
            if (rand() % 2 == 0) {
                mano.push_back(robarCarta());
                cout << "Jugador " << jugador_id << " pide carta: " << mano.back().nombre << endl;
            } else {
                cout << "Jugador " << jugador_id << " se planta con " << total << " puntos.\n";
                return true;
            }
        } else {
            cout << "Jugador " << jugador_id << " se planta con " << total << " puntos.\n";
            return true;
        }
    }
}

bool turnoCrupier(vector<Carta>& mano) {
    while (true) {
        int total = valorMano(mano);
        cout << "Crupier tiene " << total << " puntos.\n";

        if (total == 21) {
            cout << "Crupier alcanza 21 puntos! Se planta automáticamente.\n";
            return true;
        }
        if (total > 21) {
            cout << "El Crupier se pasa de 21!\n";
            return false;
        }

        cout << "¿Desea pedir otra carta? (s/n): ";
        char opc;
        cin >> opc;

        if (opc == 's') {
            mano.push_back(robarCarta());
            cout << "Crupier roba: " << mano.back().nombre << endl;
        } else {
            cout << "Crupier se planta.\n";
            return true;
        }
    }
}

bool esBlackjackNatural(vector<Carta>& mano) {
    return (mano.size() == 2 && valorMano(mano) == 21);
}

int main() {
    srand(time(0));
    int rondas;
    cout << "Ingrese cantidad de rondas: ";
    cin >> rondas;

    cin.ignore(); // Limpia buffer después del primer cin

    for (int r = 1; r <= rondas; ++r) {
        cout << "\n=== Ronda " << r << " ===\n";
        inicializarMazo();

        vector<vector<Carta>> manos(5);
        vector<bool> blackjackNatural(5, false);

        for (int i = 0; i < 5; ++i) {
            manos[i].push_back(robarCarta());
            manos[i].push_back(robarCarta());
            if (esBlackjackNatural(manos[i])) {
                blackjackNatural[i] = true;
            }
        }

        cout << "Carta visible del crupier: " << manos[0][0].nombre << endl;

        if (blackjackNatural[0]) {
            cout << "El Crupier tiene un Blackjack Natural!\n";
            for (int i = 1; i <= 4; ++i) {
                if (blackjackNatural[i]) {
                    cout << "Jugador " << i << " también tiene Blackjack. Empate (push).\n";
                } else {
                    cout << "Jugador " << i << " pierde.\n";
                }
            }
            puntos[0] += 2;
            cout << "\nPresiona Enter para continuar a la siguiente ronda...";
            cin.get();
            continue;
        }

        for (int i = 1; i <= 4; ++i) {
            if (blackjackNatural[i]) {
                cout << "Jugador " << i << " tiene un Blackjack Natural!\n";
            }
        }

        for (int i = 1; i <= 4; ++i) {
            if (!blackjackNatural[i]) {
                pid_t pid = fork();
                if (pid == 0) {
                    turnoJugador(manos[i], i);
                    exit(valorMano(manos[i]));
                }
            }
        }

        for (int i = 1; i <= 4; ++i) {
            if (!blackjackNatural[i]) {
                int status;
                wait(&status);
                int valor = WEXITSTATUS(status);
                if (valor > 21) valor = 0;
                manos[i].clear();
                manos[i].push_back({"", valor});
            }
        }

        cout << "Turno del Crupier\n";
        bool crupierNoSePaso = turnoCrupier(manos[0]);
        int valor_crupier = valorMano(manos[0]);
        if (!crupierNoSePaso) {
            valor_crupier = 0;
        }

        cout << "Crupier termina con " << valor_crupier << " puntos.\n";

        int ganadores = 0;
        for (int i = 1; i <= 4; ++i) {
            int valor_jugador = (blackjackNatural[i]) ? 21 : manos[i][0].valor;

            if (blackjackNatural[i]) {
                if (valor_crupier == 21 && manos[0].size() == 2) {
                    cout << "Jugador " << i << " empata (ambos tienen Blackjack).\n";
                } else {
                    cout << "Jugador " << i << " gana con Blackjack.\n";
                    puntos[i]++;
                    ganadores++;
                }
            } else if ((valor_jugador > valor_crupier && valor_jugador <= 21) || (valor_crupier == 0 && valor_jugador <= 21)) {
                cout << "Jugador " << i << " gana la ronda.\n";
                puntos[i]++;
                ganadores++;
            } else if (valor_jugador == valor_crupier && valor_jugador != 0) {
                cout << "Jugador " << i << " empata con el Crupier (push).\n";
            } else {
                cout << "Jugador " << i << " pierde.\n";
            }
        }

        if (ganadores == 0) {
            puntos[0] += 2;
            cout << "El Crupier gana todos los duelos. +2 puntos.\n";
        } else if (ganadores <= 2) {
            puntos[0]++;
            cout << "El Crupier gana la mayoría. +1 punto.\n";
        } else {
            cout << "El Crupier no gana puntos esta ronda.\n";
        }

        // Esperar Enter antes de la siguiente ronda
        cout << "\nPresiona Enter para continuar a la siguiente ronda...";
        cin.get();
    }

    cout << "\n=== Resultados Finales ===\n";
    for (int i = 0; i <= 4; ++i) {
        if (i == 0)
            cout << "Crupier: " << puntos[i] << " puntos\n";
        else
            cout << "Jugador " << i << ": " << puntos[i] << " puntos\n";
    }

    int max_puntos = *max_element(puntos.begin(), puntos.end());
    cout << "Ganadores: ";
    for (int i = 0; i <= 4; ++i) {
        if (puntos[i] == max_puntos) {
            if (i == 0)
                cout << "Crupier ";
            else
                cout << "Jugador" << i << " ";
        }
    }
    cout << endl;

    return 0;
}
