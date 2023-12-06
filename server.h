#ifndef SERVER_H
# define SERVER_H

#include<unistd.h>
#include<errno.h>
#include<sys/fcntl.h>
#include<sys/types.h>
#include<sys/fcntl.h>
#include<stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <sys/stat.h>

#include <string.h>
// 1MB = 1048576
// 1/4 MB = 262144
#define MB 262144
#define IN 0
#define OUT 1

#define COMM 0
#define COMP 1
#define IO 2
#define SEC 1000000

static void	debug_result(void);
static int	ft_compare(const void *a, const void *b);
static void	do_compute_node(int *dump);
static void	comm_init(int id, int server2server[4][2]);
static void send_server(int id, char fifo_name[1024]);
static void	writeTimeAdvLock(int index, int time_result);
static void	do_comm_node(int id, char fifo_name[1024]);
static void	parent(char *str);
static void	Client2Server(int i);
static void	parallel_operation(void);
static void	do_io_node(int id, int dump[MB]);

#endif