# tp-2016-1c-Con-16-bits-me-hago-alto-kernel

## Compilar todos los modulos desde eclipse
In the Eclipse main menu

    Click derecho sobre el proyecto->build configurations->build all.

No hacer el build all desde otro lado, ya que por alguna razon cosmica no anda.

Los binarios quedan en workspace/tp-2016-1c-Con-16-bits-me-hago-alto-kernel/"UnModulo"/, donde "unModulo" es el modulo que uno busca.



## Compilar todos los modulos desde consola
Algun dia de estos va a haber un mini script para eso :p
La idea de esto es que queden todos los modulos en la carpeta donde se haga el gitclone y no separados en distintas carpetas.



## Como testear desde eclipse uno de los modulos:
Sirve para correr un modulo sobre eclipse y usar el debugger, mientras puede o no haber otros en consolas de linux. No hace correr los demas modulos en background ni nada asi :(

In the Eclipse main menu

    select "Project" -> "Properties" (alternatively, just hold the ALT-key down and hit Enter)

In the "Properties for (project name)" window

    select "C/C++ Build" in the list on the left

In the "C/C++ Build" configuration that is displayed on the right side

    select "Manage configurations"

In the "Manage configurations..." window

    choose "Release"
    select "Set Active"
    select "OK"
    select "OK" again to close Project Properties

Nota: Donde dice "choose Release", en vez de poner Release se selecciona el nombre del modulo que se quiere testear!
