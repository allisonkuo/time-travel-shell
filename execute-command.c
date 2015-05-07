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
#include <stdbool.h>

/* FIXME: You may need to add #include directives, macro definitions,
   static function definitions, etc.  */

typedef struct graph_node graph_node;
typedef struct queue queue;
typedef struct dependency_graph dependency_graph;
typedef struct building_list building_list;
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
  int count;
};

// ===============implement a QUEUE and its function=========
struct queue {
  graph_node* graphs[100];
  int num_items;
  int push_spot;
  int current;
};

void q_push(queue *q, graph_node* g)
{
  q->graphs[q->push_spot] = g;
  q->push_spot++;
  q->num_items++;
}

graph_node* q_pop(queue *q)
{
  if(q->num_items == 0)
    return NULL;
  else
    {
      q->current++;
      q->num_items--;
      return q->graphs[q->current - 1];
    }
}

bool q_empty(queue *q)
{
  if (q->num_items == 0)
    return true;
  else
    return false;
}

// ================== end of queue ====================


// ================== dependency_graph ================
struct dependency_graph {
  queue* no_dependencies;
  queue* dependencies;
};

// ============== end of dependency_graph ============


// ========== building_list ===================

struct building_list {
  graph_node* gn;
  building_list* next;
  char** read;
  char** write;
  int read_count;
  int write_count;
};

// initialize a building list node
building_list* create_building_list_node()
{
  building_list* d = malloc(sizeof(building_list));

  d->gn = malloc(sizeof(graph_node));
  d->gn->before = NULL;
  d->gn->count = 0;
  d->gn->pid = -1;
  d->read = malloc(sizeof(read));
  d->write = malloc(sizeof(write));
  if (d->read == NULL || d->write == NULL || d == NULL || d->gn == NULL)
  {
      error(2, 0, "Error in allocating new memory.");
      return NULL;
  }

  d->next = NULL;
  d->read_count = 0;
  d->write_count = 0;
  return d;
}

// ============ end of building_list =================


void process_command(command_t c, building_list* d)
{
  // for each command,
  if (c->type == SIMPLE_COMMAND)
    {
      // store c->input, c->u.word[1] into read list(filter for options)
      if (c->input != NULL)
	{
	  d->read[d->read_count] = c->input;
	  d->read_count++;
	}
      int i;
      for (i = 1; ; i++)
	{
	  if (c->u.word[i] == NULL)
	    break;
	  else if (c->u.word[i][0] == '-')
	    continue;
	  else
	    {
	      d->read = realloc(d->read, sizeof(d->read) * 8);
	      if (d->read == NULL)
		{
		  error(2, 0, "Error allocating memory.");
		  return;
		}
	      d->read[d->read_count] = c->u.word[i];
	      d->read_count++;
	      break;
	    }
	}
      
      // store output into write list
      if (c->output != NULL)
	{
	  d->write[d->write_count] = c->output;
	  d->write = realloc(d->write, sizeof(d->read) * 8);
	  if (d->write == NULL)
	    {
	      error(2, 0, "Error allocating memory.");
	      return;
	    }
	  d->write_count++;
	}
    }
  else if (c-> type == SUBSHELL_COMMAND)
    {
      // store c->input into read list
      if (c->input != NULL)
	{
	  d->read[d->read_count] = c->input;
	  d->read = realloc(d->read, sizeof(d->read) * 8);
	  if (d->read == NULL)
	    {
	      error(2, 0, "Error allocating memory.");
	      return;
	    }
	  d->read_count++;
	}
      
      // store c->output into write list
      if (c->output != NULL)
	{
	  d->write[d->write_count] = c->output;
	  d->write = realloc(d->write, sizeof(d->read) * 8);
	  if (d->write == NULL)
	    {
	      error(2, 0, "Error allocating memory.");
	      return;
	    }
	  d->write_count++;
	}
      
      // process actual commands within the subshell
      process_command(c->u.subshell_command, d);
    }
  else
    {
      process_command(c->u.command[0], d);
      process_command(c->u.command[1], d);
    }
}

bool check_dependency(building_list* b1, building_list* b2)
{
  int i, j;

  // if b1's read and b2's write intersects (WAR)
  for (i = 0; i < b1->read_count; i++)
    for (j = 0; j < b2->write_count; j++)
      if (b1->read[i] != NULL && b2->write[j] != NULL &&  strcmp(b1->read[i], b2->write[j]) == 0)
	return true;
  
  // if b1's write and b2's read intersects (RAW)
  for (i = 0; i < b1->write_count; i++)
    for (j = 0; j < b2->read_count; j++)
      if (b1->write[i] != NULL && b2->read[j] != NULL &&  strcmp(b1->write[i], b2->read[j]) == 0)
	return true;
  
  // if b1's write and b2's write intersects (WAW)
  for (i = 0; i < b1->write_count; i++)
    for (j = 0; j < b2->write_count; j++)
      if (b1->write[i] != NULL && b2->write[j] != NULL &&  strcmp(b1->write[i], b2->write[j]) == 0)
	return true;

  return false;
}

void add_before(graph_node* g1, graph_node* g2)
{
  // CHECK LOL
  g1->before = realloc(g1->before, sizeof(graph_node));
  g1->before[g1->count] = g2;
  g1->count++;
  g1->before[g1->count] = NULL;
}


dependency_graph* create_graph(command_stream_t c)
{
  // head = arbitrary dependency list node. ignore it.
  dependency_graph* d = malloc(sizeof(dependency_graph));

  // initialize the dependency_graph queues
  d->dependencies = malloc(sizeof(queue));
  d->dependencies->num_items = 0;
  d->dependencies->push_spot = 0;
  d->dependencies->current = 0;

  d->no_dependencies = malloc(sizeof(queue));
  d->no_dependencies->num_items = 0;
  d->no_dependencies->push_spot = 0;
  d->dependencies->current = 0;
  
  if (d == NULL || d->dependencies == NULL || d->no_dependencies == NULL)
    {
      error(2, 0, "Error in allocating memory.");
      return NULL;
    }
  
  building_list* head = create_building_list_node();
  building_list* temp = head;

  // go through each command tree
  while (c->head != NULL)
    {
      // top of each command tree
      temp->next = create_building_list_node();
      temp = temp->next;
      temp->gn->command = c->head->command;
      temp->next = NULL;

      // should process the whole command tree
      process_command(c->head->command, temp); 

      building_list* iterator = head->next;
      while (iterator->next != NULL)
	{
	  // check if there are any read/write dependencies. if so, add to read/write list
	  if(check_dependency(temp, iterator)) 
	    add_before(temp->gn, iterator->gn); 
	  iterator = iterator->next;
	}

      if (temp->gn->before != NULL)
	q_push(d->dependencies, temp->gn);
      else
	q_push(d->no_dependencies, temp->gn);

      c->head = c->head->next;
    }

  return d;
}

void execute_no_dependencies(queue* no_dependencies)
{
  while (!q_empty(no_dependencies))
    {
      graph_node* temp = q_pop(no_dependencies);
      pid_t pid = fork();
      if (pid == 0)
	{
	  execute_command(temp->command, true);
	  exit(1);
	}
      else
	temp->pid = pid;
    }

}

void execute_dependencies(queue* dependencies)
{
  while(!q_empty(dependencies))
    {
      int status = 0, i;
      graph_node* temp = q_pop(dependencies);
      for (i = 0; i < temp->count; i++)
	  waitpid(temp->before[i]->pid, &status, 0);
      pid_t pid = fork();
      if (pid == 0)
	{
	  execute_command(temp->command, true);
      	  exit(1);
	}
      else
	temp->pid = pid;
    }
}
      
int execute_graph(dependency_graph* graph)
{
  execute_no_dependencies(graph->no_dependencies);
  execute_dependencies(graph->dependencies);

  // don't care what main returns, so just return something.
  return 1;
}

// ========================= END OF PART C =========================

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

  pid_t pid = fork();
  
  if (pid == 0)
    {
      if (strcmp(a[0], exec) == 0)
	execvp(a[1], &a[1]);
      else
	execvp(a[0], a);
    }
  
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

      execute_command(c->u.command[1], false);
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

	  execute_command(c->u.command[0], false);
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

  // just to make the freaking warning go away
  if (time_travel)
    time_travel = true;
   


  /* FIXME: Replace this with your implementation.  You may need to
     add auxiliary functions and otherwise modify the source code.
     You can also use external functions defined in the GNU C Library.  */
  //error (1, 0, "command execution not yet implemented");
}

