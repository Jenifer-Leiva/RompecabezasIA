/*
  sliding_puzzle_vscode.cpp
  Programa de consola en C++ para Windows (pensado para ejecutarse en Visual Studio Code).
  - Pide tamaño n por teclado.
  - Genera tablero aleatorio solvable n x n.
  - Muestra tablero bonito en "cajitas".
  - Menú:
      1) Resolver con BFS (cola FIFO) -> muestra tableros intermedios (solo para n=2 ó n=3).
      2) Jugar manualmente -> mover hueco con flechas; 'S' para salir al prompt de tamaño.
  Comentarios en español, paso a paso.
*/

#include <iostream>
#include <vector>
#include <queue>
#include <unordered_set>
#include <unordered_map>
#include <map>
#include <algorithm>
#include <random>
#include <chrono>
#include <thread>
#include <sstream>
#include <iomanip>
#include <conio.h>    // _getch en Windows
#include <windows.h>  // para colores y Sleep
using namespace std;

/* ---------------------- Utilidades de consola (colores) ---------------------- */

// Cambia el color de texto en la consola de Windows.
// Puedes pasar atributos como 7 (gris claro/default), 9 (azul), 10 (verde), 11 (cian), 12 (rojo), 13 (magenta), 14 (amarillo)
void setColor(int attr) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, (WORD)attr);
}

/* ---------------------- Funciones para representar y mostrar el tablero ---------------------- */

/*
  Imprime el tablero (representado en vector<int> de longitud n*n) de forma
  alineada en "cajitas" para que sea más legible.
  - 0 representa el hueco.
  - n es la dimensión (anchura/alto).
*/
void printBoard(const vector<int>& board, int n) {
    //system("cls"); // limpia la pantalla en Windows

    int N = n * n;
    // calcular ancho para cada número (número más grande = N-1)
    int maxNum = N - 1;
    int width = to_string(maxNum).size();

    cout << "\n";
    for (int r = 0; r < n; ++r) {
        cout << "   "; // margen izquierdo
        for (int c = 0; c < n; ++c) {
            int val = board[r * n + c];
            // Elegimos un color simple por tile (solo foreground, para evitar problemas)
            if (val == 0) {
                setColor(8); // gris oscuro para el hueco
            }
            else {
                // paleta simple (repetitiva)
                int pal[7] = { 9, 10, 11, 12, 13, 14, 15 };
                int idx = val % 7;
                setColor(pal[idx]);
            }

            // imprimir la "cajita" con espacios para alinear según width
            cout << "[";
            if (val == 0) {
                for (int k = 0; k < width; ++k) cout << " ";
            }
            else {
                // centrar el número dentro del ancho
                string s = to_string(val);
                int leftPad = (width - (int)s.size()) / 2;
                int rightPad = width - (int)s.size() - leftPad;
                for (int k = 0; k < leftPad; ++k) cout << " ";
                cout << s;
                for (int k = 0; k < rightPad; ++k) cout << " ";
            }
            cout << "] ";

            setColor(7); // reset color
        }
        cout << "\n\n";
    }
    cout << flush;
}

/* ---------------------- Utilidades de estado y solubilidad ---------------------- */

/*
  Convierte el tablero (vector<int>) a string (clave) para usar en visited/parent.
  Formato: números separados por comas, ej: "1,2,3,0,4"
*/
string boardToKey(const vector<int>& b) {
    ostringstream oss;
    for (size_t i = 0; i < b.size(); ++i) {
        if (i) oss << ',';
        oss << b[i];
    }
    return oss.str();
}

/*
  Convierte clave a tablero (vector<int>).
*/
vector<int> keyToBoard(const string& key) {
    vector<int> b;
    stringstream ss(key);
    string token;
    while (getline(ss, token, ',')) {
        b.push_back(stoi(token));
    }
    return b;
}

/*
  Cuenta inversions para determinar solvabilidad.
  Regla:
   - Si n es impar: solvable si inversions es par.
   - Si n es par: solvable si (inversions + fila_del_hueco_desde_abajo) % 2 == 1
     (fila desde abajo: 1 = última fila, 2 = penúltima, ...).
*/
bool isSolvable(const vector<int>& board, int n) {
    int N = n * n;
    int inv = 0;
    for (int i = 0; i < N; ++i) {
        if (board[i] == 0) continue;
        for (int j = i + 1; j < N; ++j) {
            if (board[j] != 0 && board[j] < board[i]) ++inv;
        }
    }
    if (n % 2 == 1) {
        return (inv % 2 == 0);
    }
    else {
        int zeroIndex = -1;
        for (int i = 0; i < N; ++i) if (board[i] == 0) { zeroIndex = i; break; }
        int rowFromTop = zeroIndex / n;         // 0-based
        int rowFromBottom = n - rowFromTop;     // 1-based
        // condición conocida para tablero de anchura par
        return ((inv + rowFromBottom) % 2 == 1);
    }
}

/*
  Genera un tablero aleatorio pero **solvable** (y distinto del objetivo).
  - construye 0..N-1, lo baraja hasta que isSolvable sea true y no sea el objetivo.
*/
vector<int> generateSolvableBoard(int n) {
    int N = n * n;
    vector<int> board(N);
    for (int i = 0; i < N; ++i) board[i] = i;

    // objetivo para comparar: 1,2,3,...,N-1,0
    vector<int> goal(N);
    for (int i = 0; i < N - 1; ++i) goal[i] = i + 1;
    goal[N - 1] = 0;

    // generador aleatorio
    std::mt19937 rng((unsigned)chrono::high_resolution_clock::now().time_since_epoch().count());

    do {
        shuffle(board.begin(), board.end(), rng);
    } while (!isSolvable(board, n) || board == goal); // evitar estado resuelto inicial

    return board;
}

/* ---------------------- Aplicación de movimientos ---------------------- */

/*
  Aplica un movimiento al estado:
   'U' = mover hueco hacia arriba (intercambia con la casilla de arriba)
   'D' = hacia abajo
   'L' = izquierda
   'R' = derecha
  Devuelve true si el movimiento fue válido y aplicado.
*/
bool applyMove(vector<int>& state, int n, char move) {
    int N = n * n;
    int zero = -1;
    for (int i = 0; i < N; ++i) if (state[i] == 0) { zero = i; break; }
    int zr = zero / n, zc = zero % n;
    int nr = zr, nc = zc;
    if (move == 'U') nr = zr - 1;
    else if (move == 'D') nr = zr + 1;
    else if (move == 'L') nc = zc - 1;
    else if (move == 'R') nc = zc + 1;
    else return false;

    if (nr < 0 || nr >= n || nc < 0 || nc >= n) return false;
    swap(state[zero], state[nr * n + nc]);
    return true;
}

/* ---------------------- BFS (cola FIFO) que devuelve la secuencia de estados ---------------------- */

/*
  bfsSolve:
    - start: estado inicial (vector<int> tamaño n*n)
    - n: dimensión
    - maxNodes: límite de nodos explorados (seguridad)
    - timeLimitSeconds: límite de tiempo en segundos (seguridad)
  Devuelve: pair<bool, vector<string>>
    - first = true si se encontró solución
    - second = secuencia de claves (keys) desde inicio hasta objetivo (incluyendo ambos)
*/


// ❤️ ❤️ ❤️ APLICAMOS BFS CON COLA 

/*
  bfsSolve:
    - start: tablero inicial (vector<int> de tamaño n*n)
    - n: dimensión del puzzle
    - maxNodes: límite de nodos explorados (seguridad)
    - timeLimitSeconds: límite de tiempo en segundos (seguridad)
  Devuelve: pair<bool, vector<string>>
    - first = true si se encontró solución
    - second = vector de strings con la secuencia de tableros desde inicio hasta objetivo
*/

// nuestra funcion bfsSolve devuelve 2 cosas
// un bool para si encontro o no la solucion
// un vector lista de strings que representa la secuencia de tableros desde el estado inicial hasta el objetivo.
// Cada string se obtiene con la función boardToKey, que convierte un tablero en un string del tipo "1,2,3,0,4".
// Así podemos reconstruir todos los pasos que BFS encontró para llegar a la solución.
pair<bool, vector<string>> bfsSolve(const vector<int>& start, int n, size_t maxNodes = 600000, int timeLimitSeconds = 30) 
{
    vector<string> emptyRes; // resultado vacío para devolver si no hay solución

    // limitar BFS a n <= 3 por practicidad (2x2 y 3x3)
    if (n > 3) return { false, emptyRes };

    int N = n * n;

    // ---------------- Construir tablero objetivo ----------------
    vector<int> goalVec(N);
    for (int i = 0; i < N - 1; ++i) goalVec[i] = i + 1; // números 1..N-1
    goalVec[N - 1] = 0; // hueco al final
    string goalKey = boardToKey(goalVec); // clave string del objetivo
    string startKey = boardToKey(start);  // clave string del tablero inicial

    if (startKey == goalKey) { // si ya está resuelto
        return { true, vector<string>{startKey} };
    }

    // ---------------- Inicializar estructuras BFS ----------------
    queue<vector<int>> q;            // CREACION de cola FIFO de tableros por explorar
    unordered_set<string> visited;   // conjunto de tableros ya visitados
    unordered_map<string, string> parent;   // mapa de padres para reconstruir camino
    unordered_map<string, char> moveTaken; // movimiento que generó cada tablero

    q.push(start);         // agregar tablero inicial a la cola
    visited.insert(startKey); // marcar como visitado
    parent[startKey] = "";    // raíz no tiene padre

    // ---------------- Definir movimientos posibles ----------------
    int dr[4] = { -1, 1, 0, 0 }; // cambio de fila para U,D,L,R
    int dc[4] = { 0, 0, -1, 1 }; // cambio de columna para U,D,L,R
    char mc[4] = { 'U','D','L','R' }; // representación del movimiento

    size_t nodes = 0;  // contador de nodos explorados
    auto t0 = chrono::steady_clock::now(); // tiempo inicial

    // ---------------- Bucle principal BFS ----------------
    while (!q.empty()) {
        // ---------- Límites de seguridad ----------
        auto now = chrono::steady_clock::now();
        auto elapsed = chrono::duration_cast<chrono::seconds>(now - t0).count();
        if (elapsed > timeLimitSeconds) return { false, emptyRes }; // timeout
        if (nodes > maxNodes) return { false, emptyRes };           // límite de nodos

        // ---------- Tomar tablero actual ----------
        vector<int> cur = q.front(); q.pop(); // sacar de la cola
        ++nodes;                               // contar nodo explorado
        string curKey = boardToKey(cur);       // clave string

        // ---------- Ubicar el hueco ----------
        int zero = -1;
        for (int i = 0; i < N; ++i) if (cur[i] == 0) { zero = i; break; }
        int zr = zero / n;  // fila del hueco
        int zc = zero % n;  // columna del hueco

        // ---------- Generar tableros hijos ----------
        for (int k = 0; k < 4; ++k) {
            int nr = zr + dr[k];  // nueva fila tras movimiento
            int nc = zc + dc[k];  // nueva columna tras movimiento
            if (nr < 0 || nr >= n || nc < 0 || nc >= n) continue; // fuera del tablero

            vector<int> nxt = cur;
            swap(nxt[zero], nxt[nr * n + nc]); // mover hueco
            string nxtKey = boardToKey(nxt); // clave del tablero hijo

            // ---------- Revisar si ya fue visitado ----------
            if (visited.find(nxtKey) != visited.end()) continue; // ya visto
            visited.insert(nxtKey);      // marcar visitado
            parent[nxtKey] = curKey;     // guardar padre
            moveTaken[nxtKey] = mc[k];   // guardar movimiento

            // ---------- Comprobar si llegamos al objetivo ----------
            if (nxtKey == goalKey) {
                vector<string> path;
                string curk = nxtKey;
                while (!curk.empty()) {   // reconstruir camino desde objetivo hasta inicio
                    path.push_back(curk);
                    curk = parent[curk];
                }
                reverse(path.begin(), path.end()); // ordenar de inicio a objetivo
                return { true, path };              // devolver solución
            }

            // ---------- Agregar tablero hijo a la cola ----------
            q.push(nxt); // se explorará más adelante
        }
    }

    // si la cola se vacía y no encontramos solución
    return { false, emptyRes };
}


/* ---------------------- Interacción: modo jugar (flechas) ---------------------- */

/*
  Modo interactivo:
   - state: tablero inicial (vector<int>).
   - n: dimensión.
   - Lee flechas con _getch(). Si el usuario pulsa 'S' o 's', sale y retorna al prompt.
*/
void playMode(vector<int> state, int n) {
    while (true) {
        printBoard(state, n);
        cout << "Modo jugar: usa flechas para mover el hueco. Presiona 'S' para salir.\n";

        int ch = _getch();
        if (ch == 224) { // teclas especiales en Windows
            int ch2 = _getch();
            if (ch2 == 72) applyMove(state, n, 'U'); // ↑
            else if (ch2 == 80) applyMove(state, n, 'D'); // ↓
            else if (ch2 == 75) applyMove(state, n, 'L'); // ←
            else if (ch2 == 77) applyMove(state, n, 'R'); // →
        }
        else {
            // tecla normal
            if (ch == 'S' || ch == 's') {
                cout << "S detectada. Saliendo al prompt de tamaño...\n";
                this_thread::sleep_for(chrono::milliseconds(400));
                return;
            }
        }

        // comprobar si ya se resolvió (opcional)
        bool solved = true;
        for (int i = 0; i < n * n - 1; ++i) if (state[i] != i + 1) { solved = false; break; }
        if (solved && state[n * n - 1] == 0) {
            printBoard(state, n);
            cout << "¡Felicidades! Has resuelto el puzzle. Volviendo al prompt de tamaño...\n";
            this_thread::sleep_for(chrono::milliseconds(8000));
            return;
        }
    }
}

/* ---------------------- Programa principal ---------------------- */

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    while (true) {
        system("cls");
        cout << "========================================\n";
        cout << "   SLIDING TILES - Puzzle deslizante\n";
        cout << "========================================\n\n";
        cout << "Ingrese el tamano del puzzle (n): (ej: 2 para 2x2, 3 para 3x3, 0 para salir): ";
        int n;
        if (!(cin >> n)) {
            cin.clear();
            string junk; getline(cin, junk);
            cout << "Entrada invalida. Intenta de nuevo.\n";
            continue;
        }
        if (n == 0) break;
        if (n < 2) {
            cout << "n debe ser >= 2. Intenta de nuevo.\n";
            continue;
        }

        // Generar tablero aleatorio y solvable
        cout << "Generando tablero aleatorio (puede tardar un instante)...\n";
        vector<int> board = generateSolvableBoard(n);
        printBoard(board, n);

        // Mostrar menu de opciones
        cout << "Menu:\n";
        cout << "  1) Resolver con Busqueda en Anchura (BFS)  (usa cola FIFO)\n";
        cout << "  2) Jugar manualmente (flechas). Presiona 'S' para salir.\n";
        cout << "Elige opcion (1 o 2): ";
        int opcion;
        if (!(cin >> opcion)) {
            cin.clear();
            string junk; getline(cin, junk);
            cout << "Entrada invalida. Volviendo al prompt de tamaño.\n";
            this_thread::sleep_for(chrono::milliseconds(800));
            continue;
        }

        if (opcion == 2) {
            // limpiar buffer antes de entrar a modo interactivo
            cin.clear();
            string junk; getline(cin, junk);
            playMode(board, n);
            // cuando playMode termina, volvemos a preguntar tamaño (segun tu requerimiento)
            continue;
        }

        if (opcion == 1) {
            // Intentar resolver con BFS (solo si n <= 3)
            if (n > 3) {
                cout << "\n\nAtencion: BFS solo se ejecuta para n = 2 o n = 3 (puzzles mayores son impracticables con BFS).\n";
                cout << "Esto es porque Busqueda primero en anchura lee absolutamente todos los nodos, como leer un texto .\n";
                cout << "Explorando TODOS los estados posibles desde el tablero inicial hasta el objetivo...\n";
                cout << "Osea explora cuantos tableros diferentes se pueden generar y encuentra cuales de ellos al ´unirlos´ serian la ruta mas corta...\n";

                cout << "ALMACENA todos esos posibles estado en la estructura cola: unordered_set.\n\n";
                cout << "El hecho es que...\n";
                cout << "Para un puzzle de 2*2 ---> tenemos 4 casillas  --> osea  4! =       24 posibles estados...\n";
                cout << "Para un puzzle de 3*3 ---> tenemos 9 casillas  --> osea  9! =  362,880 posibles estados...\n";
                cout << "Para un puzzle de 4*4 ---> tenemos 16 casillas --> osea 16! = 20*10^13 posibles estados...\n\n";
                cout << "Si para 3*3 requerimos una memoria alta, para 4*4 requeririamos una super computadora y ademas a\244os de cpu...\n";
                cout << "Este metodo al recorrer absolutamente todos los estados posibles, requiere memoria+tiempo\n";
                cout << "Lo cual lo hace impracticable para n*n, solo aplica para 2*2 y 3*3 \n";

                cout << "\nSi deseas, puedes jugar manualmente (opcion 2) o probar otro algoritmo (A*).\n";
                cout << "Presiona una tecla para volver al menu de tamanos...\n";
                _getch();
                continue;
            }

            cout << "\nIniciando BFS (cola FIFO). Esto puede tardar algunos segundos para 3x3...\n";
            auto result = bfsSolve(board, n, 600000, 30); // limites: nodos, tiempo
            if (!result.first) {
                cout << "BFS no encontro solucion dentro de los limites establecidos (o se produjo timeout).\n";
                cout << "Intenta volver a generar tablero o usa n=2 para ver un ejemplo rapido.\n";
                cout << "Presiona una tecla para continuar...\n";
                _getch();
                continue;
            }

            // result.second contiene la secuencia de claves desde inicio hasta objetivo
            vector<string> pathKeys = result.second;
            cout << "Solucion encontrada en " << (pathKeys.size() - 1) << " movimientos. Mostrando pasos...\n";
            this_thread::sleep_for(chrono::milliseconds(600));
            int stepglobal = 0;
            // Para mostrar movimientos legibles, reconstruimos la secuencia de movimientos comparando estados consecutivos.
            for (size_t step = 0; step < pathKeys.size(); ++step) {
                vector<int> st = keyToBoard(pathKeys[step]);
                
                if (step == 0) {
                    cout << "Estado inicial\n";
                }
                else {
                    // determinar movimiento comparando st con prev
                    vector<int> prev = keyToBoard(pathKeys[step - 1]);
                    // encontrar posicion del 0 en prev y en st
                    int zPrev = -1, zCur = -1;
                    for (int i = 0;i < n * n;i++) {
                        if (prev[i] == 0) zPrev = i;
                        if (st[i] == 0) zCur = i;
                    }
                    int rPrev = zPrev / n, cPrev = zPrev % n;
                    int rCur = zCur / n, cCur = zCur % n;
                    string mv;
                    if (rCur == rPrev - 1 && cCur == cPrev) mv = "ARRIBA";
                    else if (rCur == rPrev + 1 && cCur == cPrev) mv = "ABAJO";
                    else if (rCur == rPrev && cCur == cPrev - 1) mv = "IZQUIERDA";
                    else if (rCur == rPrev && cCur == cPrev + 1) mv = "DERECHA";
                    else mv = "DESCONOCIDO";
                    cout << "Paso " << step << " -> Mover hueco: " << mv << "\n";
                    stepglobal = step;
                }
                printBoard(st, n);
                // pequeña pausa para ver cada paso (ajusta ms si quieres más lento/rápido)
                //Sleep(500); // 500 ms (Windows)
            }

            cout << "Fin de la solucion  \n";
            cout << "Se hicieron :  "<< stepglobal <<" pasos, para llegar a la solucion \n\n";
            cout << "Presiona una tecla para volver al prompt de tamanos...  \n";
            _getch();
            continue;
        }

        // si llega aquí, opción inválida
        cout << "Opcion invalida. Volviendo al prompt de tamanos...\n";
        this_thread::sleep_for(chrono::milliseconds(600));
    } // fin while principal

    cout << "Gracias por usar el programa. ¡Hasta luego!\n";
    return 0;
}
