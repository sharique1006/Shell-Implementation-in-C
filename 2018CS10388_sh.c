#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>  
#include <fcntl.h>
#include <readline/readline.h>
#include <readline/history.h>

#define MAXCOM 1000
#define MAXLIST 100

//Functions to implement various commands.
int myshell_cd(char **args);
int myshell_pwd(char **args);
int myshell_mkdir(char **args);
int myshell_rmdir(char **args);
int myshell_output(char **args);

//Commands made - cd, pwd, mkdir, rmdir, exit
char *builtin_commands[] = {"cd", "pwd", "mkdir", "rmdir"};

int (*builtin_func[]) (char **) = {&myshell_cd, &myshell_pwd, &myshell_mkdir, &myshell_rmdir};

int myshell_total_commands() {
	return sizeof(builtin_commands)/sizeof(char *);
}

//Function to change directory
int myshell_cd(char **args) {
	if(args[1] == NULL) {
		fprintf(stderr, "myshell: expected argument to \"cd\"\n");
	}
	else {
		if(chdir(args[1]) != 0) {
			perror("cd");
		}
	}
	return 1;
} 

//Function to print the path of current directory.
int myshell_pwd(char **args) {
	char *buf = (char *)args;
	char *ptr;
	
	ptr = getcwd(buf, 0777);
	printf("%s\n", ptr);
	return 1;
}

//Function to create a new directory if not existing.
int myshell_mkdir(char **args) {
	int x1 = mkdir(args[1], 0777);
	if(x1 < 0) { 
		perror("mkdir");
	}
	return 1;
}

//Function to remove an existing directory
int myshell_rmdir(char **args) {
	int x1 = rmdir(args[1]);
	if(x1 < 0) {
		perror("rmdir");
	}
	return 1;
}

//Read the inputs given by user.
int takeInput(char* str) { 
    	char* buf; 
  
    	buf = readline("myshell>>> ");
    	if (strlen(buf) != 0) { 
        	add_history(buf); 
        	strcpy(str, buf); 
        	return 0; 
    	} 
	else { 
        	return 1; 
    	} 
}

//Function to execute the commands where pipe is absent.
void myshell_execute(char** parsed) { 
	
	//Call fork to make a duplicate of the process so that it starts running bothh child & parent.
    	pid_t pid = fork();  		
	if (pid == -1) { 
        	printf("\nFailed forking child.."); 
        	return; 
    	} 
	else if (pid == 0) {
		if(parsed[1] == NULL) {		
			if(execvp(parsed[0], parsed) == -1) {
				printf("\nCould not execute command");
			}
			exit(0);
		}
		else {
			int in, out;
			if(strcmp(parsed[1], "<") == 0) {
				//open the file to be read.
				in = open(parsed[2], O_RDONLY);
				if(in < 0) {
					perror("in");
					exit(1);
				}
				//Use dup2(filename, 0) to read the input from a file & display on terminal.
				dup2(in, 0);
				close(in);
			}
			if(strcmp(parsed[1], ">") == 0) {
				//create the file where output is to be redirected.
				out = open(parsed[2], O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
				if(out < 0) {
					perror("out");
					exit(1);
				}
				//Use dup2(outfile, 1) to redirect the stdout to outfile.
				dup2(out, 1);
				close(out);
			}
			if(execvp(parsed[0], parsed) == -1) {
				perror("myshell");
			}
			exit(0);
		}
	}
	else {
		wait(NULL);
		return;
	}
}

//Function to execute the commands where pipe is present.
void myshell_execute_piped(char** parsed, char** parsedpipe) {
	char* s1 = "./";
	char* s2 = (char*)parsed[0];
	char* r1 = malloc(strlen(s1) + strlen(s2) + 1);
	char* r2 = malloc(strlen(s1) + strlen(s2) + 1);
	strcpy(r1, s1);
	strcpy(r2, s2);
	strcat(r1, r2);
	parsed[0] = r1;

	char* s3 = "./";
	char* s4 = (char*)parsedpipe[0];
	char* r3 = malloc(strlen(s3) + strlen(s4) + 1);
	char* r4 = malloc(strlen(s3) + strlen(s4) + 1);
	strcpy(r3, s3);
	strcpy(r4, s4);
	strcat(r3, r4);
	parsedpipe[0] = r3;
	//Here two commands are to be executed, use the pipe command to pass the information from one process to another. Here 0 is read, 1 is write. 
    	int pipefd[2];  
    	pid_t p1, p2; 
  
    	if (pipe(pipefd) < 0) { 
        	printf("\nPipe could not be initialized"); 
        	return; 
    	} 
	//fork to create a new process
    	p1 = fork(); 
    	if (p1 < 0) { 
        	printf("\nCould not fork"); 
        	return; 
    	} 	
  
    	if (p1 == 0) {  
		//Execute child1.
        	close(pipefd[0]); 
        	dup2(pipefd[1], STDOUT_FILENO); 
        	close(pipefd[1]); 
  
        	if (execvp(parsed[0], parsed) < 0) { 
            		printf("\nCould not execute command 1.."); 
            		exit(0); 
        	} 
    	} 
	else {  
        	p2 = fork(); 
  
		if (p2 < 0) { 
			printf("\nCould not fork"); 
            		return; 
		}
		//Execute childc 
		if (p2 == 0) { 
			close(pipefd[1]); 
			dup2(pipefd[0], STDIN_FILENO); 
			close(pipefd[0]); 
            		if (execvp(parsedpipe[0], parsedpipe) < 0) { 
                		printf("\nCould not execute command 2.."); 
                		exit(0); 
            		} 
        	} 
		else {  
			//wait for child processes.
            		wait(NULL); 
            		wait(NULL); 
        	} 
    	} 
} 

//Function to call the builtin functions as cd, pwd, mkdir, rmdir.
int BuiltinFunc(char** args) {
	int ret = 0;
	for (int i = 0; i < myshell_total_commands(); i++) {
    		if (strcmp(args[0], builtin_commands[i]) == 0) {
			ret = 1;
      			return (*builtin_func[i])(args);
    		}
	}
	return ret;
}

//Function to parse pipe & seperate the cmd commands as different arguments.
int ParsebySymbol(char* st, char **stripe, char* c) {
	for (int i = 0; i < 2; i++) { 
		stripe[i] = strsep(&st, c);
		if(stripe[i] == NULL) 
			break;
	}
	if(stripe[1] == NULL)
		return 0;
	else {
		return 1;
	}
}

//Function to parse Space between commands & arguments, eg: mkdir hello - mkdir is command & hello is argument.
void ParsebySpace(char* st, char** parsed) {
	for (int i = 0; i < MAXLIST; i++) {
		parsed[i] = strsep(&st, " ");
		if(parsed[i] == NULL)
			break;
		if(strlen(parsed[i]) == 0)
			i--;
	}
}

//Main to keep running the shell till user calls exit.
int main(int argc, char **argv) {
	char inp[MAXCOM], *parsedArgs[MAXLIST]; 
    	char* parsedArgsPiped[MAXLIST]; 
    	int checkpipe = 0; 
	char* strpd[2];
	int is_piped = 0;
	
	while(1) {
		//Take the input from user
		if (takeInput(inp)) 
            		continue;

		//If inp is exit break the loop & exit your shell.
		if(strcmp(inp, "exit") == 0)
			break;
		
		//Check if pipe is present in the command given by user, i.e "|" if present
		is_piped = ParsebySymbol(inp, strpd, "|");
		if(is_piped == 1) {
			ParsebySpace(strpd[0], parsedArgs);
			ParsebySpace(strpd[1], parsedArgsPiped);
		}
		else {
			ParsebySpace(inp, parsedArgs);
		}

		//if the parsedArgs is one of the builtin functions execute it by calling those functions.
		if(BuiltinFunc(parsedArgs))
			checkpipe = 0;
		else 
			checkpipe = 1 + is_piped;
	
		//If pipe is absent just execute simply by execvp.
		if(checkpipe == 1) {
			if(strcmp(parsedArgs[0], "ls") == 0 || strcmp(parsedArgs[0], "gcc") == 0){
				pid_t pid = fork();
				if(pid == 0) {
					if(execvp(parsedArgs[0], parsedArgs) == -1) {
							printf("\nCould not execute command");
					}
					exit(0);
				}
				else {
					wait(NULL);
				}
			}
			else if(parsedArgs[1] == NULL && strstr(parsedArgs[0], ">") == NULL && strstr(parsedArgs[0], "<") == NULL) {
				char* Str1 = "./";
				char* Str2 = (char*)parsedArgs[0];
				char* result1 = malloc(strlen(Str1) + strlen(Str2) + 1);
				char* result2 = malloc(strlen(Str1) + strlen(Str2) + 1);
				strcpy(result1, Str1);
				strcpy(result2, Str2);
				strcat(result1, result2);
				parsedArgs[0] = result1;
				myshell_execute(parsedArgs);
			}
			else if(parsedArgs[1] ==  NULL && strchr(parsedArgs[0], '>') && strchr(parsedArgs[0], '<')) {
				char* stripdd[2];
				char* cmd[MAXLIST];
				char* remaining[MAXLIST];
				char* infile[MAXLIST];
				char* outfile[MAXLIST];
				int isLsymbol = 0;
				isLsymbol = ParsebySymbol(parsedArgs[0], stripdd, "<");
				if(isLsymbol == 1) {
					ParsebySpace(stripdd[0], cmd);
					ParsebySpace(stripdd[1], remaining);
				}
				char* strippp[2];
				int isGsymbol = ParsebySymbol(remaining[0],strippp, ">");
				if(isGsymbol == 1) {
					ParsebySpace(strippp[0], infile);
					ParsebySpace(strippp[1], outfile);
				}
				char* Str1 = "./";
				char* Str2 = (char*)cmd[0];
				char* result1 = malloc(strlen(Str1) + strlen(Str2) + 1);
				char* result2 = malloc(strlen(Str1) + strlen(Str2) + 1);
				strcpy(result1, Str1);
				strcpy(result2, Str2);
				strcat(result1, result2);
				cmd[0] = result1;
				pid_t pid = fork();
				if(pid == 0) {
					int outf;
					outf = open(outfile[0], O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
					dup2(outf, 1);
					close(outf);
					int inf;
					inf = open(infile[0], O_RDONLY);
					dup2(inf, 0);
					close(inf);
					if(execvp(cmd[0], parsedArgs) == -1) { 
						printf("%s\n", "failed");
					}
					exit(0);
				}
				else {
					wait(NULL);
				}
				  
			}
			else if(parsedArgs[1] == NULL && strchr(parsedArgs[0], '>')) {
				int isGsymbol = 0;
				char* stripdd[2];
				char* cmd[MAXLIST];
				char* file[MAXLIST];
				isGsymbol = ParsebySymbol(parsedArgs[0], stripdd, ">");
				if(isGsymbol == 1) {
					ParsebySpace(stripdd[0], cmd);
					ParsebySpace(stripdd[1], file);
					char* Str1 = "./";
					char* Str2 = (char*)cmd[0];
					char* result1 = malloc(strlen(Str1) + strlen(Str2) + 1);
					char* result2 = malloc(strlen(Str1) + strlen(Str2) + 1);
					strcpy(result1, Str1);
					strcpy(result2, Str2);
					strcat(result1, result2);
					cmd[0] = result1;
					pid_t pid = fork();
					if(pid == 0) {
						int outf;
						outf = open(file[0], O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
						dup2(outf, 1);
						close(outf);
						execvp(cmd[0], cmd);
						exit(0);
					}
					else {
						wait(NULL);
					}
				}
				
			}
			else if(parsedArgs[1] == NULL && strchr(parsedArgs[0], '<')) {
				int isLsymbol = 0;
				char* stripdd[2];
				char* cmd[MAXLIST];
				char* file[MAXLIST];
				isLsymbol = ParsebySymbol(parsedArgs[0], stripdd, "<");
				if(isLsymbol == 1) {
					ParsebySpace(stripdd[0], cmd);
					ParsebySpace(stripdd[1], file);
					char* Str1 = "./";
					char* Str2 = (char*)cmd[0];
					char* result1 = malloc(strlen(Str1) + strlen(Str2) + 1);
					char* result2 = malloc(strlen(Str1) + strlen(Str2) + 1);
					strcpy(result1, Str1);
					strcpy(result2, Str2);
					strcat(result1, result2);
					cmd[0] = result1;
					pid_t pid = fork();
					if(pid == 0) {
						int inf;
						inf = open(file[0], O_RDONLY);
						dup2(inf, 0);
						close(inf);
						execvp(cmd[0], cmd);
						exit(0);
					}
					else {
						wait(NULL);
					}
				}
				
			}
			else if(parsedArgs[2] == NULL && strchr(parsedArgs[1], '>')) {
				int isGsymbol = 0;
				char* stripdd[2];
				char* sym[MAXLIST];
				char* file[MAXLIST];
				isGsymbol = ParsebySymbol(parsedArgs[1], stripdd, ">");
				if(isGsymbol == 1) {
					ParsebySpace(stripdd[0], sym);
					ParsebySpace(stripdd[1], file);
					char* Str1 = "./";
					char* Str2 = (char*)parsedArgs[0];
					char* result1 = malloc(strlen(Str1) + strlen(Str2) + 1);
					char* result2 = malloc(strlen(Str1) + strlen(Str2) + 1);
					strcpy(result1, Str1);
					strcpy(result2, Str2);
					strcat(result1, result2);
					parsedArgs[0] = result1;
					pid_t pid = fork();
					if(pid == 0) {
						int outf;
						outf = open(file[0], O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
						dup2(outf, 1);
						close(outf);
						execvp(parsedArgs[0], parsedArgs);
						exit(0);
					}
					else {
						wait(NULL);
					}
				}
			}
			else if(parsedArgs[2] == NULL && strchr(parsedArgs[0], '>')) {
				int isGsymbol = 0;
				char* stripdd[2];
				char* sym[MAXLIST];
				char* cmd[MAXLIST];
				isGsymbol = ParsebySymbol(parsedArgs[0], stripdd, ">");
				if(isGsymbol == 1) {
					ParsebySpace(stripdd[0], cmd);
					ParsebySpace(stripdd[1], sym);
					char* Str1 = "./";
					char* Str2 = (char*)cmd[0];
					char* result1 = malloc(strlen(Str1) + strlen(Str2) + 1);
					char* result2 = malloc(strlen(Str1) + strlen(Str2) + 1);
					strcpy(result1, Str1);
					strcpy(result2, Str2);
					strcat(result1, result2);
					cmd[0] = result1;
					pid_t pid = fork();
					if(pid == 0) {
						int outf;
						outf = open(parsedArgs[1], O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
						dup2(outf, 1);
						close(outf);
						execvp(cmd[0], parsedArgs);
						exit(0);
					}
					else {
						wait(NULL);
					}
				}
			}
			else if(parsedArgs[2] == NULL && strchr(parsedArgs[0], '<')) {
				int isLsymbol = 0;
				char* stripdd[2];
				char* sym[MAXLIST];
				char* cmd[MAXLIST];
				isLsymbol = ParsebySymbol(parsedArgs[0], stripdd, "<");
				if(isLsymbol == 1) {
					ParsebySpace(stripdd[0], cmd);
					ParsebySpace(stripdd[1], sym);
					char* Str1 = "./";
					char* Str2 = (char*)cmd[0];
					char* result1 = malloc(strlen(Str1) + strlen(Str2) + 1);
					char* result2 = malloc(strlen(Str1) + strlen(Str2) + 1);
					strcpy(result1, Str1);
					strcpy(result2, Str2);
					strcat(result1, result2);
					cmd[0] = result1;
					pid_t pid = fork();
					if(pid == 0) {
						int inf;
						inf = open(parsedArgs[1], O_RDONLY);
						dup2(inf, 0);
						close(inf);
						execvp(cmd[0], parsedArgs);
						exit(0);
					}
					else {
						wait(NULL);
					}
				}
			}
			else if(parsedArgs[2] == NULL && strchr(parsedArgs[1], '<')) {
				int isLsymbol = 0;
				char* stripdd[2];
				char* sym[MAXLIST];
				char* file[MAXLIST];
				isLsymbol = ParsebySymbol(parsedArgs[1], stripdd, "<");
				if(isLsymbol == 1) {
					ParsebySpace(stripdd[0], sym);
					ParsebySpace(stripdd[1], file);
					char* Str1 = "./";
					char* Str2 = (char*)parsedArgs[0];
					char* result1 = malloc(strlen(Str1) + strlen(Str2) + 1);
					char* result2 = malloc(strlen(Str1) + strlen(Str2) + 1);
					strcpy(result1, Str1);
					strcpy(result2, Str2);
					strcat(result1, result2);
					parsedArgs[0] = result1;
					pid_t pid = fork();
					if(pid == 0) {
						int inf;
						inf = open(file[0], O_RDONLY);
						dup2(inf, 0);
						close(inf);
						execvp(parsedArgs[0], parsedArgs);
						exit(0);
					}
					else {
						wait(NULL);
					}
				}
			}
			else {
				char* Str1 = "./";
				char* Str2 = (char*)parsedArgs[0];
				char* result1 = malloc(strlen(Str1) + strlen(Str2) + 1);
				char* result2 = malloc(strlen(Str1) + strlen(Str2) + 1);
				strcpy(result1, Str1);
				strcpy(result2, Str2);
				strcat(result1, result2);
				parsedArgs[0] = result1;
				myshell_execute(parsedArgs);
			}
		}
		//If pipe is present use pipe command & fork() the process & execute using execvp.
		if(checkpipe == 2) 
			myshell_execute_piped(parsedArgs, parsedArgsPiped);  
	}
	return 0;
}








