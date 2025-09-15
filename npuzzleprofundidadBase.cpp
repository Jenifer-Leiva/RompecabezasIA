#include <iostream>
#include <vector>
#include <stack>

using namespace std;

//Para los atributos que contiene cada posible estado
class Nodo {
public:
    //Atributos con valores por defecto
    vector<vector<int>> estado = {{1,2,3},{4,5,6},{7,8,0}};;
    vector<int> accion = {0,0};
    vector<int> posicion_hueco = {0,0};
    int profundidad = 0;

    Nodo(vector<vector<int>> est, vector<int> acc, vector<int> pos, int prof) {
        estado = est;
        accion = acc;
        posicion_hueco = pos;
        profundidad = prof;
    }
};

class Rompecabezas_DLS {
public:
    vector<vector<int>> estado_inicial;
    vector<vector<int>> estado_objetivo;
    int n = 0;
    int limite_profundidad = 0;

    Rompecabezas_DLS(vector<vector<int>> est_init, vector<vector<int>> est_obj, int tam, int lim) {
        estado_inicial = est_init;
        estado_objetivo = est_obj;
        n = tam;
        limite_profundidad = lim;
    }

    bool es_objetivo(vector<vector<int>> estado_actual) {
        return estado_actual == estado_objetivo;
    }

    bool es_profundidad(int profundidad_actual) {
        return profundidad_actual == limite_profundidad;
    }
};

void expandir_nodos(Nodo nodo_actual, stack<Nodo>& pila_por_revisar, int n)
{
    //Se define como se puede mover el hueco en filas y columnas 
    //Teniendo en cuenta que no se repitan movimientos, en caso de que se evalue la accion opuesta a la anterior se debe saltar esa iteración

    vector<vector<int>> acciones_posibles = {
        {0, 1},    //derecha
        {0, -1},  //izquierda
        {1, 0},   //abajo
        {-1, 0}  //arriba
    };

    for (int i = 0; i < acciones_posibles.size(); i++) {

        if(acciones_posibles[i][0] == -nodo_actual.accion[0] && acciones_posibles[i][1] == -nodo_actual.accion[1])
            continue;

        vector<int> posicion_hueco_nuevo = {
        nodo_actual.posicion_hueco[0] + acciones_posibles[i][0],
        nodo_actual.posicion_hueco[1] + acciones_posibles[i][1]
        };

        //Se verifica que la nueva posicion sea valida
        if((posicion_hueco_nuevo[0] < n && posicion_hueco_nuevo[0] >= 0) && (posicion_hueco_nuevo[1] < n && posicion_hueco_nuevo[1] >= 0))
        {
            vector<vector<int>> nuevo_estado = nodo_actual.estado;

            //Se hace el intercambio de posicion de hueco
            int auxiliar = nuevo_estado[posicion_hueco_nuevo[0]][posicion_hueco_nuevo[1]];
            nuevo_estado[posicion_hueco_nuevo[0]][posicion_hueco_nuevo[1]] = nuevo_estado[nodo_actual.posicion_hueco[0]] [nodo_actual.posicion_hueco[1]];
            nuevo_estado[nodo_actual.posicion_hueco[0]][nodo_actual.posicion_hueco[1]] = auxiliar;

            //Se define el nuevo nodo a agregar a la pila
            Nodo nuevo_nodo(nuevo_estado, acciones_posibles[i], posicion_hueco_nuevo, nodo_actual.profundidad + 1);

            pila_por_revisar.push(nuevo_nodo);
        }
    }
}

string busqueda_profundidad_limitada(Rompecabezas_DLS rompecabezas,  stack<Nodo>& pila_por_revisar,  stack<Nodo>& pila_visitados)
{

    //Como se va a proceder a agregar los hijos, se elimina el nodo de la pila por revisar y se coloca en los ya visitado
    Nodo nodo_visitado = pila_por_revisar.top();
    pila_visitados.push(nodo_visitado);
    pila_por_revisar.pop();

    //Se verifica si es el estado_objetivo
    if (rompecabezas.es_objetivo(pila_visitados.top().estado))
        return "objetivo";
    
    if(rompecabezas.es_profundidad(pila_visitados.top().profundidad))
        return "limite";
 
    expandir_nodos(pila_visitados.top(), pila_por_revisar, rompecabezas.n);    

    //Mientras la pila por revisar no este vacia
    while (!pila_por_revisar.empty())
    {
        string resultado = busqueda_profundidad_limitada(rompecabezas, pila_por_revisar, pila_visitados);
        if(resultado == "objetivo")
        {
            return "objetivo";
        }else if(resultado == "limite"){//Si llega al limite de profundidad pasa al siguiente nodo en la lista por revisar
            cout << "limite alcanzado rama" << endl;
            continue;
        }
    }

    //Si esta vacia quiere decir que ya no es posible expandir más nodos cumpliendo con el limite
    return "fracaso";
}

//Imprimir con lógica de pila sin modificar la original
void imprimirPila(stack<Nodo> pila) { 
    cout << "Nodos Visitados" << endl;
    while (!pila.empty()) {
        cout << "\nEstado:" << endl;
        for (int i = 0; i < pila.top().estado.size(); i++) {
            for (int j = 0; j < pila.top().estado[i].size(); j++) {
                cout << pila.top().estado[i][j] << " ";
            }
            cout << endl;
        }

        cout << "Accion: ";
        for (int i = 0; i < pila.top().accion.size(); i++) {
            cout << pila.top().accion[i] << " ";
        }
        cout << endl;

        cout <<  "Profundidad: " << pila.top().profundidad << " ";
        pila.pop();
    }
}
int main()
{
    main_busqueda_profundidad_limitada();
}


int main_busqueda_profundidad_limitada()
{
    vector<vector<int>> estado_inicial = {
        {1, 2, 3},
        {4, 5, 6},
        {7, 8, 0}
    };

    vector<vector<int>> estado_objetivo = {
        {1, 2, 3},
        {4, 0, 6},
        {7, 5, 8}
    };

    //Tamaño rompecabezas dado por el usuario
    int n = 3;

    //Limite de profundidad dado por el usuario
    int limite_profundidad = 4;

    Rompecabezas_DLS rompecabezas(estado_inicial,estado_objetivo,n,limite_profundidad);

    //Pilas necesarias para hacer la busqueda
    stack<Nodo> pila_visitados;
    stack<Nodo> pila_por_revisar;

    vector<int> posicion_hueco;

    for (int i = 0; i < estado_inicial.size(); i++) {
        for (int j = 0; j < estado_inicial[i].size(); j++) {
            if (estado_inicial[i][j] == 0) {
                posicion_hueco = {i, j};
            }
        }
    }

    Nodo raiz(estado_inicial, {0,0}, posicion_hueco, 0);
    pila_por_revisar.push(raiz);

    string resultado = busqueda_profundidad_limitada(rompecabezas, pila_por_revisar, pila_visitados);

    if( resultado == "objetivo"){
        imprimirPila(pila_visitados);
    }
    else{
        cout << "No se encuentra solucion dentro del limite de profundidad" << endl;
    }
}






