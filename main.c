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

int numProcess = 0;
int process[1000];
int procStatus;
int numOfArgs = 0;
int allowBackground = 1;
bool isBackground = false;

struct sigaction SIGINTAction;	// SIGINT
struct sigaction SIGTSTPAction; // SIGTSTP


void handle_SIGSTP()
{
	char* status;
	int statusSize;
	char* prompt = ": ";

	if (allowBackground == 0) {
		status = "\nExiting foreground-only mode\n";
		statusSize = 40;
		allowBackground = -1;
		return;
	}
	else if (allowBackground == 1)
	{
		status = "\nEntering forground-only mode ($ is ignored)\n";
		statusSize = 60;
		allowBackground = 0;
		return;
	}
	else {
		status = "\nError: allowBackground is not 0 or 1\n";
		statusSize = 40;
		allowBackground = 1;
	}

	write(STDOUT_FILENO, status, statusSize);
	write(STDOUT_FILENO, prompt, 2);
}


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
	fflush(stdout);
	return 1;
}

void status_com(int* errorSig) {
	int errHold = 0;
	int signalHold = 0;
	int exitValue;

	waitpid(getpid(), &procStatus, 0);


	if (WIFSIGNALED(procStatus))		// reutrn status of abnormal child
	{
		signalHold = WTERMSIG(procStatus);
	}

	if (WIFEXITED(procStatus))
	{
		errHold = WEXITSTATUS(procStatus);		//return status of normally terminated child
	}

	exitValue = errHold + signalHold == 0 ? 0 : 1;

	if (signalHold == 0)
	{
		printf("exit value %d\n", exitValue);
	}
	else
	{
		*errorSig = 1;
		printf("terminated by signal %d\n", signalHold);
	}
	fflush(stdout);

}

void childProcess(char* args[])
{
	int i = 0;
	bool hasInputFile = false;
	bool hasOutputFile = false;
	char inputFile[MAX_ARG_LEN];
	char outputFile[MAX_ARG_LEN];

	while (args[i] != NULL)
	{
		if (strcmp(args[i], ">") == 0)
		{
			hasOutputFile = true;	// flag for output redirection
			args[i] = NULL;
			strcpy(outputFile, args[i + 1]);
			i++;

		}
		else if (strcmp(args[i], "<") == 0) {
			hasInputFile = true;	// flag for input redirection
			args[i] == NULL;
			strcpy(inputFile, args[i + 1]);
			i++;
		}
	}

	if (hasOutputFile == true)
	{
		int outFileDestination = 0;
		if ((outFileDestination = open(outputFile, O_RDONLY)) < 0) {
			fprintf(stderr, "cannot open %s for input \n", outputFile);
			fflush(stdout);
			exit(1);
		}
		dup2(outFileDestination, 0);
		close(outFileDestination);
	}


	if (hasInputFile == true)
	{
		int inFileDestination = 0;
		if ((inFileDestination = open(inputFile, O_RDONLY)) < 0) {
			fprintf(stderr, "cannot open %s for input \n", inputFile);
			fflush(stdout);
			exit(1);
		}
		dup2(inFileDestination, 0);
		close(inFileDestination);
	}

	// terminate forground process and give ctrl-c new handler
	if (isBackground == false)
	{
		SIGINTAction.sa_handler = SIG_DFL;
		sigaction(SIGINT, &SIGINTAction, NULL);
	}

}

void otherCommand(char* args[], int* errorSignal)
{
	int childStatus;
	isBackground = false;

	//fork a new process
	pid_t spawnPID = fork();
	process[numProcess] = spawnPID;
	numProcess++;

	if (strcmp(args[numOfArgs - 1], "&") == 0)
	{
		if (allowBackground == 1)
		{
			isBackground = true;
		}
		else {
			args[numOfArgs - 1] = NULL;
		}
	}	


	switch (spawnPID) {
	case -1:
		perror("fork()\n");
		//exit(1);
		break;

	case 0:
		childProcess(args);
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

	while ((spawnPID = waitpid(-1, &procStatus, WNOHANG)) > 0)
	{
		printf("background pid %d is done: ", spawnPID);
		fflush(stdout);
		status_com(errorSignal);
	}


}



void exit_com() {
	int i = 0;
	while (numProcess != 0)
	{
		kill(process[i], SIGTERM);
	}
	exit(1);
}


int main(int argc, char** argv)
{
	while (1)
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
			
		numOfArgs = size;

		//for (int j = 0; j < size; j++)
			//printf("%s\n", command_array[j]);


		
		// re-allocate the array size
		command_array = realloc(command_array, sizeof(char*) * (size + 1));

		int errorSig = 0;

		if (command_array[0] == NULL || strcmp(command_array[0], "#") == 0)
			continue;

		// cd
		if (strcmp(command_array[0], "cd") == 0)
		{
			printf("Entering CD call\n");
			cd_com(command_array);
		}
		// status
		else if (strcmp(command_array[0], "exit") == 0)
		{
			exit_com();

		}
		// exit
		else if (strcmp(command_array[0], "status") == 0)
		{
			status_com(&errorSig);
		}
		else
		{
			printf("arg1 = %s\n", command_array[0]);
			printf("arg2 = %s\n", command_array[1]);
			printf("arg3 = %s\n", command_array[2]);
			otherCommand(command_array, &errorSig);
		}




		// dont forget to free command_array[i] and commandArray
	}

}


