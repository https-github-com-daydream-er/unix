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
		sprintf(cmd, "od -i IOnode_server/IOnode_#%d | more", i);
		system(cmd);
	}
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

static void	comm_init(int id, int server2server[4][2])
{
	int			i;
	char		fifo_name[64];

	for (i = 0; i < 4; i++)
	{
		if (id == i)
			continue;
		sprintf(fifo_name, "tmp/serverfifo%d2%d", i, id);
		server2server[i][0] = open(fifo_name, O_RDONLY | O_NONBLOCK); // i -> id: read
	}
	for (i = 0; i < 4; i++)	{
		if (id == i)
			continue;
		sprintf(fifo_name, "tmp/serverfifo%d2%d", id, i);
		server2server[i][1] = open(fifo_name, O_WRONLY); // id -> i: write
	}
}

static void	send_server(int id, char fifo_name[1024])
{
	int		i;
	int		fd;
	int		pip;
	char	file_name[1024];
	int		data[MB];
	int		ret;

	sprintf(file_name, "data/p%d.dat", id + 1);
	fd = open(file_name, O_RDONLY);
	if ((ret = read(fd, data, MB * sizeof(int))) < 0)
		exit(1);
	pip = open(fifo_name, O_WRONLY);
	i = 0;
	while (i < MB)
	{
		write(pip, &data[i], sizeof(int) * 8);
		i += 8;
	}
	close(pip);
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
static void	do_comm_node(int id, char fifo_name[1024])
{
	int		ret;
	int		chunk[8];
	int		server2server[4][2];
	int		i;
	int		j;
	int		dump[MB];
	int		dump_idx;
	int		pip;
	int		chunk_size;

#ifdef TIMES
	int time_result;
	struct timeval stime, etime;

	gettimeofday(&stime, NULL);
#endif
	dump_idx = -1;
	comm_init(id, server2server);
	pip = open(fifo_name, O_RDONLY);
	while ((chunk_size = read(pip, chunk, sizeof(int) * 8)) > 0)
	{
		for (i = 0; i < 8; i++) // 8개의 데이터를 파이프에 쓴다.
		{
			int	remain = chunk[i] % 32;

			if ((8 * id + 1 <= remain && remain <= 8 * id + 8) || (id == 3 && remain == 0))
				dump[++dump_idx] = chunk[i];
			else if (1 <= remain && remain <= 8)
				ret = write(server2server[0][1], &chunk[i], sizeof(int)); // id -> 0; write
			else if (9 <= remain && remain <= 16)
				ret = write(server2server[1][1], &chunk[i], sizeof(int)); // id -> 1; write
			else if (17 <= remain && remain <= 24)
				ret = write(server2server[2][1], &chunk[i], sizeof(int)); // id -> 2; write
			else if ((25 <= remain && remain < 32) || remain == 0)
				ret = write(server2server[3][1], &chunk[i], sizeof(int)); // id -> 3; write
			else
			{
				perror("wrong chunk number");
				exit(1);
			}
		}
		for (j = 0; j < 3; j++) // 파이프를 비우기 위해서는 최소 8번씩 반복해야 한다.
		{
			for (i = 0; i < 4; i++) // 하지만, 동시성 문제를 극복하기 위해 12번 정도 여유롭게 반복해서 파이프를 비운다.
			{
				int	buffer;

				if (id == i)
					continue;
				ret = read(server2server[i][0], &buffer, sizeof(int)); // i -> id; read
				if (ret > 0)
					dump[++dump_idx] = buffer;
			}
		}
	}
	while (1)
	{
		for (i = 0; i < 4; i++)
		{
			int	buffer;

			if (id == i)
				continue;
			ret = read(server2server[i][0], &buffer, sizeof(int)); // i -> id; read
			if (ret > 0)
				dump[++dump_idx] = buffer;
		}
		// dump_idx == MB - 1의 의미는 dump[MB - 1]에 write를 완료
		if (dump_idx == MB - 1)
			break ;
	}
#ifdef TIMES
	gettimeofday(&etime, NULL);
	time_result = (etime.tv_usec - stime.tv_usec);
	writeTimeAdvLock(COMM, time_result);
#endif
	do_compute_node(dump); // 정렬하기
	do_io_node(id, dump); // io_node로 데이터 저장하기
	for (i = 0; i < 4; i++)
		close(server2server[i][0]);
	for (i = 0; i < 4; i++)
		close(server2server[i][1]);
	close(pip);
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

static void	do_io_node(int id, int dump[MB]) // 모아진 데이터를 한번에 저장?
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
}

static void	Client2Server(int i)
{
	int	pid;
	int	status;
	char	server_fifo[1024];

	sprintf(server_fifo, "tmp/server2client%d", i);
	mkfifo(server_fifo, 0644);
	pid = fork();
	if (pid == 0) // client
		send_server(i, server_fifo);
	else // server
	{
		do_comm_node(i, server_fifo);
		pid = wait(&status);
		printf("[DEBUG] Client2Server, pid : %d, status: %d done\n", pid, status);
	}
	unlink(server_fifo);
}

static void	parallel_operation(void)
{
	int	i;
	int	j;
	char	server2server[1024];

	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			int	ret;

			sprintf(server2server, "tmp/serverfifo%d2%d", i, j);
			ret = mkfifo(server2server, 0644);
		}
	}
	for (i = 0; i < 4; i++)
	{
		int	pid;

		pid = fork();
		if (pid == 0)
		{
			Client2Server(i);
			exit(0);
		}
	}
	parent("parallel_operation");
	debug_result();
	for (i = 0; i < 4; i++)
	{
		for (j = 0; j < 4; j++)
		{
			sprintf(server2server, "tmp/serverfifo%d2%d", i, j);
			unlink(server2server);
		}
	}
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
	time_result = etime.tv_usec - stime.tv_usec;
	if (time_result < 0)
		time_result += SEC;
	printf("Server_oriented_io TIMES == %ld %ld %ld\n", (long)etime.tv_usec, (long)stime.tv_usec, (long)time_result);
#endif
	return (1);
}

// int	main()
// {
// 	server_oriented_io();
// }