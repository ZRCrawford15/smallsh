#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <time.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>

#define MAX_ARG_LEN 2048
#define MAX_ARGS 512



int cd_com(char** args)
{
	if (args[1] == NULL)
	{
		fprintf(stderr, "lsh: expected argument to \"cd\"\n");
	} 
	else {
		if (chdir(args[1]) != 0)
			perror("lsh");
	}
	return 1;
}




int otherCommand(char* args[])
{
	int childStatus;

	//fork a new process
	pid_t spawnPID = fork();

	switch (spawnPID) {
	case -1:
		perror("fork()\n");
		//exit(1);
		break;

	case 0:
		execvp(args[0], args);
		perror("execvp");
		//exit(2);
		break;

	default:
		// wait for childs process
		spawnPID = waitpid(spawnPID, &childStatus, 0);
		//exit(0);
		break;
	}
	return 1;
}









int main(int argc, char** argv)
{
	while (true)
	{
		char* line = malloc(sizeof(char*) * MAX_ARG_LEN);
		char** command_array = malloc(sizeof(char*) * MAX_ARGS);
		char* saveptr;
		int size = 0;
		int status;


		printf(": ");
		fgets(line, MAX_ARG_LEN, stdin);
		line[strlen(line) - 1] = '\0';

		char* token = strtok_r(line, " ", &saveptr);


		while (token)
		{
			char* commandElement = malloc(sizeof(char *) * MAX_ARG_LEN);
			strncpy(commandElement, token, strlen(token) + 1);

			command_array[size] = commandElement;

			token = strtok_r(NULL, " ", &saveptr);
			size++;
		}
			
		// add null to the end of the array


		//for (int j = 0; j < size; j++)
			//printf("%s\n", command_array[j]);


		
		// re-allocate the array size
		command_array = realloc(command_array, sizeof(char*) * (size + 1));

		// cd
		if (strcmp(command_array[0], "cd") == 0)
		{
			printf("Entering CD call\n");
			cd_com(command_array);
		}
		// status
		if (strcmp(command_array[0], "cd") == 0)
		{
			printf("Entering CD call\n");
			cd_com(command_array);
		}
		// exit
		if (strcmp(command_array[0], "cd") == 0)
		{
			printf("Entering CD call\n");
			cd_com(command_array);
		}
		else
		{
			printf("arg1 = %s\n", command_array[0]);
			printf("arg2 = %s\n", command_array[1]);
			printf("arg3 = %s\n", command_array[2]);
			status = otherCommand(command_array);
		}




		// dont forget to free command_array[i] and commandArray
	}
}


