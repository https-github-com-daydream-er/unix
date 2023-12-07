#ifndef CLIENT_H
# define CLIENT_H

#include<unistd.h>
#include<errno.h>
#include<sys/fcntl.h>
#include<sys/types.h>
#include<sys/fcntl.h>
#include<stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include"mytest.h"

// 1MB = 1048576
// 1/4 MB = 262144
#define MB 262144
#define IN 0
#define OUT 1

#define COMM 0
#define COMP 1
#define IO 2
#define SEC 1000000

// clinet.c
static void	debug_result4client(void);
static void	do_comm_node4client(int id, int client2client[4][4][2], int pip[2]);
static void	do_io_node4client(int id, int pip[2]);

static void	parallel_operation4client(void); // Client2Server와 parent 실행
static void	parent4client(char *str); // 4개의 client2Server의 부모
static void	Client2Server4client(int i, int client2client[4][4][2]); // do_io_node, do_compute_node, do_comm_node

static int  ft_compare4client(const void *a, const void *b);
static void	do_compute_node4client(int *dump); // compute -> sort data
static void	comm_init4client(int id, int client2client[4][4][2], int pip[2], int data[MB]); // communicate
static void send_server4client(int pip[2], int dump[MB]); // server 전달
static void	writeTimeAdvLock4client(int index, int time_result);

#endif