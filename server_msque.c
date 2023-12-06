#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>

//틀만 짜놓음. 
//pid[ ] 0 1 2 3 이 서버 4 5 6 7이 클라.

struct mymsgbuf {
	long mtype;
	int mdata[256];
	int sender;
};

void server(int n) {

}

void client(int n) {
	
}

int main() {
	pid_t pid[8];

	pid[0] = getpid();
	switch (fork()) {
	case -1:
		perror("fork1");
		exit(1);
	case 0:
		pid[1] = getpid();
	}

	switch (fork()) {
	case -1:
		perror("fork2");
		exit(1);
	case 0:
		if (getppid() == pid[0])
			pid[2] = getpid();
		else if (getppid() == pid[1])
			pid[3] = getpid();
	}

	switch (fork()) {
	case -1:
		perror("fork3");
		exit(1);
	case 0:
		if (getppid() == pid[0])
			pid[4] = getpid();
		else if (getppid() == pid[2])
			pid[5] = getpid();
		else if (getppid() == pid[1])
			pid[6] = getpid();
		else if (getppid() == pid[3])
			pid[7] = getpid();
	}

	for (int i = 0; i < 8; i++) {
		if (getpid() != pid[i])
			continue;
		if (i < 4)
			server(i);
		else
			client(i);
	}	
}
