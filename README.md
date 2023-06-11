**Universidad de La Habana**

*Segundo proyecto de la asignatura: Sistema Operativo*

Estudiante: David Sánchez Iglesias

Usuario de telegram: @ElBestion

Este proyecto consiste en un servidor web capaz de mostrar los archivos presentes en un directorio de la computadora y que permite navegar a traves de ellos. Al hacer click en un directorio, este se "abre" y muestra su contenido; si se hace click en un archivo (no directorio o carpeta) este se descarga.
El servidor soporta varios clientes conectados a la vez y una descarga por cliente. Para cada cliente crea un proceso, siempre dejando otro para recibir la señal de otros clientes y conectarlos. Las descargas se manejan por hilos, para cada una se crea un hilo donde se procesa la descarga; esto por cada cliente.
Este proyecto fue diseñado para funcionar en el sistema operativo Linux; y para probarlo es necesario pasarle la dirección inicial que se desea visualizar en el servidor y el puerto. Como valor predeterminado, la IP del servidor es 127.0.0.1
