
# Proyecto realizado por:
- Patricio Covarrubias Ch.
- Esteban Suazo M.

## Compilación
1. Primeramente se debe tener instalado el compilador de C++ (g++) y ejecutar:

```
g++ *.cpp -o MiShell
```
## Ejecución de comandos básicos

- Para ejecutar el programa se debe ejecutar el comando

```
./MiShell
```
- Una vez dentro de la shell se pueden ejecutar los comandos básicos de unix 
- Ejemplo:

```
MiShell~>: ls
MiShell~>: ls -lf | grep main
MiShell~>: ls -lf | grep main | wc
```
Entre otros

## Ejecución del proceso daemon
- Primeramente se debe dar permisos de escritura en el directorio "/var/logs" mediante el comando

```
sudo chmod o+w /var/log
```
Una vez realizado se puede ejecutar el daemon con:

```
MiShell~>: syslog t p
```
siendo t y p valores numericos

- Para comprobar la salida del proceso daemon se debe ejecutar en una bash externa: 

```
cat /var/log/syslog.log
```
