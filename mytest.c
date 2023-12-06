#include "mytest.h"

int main() {

	create_source_data(); // one-time execution

        client_oriented_io();
        // server_oriented_io();
        printf("Client_oriented_io time result\n");
#ifdef TIMES
	fd = open("clientOrientedTime", O_RDONLY);
	int	clientTime[4];
	int	i;

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
#endif
        printf("Server_oriented_io time result\n");
}
