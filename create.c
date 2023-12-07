#include"mytest.h"
#include"create.h"

void	debug_file(void)
{
	int		i;
	char	cmd[1024];

	for (i = 0; i < 4; i++)
	{
		sprintf(cmd, "od -i client_data/p%d.dat | more", i + 1);
		system(cmd);
	}
	for (i = 0; i < 4; i++)
	{
		sprintf(cmd, "od -i server_data/p%d.dat | more", i + 1);
		system(cmd);
	}
}

void    child_proc(int id)
{
	char	file_name[1024];
	int		fd1;
	int		fd2;
	int		data[MB];
	int		ret;
	int		i;

	sprintf(file_name, "client_data/p%d.dat", id);
	fd1 = open(file_name, O_CREAT | O_WRONLY | O_TRUNC, 0644);
	sprintf(file_name, "server_data/p%d.dat", id);
	fd2 = open(file_name, O_CREAT | O_WRONLY | O_TRUNC, 0644);
	if (fd1 == -1 || fd2 == -1)
	{
		perror("fd fail");
		exit(1);
	}
	for (i = 0; i < MB; i++)
		data[i] = 4 * i + id;
	if ((ret = write(fd1, data, MB * sizeof(int))) != MB * 4)
	{
		perror("client write fail");
		exit(1);
	}
	if ((ret = write(fd2, data, MB * sizeof(int))) != MB * 4)
	{
		perror("server write fail");
		exit(1);
	}
	exit(0);
}

int create_source_data() {

	/* create per-process, distrinuted input data. The size of each proc is of 1MB (256K of integer).
	create one time, if possible. After creating, comment out to program the remaining functions. */

	printf("**Distribute input data across processes.\n");
	
	int     i;
	int     pid;

	i = 0;
	for (i = 1; i <= 4; i++)
	{
		pid = fork();
		if (pid == -1)
		{
			perror("fork fail");
			exit(1);
		}
		else if (pid == 0)
			child_proc(i);
	}
	i = 0;
	int     status;
	while (i < 4)
	{
		pid = waitpid(-1, &status, 0);
		if (pid == -1)
		{
			perror("pid error");
			exit(1);
		}
		i++;
	}
	// debug_file();
	return (0);
}

// int	main(void)
// {
// 	create_source_data();
// 	return (0);
// }
