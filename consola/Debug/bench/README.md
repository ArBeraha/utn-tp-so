Para correrlo hace falta abrir uno o mas cores, y ejecutar el starter configurando !search como el numero a buscar y c como la cantidad de cores a ejecutar.


El Kernel tiene que tener los siguientes semaforos:
 * start = 0
 * done = 0
 * mutex = 1

Y las siguientes variables compartidas **(la inicializacion no importa)**:
 * a
 * b
 * search
 * step
 * terminar

El benchmark de performance po el grupo *escriba su nombre aqui*, promediando en *3,76* minutos para encontrar los factores del numero *121*
![Performance](http://i.imgur.com/hjFwnHk.png)