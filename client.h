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
void	debug_result(void);
void	do_comm_node(int id, int client2client[4][4][2], int pip[2]);
void	do_io_node(int id, int pip[2]);

int     client_oriented_io(); // parallel_operation 실행 및 시간 측정
void	parallel_operation(void); // Client2Server와 parent 실행
void	parent(char *str); // 4개의 client2Server의 부모
void	Client2Server(int i, int client2client[4][4][2]); // do_io_node, do_compute_node, do_comm_node

int     ft_compare(const void *a, const void *b);
void	do_compute_node(int *dump); // compute -> sort data
void	comm_init(int id, int client2client[4][4][2], int pip[2], int data[MB]); // communicate
void    send_server(int pip[2], int dump[MB]); // server 전달
void	writeTimeAdvLock(int index, int time_result);

#endif