#include "mytest.h"
#include "server.h"

#define CLIENT_H

static void	debug_result(void)
{
	int		i;
	char	cmd[1024];
	int		fd;
	int		buffer[MB];

	for (i = 1; i <= 4; i++)
	{
		sprintf(cmd, "IOnode_server/IOnode_#%d", i);
		fd = open(cmd, O_RDONLY);
		if (fd < 0)
		{
			perror("fd error");
			exit(1);
		}
		printf("%d result byte: %d, MB byte: %d\n", i, (int)read(fd, buffer, MB * sizeof(int)), (int)(MB * sizeof(int)));
	}
}

// 오름차순 비교 함수 구현
static int ft_compare(const void *a, const void *b)
{
    int num1 = *(int *)a;
    int num2 = *(int *)b;

    if (num1 < num2)
        return (-1);
    if (num1 > num2)
        return (1);
    return (0);
}

// rest_time;
static void	do_compute_node(int *dump)
{
#ifdef TIMES
	int time_result;
	struct timeval stime, etime;

	gettimeofday(&stime, NULL);
#endif
	qsort(dump, MB, sizeof(int), ft_compare);
#ifdef TIMES
	gettimeofday(&etime, NULL);
	time_result = (etime.tv_usec - stime.tv_usec);
	writeTimeAdvLock(COMP, time_result);
#endif

}

static void	comm_init(int id, int client2server[4])
{
	int			i;
	char		fifo_name[64];

	for (i = 0; i < 4; i++)
	{
		sprintf(fifo_name, "tmp/serverfifo%d2%d", i, id);
		client2server[i] = open(fifo_name, O_RDONLY | O_NONBLOCK); // i -> id: read
	}
}

static void	send_server(int id)
{
	int		i;
	int		fd;
	char	file_name[1024];
	char	fifo_name[128];
	int		client2server[4];
	int		data[MB];
	int		ret;
	char	cmd[1024];

	sprintf(cmd, "od -i server_data/p%d.dat > dump/server_data%d.dump", id + 1, id + 1);
	system(cmd);
	sprintf(file_name, "server_data/p%d.dat", id + 1);
	fd = open(file_name, O_RDONLY);
	if ((ret = read(fd, data, MB * sizeof(int))) < 0)
		exit(1);
	for (i = 0; i < 4; i++)	
	{
		sprintf(fifo_name, "tmp/serverfifo%d2%d", id, i);
		client2server[i] = open(fifo_name, O_WRONLY); // id -> i: write
	}
	i = 0;
	while (i < MB)
	{
		int	remain;

		remain = data[i] % 32;
		if (1 <= remain && remain <= 8)
			ret = write(client2server[0], &data[i], sizeof(int)); // id -> 0; write
		else if (9 <= remain && remain <= 16)
			ret = write(client2server[1], &data[i], sizeof(int)); // id -> 1; write
		else if (17 <= remain && remain <= 24)
			ret = write(client2server[2], &data[i], sizeof(int)); // id -> 2; write
		else if ((25 <= remain && remain < 32) || remain == 0)
			ret = write(client2server[3], &data[i], sizeof(int)); // id -> 3; write
		else
		{
			perror("wrong chunk number");
			exit(1);
		}
		i++;
	}
	for (i = 0; i < 4; i++)
		close(client2server[i]);
	close(fd);
}

static void	writeTimeAdvLock(int index, int time_result)
{
	struct flock	myLock;
	int				fd;
	int				buffer;

	if (time_result < 0)
		time_result += SEC;
	fd = open("serverOrientedTime", O_RDWR, 0644);
	myLock.l_type = F_WRLCK;
	myLock.l_whence = SEEK_SET;
	myLock.l_start = index * sizeof(int);
	myLock.l_len = sizeof(int);
	fcntl(fd, F_SETLKW, &myLock); // F_SETLKW로 쓰기 lock
	lseek(fd, index * sizeof(int), SEEK_SET); // index로 위치 이동
	read(fd, &buffer, sizeof(int)); // 읽기
	time_result += buffer;
	// printf("[DEBUG] (index, %d) = %d\n", index, time_result);
	lseek(fd, index * sizeof(int), SEEK_SET);
	write(fd, &time_result, sizeof(int)); // 쓰기
	myLock.l_type = F_UNLCK; // F_SETLKW 해제
	fcntl(fd, F_SETLKW, &myLock);
	close(fd);
}

// do_comm -> do_compute -> do_io_node
static void	do_comm_node(int id)
{
	int		ret;
	int		chunk[8];
	int		client2server[4];
	int		i;
	int		dump[MB];
	int		dump_idx;

#ifdef TIMES
	int time_result;
	struct timeval stime, etime;

	gettimeofday(&stime, NULL);
#endif
	comm_init(id, client2server);

	dump_idx = 0;
	while (dump_idx < MB)
	{
		for (i = 0; i < 4; i++)
		{
			ret = read(client2server[i], chunk, sizeof(int) * 8); // NON_BLOCK, chunk 단위로 읽으려고 시도
			// ret은 바이트 단위이고, dump_idx는 sizeof(int) 단위이다.
			if (ret > 0) // fifo에 chunk 단위로 들어가 있다면 읽는다.
			{
				memcpy(&dump[dump_idx], chunk, ret); // ret만큼만 적는다.
				dump_idx += (ret / sizeof(int)); // write에서 sizeof(int) 단위로 찍기 때문에 sizeof(int)의 배수
			}
		}
	}
#ifdef TIMES
	gettimeofday(&etime, NULL);
	time_result = (etime.tv_usec - stime.tv_usec);
	writeTimeAdvLock(COMM, time_result);
#endif
	do_compute_node(dump); // 정렬하기
	do_io_node(id, dump); // io_node로 데이터 저장하기
	for (i = 0; i < 4; i++)
		close(client2server[i]);
}

static void	parent(char *str)
{
	int		pid;
	int		status;
	int		i;
	char	buffer[1024];

	for (i = 0; i < 4; i++)
	{
		pid = waitpid(-1, &status, 0);
		if (pid == -1)
		{
			sprintf(buffer, "[PID ERROR : %s]", str);
			perror(buffer);
			exit(1);
		}
		printf("[DEBUG] %s, pid : %d, status: %d done\n", str, pid, status);
	}
}

static void	do_io_node(int id, int dump[MB]) // 모아진 데이터를 한번에 저장
{
	int		fd;
	char	file_name[25];
#ifdef TIMES
	int time_result;
	struct timeval stime, etime;

	gettimeofday(&stime, NULL);
#endif
	sprintf(file_name, "IOnode_server/IOnode_#%d", id + 1);
	printf("file_name : [%s]\n", file_name);
	fd = open(file_name, O_CREAT | O_WRONLY | O_TRUNC, 0644);
	if (fd < 0)
	{
		perror("I/O node CREATE FAIL");
		exit(1);
	}
	write(fd, dump, sizeof(int) * MB);
#ifdef TIMES
	gettimeofday(&etime, NULL);
	time_result = (etime.tv_usec - stime.tv_usec);
	writeTimeAdvLock(IO, time_result);
#endif
	char	cmd[1024];

	sprintf(cmd, "od -i IOnode_server/IOnode_#%d > dump/server_result%d.dump", id + 1, id + 1);
	system(cmd);
	close(fd);
}

static void	ft_Client2Server(int i)
{
	int	pid;
	int	status;

	pid = fork();
	if (pid == 0) // client
		send_server(i);
	else // server
	{
		do_comm_node(i);
		pid = wait(&status);
		printf("[DEBUG] Client2Server, pid : %d, status: %d done\n", pid, status);
	}
}

static void	parallel_operation(void)
{
	int	i;
	int	j;
	char	client2server[1024];
	char	cmd[1024];

	for (i = 0; i < 4; i++)
	{
		for (j = 0; j < 4; j++)
		{
			int	ret;

			sprintf(client2server, "tmp/serverfifo%d2%d", i, j);
			ret = mkfifo(client2server, 0644);
		}
	}
	for (i = 0; i < 4; i++)
	{
		int	pid;

		pid = fork();
		if (pid == 0)
		{
			ft_Client2Server(i);
			exit(0);
		}
	}
	parent("parallel_operation");
	debug_result();
}

int server_oriented_io() {

#ifdef TIMES
	struct timeval stime, etime;
	int		time_result;
#endif
	/* Client_oriented_io. Measure io time, communication time, and time for the rest.
	*/

#ifdef TIMES
	gettimeofday(&stime, NULL);
#endif

	int	fd;
	int	ZERO;

	unlink("serverOrientedTime"); // serverOrientedTime은 server_oriented_io가 실행될 때마다 초기화된다.
	fd = open("serverOrientedTime", O_CREAT | O_WRONLY, 0644);
	ZERO = 0;
	write(fd, &ZERO, sizeof(int));
	write(fd, &ZERO, sizeof(int));
	write(fd, &ZERO, sizeof(int));
	close(fd);
	parallel_operation();

#ifdef TIMES
	gettimeofday(&etime, NULL);
	time_result = (etime.tv_sec - stime.tv_sec) * 1000000 + etime.tv_usec - stime.tv_usec;
	printf("Server_oriented_io TIMES == %ld %ld %ld\n", (long)etime.tv_usec, (long)stime.tv_usec, (long)time_result);
#endif
	return (1);
}

// int	main()
// {
// 	server_oriented_io();
// }