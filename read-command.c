// UCLA CS 111 Lab 1 command reading

#include "command.h"
#include "command-internals.h"

#include <error.h>
#include <stdbool.h>

/* FIXME: You may need to add #include directives, macro definitions,
   static function definitions, etc.  
   
   included stdbool.h to use booleans throughout 

*/

/* Implement a STACK and its basic functions to use later */
typedef struct stack stack;

typedef stack{
  command_t commands[100];
  int num_items;
};

int size(stack *s)
{
  return s->num_items;
}

bool empty(stack *s)
{
  if (s->num_items == 0)
    return true;
  return false;
}

command_t top(stack *s)
{
  top_spot = s->num_items - 1;
  return s->commands[top_spot];
}

void push(stack *s, command_t c)
{
  s->commands[s->num_items] = c;
  num_items++;
}

command_t pop(stack *s, command_t c)
{

  /* after pushed, you incremented size. to pop, must decrement size first */
  s->num_items--;
  return s->commands[s->num_items];
}

/* FIXME: Define the type 'struct command_stream' here.  This should
   complete the incomplete type declaration in command.h.  */

command_stream_t
make_command_stream (int (*get_next_byte) (void *),
		     void *get_next_byte_argument)
{
  /* FIXME: Replace this with your implementation.  You may need to
     add auxiliary functions and otherwise modify the source code.
     You can also use external functions defined in the GNU C Library.  */
  error (1, 0, "command reading not yet implemented");
  return 0;
}

command_t
read_command_stream (command_stream_t s)
{
  /* FIXME: Replace this with your implementation too.  */
  error (1, 0, "command reading not yet implemented");
  return 0;
}
