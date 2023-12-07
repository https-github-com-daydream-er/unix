#include "mytest.h"
#include "client.h"

#define	SERVER_H

static void	debug_result4client(void)
{
	int		i;
	int		fd;
	int		buffer[MB];
	char	cmd[1024];

	for (i = 1; i <= 4; i++)
	{
		sprintf(cmd, "IOnode_client/IOnode_#%d", i);
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
static int ft_compare4client(const void *a, const void *b)
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
static void	do_compute_node4client(int *dump)
{
#ifdef TIMES
	int time_result;
	struct timeval stime, etime;

	gettimeofday(&stime, NULL);
#endif
	qsort(dump, MB, sizeof(int), ft_compare4client);
#ifdef TIMES
	gettimeofday(&etime, NULL);
	time_result = (etime.tv_usec - stime.tv_usec);
	writeTimeAdvLock4client(COMP, time_result);
#endif

}

static void	comm_init4client(int id, int client2client[4][4][2], int pip[2], int data[MB])
{
	char		file_name[1024];
	char		cmd[1024];
	int			fd;
	int			i;

	sprintf(cmd, "od -i client_data/p%d.dat > dump/client_data%d.dump", id + 1, id + 1);
	system(cmd);
	sprintf(file_name, "client_data/p%d.dat", id + 1);
	fd = open(file_name, O_RDONLY);
	if (fd < 0)
		exit(1);
	read(fd, data, MB * sizeof(int));
	close(pip[0]);
	for (i = 0; i < 4; i++)
		close(client2client[id][i][0]); // id -> i :  close(read)
	for (i = 0; i < 4; i++)
		close(client2client[i][id][1]); // i -> id : close(write)
	// client2client O_NONBLOCK
	for (i = 0; i < 4; i++)
	{
		if (i == id)
			continue;
		fcntl(client2client[i][id][0], F_SETFL, O_NONBLOCK);
	}
}

static void	send_server4client(int pip[2], int dump[MB])
{
	int	i;

	i = 0;
	while (i < MB)
	{
		write(pip[1], &dump[i], sizeof(int) * 8);
		i += 8;
	}
}

static void	writeTimeAdvLock4client(int index, int time_result)
{
	struct flock	myLock;
	int				fd;
	int				buffer;

	if (time_result < 0)
		time_result += SEC;
	fd = open("clientOrientedTime", O_RDWR, 0644);
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

// comm_time
static void	do_comm_node4client(int id, int client2client[4][4][2], int pip[2])
{
	int			data[MB];
	int			dump[MB];
	int			ret;
	int			i;
	int			dump_idx;

	comm_init4client(id, client2client, pip, data);
	ret = 0;
	dump_idx = -1;

#ifdef TIMES
	int time_result;
	struct timeval stime, etime;

	gettimeofday(&stime, NULL);
#endif

	for (i = 0; i < MB; i++)
	{
		int remain = data[i] % 32;

		if ((8 * id + 1 <= remain && remain <= 8 * id + 8) || (id == 3 && remain == 0))
			dump[++dump_idx] = data[i];
		else if (1 <= remain && remain <= 8)
			ret = write(client2client[id][0][1], &data[i], sizeof(int));
		else if (9 <= remain && remain <= 16)
			ret = write(client2client[id][1][1], &data[i], sizeof(int));
		else if (17 <= remain && remain <= 24)
			ret = write(client2client[id][2][1], &data[i], sizeof(int));
		else if ((25 <= remain && remain < 32) || remain == 0)
			ret = write(client2client[id][3][1], &data[i], sizeof(int));
		else
		{
			char	buffer[1024];

			sprintf(buffer, "[Error : %d] %d", id, remain % 512);
			perror(buffer);
			exit(1);
		}
		int	j;

		for (j = 0; j < 4; j++)
		{
			int	buffer;

			if (id == j)
				continue;
			// read쪽 pipe buffer 계속 비워줘야 한다.
			ret = read(client2client[j][id][0], &buffer, sizeof(int));
			if (ret > 0)
				dump[++dump_idx] = buffer;
		}
	}
	// NON-BLOCKING -> busy-waiting 발생
	// BLOCKING -> DEADLOCK 발생
	while (1) 
	{
		for (i = 0; i < 4; i++)
		{
			int	buffer;

			if (id == i)
				continue;
			ret = read(client2client[i][id][0], &buffer, sizeof(int));
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
	writeTimeAdvLock4client(COMM, time_result);
#endif

	do_compute_node4client(dump); // 정렬하기
	send_server4client(pip, dump); // server에게 전달
	for (i = 0; i < 4; i++)
		close(client2client[id][i][1]); // id -> i :  close(write)
	for (i = 0; i < 4; i++)
		close(client2client[i][id][0]); // i -> id : close(read)
	close(pip[1]);
}

// io_time
static void	do_io_node4client(int id, int pip[2])
{
	int		ret;
	int		chunk[8];
	char	file_name[25];
	int		fd;

	close(pip[1]);
	sprintf(file_name, "IOnode_client/IOnode_#%d", id + 1);
	fd = open(file_name, O_CREAT | O_WRONLY | O_TRUNC, 0644);
	if (fd < 0)
	{
		perror("I/O node CREATE FAIL");
		exit(1);
	}
#ifdef TIMES
	int time_result;
	struct timeval stime, etime;

	gettimeofday(&stime, NULL);
#endif
	while ((ret = read(pip[0], chunk, sizeof(int) * 8)) > 0)
		write(fd, chunk, sizeof(int) * 8);
#ifdef TIMES
	gettimeofday(&etime, NULL);
	time_result = (etime.tv_usec - stime.tv_usec);
	writeTimeAdvLock4client(IO, time_result);
#endif
	char	cmd[1024];

	sprintf(cmd, "od -i IOnode_client/IOnode_#%d > dump/client_result%d.dump", id + 1, id + 1);
	system(cmd);
	close(pip[0]);
	close(fd);
}

static void	parent4client(char *str)
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

static void	Client2Server4client(int i, int client2client[4][4][2])
{
	int	pid;
	int	pip[2];
	int	status;

	pipe(pip);
	pid = fork();
	if (pid == 0) // client
		do_comm_node4client(i, client2client, pip);
	else // server
	{
		do_io_node4client(i, pip);
		wait(&status);
		printf("[DEBUG] Client2Server, pid : %d, status: %d done\n", pid, status);
	}
}

static void	parallel_operation4client(void)
{
	int	client2client[4][4][2];
	int	i;
	int	j;
	char	cmd[1024];

	for (i = 0; i < 4; i++)
	{
		for (j = 0; j < 4; j++)
			pipe(client2client[i][j]);
	}
	for (i = 0; i < 4; i++)
	{
		int	pid;

		pid = fork();
		if (pid == 0)
		{
			Client2Server4client(i, client2client);
			exit(0);
		}
	}
	parent4client("parallel_operation");
	debug_result4client();
}

int client_oriented_io() {

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

	unlink("clientOrientedTime"); // clientOrientedTime은 client_oriented_io가 실행될 때마다 초기화된다.
	fd = open("clientOrientedTime", O_CREAT | O_WRONLY, 0644);
	ZERO = 0;
	write(fd, &ZERO, sizeof(int));
	write(fd, &ZERO, sizeof(int));
	write(fd, &ZERO, sizeof(int));
	close(fd);
	parallel_operation4client();

#ifdef TIMES
	gettimeofday(&etime, NULL);
	time_result = etime.tv_usec - stime.tv_usec;
	if (time_result < 0)
		time_result += SEC;
	printf("Client_oriented_io TIMES == %ld %ld %ld\n", (long)etime.tv_usec, (long)stime.tv_usec, (long)time_result);
#endif
	return (1);
}

// int	main()
// {
// 	client_oriented_io();
// }