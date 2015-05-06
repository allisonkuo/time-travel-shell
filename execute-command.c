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

typedef struct graph_node graph_node;
typedef struct queue queue;
typedef struct dependency_graph dependency_graph;
typedef struct dependency_list dependency_list;
typedef struct command_node command_node;

struct command_node {
  command_t command;
  command_node *next;
};

struct command_stream {
  command_node *head;
  command_node *tail;
  command_node *cursor;
};


struct graph_node {
  command_t command;
  struct graph_node** before;
  pid_t pid;
};

// implement a QUEUE and its function
struct queue {
  graph_node* graphs[100];
  int num_items;
  int current;
};

void push(queue *q, graph_node* g)
{
  q->graphs[q->num_items] = g;
  q->num_items++;
}

graph_node* pop(queue *q)
{
  if(q->num_items == q->current)
    return NULL;
  else
    {
      q->current++;
      return q->graphs[q->current - 1];
    }
}

struct dependency_graph {
  queue* no_dependencies;
  queue* dependencies;
};


struct dependency_list {
  graph_node* g;
  dependency_list* next;
  char** read;
  char** write;
  int read_count;
  int write_count;
};

// initialize a dependency list node
dependency_list* create_dependency_node()
{
  dependency_list* d = malloc(sizeof(dependency_list));
  d->read = malloc(sizeof(read));
  d->write = malloc(sizeof(write));
  if (d->read == NULL || d->write == NULL)
    {
      error(2, 0, "Error in allocating new memory.");
      return NULL;
    }

  d->next = NULL;
  d->read_count = 0;
  d->write_count = 0;

  return d;
}

void process_command(command_t c, dependency_list* d)
{
  // for each command,
  if (c->type == SIMPLE_COMMAND)
    {
      // store c->input, c->u.word[1] into read list(filter for options)
      // store output into write list
      return;
    }
  else if (c-> type == SUBSHELL_COMMAND)
    {
      // store c->input into read list
      // store c->output into write list
      //process_command(c->u.subshell_command);
      return;
    }
  else
    {
      //process_command(c->u.command[0]);
      //process_command(c->u.command[1]);
      return;
    }
}
  
dependency_graph* create_graph(command_stream_t c)
{
  dependency_graph* dep = 
  /*
  dependency_list* head = create_dependency_node();
  dependency_list* temp = head;
  dependency_list* d = head;
  while (c != NULL)
    {
      while (c->cursor != NULL)
	{
	  command_t cmd = c->cursor->command;
	  process_command(cmd, d);
	  c->cursor = c->cursor->next;
	}
      c = c->tail;

      if (c != NULL)
	{
	  dependency_list* d = create_dependency_node();
	  temp = d;
	}

      if (head->next == NULL)
	head->next = temp;
      else
	{
	  temp->next = d;
	  temp = temp->next;
	}
    }
  return head;
  */
}

int
command_status (command_t c)
{
  return c->status;
}

// for 1b, pass in true arbitrarily for bool time_travel argument of
// execute_command because not using that variable yet.
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
      int fd = open(c->input, O_RDONLY, 0644);

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
    {
      execute_command(c->u.command[0], true);
      _exit(c->u.command[0]->status);
    }
  else
    {
      int status;
      
      waitpid(p, &status, 0);
      c->status = WEXITSTATUS(status);

      if (c->status == 0)
	{
	  execute_command(c->u.command[1], true);
	  _exit(c->u.command[1]->status);
	}
    }
}

void execute_or (command_t c)
{
  pid_t p = fork();
  if (p == 0)
    {
      execute_command(c->u.command[0], true);
      _exit(c->u.command[0]->status);
    }
  else
    {
      int status;

      waitpid(p, &status, 0);
      c->status = WEXITSTATUS(status);

      if (c->status != 0)
	{
	  execute_command(c->u.command[1], true);
	  _exit(c->u.command[1]->status);
	}
    }
}

void execute_sequence (command_t c)
{
  pid_t p = fork();
  if (p == 0)
    {
      execute_command(c->u.command[0], true);
      _exit(c->u.command[0]->status);
    }
  else
    {
      int status;
      waitpid(p, &status, 0);
      execute_command(c->u.command[1], true);
      _exit(c->u.command[1]->status);
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
      _exit(c->u.command[1]->status);
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
	  _exit(c->u.command[0]->status);
	}
    }

  close(fd[0]);
  close(fd[1]);
  pid_t return_pid = waitpid(-1, &status, 0);
  if (second_pid == return_pid)
    {
      waitpid(first_pid, &status, 0);
      c->status = WEXITSTATUS(status);
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

    if (c->input != NULL)
    {
      int fd = open(c->input, O_RDONLY, 0644);

      if (fd < 0)
	error(1, 0, "Error opening file.");

      if (dup2(fd, 0) < 0)
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

