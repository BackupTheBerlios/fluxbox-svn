$set 14 #main

$ #RCRequiresArg
# error: '-rc' requiere un argumento
$ #DISPLAYRequiresArg
# error: '-display' requiere un argumento
$ #WarnDisplaySet
# cuidado: no se puede establecer la variable de ambiente 'DISPLAY'
$ #Usage
# Fluxbox %s: (c) %s Henrik Kinnunen\n\n\
  -display <string> conexi�n de despliegue.\n\
  -rc <string>      archivo alternativo de recuros.\n\
  -version          mostrar la versi�n y cerrar.\n\
  -info\t\t\t\tdisplay some useful information.\n\
  -log <filename>\t\t\tlog output to file.\n\
  -help             mostrar este texto de ayuda y cerrar.\n\n
$ #CompileOptions
# Opciones durante la compilaci�n:\n\
  Informaci�n extra para depuraci�n:               %s\n\
  Entrelazado:                                     %s\n\
  Forma:                                           %s\n\
  Slit:                                            %s\n\
  8bpp simulaci�n ordenada de colores en im�genes: %s\n\n
