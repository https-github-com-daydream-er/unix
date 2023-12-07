gcc -c create.c
gcc -c -DTIMES client.c
gcc -c -DTIMES server.c
ar rc libmytest.a create.o 
ar rc libmytest.a client.o 
ar rc libmytest.a server.o 
gcc -o mytest mytest.c -L./ -lmytest
