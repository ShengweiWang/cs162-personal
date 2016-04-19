#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include "tokenizer.h"

/* Whether the shell is connected to an actual terminal or not. */
bool shell_is_interactive;

/* File descriptor for the shell input */
int shell_terminal;

/* Terminal mode settings for the shell */
struct termios shell_tmodes;

/* Process group id for the shell */
pid_t shell_pgid;

int cmd_exit(struct tokens *tokens);
int cmd_help(struct tokens *tokens);
int cmd_pwd(struct tokens *tokens);
int cmd_cd(struct tokens *tokens);
int path_resolve(char ** tokens_argv);

/* Built-in command functions take token array (see parse.h) and return int */
typedef int cmd_fun_t(struct tokens *tokens);

/* Built-in command struct and lookup table */
typedef struct fun_desc {
  cmd_fun_t *fun;
  char *cmd;
  char *doc;
} fun_desc_t;

fun_desc_t cmd_table[] = {
  {cmd_help, "?", "show this help menu"},
  {cmd_exit, "exit", "exit the command shell"},
  {cmd_pwd, "pwd", "show current working directory"},
  {cmd_cd, "cd", "change the directory to the given path"}
};

/* Prints a helpful description for the given command */
int cmd_help(struct tokens *tokens) {
  for (int i = 0; i < sizeof(cmd_table) / sizeof(fun_desc_t); i++)
    printf("%s - %s\n", cmd_table[i].cmd, cmd_table[i].doc);
  return 1;
}

/* Exits this shell */
int cmd_exit(struct tokens *tokens) {
  exit(0);
}

/*Show current working directory*/
int cmd_pwd(struct tokens *tokens){
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) !=NULL)
	fprintf(stdout, "%s\n", cwd);
    else
	perror("getcwd() error");
    return 0;
}

/*Change the directory to the file name */
int cmd_cd(struct tokens *tokens){
    char * dirPath = tokens_get_token(tokens, 1);
    if (chdir(dirPath) == 0)
	fprintf(stdout, "dir changed to:%s\n", dirPath);
    else
	perror("wrong dir!");
}

/* Looks up the built-in command, if it exists. */
int lookup(char cmd[]) {
  for (int i = 0; i < sizeof(cmd_table) / sizeof(fun_desc_t); i++)
    if (cmd && (strcmp(cmd_table[i].cmd, cmd) == 0))
      return i;
  return -1;
}

/* Intialization procedures for this shell */
void init_shell() {
  /* Our shell is connected to standard input. */
  shell_terminal = STDIN_FILENO;

  /* Check if we are running interactively */
  shell_is_interactive = isatty(shell_terminal);

  if (shell_is_interactive) {
    /* If the shell is not currently in the foreground, we must pause the shell until it becomes a
     * foreground process. We use SIGTTIN to pause the shell. When the shell gets moved to the
     * foreground, we'll receive a SIGCONT. */
    while (tcgetpgrp(shell_terminal) != (shell_pgid = getpgrp()))
      kill(-shell_pgid, SIGTTIN);

    /* Saves the shell's process id */
    shell_pgid = getpid();

    /* Take control of the terminal */
    tcsetpgrp(shell_terminal, shell_pgid);

    /* Save the current termios to a variable, so it can be restored later. */
    tcgetattr(shell_terminal, &shell_tmodes);
  }
}
int path_resolve(char ** tokens_argv){
    char * line = getenv("PATH");
    int i=0;
    while( *(line+i) != NULL){
	if( *(line+i) == ':')
	    *(line+i) = ' ';
	i++;
    }
    struct tokens *path_tokens = tokenize(line);
    //test for path get
    for(int i = 0; i < tokens_get_length(path_tokens); i++)
	fprintf(stdout, "path: %s\n", tokens_get_token(path_tokens, i));
    DIR* dir;
    struct dirent* ent;
    for(int i = 0; i < tokens_get_length(path_tokens); i++){
	if ((dir = opendir(tokens_get_token(path_tokens, i))) == NULL)
	    continue;
	else{
	    while ((ent = readdir(dir)) != NULL){
		if(strcmp(ent->d_name, *tokens_argv) == 0){
		    strncpy(*tokens_argv, tokens_get_token(path_tokens, i),strlen(tokens_get_token(path_tokens, i)));
		    strncat(*tokens_argv,"/",1);
		    strncat(*tokens_argv,ent->d_name,strlen(ent->d_name));
		    //print success message
		    //fprintf(stdout,"found!! %s\n", *tokens_argv);
		    return 1;
		}
	    }
	}
    }
    return -1;
}




int main(int argc, char *argv[]) {
  init_shell();

  static char line[4096];
  int line_num = 0;

  /* Please only print shell prompts when standard input is not a tty */
  if (shell_is_interactive)
    fprintf(stdout, "%d: ", line_num);

  while (fgets(line, 4096, stdin)) {
    /* Split our line into words. */
    struct tokens *tokens = tokenize(line);

    /* Find which built-in function to run. */
    int fundex = lookup(tokens_get_token(tokens, 0));

    if (fundex >= 0) {
      cmd_table[fundex].fun(tokens);
    } else {
      /* REPLACE this to run commands as programs. */
      int mainpid = getpid();
      fprintf(stdout, "this pid is %d\n",mainpid);
      pid_t pid;
      pid = fork();
      if(pid < 0)
      {
	  fprintf(stderr, "fork failed\n");
      }
      else{
	  if(pid == 0){
	      wait(NULL);
	      fprintf(stdout, "this is main %d\n",pid);
	  }else{
	      char * tokens_argv[tokens_get_length(tokens)+1];
	      int i;
	      for(i = 0; i < tokens_get_length(tokens); i++)
		  (tokens_argv[i]) = tokens_get_token(tokens,i);
	      tokens_argv[i] = NULL;
	      if(path_resolve(tokens_argv) > 0)
		  //fprintf(stdout, "path solved successfully!\n");
	      else{
		  //fprintf(stdout,"failed\n");
		  //exit(0);
	      }
	      if(execv(tokens_argv[0],tokens_argv) < 0)
	          fprintf(stderr, "%s %d: Command not found\n", tokens_argv[1] ,errno);
	      else
		  fprintf(stdout, "exec successed\n");
	  }
      }
    }


    if (shell_is_interactive)
      /* Please only print shell prompts when standard input is not a tty */
      fprintf(stdout, "%d: ", ++line_num);

    /* Clean up memory */
    tokens_destroy(tokens);
  }

  return 0;
}
