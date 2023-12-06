#ifndef CREATE_H
# define CREATE_H

#include<unistd.h>
#include<errno.h>
#include<sys/fcntl.h>
#include<sys/types.h>
#include<sys/fcntl.h>
#include<stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>

// 1MB = 1048576
// 1/4 MB = 262144
#define MB 262144
#define IN 0
#define OUT 1

#define COMM 0
#define COMP 1
#define IO 2
#define SEC 1000000

void	debug_file(void);
void    child_proc(int id);
int     create_source_data();

#endif