# Trabajo Práctico de Sistemas Operativos

## Introducción

El trabajo práctico consiste en simular ciertos aspectos de un sistema multiprocesador con la capacidad de interpretar la ejecución de scripts escritos en un lenguaje creado para esta ocasión. Este sistema planificará y ejecutará estos scripts (en adelante “Programas”) controlando sus solicitudes de memoria y administrando los accesos a recursos, como los dispositivos de entrada/salida y los semáforos compartidos.

## Enunciado
https://www.gitbook.com/book/sisoputnfrba/elestac-tp-1c2016

## Compilación

### Compilar de a un módulo

    cd TP-SO/<<nombreDelModulo>>/Debug
    make clean && make all
    
### Compilar todos los módulos
Para compilar todo por consola de una:

    cd TP-SO
    sh compilar

## Otros comandos en bash
#### Stress Testing
    cd TP-SO/Consola/Debug/
    for n in {1..100}; do ./facil.ansisop; done
