# tp-2016-1c-Con-16-bits-me-hago-alto-kernel

# Mandar archivos ansisop a rolete :D
[![Link](https://github.com/sisoputnfrba/tp-2016-1c-Con-16-bits-me-hago-alto-kernel/wiki/Mandar-archivos-a-lo-loco)](https://github.com/sisoputnfrba/tp-2016-1c-Con-16-bits-me-hago-alto-kernel/wiki/Mandar-archivos-a-lo-loco)

# Agregas alias, comandos útiles
en el home:

    sudo leafpad .bashrc

    al final agregar:
    alias tp='cd /home/utnso/workspace/tp-2016-1c-Con-16-bits-me-hago-alto-kernel'
    alias nucleo='clear && make clean && make all && ./nucleo'
    alias consola='clear && make clean && make all && ./consola'
    alias cpu='clear && make clean && make all && ./cpu'
    alias umc='clear && make clean && make all && ./umc'
    alias swap='clear && make clean && make all && ./swap'
Salvo el primero, los demas se corren desde la carpeta Debug del modulo correspondiente. Para que anden hay que cerrar y volver a abrir la consola despues de editar el archivo.

Otro comando útil es 'cat', que muestra el contenido de un archivo en pantalla:cat umc.log (mostraria el log de umc en la consola.)

Para borrar los logs, estando en la carpeta Debug del modulo: rm -rf *.log

La joya: el comando grep sirve para filtrar lineas que cumplan con un patron. Por ejemplo, si quiero filtrar los errores del log de nucleo: cat nucleo.log | grep ERROR    (distingue entre mayusculas y minusculas! ojo con eso!)

# Como agregar todo esto en eclipse

    cd workspace

    git clone https://github.com/sisoputnfrba/tp-2016-1c-Con-16-bits-me-hago-alto-kernel.git

    Abrir eclipse -> Switch to workspace -> other -> seleccionar tp-2016-1c-Con-16-bits-me-hago-alto-kernel (carpeta que aparece por hacer gitclone en el workspace)

    Eclipse -> importar proyecto -> seleccionar las carpetas de los modulos (están todas en el nuevo workspace, osea, tp-2016-1c-Con-16-bits-me-hago-alto-kernel) una por una, y la carpeta "compartido" para los archivos compartidos.

Los archivos compartidos aparecen en todos los modulos, pero en realidad están linkeados. Cuando se modifican en un lugar, el cambio impacta en todos lados.

# Protocolo
[![Link del protocolo](https://github.com/sisoputnfrba/tp-2016-1c-Con-16-bits-me-hago-alto-kernel/wiki/Protocolo)](https://github.com/sisoputnfrba/tp-2016-1c-Con-16-bits-me-hago-alto-kernel/wiki/Protocolo)


# Compilacion

## Compilar todos los modulos desde eclipse
Darle al botoncito de la hoja con 0s y 1s de la toolbar.

Los binarios quedan en workspace/tp-2016-1c-Con-16-bits-me-hago-alto-kernel/"UnModulo"/Debug/, donde "unModulo" es el modulo que uno busca.


## Compilar de a un modulo

    cd tp-2016-1c-Con-16-bits-me-hago-alto-kernel/unModulo/Debug/

    make clean && make all                 [USAR MAKE ALL, NO MAKE]


## Compilar todos los modulos desde consola
Para compilar todo por consola de una:

    En workspace o donde se haga gitclone -> cd tp-2016-1c-Con-16-bits-me-hago-alto-kernel -> sh compilar

Con eso van a estar los 5 ejecutables, cada uno en la carpeta de su modulo/Debug. Se puede correr todas las veces que uno quiera dentro de la misma carpeta.


# Distribución de módulos
No es la idea que sea algo exacto para que se de tal cual a rajatabla. Solamente lo pongo para que cuando los modulos necesiten pasarse cosas, cada quien sepa con quien o quienes tiene que hablar para ponerse de acuerdo. Todos tienen permisos de edición de la planilla si se loguean con su cuenta de gmail :)

[![Link de la planilla :)](https://docs.google.com/spreadsheets/d/1_3iUmtMuKK-n50n-ggTaDyBi7pzAeRoAmYej9HYjF7k/edit#gid=0)](https://docs.google.com/spreadsheets/d/1_3iUmtMuKK-n50n-ggTaDyBi7pzAeRoAmYej9HYjF7k/edit#gid=0)


# Links de interés sobre las herramientas y la teoría

## Git :)
[![asciicast](https://lh3.googleusercontent.com/-H6xZCx4HCeE/TqqR8Tp_5QI/AAAAAAAAe0w/5rpSC6gDi4A/w1565-h1124/EntendiendoGIT.png)](https://lh3.googleusercontent.com/-H6xZCx4HCeE/TqqR8Tp_5QI/AAAAAAAAe0w/5rpSC6gDi4A/w1565-h1124/EntendiendoGIT.png)

## Paginacion por demanda en 5 minutos
https://www.youtube.com/watch?v=dOVrEbZVeoU

## Algoritmos de reemplazo de páginas (PPT con ejemplos)
Incluye LRU, CLOCK y CLOCK MEJORADO, entre otros.
[![Link de la PPT :)](https://drive.google.com/open?id=0B0X0toyFFvk5bzJpRWtEZjVIYVU)](https://drive.google.com/open?id=0B0X0toyFFvk5bzJpRWtEZjVIYVU)
