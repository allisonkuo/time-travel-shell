// UCLA CS 111 Lab 1 command execution

#include "command.h"
#include "command-internals.h"

#include <error.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

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

  if (strcmp(a[0], exec) == 0)
      execvp(a[1], &a[1]);
  else
    execvp(a[0], a);
}

void execute_and (command_t c)
{
  int p = fork();
  if (p == 0)
      execute_simple(c->u.command[0]);
  else
    {
      int status;
      int exit_status;
      
      waitpid(p, &status, 0);
      exit_status = WEXITSTATUS(status);

      if (exit_status == 0)
	  execute_simple(c->u.command[1]);
    }
}


void
execute_command (command_t c, bool time_travel)
{
  switch (c->type)
    {
    case SIMPLE_COMMAND:
      {
	execute_simple(c);
	break;
      }
    case AND_COMMAND:
      {
	execute_and(c);
	break;
      }
    case OR_COMMAND:
      {
	break;
      }
    case SEQUENCE_COMMAND:
      {
	break;
      }
    case PIPE_COMMAND:
      {
	break;
      }
    case SUBSHELL_COMMAND:
      {
	break;
      }
     
    }
  /* FIXME: Replace this with your implementation.  You may need to
     add auxiliary functions and otherwise modify the source code.
     You can also use external functions defined in the GNU C Library.  */
  error (1, 0, "command execution not yet implemented");
}

