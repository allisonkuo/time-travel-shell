// UCLA CS 111 Lab 1 command reading
#include <stdio.h>

#include <error.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

/* FIXME: You may need to add #include directives, macro definitions,
   static function definitions, etc.  
   
   included stdbool.h to use booleans throughout 
   included ctype.h to use isdigit() and isalpha()
   included stdlib.h to use for dealing with memory
   included string.h to use for string manipulation

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


// Convert the inputted script into token stream. HARD PART. FIGURE THIS OUT. glhf.
// note to self: size_t is for size of object
token_stream* convert_to_stream(char* input, size_t input_size)
{
  // Allocate space for the token stream. Set head equal to a new token
  token *new_token = make_token(HEAD, NULL);

  token_stream *new_stream = malloc(sizeof(token_stream));
  if (new_stream == NULL)
    {
      error(2, 0, "Error in allocating new memory."); // TODO figure out if this is right
      return NULL;
    } 
  
  token_stream *head_stream = new_stream;
  new_stream->head = new_token; //head is first token
  new_stream->tail = NULL;
  // Need line number for error message. 
  int line_num = 1;
  size_t count = 0;
  
  // For easy cases, make a new token, add it to the stream, increment everything
  //*********** HAVE TO CHECK REDIRECTS???? **********
  while (count < input_size)
    {
      switch(*input)
	{
	  // < redirect case
	case '<':
	  {
	    token *temp = make_token(INPUT, NULL);
	    new_token->next = temp;
	    new_token = new_token->next;
	    
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
	  
	    // Check current token-type
	    // 2 cases if \n follows word or subshell:
	    // a \n b -- treat \n as ;
	    // a \n \n b -- treat as new command tree
	    
	    if (new_token->type == SUBSHELL || new_token->type == WORD)
	      {
	       // If 2 \n in a row, make a new command tree if current stream already used
		if(*(input++) == '\n' && new_token->type != HEAD)
		  {
		    
		    new_stream->tail = malloc(sizeof(token_stream));
		    new_stream = new_stream->tail;
		    new_stream->tail = NULL;
		    new_token = make_token(HEAD, NULL);
		    new_stream->head = new_token;		  
		  }
		// Only a single \n, treat as a semicolon
		else
		  {
		    token *temp = make_token(SEMICOLON, NULL);
		    new_token->next = temp;
		    new_token = new_token->next;
		    
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
		line_num++;
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
	  
	    // SUBSHELL case.  
	  case '(':
	    {
	      size_t subshell_size = 20;
	      size_t subshell_counter = 0;
	      
	      // Need to remember subshell so allocate space for it
	      char *subshell = malloc(sizeof(subshell_size));
	      
	      // If subshell exists, need subshell_line_num for the error msg w/o messing up line_num
	      int subshell_line_num = line_num;
	      bool nested_subshell = true;
	      int nested_count = 0;
	      
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
			    break;
			  }
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
			    
			    // Single \n = treat as semicolon in token info
			    count++;
			    input++;
			  }
			ch = ';';
			line_num++;
			subshell_line_num++;		      
			break;
		      }
		    }
		    
		  // Reallocate more space for subshell if necessary
		  if (subshell_counter == subshell_size)
		    {
		      subshell = realloc(subshell, subshell_size * 2);
		      if (subshell == NULL)
			{
			  error(12, 0, "Line %d: Error reallocating memory.", line_num);
			  return NULL;
			}
		    }
		  
		  subshell[subshell_counter] = ch;
		  subshell_counter++;
		}
	      
	      token *temp = make_token(SUBSHELL,NULL);
	      new_token->next = temp;
	      new_token = new_token->next;
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
	      if (is_valid_word(*input))//why did you have *script here?
		{
		  size_t word_size = sizeof(char);
		  size_t word_counter = 0;
		  
		  // Need to remember word so allocate space for it
		  char *word = malloc(word_size);
		  if (word == NULL)
		    {
		      error(2, 0, "Line %d: Error in allocating new memory.", line_num); 
		      return NULL;
		    }

		  
		  // Loop through to get the whole word 
		  while (count < input_size && is_valid_word(*input))//again, *script
		    {
		      word[word_counter] = *input; //again *script
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

void outputEverything(token_stream* output)
{
  //token and token_stream's next is NULL when it's the end
  while(output->tail != NULL)
    {
      token* temp = output->head->next;
      while(temp != NULL)
	{
	  //output whatever in this token
	  temp = temp->next;
	}
      output = output->tail;
    }
}
int main()
{
  char stream[4] = "a\n\nb";
  token_stream* final = convert_to_stream(stream, sizeof(stream));
  int i,j;
  outputEverything(final);
  
  /*  for(j = 0; j < strlen(final->tail->head->next->info); j++)
    {
      printf("%c", (final->tail->head->next->info)[j]);
      printf("\n");
      }*/
}
    
/*NOTES

first node of each token will be invalid
*/
