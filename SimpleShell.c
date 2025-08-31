#include  <stdio.h>
#include  <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include<signal.h>
#include<fcntl.h>
#include <stdbool.h>


#define MAX_INPUT_SIZE 1024
#define MAX_TOKEN_SIZE 64
#define MAX_NUM_TOKENS 64

/* Splits the string by space and returns the array of tokens
*
*/
char **tokenize(char *line)
{
  char **tokens = (char **)malloc(MAX_NUM_TOKENS * sizeof(char *));
  char *token = (char *)malloc(MAX_TOKEN_SIZE * sizeof(char));
  int i, tokenIndex = 0, tokenNo = 0;

  for(i =0; i < strlen(line); i++){

    char readChar = line[i];

    if (readChar == ' ' || readChar == '\n' || readChar == '\t'){
      token[tokenIndex] = '\0';
      if (tokenIndex != 0){
	tokens[tokenNo] = (char*)malloc(MAX_TOKEN_SIZE*sizeof(char));
	strcpy(tokens[tokenNo++], token);
	tokenIndex = 0; 
      }
    } else {
      token[tokenIndex++] = readChar;
    }
  }
 
  free(token);
  tokens[tokenNo] = NULL ;
  return tokens;
}


int main(int argc, char* argv[]) {
	char  line[MAX_INPUT_SIZE];            
	char  **tokens;              
	int i;
	int bgList[200];
	int bg_cnt=0;

	while(1) {		
		int stat,pid_w;
		while((pid_w=waitpid(-1,&stat,WNOHANG))>0){
			printf("Process with PID : %d completed\n", pid_w);
			int i=0;
			for(i=0;i<bg_cnt;i++){
				if(bgList[i]==pid_w)break;
			}
			for(i;i<bg_cnt-1;i++){
				bgList[i]=bgList[i+1];
			}
			bg_cnt--;
		}	

/*-----Printing the PWD-----*/
		int size_buff=200;
		char* buff=malloc(size_buff);
		if(getcwd(buff, size_buff)<0){error("getcwd failed");}
		else printf("%s ", buff);
		free(buff);
/*-----Printing the PWD-----*/


		/* BEGIN: TAKING INPUT */
		bzero(line, sizeof(line));
		printf("$ ");
/*----- Registering CTRL+D -----*/
		if(scanf("%[^\n]", line)==EOF){
			printf("\nExiting shell.\n");
            break;
		}
/*----- Registering CTRL+D -----*/
		getchar();

		/* END: TAKING INPUT */


		line[strlen(line)] = '\n'; //terminate with new line
		tokens = tokenize(line);

/*-----Very Very important step-----*/
		if(tokens[0]==NULL){
			free(tokens);
			continue;
		}
/*-----Very Very important step-----*/

		/*-----Implementing cd-----*/
		if(strcmp(tokens[0],"cd")==0){
			if(tokens[1]==NULL)printf("Missing Arguments");
			else if(chdir(tokens[1])<0){
				for(i=0;tokens[i]!=NULL;i++){
					free(tokens[i]);
				}
				free(tokens);
				error("chdir failed");
			}
			else{
				for(i=0;tokens[i]!=NULL;i++){
					free(tokens[i]);
				}
				free(tokens);
				continue;
			}
		}
/*-----Implementing cd-----*/

/*Scanning for &*/

		bool bg=false;
		int token_itr=0;
		while(tokens[token_itr]!=NULL){
			token_itr++;
		}
		if(token_itr > 0 && strcmp(tokens[token_itr-1],"&")==0){
			bg=true;
			tokens[token_itr-1]=NULL;
		}
/*Scanning for &*/
/* Scanning for < or >*/

		token_itr=0;
		char* file;
		bool left=false;
		bool right=false;
		while(tokens[token_itr]!=NULL){
			if(strcmp(tokens[token_itr],"<")==0 || strcmp(tokens[token_itr],">")==0){
				if(strcmp(tokens[token_itr],"<")==0)left=true;
				else right=true;
				file=tokens[token_itr+1];	
				tokens[token_itr]=NULL;
				tokens[token_itr+1]=NULL;
				break;
			}
			token_itr++;
		}
/* Scanning for < or >*/
		

/*Scanning for | pipe*/

		token_itr=0;
		char ** command;
		bool pipeExist=false;
		while(tokens[token_itr]!=NULL){
			if(strcmp(tokens[token_itr],"|")==0){
				tokens[token_itr]=NULL;
				command=&tokens[token_itr+1];
				pipeExist=true;
				break;
			}
			token_itr++;
		}
		if(pipeExist){
			int pipe_fd[2];
			if (pipe(pipe_fd) == -1) {
				error("pipe"); // Print an error message
			}
			int writer=fork();
			if(writer<0)error("fork failed");
			else if(writer==0){
				dup2(pipe_fd[1],STDOUT_FILENO);
				close(pipe_fd[0]);close(pipe_fd[1]);
				execvp(tokens[0],tokens);
			}
			
			int reader=fork();
			if(reader<0)error("fork failed");
			else if(reader==0){
				dup2(pipe_fd[0],STDIN_FILENO);
				close(pipe_fd[0]);close(pipe_fd[1]);
				execvp(*command, command);
			}

			close(pipe_fd[0]);close(pipe_fd[1]);
			waitpid(reader, NULL, 0);
			waitpid(writer, NULL, 0);
		
			for(i = 0; tokens[i] != NULL; i++) { free(tokens[i]); }
            if (command != NULL) {
                 for(i = 0; command[i] != NULL; i++) { free(command[i]); }
            }
            free(tokens);
            continue;
		}
/*Scanning for | pipe*/

/*-----Exit Functionality-----*/
		if(strcmp(tokens[0],"exit")==0){
			for(int i=0;i<bg_cnt;i++){
				kill(bgList[i], SIGKILL);
			}
			while((pid_w=waitpid(-1,NULL,WNOHANG))>0);
			for(i=0;tokens[i]!=NULL;i++){
				free(tokens[i]);
			}
			free(tokens);
			printf("Exiting Shell\n");
			exit(0);
		}
/*-----Exit Functionality-----*/

/*Forking and Handling the Child Processes*/
		int pid_c=fork();
		if(pid_c<0){
			error("Fork Failed");
		}
		else if(pid_c==0){
			if(!bg){
				signal(SIGINT, SIG_DFL);
			}
			else{
				setpgid(0,0);
			}
			if(right){
				int fp = open(file, O_WRONLY | O_CREAT | O_TRUNC);
				dup2(fp, 1);
				close(fp);
			}
			else if(left){
				int fp = open(file, O_RDONLY);
				dup2(fp,0);
				close(fp);
			}
			execvp(tokens[0],tokens);
			error("exec failed");
		}
		else{
			if(bg){
				bgList[bg_cnt]=pid_c;
				bg_cnt++;
				printf("Background process with PID : %d started\n", pid_c);
			}
			else{
				int status;
				waitpid(pid_c,&status,0);
				if(WIFEXITED(status)){
					printf("\nEXITSTATUS: %d\n", WEXITSTATUS(status));
				}
			}
		}
/*Forking and Handling the Child Processes*/
       
		// Freeing the allocated memory	
		for(i=0;tokens[i]!=NULL;i++){
			free(tokens[i]);
		}
		free(tokens);

	}
	return 0;
}
