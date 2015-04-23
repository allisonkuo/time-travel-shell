// UCLA CS 111 Lab 1 command execution

#include "command.h"
#include "command-internals.h"

#include <error.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <fcntl.h>

/* FIXME: You may need to add #include directives, macro definitions,
   static function definitions, etc.  */

int
command_status (command_t c)
{
  return c->status;
}

void execute_simple (command_t c)
{
  char **a = c->u.word;
  const char* exec = "exec";

  if (c->output != NULL)
    {
      int fd = open(c->output, O_CREAT | O_TRUNC | O_WRONLY, 0644);
      
      if (fd < 0)
	error(1, 0, "Error opening file.");
      
      if (dup2(fd, 1) < 0)
	error(1, 0, "Error in dup2.");
    }

  if (c->input != NULL)
    {
      printf("hi");
      int fd = open(c->input, O_CREAT | O_TRUNC | O_RDONLY, 0644);

      if (fd < 0)
	error(1, 0, "Error opening file.");

      if (dup2(fd, 0) < 0)
	error(1, 0, "Error in dup2.");
    }
  
  if (strcmp(a[0], exec) == 0)
    execvp(a[1], &a[1]);
  else
    execvp(a[0], a);

}

void execute_and (command_t c)
{
  pid_t p = fork();
  if (p == 0)
    execute_command(c->u.command[0], true);
  else
    {
      int status;
      int exit_status;
      
      waitpid(p, &status, 0);
      exit_status = WEXITSTATUS(status);

      if (exit_status == 0)
	execute_command(c->u.command[1], true);
    }
}

void execute_or (command_t c)
{
  pid_t p = fork();
  if (p == 0)
    execute_command(c->u.command[0], true);
  else
    {
      int status;
      int exit_status;

      waitpid(p, &status, 0);
      exit_status = WEXITSTATUS(status);

      if (exit_status != 0)
	execute_command(c->u.command[1], true);
    }
}

void execute_sequence (command_t c)
{
  pid_t p = fork();
  if (p == 0)
    execute_command(c->u.command[0], true);
  else
    {
      int status;
      waitpid(p, &status, 0);
      execute_command(c->u.command[1], true);


    }
}

void execute_pipe (command_t c)
{
  int status;
  int fd[2];
  pid_t second_pid; 
  if (pipe(fd) < 0)
    error(1, 0, "Constructing pipe failed.");

  pid_t first_pid = fork();
  if (first_pid < 0)
    error(1, 0, "Forking failed");

  if (first_pid == 0)
    {
      close(fd[1]);
      if (dup2(fd[0], 0) < 0)
	error(1, 0, "Error in dup2");

      execute_command(c->u.command[1], true);
      //_exit(c->u.command[1]->status);
    }
  else
    {
      second_pid = fork();
      if (second_pid < 0)
	error(1, 0, "Forking failed");

      if (second_pid == 0)
	{
	  close(fd[0]);

	  if (dup2(fd[1], 1) < 0)
	    error(1, 0, "Error in dup2");

	  execute_command(c->u.command[0], true);
	}
      //_exit(c->u.command[0]->status);
    }

  close(fd[0]);
  close(fd[1]);
  pid_t return_pid = waitpid(-1, &status, 0);
  if (second_pid == return_pid)
    {
      waitpid(first_pid, &status, 0);
      //exit_status = WEXITSTATUS(status);
    }
  if (first_pid == return_pid)
    waitpid(second_pid, &status, 0);

  c->status = WEXITSTATUS(status);
}

void execute_subshell (command_t c)
{
  if (c->output != NULL)
    {
      int fd = open(c->output, O_CREAT | O_TRUNC | O_WRONLY, 0644);
      
      if (fd < 0)
	error(1, 0, "Error opening file.");
      
      if (dup2(fd, 1) < 0)
	error(1, 0, "Error in dup2.");
    }

  execute_command(c->u.subshell_command, true);
}

void
execute_command (command_t c, bool time_travel)
{
  switch (c->type)
    {
    case SIMPLE_COMMAND:
	execute_simple(c);
	break;
    case AND_COMMAND:
	execute_and(c);
	break;
    case OR_COMMAND:
	execute_or(c);
	break;
    case SEQUENCE_COMMAND:
	execute_sequence(c);
	break;
    case PIPE_COMMAND:
	execute_pipe(c);
	break;
    case SUBSHELL_COMMAND:
      	execute_subshell(c);
	break;
     
    }
  /* FIXME: Replace this with your implementation.  You may need to
     add auxiliary functions and otherwise modify the source code.
     You can also use external functions defined in the GNU C Library.  */
  //error (1, 0, "command execution not yet implemented");
}

