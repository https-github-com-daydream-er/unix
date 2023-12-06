gcc -c create.c -I create.h
gcc -c -DTIMES client.c -I client.h
gcc -c -DTIMES server.c -I server.h
ar rc libmytest.a create.o 
ar rc libmytest.a client.o 
ar rc libmytest.a server.o 
gcc -o mytest mytest.c -L. -lmytest