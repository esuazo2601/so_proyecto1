#include <iostream>
#include <sys/types.h>
#include <vector>
#include <cstring>
#include <unistd.h>
#include <sys/wait.h>
#include "shell.h"
#include <sstream>

using namespace std;

int main() {
    string comando = "";

    while (1) {
        do { // Imprime el prompt 
            printf("MiShell ~>: ");
            getline(cin, comando);
        } while (comando.empty());

        if (comando == "exit") {
            break;
        }

        // En el caso que no se encuentren pipes en la línea de comandos
        if (hayPipe(comando) == 0) {
            pid_t pid = fork(); // hace un proceso hijo para el comando ingresado

            //Si el comando ingresado contiene syslog, se ejecuta el daemon
            if (contieneSubstring(comando, "syslog")) {
                if (pid == 0) {
                    pid_t demonio_pid = fork();
                    if (demonio_pid == 0) {
                        int ret = daemon(comando);
                        if(ret == 1){ cout<<"Ingrese parametros válidos"<<endl; exit(0);}
                        exit(0);
                    }
                    exit(0);
                }
            }

            if (pid == 0) {
                //en caso de que el comando ejecutado no se pueda ejecutar se imprime el mensaje de error
                if (ejecutar(comando) == -1) {
                    cout << "No se reconoce el comando: " << comando << endl;
                    exit(1); 
                }
                exit(0);
            } else {
                // Este es el proceso padre.
                waitpid(pid, NULL, 0); // Esperar a que termine el proceso hijo.
            }
        } else if (hayPipe(comando) > 0) { //en el caso de que hayan pipes en el comando entrante
            int status = ejecutarConPipes(comando);
            if (status == -1) {
                cout << "Los comandos ingresados " << comando << " no existen" << endl;
            }
        }
    }

    return 0;
}
