#include <vector>
#include <iostream>
#include <string>
#include <cstring>
#include <sstream>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <syslog.h>
#include "shell.h"
#include <fstream>
#include <chrono>
#include <thread>

using namespace std;


vector<char*> stringVectorToCharVector(const vector<string>& strings) {
    vector<char*> charVector;
    charVector.reserve(strings.size());
    for (const string& str : strings) {
        char* cstr = new char[str.size() + 1];     //Convierte de string a char los argumentos de comando para que se ejecuten
        strcpy(cstr, str.c_str());                 //con execvp
        charVector.push_back(cstr);
    }
    return charVector;
}

//Esta función lee los stats para el daemon
vector<string> lee_Stats() {
    vector<string> valor;
    try {
        ifstream archivo("/proc/stat");
        if (!archivo.is_open()) {
            throw runtime_error("No se pudo abrir el archivo /proc/stat");
        }

        string linea;
        while (getline(archivo, linea)) {
            if (linea.substr(0, 9) == "processes") {
                valor.push_back(linea.substr(10));
            }
            if (linea.substr(0, 13) == "procs_running") {
                valor.push_back(linea.substr(14));
            }
            if (linea.substr(0, 13) == "procs_blocked") {
                valor.push_back(linea.substr(14));
            }
        }
        archivo.close();
    } catch (const exception &e) {
        cerr << "Error al leer /proc/stat: " << e.what() << endl;
    }
    return valor;
}

//esta función sirve para distinguir si en una cadena existe un substring, se usa para el daemon syslog
bool contieneSubstring(string& cadena, string subcadena) {
    // Utiliza la función find para buscar la subcadena en la cadena
    size_t found = cadena.find(subcadena);

    // Si found es diferente de std::string::npos, la subcadena fue encontrada
    if (found != string::npos) {
        return true;
    } else {
        return false;
    }
}

//Transforma los comandos que contienen pipes a char* para que sean ejecutados posteriormente
vector<vector<char*>> parseCommands(const string& input) {
    vector<string> aux;
    //Primero separa los comandos en base a '|'
    istringstream stream(input);
    string token;
    while (getline(stream, token, '|')) {
        aux.push_back(token);
    }

    vector<vector<char*>> res;
    res.reserve(aux.size());

    for (const string& actual : aux) {
        // Elimina el espacio inicial si existe
        size_t start = (actual[0] == ' ') ? 1 : 0;
        istringstream stream(actual.substr(start));
        string token;
        vector<string> tokens;

        while (getline(stream, token, ' ')) {
            tokens.push_back(token);
        }

        // Convierte el vector de strings en un vector de char*
        vector<char*> charVector = stringVectorToCharVector(tokens);

        // Agrega nullptr al final del vector de char*
        charVector.push_back(nullptr);

        // Agrega el vector de char* al resultado
        res.push_back(charVector);
    }

    return res;
}

//Separa por espacios en el caso de los comandos sin pipes
vector<string> separarPorEspacios(const string& input) {
    vector<string> resultado;
    istringstream stream(input);
    string token;
    
    while (stream >> token) {
        resultado.push_back(token);
    }
    
    return resultado;
}

//Cuenta la cantidad de pipes en la entrada
int hayPipe(const string& input){
    int pipes=0;
    int len = input.length();
    for(int i=0;i<len;i++){
        if (input[i] == '|') pipes++;               
    }
    return pipes;
}


int ejecutarConPipes(string &comandos) {
    vector<vector<char*>> tokens = parseCommands(comandos);
    int num_pipes = hayPipe(comandos);
    int pipe_fds[num_pipes][2]; // Array de descriptores de archivo para los pipes

    int status;
    pid_t pid;

    // Crear pipes
    for (int i = 0; i < num_pipes; i++) {
        if (pipe(pipe_fds[i]) == -1) {
            perror("Error al crear el pipe");
            return 1;
        }
    }
    //se itera entre la cantidad de pipes 
    for (int i = 0; i <= num_pipes; i++) {
        //Se crea un proceso hijo por pipe
        pid = fork();

        if (pid == 0) { // Proceso hijo
            if (i > 0) { // Configurar la entrada desde el pipe anterior
                dup2(pipe_fds[i - 1][0], STDIN_FILENO); //Copia la entrada estandar al extremo de lectura
                close(pipe_fds[i - 1][0]); //Se cierra el extremo de lectura original
            }

            if (i < num_pipes) { // Configurar la salida hacia el próximo pipe
                dup2(pipe_fds[i][1], STDOUT_FILENO); //Copia la salida estandar al extremo de escritura del pipe
                close(pipe_fds[i][1]); //Se cierra el extremo de lectura original
            }

            // Cerrar todos los descriptores de archivo que no son usados
            for (int j = 0; j < num_pipes; j++) {
                close(pipe_fds[j][0]);
                close(pipe_fds[j][1]);
            }
            //Se ejecutan los comandos 
            execvp(tokens[i][0], tokens[i].data());
            perror("Error al ejecutar el comando");
            exit(1);
        } else if (pid < 0) {
            perror("Error al crear el proceso hijo");
            return 1;
        }
    }

    // Cerrar todos los descriptores de archivo en el proceso padre
    for (int i = 0; i < num_pipes; i++) {
        close(pipe_fds[i][0]);
        close(pipe_fds[i][1]);
    }

    // Esperar a que todos los procesos hijos terminen
    for (int i = 0; i <= num_pipes; i++) {
        wait(&status);
    }

    return 0;
}

//Funcion que se encarga de ejecutar los comandos sin pipes
int ejecutar(string entrada){
    vector<string>parsed;
    int status;
    parsed = separarPorEspacios(entrada);
    char** argv = new char*[parsed.size()]; // Crea un arreglo de punteros a caracteres

    for (int i = 0; i < parsed.size(); ++i) {
        argv[i] = const_cast<char*>(parsed[i].c_str()); // Convierte std::string a char*
    }
    argv[parsed.size()] = nullptr;
    status =  execvp(argv[0],argv);
    
    return status;
}

//Crea el proceso daemon del parte 2 del proyecto
int daemon(const string &comandos) {
    //Se separan los comandos para extraer t y p
    vector<string> parsed = separarPorEspacios(comandos);
    pid_t sid;

    int t,p;

    try {
            t = stoi(parsed[1]);  //intervalo 
            p = stoi(parsed[2]);  //tiempo total
        } catch (const std::invalid_argument& e) {
            return 1;
        }

    // Cambiar el directorio de trabajo y crear un nuevo SID para el demonio
    umask(0);
    sid = setsid();

    //No se pudo crear el daemon
    if (sid < 0) {
        cerr<<"Error al crear el proceso daemon\n";
        exit(EXIT_FAILURE);
    }

    chdir("/");
    int tiempo_transcurrido = 0;

    // Abre el archivo de registro en modo de agregación (append)
    ofstream log_stream("/var/log/syslog.log", ios::trunc);

    openlog("MiDemonio", LOG_PID, LOG_DAEMON);
    syslog(LOG_NOTICE, "Demonio iniciado.");

    while (tiempo_transcurrido < p) {
        vector<string> lectura = lee_Stats();
        log_stream << "-------------------------" << endl;
        for (int i = 0; i < lectura.size()-2; i++) {
            //cout << lectura[i] <<endl;
            // Escribe los datos en el archivo de registro
            
            log_stream << "processes: "<< lectura[i] << endl;
            log_stream << "procs_running: "<< lectura[i+1] << endl;
            log_stream << "procs_blocked: "<< lectura[i+2] << endl;
            log_stream << "-------------------------" << endl;

        }
        log_stream << endl;

        std::this_thread::sleep_for(chrono::seconds(t));
        tiempo_transcurrido += t;
    }

    // Cierra el archivo de registro
    log_stream.close();

    // Cerrar el demonio
    syslog(LOG_NOTICE, "Demonio finalizado.");
    closelog();
    return 0;
}