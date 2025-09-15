#include <TGUI/TGUI.hpp>
#include <TGUI/Backend/SFML-Graphics.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <queue>
#include <unordered_set>
#include <unordered_map>
#include <chrono>
#include <algorithm>
#include <sstream>
#include <iostream>
#include <vector>
#include <string>
using namespace std;

// Función para crear el tablero dinámico
void crearTableroDesdeVector(int n, float startX, float startY,
                  const std::vector<int>& numeros,
                  std::vector<sf::RectangleShape>& tablero,
                  std::vector<tgui::Label::Ptr>& labelsFichas,
                  tgui::Gui& gui)
{
    tablero.clear();
    for (auto& label : labelsFichas)
        gui.remove(label);
    labelsFichas.clear();

    const float cellSize = 500.f / n;
    const float gap = 5.f;

    for (int i = 0; i < n * n; ++i)
    {
        int fila = i / n;
        int col = i % n;

        float x = startX + col * (cellSize + gap);
        float y = startY + fila * (cellSize + gap);

        if (numeros[i] == 0)
            continue; // No dibujar celda para el espacio vacío

        // Crear celda
        sf::RectangleShape celda(sf::Vector2f(cellSize, cellSize));
        celda.setFillColor(sf::Color(153,221,255));
        celda.setOutlineColor(sf::Color(0, 0, 0));
        celda.setOutlineThickness(2);
        celda.setPosition(sf::Vector2f(x, y));

        tablero.push_back(celda);

        // Crear etiqueta del número
        auto labelFicha = tgui::Label::create();
        labelFicha->setText(std::to_string(numeros[i]));
        labelFicha->setPosition(x, y);
        labelFicha->setSize(cellSize, cellSize);
        labelFicha->setHorizontalAlignment(tgui::Label::HorizontalAlignment::Center);
        labelFicha->setVerticalAlignment(tgui::Label::VerticalAlignment::Center);
        labelFicha->setTextSize(32);
        labelFicha->getRenderer()->setTextColor(tgui::Color::Black);
        labelFicha->getRenderer()->setBackgroundColor(tgui::Color::Transparent);

        gui.add(labelFicha);
        labelsFichas.push_back(labelFicha);
    }
}

bool isSolvable(const std::vector<int>& board, int n)
{
    int inversions = 0;
    for (size_t i = 0; i < board.size(); ++i)
    {
        if (board[i] == 0) continue;
        for (size_t j = i + 1; j < board.size(); ++j)
        {
            if (board[j] == 0) continue;
            if (board[i] > board[j])
                ++inversions;
        }
    }

    if (n % 2 == 1)
    {
        // n impar: solvable si número de inversiones es par
        return inversions % 2 == 0;
    }
    else
    {
        // n par: necesitamos saber la fila del 0 desde abajo
        int pos0 = std::find(board.begin(), board.end(), 0) - board.begin();
        int filaDesdeArriba = pos0 / n;
        int filaDesdeAbajo = n - filaDesdeArriba;

        // solvable si (inversions + filaDesdeAbajo) es impar
        return (inversions + filaDesdeAbajo) % 2 == 1;
    }
}




std::vector<int> generarDisposicionAleatoria(int n)
{
    std::vector<int> numeros;
    for (int i = 1; i < n * n; ++i)
        numeros.push_back(i);

    numeros.push_back(0); // Espacio vacío

    do {
        std::random_shuffle(numeros.begin(), numeros.end());
    } while (!isSolvable(numeros, n)); // Repetir hasta que sea solvable

    return numeros;
}

std::string estadoTableroAString(const std::vector<int>& numeros)
{
    std::string estado;
    for (size_t i = 0; i < numeros.size(); ++i)
    {
        estado += std::to_string(numeros[i]);
        if (i != numeros.size() - 1)
            estado += ",";
    }
    return estado;
}

/* ---------------------- BFS (cola FIFO) que devuelve la secuencia de estados ---------------------- */

pair<bool, vector<string>> bfsSolve(const vector<int>& start, int n, size_t maxNodes = 600000, int timeLimitSeconds = 30) 
{
    vector<string> emptyRes;
    if (n > 3) return { false, emptyRes };

    int N = n * n;
    vector<int> goalVec(N);
    for (int i = 0; i < N - 1; ++i) goalVec[i] = i + 1;
    goalVec[N - 1] = 0;

    string goalKey = estadoTableroAString(goalVec);
    string startKey = estadoTableroAString(start);

    if (startKey == goalKey) {
        return { true, vector<string>{startKey} };
    }

    queue<vector<int>> q;
    unordered_set<string> visited;
    unordered_map<string, string> parent;
    unordered_map<string, char> moveTaken;

    q.push(start);
    visited.insert(startKey);
    parent[startKey] = "";

    int dr[4] = { -1, 1, 0, 0 };
    int dc[4] = { 0, 0, -1, 1 };
    char mc[4] = { 'U','D','L','R' };

    size_t nodes = 0;
    auto t0 = chrono::steady_clock::now();

    while (!q.empty()) {
        auto now = chrono::steady_clock::now();
        auto elapsed = chrono::duration_cast<chrono::seconds>(now - t0).count();
        if (elapsed > timeLimitSeconds) return { false, emptyRes };
        if (nodes > maxNodes) return { false, emptyRes };

        vector<int> cur = q.front(); q.pop();
        ++nodes;
        string curKey = estadoTableroAString(cur);

        int zero = -1;
        for (int i = 0; i < N; ++i) if (cur[i] == 0) { zero = i; break; }
        int zr = zero / n;
        int zc = zero % n;

        for (int k = 0; k < 4; ++k) {
            int nr = zr + dr[k];
            int nc = zc + dc[k];
            if (nr < 0 || nr >= n || nc < 0 || nc >= n) continue;

            vector<int> nxt = cur;
            swap(nxt[zero], nxt[nr * n + nc]);
            string nxtKey = estadoTableroAString(nxt);

            if (visited.find(nxtKey) != visited.end()) continue;
            visited.insert(nxtKey);
            parent[nxtKey] = curKey;
            moveTaken[nxtKey] = mc[k];

            if (nxtKey == goalKey) {
                vector<string> path;
                string curk = nxtKey;
                while (!curk.empty()) {
                    path.push_back(curk);
                    curk = parent[curk];
                }
                reverse(path.begin(), path.end());
                return { true, path };
            }

            q.push(nxt);
        }
    }

    return { false, emptyRes };
}









int main()
{

    
    std::vector<std::vector<int>> solucion;
    size_t indiceSolucion = 0;
    bool animando = false;

    std::vector<int> numerosTablero; // Para guardar la disposición actual
    int nTablero = 0; // Tamaño actual del tablero
    sf::RenderWindow window(sf::VideoMode({800, 800}), "Rompecabezas nxn Busqueda por Anchura");
    tgui::Gui gui(window);


    // Contenedor para los componentes GUI iniciales
    auto panel = tgui::Panel::create({"100%", "100%"});
    gui.add(panel);
    panel->getRenderer()->setBackgroundColor(tgui::Color::Transparent);

    
    // Etiqueta
    auto label = tgui::Label::create("Ingrese el tamaño del rompecabezas (n):");
    label->setPosition(50, 50);
    label->setTextSize(20);
    panel->add(label);

    // Campo de entrada
    auto input = tgui::EditBox::create();
    input->setPosition(50, 100);
    input->setSize(100, 30);
    input->setDefaultText("> = 2");
    panel->add(input);

    // Botón
    auto boton = tgui::Button::create("Generar Tablero");
    boton->setPosition(110, 100);
    boton->setSize(150, 30);
    panel->add(boton);

    auto botonMezclar = tgui::Button::create("Orden aleatorio");
    botonMezclar->setPosition(300, 100);
    botonMezclar->setSize(150, 30);
    panel->add(botonMezclar);

    auto botonOrdenar = tgui::Button::create("Ordenar");
    botonOrdenar->setPosition(300, 140); // debajo de "Mezclar Fichas"
    botonOrdenar->setSize(150, 30);
    panel->add(botonOrdenar);

    auto botonResolver = tgui::Button::create("Resolver BFS");
    botonResolver->setPosition(500, 100);
    botonResolver->setSize(150, 30);
    panel->add(botonResolver);

    auto botonSalir = tgui::Button::create("Salir");
    botonSalir->setPosition(510, 20);
    botonSalir->setSize(100, 30);
    panel->add(botonSalir);



    // Variables para el tablero
    std::vector<sf::RectangleShape> tablero;
    std::vector<tgui::Label::Ptr> etiquetas;

    // Acción al hacer clic en el botón
    boton->onClick([&]() {
    std::string texto = static_cast<std::string>(input->getText());

    int n = 0;
    try {
        n = std::stoi(texto);
    } catch (...) {
        return;
    }

    if (n < 2 || n > 10) return;

    nTablero = n;
    numerosTablero.clear();
    for (int i = 1; i < n * n; ++i)
        numerosTablero.push_back(i);
    numerosTablero.push_back(0); // espacio vacío al final

    crearTableroDesdeVector(nTablero, 100, 200, numerosTablero, tablero, etiquetas, gui);
    });

    botonMezclar->onClick([&]() {
        if (nTablero == 0)
            return;

        numerosTablero = generarDisposicionAleatoria(nTablero);
        crearTableroDesdeVector(nTablero, 100, 200, numerosTablero, tablero, etiquetas, gui);

        std::string estado = estadoTableroAString(numerosTablero);
    std::cout << "Estado actual del tablero: " << estado << std::endl;

    });

     botonSalir->onPress([&]{
    window.close();
});

    botonOrdenar->onClick([&]() {
    if (nTablero == 0)
        return;

    numerosTablero.clear();
    for (int i = 1; i < nTablero * nTablero; ++i)
        numerosTablero.push_back(i);
    numerosTablero.push_back(0); // el espacio vacío al final

    crearTableroDesdeVector(nTablero, 100, 200, numerosTablero, tablero, etiquetas, gui);

    std::cout << "Tablero ordenado correctamente." << std::endl;
    });



    botonResolver->onClick([&]() {
    if (nTablero == 0) return;

    auto result = bfsSolve(numerosTablero, nTablero);
    if (!result.first) {
        std::cout << "No se encontró solución (o n > 3)." << std::endl;
        return;
    }
    std::cout << "Solución encontrada en " << result.second.size() - 1 << " movimientos." << std::endl;

     solucion.clear();
    for (auto& key : result.second) {
        std::vector<int> estado;
        std::stringstream ss(key);
        std::string token;
        while (std::getline(ss, token, ',')) {
            estado.push_back(std::stoi(token));
        }
        solucion.push_back(estado);
    }
    indiceSolucion = 0;
    animando = true;




   
    });

    // Bucle principal
    
    while (window.isOpen())
    {

        static sf::Clock relojAnim;
        if (animando && indiceSolucion < solucion.size()) {
            if (relojAnim.getElapsedTime().asMilliseconds() > 300) {
                numerosTablero = solucion[indiceSolucion];
                crearTableroDesdeVector(nTablero, 100, 200, numerosTablero, tablero, etiquetas, gui);
                indiceSolucion++;
                relojAnim.restart();
            }
            if (indiceSolucion >= solucion.size()) {
                animando = false; // terminó la animación
            }
        }

        while (auto event = window.pollEvent())
        {

            gui.handleEvent(*event);

            if (event->is<sf::Event::Closed>())
                window.close();


                // Detectar si se presiona flecha arriba
           
               if (event->is<sf::Event::KeyPressed>())
        {
            // Si no hay tablero aún, ignorar
            if (nTablero == 0)
                continue;

            const auto& keyEvent = event->getIf<sf::Event::KeyPressed>();
            if (!keyEvent) continue;

            // Buscar posición del hueco (0)
            int pos0 = std::find(numerosTablero.begin(), numerosTablero.end(), 0) - numerosTablero.begin();
            int fila = pos0 / nTablero;
            int col  = pos0 % nTablero;

            int nuevaFila = fila;
            int nuevaCol  = col;

            if (keyEvent->code == sf::Keyboard::Key::Up)
                nuevaFila = fila - 1; // mover hueco arriba
            else if (keyEvent->code == sf::Keyboard::Key::Down)
                nuevaFila = fila + 1; // mover hueco abajo
            else if (keyEvent->code == sf::Keyboard::Key::Left)
                nuevaCol = col - 1;   // mover hueco izquierda
            else if (keyEvent->code == sf::Keyboard::Key::Right)
                nuevaCol = col + 1;   // mover hueco derecha

            // Validar que no se salga del tablero
            if (nuevaFila >= 0 && nuevaFila < nTablero && nuevaCol >= 0 && nuevaCol < nTablero)
            {
                int nuevaPos = nuevaFila * nTablero + nuevaCol;

                // Intercambiar el 0 con la ficha
                std::swap(numerosTablero[pos0], numerosTablero[nuevaPos]);

                // Redibujar tablero
                crearTableroDesdeVector(nTablero, 100, 200, numerosTablero, tablero, etiquetas, gui);
            }
        }

 
        }
        window.clear(sf::Color(60, 60, 60));


        


        // Dibujar celdas
        for (auto& celda : tablero)
            window.draw(celda);

        gui.draw();
        window.display();
    }
    return 0;
    
}
