#include "mytest.h"
#include "project.h"
#include <sys/types.h>
#include <sys/stat.h>

void debug_result(void)
{
    int		i;
	char	cmd[1024];
	int		fd;
	int		buffer[MB];

	for (i = 1; i <= 4; i++)
	{
		sprintf(cmd, "od -i IOnode/IOnode_#%d | more", i);
		system(cmd);
	}
	for (i = 1; i <= 4; i++)
	{
		sprintf(cmd, "IOnode/IOnode_#%d", i);
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
int ft_compare(const void *a, const void *b)
{
    int num1 = *(int *)a;
    int num2 = *(int *)b;

    if (num1 < num2)
        return (-1);
    if (num1 > num2)
        return (1);
    return (0);
}

void do_compute_node(int *dump)
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

void comm_init(int id, char *fifo_path, int data[MB])
{
    // 명명된 파이프 생성
    mkfifo(fifo_path, 0666);

    int fd = open(fifo_path, O_RDONLY);
    if (fd < 0)
    {
        perror("Error opening FIFO for reading");
        exit(1);
    }

    // 데이터 읽기
    read(fd, data, MB * sizeof(int));
    close(fd);
}

void send_server(char *fifo_path, int dump[MB])
{
    int fd = open(fifo_path, O_WRONLY);
    if (fd < 0)
    {
        perror("Error opening FIFO for writing");
        exit(1);
    }

    // 데이터 쓰기
    write(fd, dump, MB * sizeof(int));
    close(fd);

    // 파이프 제거
    unlink(fifo_path);
}

void writeTimeAdvLock(int index, int time_result)
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

void do_comm_node(int id, char *client_fifo_path, char *server_fifo_path)
{
    int data[MB];
    int dump[MB];
    int ret;
    int i;
    int dump_idx;

    comm_init(id, client_fifo_path, data);
    ret = 0;
    dump_idx = -1;

#ifdef TIMES
    int time_result;
    struct timeval stime, etime;

    gettimeofday(&stime, NULL);
#endif

    // 서버로 데이터를 보낼 명명된 파이프 생성
    mkfifo(server_fifo_path, 0666);
    int server_fd = open(server_fifo_path, O_WRONLY);
    if (server_fd < 0)
    {
        perror("Error opening server FIFO for writing");
        exit(1);
    }

    // 모든 데이터를 서버로 전송
    for (i = 0; i < MB; i++)
    {
        ret = write(server_fd, &data[i], sizeof(int));
    }

    // 파이프 닫기
    close(server_fd);

    // 서버에서 정렬된 데이터를 읽음
    int client_fd = open(client_fifo_path, O_RDONLY);
    if (client_fd < 0)
    {
        perror("Error opening client FIFO for reading");
        exit(1);
    }

    for (i = 0; i < MB; i++)
    {
        ret = read(client_fd, &dump[i], sizeof(int));
    }

    // 파이프 닫기
    close(client_fd);

#ifdef TIMES
    gettimeofday(&etime, NULL);
    time_result = (etime.tv_usec - stime.tv_usec);
    writeTimeAdvLock(COMM, time_result);
#endif

    do_compute_node(dump); // 정렬하기
    send_server(client_fifo_path, dump); // server에게 전달
}

void do_io_node(int id, char *io_fifo_path)
{
    // I/O 노드가 사용할 명명된 파이프 생성
    mkfifo(io_fifo_path, 0666);

    int ret;
    int chunk[8];
    char file_name[25];
    int fd;

    sprintf(file_name, "IOnode/IOnode_#%d", id + 1);
    printf("file_name : [%s]\n", file_name);

    // I/O 노드가 사용할 파일 생성
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

    // 파이프에서 데이터 읽어 파일에 쓰기
    int io_fd = open(io_fifo_path, O_RDONLY);
    if (io_fd < 0)
    {
        perror("Error opening IO FIFO for reading");
        exit(1);
    }

    while ((ret = read(io_fd, chunk, sizeof(int) * 8)) > 0)
        write(fd, chunk, sizeof(int) * 8);

    // 파이프 닫기
    close(io_fd);
    close(fd);

    // 명명된 파이프 제거
    unlink(io_fifo_path);
}

void parent(char *str)
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

void Server(char *io_fifo_path)
{
    int status;
    int pid;

    pid = fork();

    if (pid == 0) // server
    {
        do_io_node(0, io_fifo_path);
        wait(&status);
        printf("[DEBUG] Server, pid : %d, status: %d done\n", pid, status);
    }
    else // clients
    {
        char client_fifo_path[1024];
        char server_fifo_path[1024];
        sprintf(client_fifo_path, "/tmp/client_fifo_%d", getpid());
        sprintf(server_fifo_path, "/tmp/server_fifo_%d", getpid());

        // 클라이언트는 서버와 통신할 명명된 파이프 경로를 전달
        do_comm_node(pid, client_fifo_path, server_fifo_path);
    }
}

void parallel_operation(char *client_fifo_path, char *server_fifo_path, char *io_fifo_path)
{
    int client2client[4][4][2];
    int i, j;

    for (i = 0; i < 4; i++)
    {
        for (j = 0; j < 4; j++)
        {
            // 클라이언트 간 통신을 위한 명명된 파이프 생성
            mkfifo(client2client[i][j][0], 0666);
            mkfifo(client2client[i][j][1], 0666);
        }
    }

    for (i = 0; i < 4; i++)
    {
        int pid = fork();
        if (pid == 0)
        {
            // 자식 프로세스는 클라이언트로 동작
            close(server_fifo_path);

            // 각 클라이언트에서 서버로 통신할 명명된 파이프 경로 설정
            char client_fifo_path[1024];
            sprintf(client_fifo_path, "/tmp/client_fifo_%d", getpid());

            // 서버로 전달할 데이터 생성 (임의의 데이터로 가정)
            int data[MB];
            for (int k = 0; k < MB; k++)
            {
                data[k] = k; // 임의의 데이터 생성
            }

            // 클라이언트에서 서버로 통신
            comm_init(i, client_fifo_path, data);
            send_server(client_fifo_path, data);

            close(server_fifo_path);

            // 클라이언트 간 통신을 위한 명명된 파이프 제거
            for (j = 0; j < 4; j++)
            {
                unlink(client2client[i][j][0]);
                unlink(client2client[i][j][1]);
            }

            exit(0);
        }
    }

    // 부모 프로세스는 서버로 동작
    for (i = 0; i < 4; i++)
    {
        close(client_fifo_path);
    }

    // 서버에서 클라이언트로 통신할 명명된 파이프 경로 설정
    sprintf(server_fifo_path, "/tmp/server_fifo_%d", getpid());

    // 서버에서 각 클라이언트로 데이터 전송
    for (i = 0; i < 4; i++)
    {
        int pid = fork();
        if (pid == 0)
        {
            // 자식 프로세스는 클라이언트로 동작
            close(server_fifo_path);

            // 각 클라이언트에서 서버로 통신할 명명된 파이프 경로 설정
            char *client_fifo_path;
            sprintf(client_fifo_path, "/tmp/client_fifo_%d", getpid());

            // 서버에서 전달받은 데이터를 클라이언트에게 전송
            int dump[MB];
            send_server(server_fifo_path, dump);

            close(client_fifo_path);

            // 클라이언트 간 통신을 위한 명명된 파이프 제거
            for (j = 0; j < 4; j++)
            {
                unlink(client2client[i][j][0]);
                unlink(client2client[i][j][1]);
            }

            exit(0);
        }
    }

    // 부모 프로세스는 I/O 노드로 동작
    for (i = 0; i < 4; i++)
    {
        int pid = fork();
        if (pid == 0)
        {
            // 자식 프로세스는 I/O 노드로 동작
            do_io_node(i, io_fifo_path);
            exit(0);
        }
    }

    // 부모 프로세스는 각 자식 프로세스의 종료를 기다림
    parent("parallel_operation");

    // 클라이언트 간 통신을 위한 명명된 파이프 제거
    for (i = 0; i < 4; i++)
    {
        for (j = 0; j < 4; j++)
        {
            unlink(client2client[i][j][0]);
            unlink(client2client[i][j][1]);
        }
    }
}

int server_oriented_io()
{
#ifdef TIMES
    struct timeval stime, etime;
    int time_result;
#endif

    char client_fifo_path[1024];
    char server_fifo_path[1024];
    char io_fifo_path[1024];

    // 각 프로세스에 대한 고유한 경로 생성
    sprintf(client_fifo_path, "/tmp/client_fifo_%d", getpid());
    sprintf(server_fifo_path, "/tmp/server_fifo_%d", getpid());
    sprintf(io_fifo_path, "/tmp/io_fifo_%d", getpid());

    // 서버 프로세스 실행
    Server(io_fifo_path);

    // 병렬 연산 수행
    parallel_operation(client_fifo_path, server_fifo_path, io_fifo_path);

#ifdef TIMES
    gettimeofday(&etime, NULL);
    time_result = etime.tv_usec - stime.tv_usec;
    if (time_result < 0)
        time_result += SEC;
    printf("Server_oriented_io TIMES == %ld %ld %ld\n", (long)etime.tv_usec, (long)stime.tv_usec, (long)time_result);
#endif

    return 1;
}

int main()
{
    server_oriented_io();

    return 0;
}
