// UCLA CS 111 Lab 1 command reading
#include "command.h"
#include "command-internals.h"

#include <stdio.h>
#include <error.h>

#include <stdbool.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
/* FIXME: You may need to add #include directives, macro definitions,
   static function definitions, etc.  
   
   included stdbool.h to use booleans throughout 
   included ctype.h to use isdigit() and isalpha()
   included stdlib.h to use for dealing with memory
   included string.h to use for string manipulation
   included limits.h to use INT_MAX for input script

*/

/* STEP 1: make input script into something usable */

// Make enum "token_type" to classify the tokens [HEAD is a filler for the head of the stream]
// Define a "token" [nodes] and "token_stream" [linked list] to handle the input

enum token_type {
  AND,
  SEMICOLON,
  OR,
  PIPE,
  WORD,
  SUBSHELL,
  INPUT,
  OUTPUT,
  HEAD, 
};

typedef struct token token;
typedef struct token_stream token_stream;

// Each node holds each "word" in the input stream within 'char *info'
struct token {
  enum token_type type;
  char *info;
  token *next;
  int line;
};

// *tail points to the next token stream and is "tail" of the first token_stream
struct token_stream {
  token *head;
  token_stream *tail;
};

// Make the token (set all its variables) 
token *make_token(enum token_type type, char *info)
{
  // Allocate space for this token
  token *new_token = malloc(sizeof(token));
  if (new_token == NULL)
    error(2, 0, "Error in allocating new memory."); // TODO figure out if this is right

  new_token->type = type;
  new_token->info = info;
  new_token->next = NULL;
  return new_token;
}

// All the non-digit/non-letter characters allowed in a word
bool special_char(char letter)
{
  switch(letter) 
    {
    case '!': 
    case '%':
    case '+':
    case ',':
    case '-':
    case '.':
    case '/':
    case ':':
    case '@':
    case '^':
    case '_':
      return true;
      break;
    default:
      return false;
    }
}

// Check to see if is a word we will care about
bool is_valid_word(char letter)
{
  if (isalpha(letter) || isdigit(letter))
    return true;
  else if (special_char(letter))
    return true;
  return false;
}

bool all_newlines(char* input)
{
  unsigned int i;
  for(i = 0; i < strlen(input); i++)
    {
      if(*input != '\n')
	{
	  return false;
	}
      input++;
    }
  return true;
}
// Convert the inputted script into token stream. HARD PART. FIGURE THIS OUT. glhf.
// note to self: size_t is for size of object
token_stream* convert_to_stream(char* input, size_t input_size)
{
  // Allocate space for the token stream. Set head equal to a new token
  token *new_token = make_token(HEAD, NULL);

  token_stream *new_stream = malloc(sizeof(token_stream));
  if (new_stream == NULL)
    {
      error(2, 0, "Error in allocating new memory."); 
      return NULL;
    } 
  
  new_stream->head = new_token; //head is first token
  new_stream->tail = NULL;
  token_stream *head_stream = new_stream;
  
  // Need line number for error message. 
  int line_num = 1;
  size_t count = 0;
  
  // For easy cases, make a new token, add it to the stream, increment everything
  //*********** HAVE TO CHECK REDIRECTS???? **********
  while (count < input_size)
    {
      if (all_newlines(input))
	break;
      //printf("%d\n", count);
      switch(*input)
	{
	  // < redirect case
	case '<':
	  {
	    token *temp = make_token(INPUT, NULL);
	    new_token->next = temp;
	    new_token = new_token->next;
	    new_token->line = line_num;
	    
	    count++;
	    input++;
	    break;
	  }
	
	  // > redirect case
	case '>':
	  {
	    token *temp = make_token(OUTPUT, NULL);
	    new_token->next = temp;
	    new_token = new_token->next;
	    new_token->line = line_num;
	    
	    count++;
	    input++;
	    break;
	  }
	    
	// SEMICOLON case  
	case ';':
	  {
	    token *temp = make_token(SEMICOLON, NULL);
	    new_token->next = temp;
	    new_token = new_token->next;
	    new_token->line = line_num;
	    
	    count++;
	    input++;
	    break;
	  }
	case '|':
	  {
	    // Check the next character
	    count++;
	    input++; //TODO: check logic
	    
	    // OR case
	    if (*input == '|')
	      {
		token *temp = make_token(OR, NULL);
		new_token->next = temp;
		new_token = new_token->next;
		new_token->line = line_num;
		
		count++;
		input++;
		break;
	      }
	    // PIPE case
	    else
	      {
		token *temp = make_token(PIPE, NULL);
		new_token->next = temp;
		new_token = new_token->next;
		new_token->line = line_num;
		break;
	      }
	  }

	case '&':
	  {
	    // Check the next character because should be &&
	    count++;
	    input++;
	    
	    if(*input == '&')
	      {
		token *temp = make_token(AND, NULL);
		new_token->next = temp;
		new_token = new_token->next;
		new_token->line = line_num;
		
		count++;
		input++;
		break;
	      }
	    else
	      {
		error(2, 0, "Line %d: Syntax Error - must have two &'s", line_num);
		return NULL;
	      }
	  }

   
  	// NEWLINE case. move to next line and go from there
	case '\n':
	  {
	    line_num++;
	    char *ch = input + 1;
	    // Check current token-type
	    // 2 cases if \n follows word or subshell:
	    // a \n b -- treat \n as ;
	    // a \n \n b -- treat as new command tree
	    
	    if (new_token->type == SUBSHELL || new_token->type == WORD)
	      {
	       // If 2 \n in a row, make a new command tree if current stream already used
		if(*ch == '\n' && new_token->type != HEAD)
		  {
		    new_stream->tail = malloc(sizeof(token_stream));
		    new_stream = new_stream->tail;
		    new_stream->tail = NULL;
		    new_token = make_token(HEAD, NULL);
		    new_stream->head = new_token;

		    line_num++;
		    count += 2;
		    input++; input++;
		    break;
		  }
		// Only a single \n, treat as a semicolon
		else
		  {
		    token *temp = make_token(SEMICOLON, NULL);
		    new_token->next = temp;
		    new_token = new_token->next;
		    new_token->line = line_num;

		    count++;
		    input++;
		    break;
		  }
	      }
	    else if (new_token->type == INPUT || new_token->type == OUTPUT)
	      {
		error(2, 0, "Line %d: Syntax Error - newline cannot follow redirect.", line_num);
		return NULL;
	      }
	    else
	      {
		count++;
		input++;
		break;
	      }
	    
	    // WHITESPACE case. do nothing
	  case '\t':
	  case ' ':
	    {
	      count++;
	      input++;
	      break;
	    }

	    //COMMENT case. ignore until newline
	    case '#':
	      {
		while(*input != '\n')
		  {
		    count++;
		    input++;
		  }
		line_num++;
		count++;
		input++;
		break;
	      }
	      
	    // SUBSHELL case.  
	  case '(':
	    {
	      size_t subshell_size = 20;
	      size_t subshell_counter = 0;
	      
	      // Need to remember subshell so allocate space for it
	      char *subshell = malloc(sizeof(subshell_size));
	      if (subshell == NULL)
		{
		  error(2, 0, "Error in allocating new memory.");
		  return NULL;
		}
	      
	      // If subshell exists, need subshell_line_num for the error msg w/o messing up line_num
	      int subshell_line_num = line_num;
	      bool nested_subshell = true;
	      int nested_count = 1;
	      
	      while (nested_subshell == true)
		{
		  count++;
		  input++;
		  char ch = *input;
		  
		  if (count == input_size)
		    {
		      error(2, 0, "Line %d: Syntax Error -- subshell never closed", subshell_line_num);
		      return NULL;
		    }
		  
		  switch(ch)
		    {
		    case '(':
		      {
			nested_subshell = true;
			nested_count++;
			break;
		      }
		    case ')':
		      {
			nested_count--;
			
			// Break out of the outermost subshell once its closed
			if (nested_count == 0)
			  {
			    nested_subshell = false;
			    count++;
			    input++;
			  }
			break;
		      }
		      // Get rid of all the whitespace in the subshell
		    case '\n':
		      {
			while (*(input++) == ' ' || *(input++) == '\n' || *(input++) == '\t')
			  {
			    // If 2 \n in a row, go down a line
			    if (*(input++) == '\n')
			      {
				line_num++;
				subshell_line_num++;
				break;
			      }
			    
			    count++;
			    input++;
			  }
			
			// Single \n = treat as semicolon in token info
			ch = ';';
			line_num++;
			subshell_line_num++;		      
			break;
		      }

		    }
		    
		  // Reallocate more space for subshell if necessary
		  if (subshell_counter == subshell_size)
		    {
		      subshell_size *= 2;
		      subshell = realloc(subshell, subshell_size);
		      if (subshell == NULL)
			{
			  error(12, 0, "Line %d: Error reallocating memory.", line_num);
			  return NULL;
			}
		    }
		  
		  // Break out of the loop so doesn't store the final )
		  if (!nested_subshell)
		    break;
		  
		  subshell[subshell_counter] = ch;
		  subshell_counter++;
		}
	      
	      token *temp = make_token(SUBSHELL, subshell);
	      new_token->next = temp;
	      new_token = new_token->next;
	      new_token->line = line_num;
	      break;
	    }
	  
	  case ')':
	    {
	      error(2, 0, "Line %d: Syntax Error - close without matching open parenthesis.", line_num);
	      return NULL;
	      break;
	    }
	  default:
	    {
	      // WORD case
	      if (is_valid_word(*input))
		{
		  size_t word_size = sizeof(char) * 2;
		  size_t word_counter = 0;
		  
		  // Need to remember word so allocate space for it
		  char *word = malloc(word_size);
		  if (word == NULL)
		    {
		      error(2, 0, "Line %d: Error in allocating new memory.", line_num); 
		      return NULL;
		    }

		  
		  // Loop through to get the whole word 
		  while (count < input_size && is_valid_word(*input))
		    {
		      word[word_counter] = *input; 
		      word_counter++;
		      
		      // Reallocate more space for word if necessary
		      if (word_counter == word_size)
			{
			  word_size = word_size * 2;
			  word = realloc(word, word_size);
			  if (word == NULL)
			    {

			      error(2, 0, "Line %d: Error reallocating memory.", line_num);
			      return NULL;
			    }
			}
		      
		      count++;
		      input++;

		    }
		  
		  token *temp = make_token(WORD, word);
		  new_token->next = temp;
		  new_token = new_token->next;
		  new_token->line = line_num;
		  break;
		}
	      else
		{
		  error(2, 0, "Line %d: Syntax Error - character unrecognized", line_num);
		  return NULL;
		}
	    }
	  }
	}
    }
	  
  return head_stream;
}

// Deallocate the memory for a single token stream
void delete_token_stream(token *head)
{
  token *temp = head;
  while (temp != NULL)
    {
      head = head->next;
      free(temp);
      temp = head;
    }
}

// Deallocate the memory for the entire token stream
void delete_all_token_streams(token_stream *stream)
{
  token_stream *temp = stream;
  while (temp != NULL)
    {
      stream = stream->tail;
      delete_token_stream(temp->head);
      free(temp);
      temp = stream;
    }
}

// Implement a STACK and its basic functions to use later
typedef struct stack stack;

struct stack{
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
  int top_spot = s->num_items - 1;
  return s->commands[top_spot];
}

void push(stack *s, command_t c)
{
  s->commands[s->num_items] = c;
  s->num_items++;
}

command_t pop(stack *s)
{
  // After pushed, incremented size. To pop, must decrement size first
  s->num_items--;
  if (s->num_items >= 0)
    return s->commands[s->num_items];
  else
    return NULL;
}

// Turn word token into a command
command_t make_command(token *t)
{
  command_t new_command = malloc(sizeof(command_t));
  if (new_command == NULL)
    {
      error(2, 0, "Error in allocating new memory.");
      return NULL;
    }

  // Same for all types of commands (for now). Will fix for operator in later function.
  new_command->status = -1;
  new_command->input = NULL;
  new_command->output = NULL;
  new_command->u.word = NULL;

  // Different command_types for the different tokens
  if (t->type == AND)
    {
      new_command->type = AND_COMMAND;
    }
  else if (t->type == SEMICOLON)
    {
      new_command->type = SEQUENCE_COMMAND;
    }
  else if (t->type == OR)
    {
      new_command->type = OR_COMMAND;
    }
  else if (t->type == PIPE)
    {
      new_command->type = PIPE_COMMAND;
    }
  else if (t->type == WORD)
    {
      new_command->type = SIMPLE_COMMAND;
    }
  else if (t->type == SUBSHELL)
    {
      new_command->type = SUBSHELL_COMMAND;
      new_command->u.word = malloc(sizeof(t->info));
      if (new_command->u.word == NULL)
	{
	  error(2, 0, "Error in allocating new memory.");
	  return NULL; 
	}
      new_command->u.word[0] = t->info;
    }

  //  new_command->u.command[0] = NULL;

  return new_command;
}

// Combine commands when involving operator into a "single" command
command_t combine_commands(command_t left, command_t op, command_t right, int line)
{
  if (left == NULL || right == NULL || op == NULL)
    {
      error(2, 0, "Line %d: Incorrect number of operands", line);
      return NULL;
    }
  
  op->u.command[0] = left;
  op->u.command[1] = right;
  
  return op;
}


bool precedence(command_t a, command_t b)
{
  if (a->type == b->type || (a->type == OR_COMMAND && b->type == AND_COMMAND) || (a->type == AND_COMMAND && b->type == OR_COMMAND))
    return true;
  else if (a->type == PIPE_COMMAND)
    return false;
  else if (b->type == PIPE_COMMAND)
    return true;
  else if (a->type == SEQUENCE_COMMAND)
    return true;
  else
    return false;
}


// Makes a tree for the head of a token stream (b/c stream corresponds to tree)
command_t make_command_tree(token_stream *stream)
{
  int count = 0;
  int num_words = 1;
  token *current_token = stream->head->next;

  stack *operators = malloc(sizeof(stack));
  if(operators == NULL)
    {
      error(2, 0, "Error in allocating new memory.");
      return NULL;
    }
  stack *operands = malloc(sizeof(stack));
  if (operands == NULL)
    {
      error(2, 0, "Error in allocating new memory.");
      return NULL;
    }
  operators->num_items = 0;
  operands->num_items = 0;

  if (current_token->type != WORD && current_token->type != SUBSHELL)
    {
      error(2, 0, "Line %d: Invalid input", current_token->line);
      return NULL;
    }
  
  while (1)
    {
      if (current_token->type == WORD)
	{
	  count = 0;
	  command_t new_command = make_command(current_token);

	  // If a simple command is multiple words, make it into one single command
	  size_t temp_size = 20;
	  size_t curr_size = 0;
	  new_command->u.word = malloc(temp_size);
	  if (new_command->u.word == NULL)
	    {
	      error(2, 0, "Error in allocating new memory.");
	      return NULL;
	    }
	  while (current_token->type == WORD)
	    {
	      curr_size += sizeof(current_token->info);
	      if (temp_size <= curr_size)
		{
		  temp_size *= 2;
		  new_command->u.word = realloc(new_command->u.word, temp_size);
		  if (new_command->u.word == NULL)
		    {
		      error(2, 0, "Error in reallocating new memory.");
		      return NULL;
		    }
		}
	      new_command->u.word[count] = current_token->info;
	      count++;
	      if (current_token->next == NULL)
		  break;
	      else if (current_token->next->type == WORD)
		current_token = current_token->next;
	      else
		break;
	    }
	  new_command->u.word[count] = NULL;

	  push(operands, new_command);
	}
      else if (current_token->type == SUBSHELL)
	{
	  command_t new_command = make_command(current_token);

	  // Process the subshell command tree and push onto the operands stack
	  new_command->u.subshell_command = make_command_tree(convert_to_stream(current_token->info, strlen(current_token->info)));

	  push(operands, new_command);
	}
      else if (current_token->type == INPUT || current_token->type == OUTPUT)
	{
	  if (current_token->next == NULL || current_token->next->type != WORD) 
	    {
	      error(2, 0, "Line %d: Invalid token following redirection", current_token->line);
	      return NULL;
	    }
	  
	  int inputtype; //1 if Input, 0 if Output
	  if (current_token->type == INPUT)
	    inputtype = 1;
	  else
	    inputtype = 0;
	  count = 0;
	  num_words = 0;

	  command_t top = pop(operands);
	  command_t nextone = make_command(current_token->next);

	  current_token = current_token->next;
	  
	  // Do the same thing as if it were a word cuz everything sucks
	  size_t temp_size = 20;
	  size_t curr_size = 0;
	  nextone->u.word = malloc(temp_size);
	  if (nextone->u.word == NULL)
	    {
	      error(2, 0, "Error in allocating new memory.");
	      return NULL;
	    }

	  while (current_token->type == WORD)
	    {
	      curr_size += sizeof(current_token->info);
	      if (temp_size <= curr_size)//CHANGE EVERYWHERE
		{
		  temp_size *= 2;
		  nextone->u.word = realloc(nextone->u.word, temp_size);
		  if (nextone->u.word == NULL)
		    {
		      error(2, 0, "Error in reallocating new memory.");
		      return NULL;
		    }
		}
	      nextone->u.word[count] = current_token->info;
	      count++;
	      num_words++;
	      if (current_token->next == NULL)
		  break;
	      else if (current_token->next->type == WORD)
		current_token->next = current_token->next->next;
	      else
		break;
	    }
	  //	  nextone->u.word[count] == NULL;

	  /* Find the energy of an array and xenophilo chemicals that
	     becomes a nonray formally known as a laser beam */
	  int i, j;
	  size_t temp_count = 0;
	  size_t size = 10;
	  char *temp2 = malloc(size);
	  if (temp2 == NULL)
	    {
	      error(2, 0, "Error in allocating new memory.");
	      return NULL;
	    }

	  for (i = 0; i < num_words; i++)
	    {
	      j = 0;
	      while (nextone->u.word[i][j] != '\0')
		{
		  if (temp_count == size)
		    {
		      // Also reallocate memory
		      size *= 2;
		      temp2 = realloc(temp2, size);
		      if (temp2 == NULL)
			{
			  error(2, 0, "Error in reallocating memory.");
			  return NULL;
			}
		    }
		  temp2[temp_count] = nextone->u.word[i][j];
		  temp_count++;
		  j++;
		}
	    }
	  
	  if (inputtype == 1)
	    top->input = temp2;
	  else
	    top->output = temp2;

	  push(operands, top);
	}
      else
	{
	  command_t new_command = make_command(current_token);
	  if (empty(operators))
	    push(operators, new_command);
	  // Pop all operators w/ >= precedence and combine into new commands
	  else
	    {
	      while (!empty(operators) && precedence(new_command, top(operators)))
		{
		  command_t ops = pop(operators);
		  command_t right = pop(operands);
		  command_t left = pop(operands);
		  int tokline = current_token->line;
		  
		  command_t current_command = combine_commands(left, ops, right, tokline);

		  push(operands, current_command);
		}
	      push(operators, new_command);
	    }
	}
      if (current_token->next == NULL)
	break;
      
      current_token = current_token->next;
    }

  // If have remaining operators after doing all the precedence stuff
  while (!empty(operators))
    {
      command_t ops = pop(operators);
      command_t right = pop(operands);
      command_t left = pop(operands);

      command_t current_command = combine_commands(left, ops, right, 1);

      push(operands,current_command);
    }

  // Last thing on operand stack should be the top of the command tree
  command_t final = pop(operands);
   return final;
}


/* FIXME: Define the type 'struct command_stream' here.  This should
   complete the incomplete type declaration in command.h.  */
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

command_stream_t
make_command_stream (int (*get_next_byte) (void *),
  void *get_next_byte_argument)
{
  /* FIXME: Replace this with your implementation.  You may need to
     add auxiliary functions and otherwise modify the source code.
     You can also use external functions defined in the GNU C Library.  */
  size_t count = 0;
  size_t buffer_size = 512;

  char ch = get_next_byte(get_next_byte_argument);
  char *buffer = malloc(buffer_size);
  if (buffer == NULL)
    {
      error(2, 0, "Error in allocating new memory.");
      return NULL;
    }

  while (ch != -1 && ch != EOF)
    {
      buffer[count] = ch;
      count++;
      ch = get_next_byte(get_next_byte_argument);
      
      if (count == buffer_size)
	{
	  buffer_size *= 2;
	  buffer = realloc(buffer, buffer_size);
	  if (buffer == NULL)
	    {
	      error(2, 0, "Error in reallocating new memory.");
	      return NULL;
	    }
	}
      else if (count == INT_MAX)
	{
	  error(1, 0, "Line -1: Input size too large.");
	  return NULL;
	}
    }

  // Process the buffer (script) into token stream
  token_stream *temp = convert_to_stream(buffer, count);
  
  // Initialize a command node 
  command_node *first = malloc(sizeof(command_node));
  if (first == NULL)
    {
      error(2, 0, "Error in allocating new memory.");
      return NULL;
    }
  first->command = make_command_tree(temp);
  first->next = NULL;

  // Initialize a command stream and point everything to the first node
  command_stream_t command_stream = malloc(sizeof(command_stream_t));
  if (command_stream == NULL)
    {
      error(2, 0, "Error in allocating new memory.");
      return NULL;
    }
  command_stream->head = first;
  command_stream->tail = first;
  command_stream->cursor = first;

  temp = temp->tail;      
  
  while (1)
    {
      if (temp == NULL)
	break;
      else
	{
	  command_node *next_node = malloc(sizeof(command_node));
	    if (next_node == NULL)
	      {
	      error(2, 0, "Error in allocating new memory.");
	      return NULL;
	      }
	  next_node->command = make_command_tree(temp);
	  next_node->next = NULL;
	  first->next = next_node;          	  
	  first = first->next;
	  command_stream->tail = first;
	}
      temp = temp->tail;           
    }

  free(buffer);
  delete_all_token_streams(temp);

  return command_stream;
}

command_t
read_command_stream (command_stream_t s)
{
  /* FIXME: Replace this with your implementation too.  */
   if (s->cursor == NULL)
    return NULL;
   command_t temp = s->cursor->command;
   s->cursor = s->cursor->next;
   return temp;
}
