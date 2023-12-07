#include "mytest.h"
#include"create.h"
#include"server.h"
#include"client.h"

int main() {
	int     fd;
	int		i;
	char	cmd[1024];

	printf("create_source_data\n");
	create_source_data(); // one-time execution
	client_oriented_io();
	sleep(3);
	server_oriented_io();
	sleep(3);
	printf("\n\n\n");
	printf("Client_oriented_io time result\n");

	fd = open("clientOrientedTime", O_RDONLY);
	int	clientTime[4];

	i = 0;
	while ((read(fd, &clientTime[i], sizeof(int)) > 0))
	{
		if (i == COMM)
			printf("communicate TIMES: %ld\n", (long)clientTime[i]);
		else if (i == COMP)
			printf("compute TIMES: %ld\n", (long)clientTime[i]);
		else if (i == IO)
			printf("IO TIMES: %ld\n", (long)clientTime[i]);
		i++;
	}
	close(fd);

	printf("\n\n\n");
	printf("Server_oriented_io time result\n");
	fd = open("serverOrientedTime", O_RDONLY);
	int     serverTime[4];

	i = 0;
	while ((read(fd, &serverTime[i], sizeof(int)) > 0))
	{
		if (i == COMM)
			printf("communicate TIMES: %ld\n", (long)serverTime[i]);
		else if (i == COMP)
			printf("compute TIMES: %ld\n", (long)serverTime[i]);
		else if (i == IO)
			printf("IO TIMES: %ld\n", (long)serverTime[i]);
		i++;
	}
	close(fd);
}
