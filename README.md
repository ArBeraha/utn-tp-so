# Trabajo Práctico de Sistemas Operativos

# Compilacion

## Compilar de a un modulo

    cd trabajoPracticoSO/<<nombreDelModulo>>/Debug/

    make clean && make all                 [USAR MAKE ALL, NO MAKE]


## Compilar todos los modulos desde consola
Para compilar todo por consola de una:

    En workspace o donde se haga gitclone -> cd trabajoPracticoSO -> sh compilar


# Otros comandos en bash
## Stress Testing

    for n in {1..100}; do ./facil.ansisop; done
    
## Testear SIGUSR1

    kill -s SIGUSR1 <<pid>>, pudiendose conocer el pid con:
    ps -a

## Monitorear consumo de recursos en tiempo real

    htop --delay=0 pid=<<PID>>

# Agregar alias, comandos útiles
en el home:

    sudo leafpad .bashrc

    al final agregar:
    alias tp='cd /home/utnso/workspace/trabajoPracticoSO'
    alias nucleo='clear && make clean && make all && ./nucleo'
    alias consola='clear && make clean && make all && ./consola'
    alias cpu='clear && make clean && make all && ./cpu'
    alias umc='clear && make clean && make all && ./umc'
    alias swap='clear && make clean && make all && ./swap'
    
Salvo el primero, los demas se corren desde la carpeta Debug del modulo correspondiente. Para que anden hay que cerrar y volver a abrir la consola despues de editar el archivo.

Para borrar los logs, estando en la carpeta Debug del modulo: rm -rf *.log

Detectar errores: cat nucleo.log | grep ERROR


# Como agregar todo esto en eclipse

    cd workspace

    git clone https://github.com/LucasEsposito/trabajoPracticoSO.git

    Abrir eclipse -> Switch to workspace -> other -> seleccionar trabajoPracticoSO

    Eclipse -> importar proyecto -> seleccionar las carpetas de los modulos (están todas en el nuevo workspace, osea, trabajoPracticoSO) una por una, y la carpeta "compartido" para los archivos compartidos.
