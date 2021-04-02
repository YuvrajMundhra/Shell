/*
 * CS252: Systems Programming
 * Purdue University
 * Example that shows how to read one line with simple editing
 * using raw terminal.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>

#define MAX_BUFFER_LINE 2048

extern void tty_raw_mode(void);
extern void tty_term_mode(void);

// Buffer where line is stored
int line_length;
char line_buffer[MAX_BUFFER_LINE];
int cursor_position = 0;
int max_history = 0;
int history_size = 100;

// Simple history array
// This history does not change. 
// Yours have to be updated.
int history_index = 0;
char * history [100]; 
//char **history;

void read_line_print_usage()
{
  char * usage = "\n"
    " ctrl-?       Print usage\n"
    " Backspace    Deletes last character\n"
    " up arrow     See last command in the history\n";

  write(1, usage, strlen(usage));
}

/* 
 * Input a line with some basic editing.
 */
char * read_line() {
  //mallocing for history
  //history = (char **) malloc(history_size*sizeof(char *));

  // Set terminal in raw mode
  tty_raw_mode();

  line_length = 0;
  cursor_position = 0;

  // Read one line until enter is typed
  while (1) {

    // Read one character in raw mode.
    char ch;
    read(0, &ch, 1);

    if (ch>=32) {
      // It is a printable character. 

      //check if cursor at end or read all the characters
      char remainingChar[line_length - cursor_position];
      if(cursor_position != line_length) {
	int j = 0;
	for(int i = cursor_position; i < line_length; i++) {
	  remainingChar[j++] = line_buffer[i];
	  //print space
	  char tempch = ' ';
	  write(1, &tempch, 1);
	}

	//getting cursor back to initial position
	for(int i = cursor_position; i < line_length; i++) {
	  //backspace
	  char tempch = 8;
	  write(1, &tempch, 1);
	}
      }

      // Do echo
      write(1,&ch,1);

      //print remaining characters
      if(line_length-cursor_position != 0) {
        for(int i = 0; i < line_length-cursor_position; i++) {
	  char tempch = remainingChar[i];
	  write(1, &tempch, 1);
	}

	//cursor back to initial position
	for(int i = 0; i < line_length-cursor_position; i++) {
	  //backspace
	  char tempch = 8;
	  write(1, &tempch, 1);
	}
      }

      // If max number of character reached return.
      if (line_length==MAX_BUFFER_LINE-2) break; 

      // add char to buffer depending on where cursor is
      if(cursor_position == line_length) {
        line_buffer[line_length]=ch;
        line_length++;
	cursor_position = line_length;
      } else {
        line_buffer[cursor_position] = ch;
	int j = 0;
	for(int i = cursor_position + 1; i <=line_length; i++) {
	  line_buffer[i] = remainingChar[j++];
	}
	line_length++;
	cursor_position++;
      }

    }
    else if (ch==10) {
      // <Enter> was typed. Return line
      
      // Print newline
      ch = 10;
      write(1,&ch,1);
      cursor_position = 0;
      break;
    }
    else if (ch == 31) {
      // ctrl-?
      read_line_print_usage();
      line_buffer[0]=0;
      cursor_position = 0;
      break;
    }
    else if (ch == 8) {
      // <backspace> was typed. Remove previous character read.
      char remainingChar[line_length-cursor_position];
      if(cursor_position != 0) {
        //storing character in front if exist
	if(cursor_position != line_length) {
	  int j = 0;
	  for(int i = cursor_position; i < line_length; i++) {
	    remainingChar[j++] = line_buffer[i];
	    char tempch = ' ';
	    write(1, &tempch, 1);
	  }

	  //go back to initial cursor position
	  char tempch = 8;
	  for(int i = 0; i < line_length-cursor_position; i++) {
	    write(1, &tempch, 1);
	  }
	}

        //Go back one character
	ch = 8;
        write(1,&ch,1);
     

        // Write a space to erase the last character read
        ch = ' ';
        write(1,&ch,1);

        // Go back one character
        ch = 8;
        write(1,&ch,1);

	if(cursor_position != line_length) {
	  char tempch;
          //print remaining characters again
	  for(int i = 0; i < line_length-cursor_position; i++) {
	    tempch = remainingChar[i];
	    write(1, &tempch, 1);
	  }  

	  //move the cursor back to initial position
          for(int i = 0; i < line_length-cursor_position; i++) {
            tempch = 8;
	    write(1, &tempch, 1);
	  }
	}

        // Remove one character from buffer
        line_length--;
	cursor_position--;

	if(line_length != cursor_position) {
	  int j = 0;
	  //changing char in buffer
	  for(int i = cursor_position; i < line_length; i++) {
	    line_buffer[i] = remainingChar[j++];
	  }
	}
      }
    }
    else if(ch == 4) {
    //delete key
      if(cursor_position != line_length) {
        //overwriting character with blank
	ch = ' ';
	write(1, &ch, 1);
       
        //storing remaining characters 
	char remainingChar[line_length-cursor_position-1];
	if(cursor_position + 1 != line_length) {
	  int j = 0;
	  for(int i = cursor_position + 1; i < line_length; i++) {
	    remainingChar[j++] = line_buffer[i];
	    char tempch = ' ';
	    write(1, &tempch, 1);
	  }

	  //cursor back to initial position
	  for(int i = cursor_position + 1; i < line_length; i++) {
	    char tempch = 8;
	    write(1, &tempch, 1);
	  }
	}

	//cursor back to intial position
	ch = 8;
	write(1, &ch, 1);
		
	//writing the remaining characters
	if(cursor_position + 1 != line_length) {
	  for(int i = 0; i < line_length - cursor_position - 1; i++) {
	    char tempch = remainingChar[i];
	    write(1, &tempch, 1);
	  }

	  //cursor back to initial position
	  for(int i = 0; i < line_length-cursor_position - 1; i++) {
	    char tempch = 8;
	    write(1, &tempch, 1);
	  }
	}
	    
        line_length--;

	int j = 0;
	//changing character in line buffer
	if(cursor_position != line_length) {
	  for(int i = cursor_position; i < line_length; i++) {
	    line_buffer[i] = remainingChar[j++];
	  } 
	}
      }
    }
    else if(ch == 1) {
      //home, cursor to start
      if(cursor_position != 0) {
        //going left until cursor reaches 0
	for(int i = 0; i < cursor_position; i++) {
	  ch = 8;
	  write(1, &ch, 1);
	}

	//setting cursor position to 0
	cursor_position = 0;
      }
    }
    else if(ch == 5) {
      //end, cursor at end
      if(cursor_position != line_length) {
        //read the next character and retype
	for(int i = cursor_position; i < line_length; i++) {
	  ch = line_buffer[i];
	  write(1, &ch, 1);
	}	
      }

      //setting cursor position to end
      cursor_position = line_length;
    }

    else if(ch == 9) {
      //tab functionality
      if(line_length != 0 && cursor_position == line_length) {
        DIR * dir = opendir(".");
	if(dir == NULL) {
	  perror("opendir");
	}
        
        //checking for space
	int spaceFlag = 0;
	int k;
	for(k = 0; k < line_length; k++) {
	  if(line_buffer[k] == ' ') {
	    spaceFlag = 1;
	    break;
	  }
	}

	int indexspace;
	if(spaceFlag == 1) {
	  indexspace = k + 1;
	  k = k + 1;
	} else {
	  k = 0;
	  indexspace = 0;
	}

	char *match[100];
	int matchCount = 0;
	int flag = 0;
	//opening current directory to read files
	struct dirent * ent;

	//checking if any string matches
	while((ent = readdir(dir)) != NULL) {
	  if(line_length - k < (int)strlen(ent->d_name)) {
	    for(int i = 0; i < line_length - k; i++) {
	      if(ent->d_name[i] != line_buffer[i + indexspace]) {
	        flag = 1;
		break;
	      }
	    }
	    //copying strings into array of strings
	    if(flag == 0) {
	      match[matchCount] = (char *)malloc(strlen(ent->d_name)*sizeof(char));
	      for(int i = 0; i < (int)strlen(ent->d_name); i++) {
	        *(match[matchCount] + i) = ent->d_name[i];
	      }
	      matchCount++;
	    }
	    flag = 0;
	    
	  }
	}

	//close dir
	closedir(dir);
	
	if(matchCount != 0) {	
	//finding smallest string
	int smallestLength = 10000;
	for(int i = 0; i < matchCount; i++) {
	  if((int)strlen(match[i]) < smallestLength) {
	    smallestLength = strlen(match[i]);
	    } 
	}


	//finding how many extra characters match
	int charFlag = 0;
	int matchedChar = 0;
	int index;
	if(k == 0) {
          index = line_length; 
	} else {
	  index = line_length - k;
	}

	char tempch = *(match[0] + index);
	while(index < smallestLength) {
	  int i;
          for(i = 0; i < matchCount; i++) {
	    if(*(match[i] + index) != tempch) {
	      charFlag = 1;
	    }    
	  }
	  if(charFlag == 0) {
            //write(1, tempch, 
	    matchedChar++;
	  } else {
	    break;
	  }
	  index++;
	  tempch = *(match[0] + index);
	}


    	//printing the extra character
	if(k == 0) {
          index = line_length; 
	} else {
	  index = line_length - k;
	}
	for(int i = 0; i < matchedChar; i++) {
	  tempch = *(match[0] + index);
	  write(1, &tempch, 1);
	  index++; 
	}

	//adding characters to buffer
	for(int i = line_length; i < line_length + matchedChar; i++) {
	  line_buffer[i] = *(match[0] + i-k);
	}
    	//changing line length and cursor position
	line_length += matchedChar;
	cursor_position = line_length;        
      }
      }
    }
    else if (ch==27) {
      // Escape sequence. Read two chars more
      //
      // HINT: Use the program "keyboard-example" to
      // see the ascii code for the different chars typed.
      //
      char ch1; 
      char ch2;
      read(0, &ch1, 1);
      read(0, &ch2, 1);
      if (ch1==91 && ch2==65) {
	// Up arrow. Print next line in history.
	  // Erase old line
	  // Print backspaces
	  //checking if cursor not at end, then sending to end
          if(cursor_position != line_length) {
	    for(int i = cursor_position; i < line_length; i++) {
	      ch = line_buffer[cursor_position];
	      write(1, &ch, 1);
	    }
	  }

            

	    int i = 0;
	    for (i =0; i < line_length; i++) {
	      ch = 8;
	      write(1,&ch,1);
	    }  

	    // Print spaces on top
	    for (i =0; i < line_length; i++) {
	      ch = ' ';
	      write(1,&ch,1);
	    }

	    // Print backspaces
	    for (i =0; i < line_length; i++) {
	      ch = 8;
	      write(1,&ch,1);
	    }	
           if(history_index > 0) {
	    strcpy(line_buffer, history[history_index - 1]);
	    line_length = strlen(line_buffer);
	    cursor_position = line_length;
	    history_index--;
	  }
	  // echo line
	  write(1, line_buffer, line_length);
        } 
      else if(ch1 == 91 && ch2 == 66) {
        //down arrow. Print next line in history.
	  //Erase old line
	  //print backspaces
	 //checking if cursor not at end, then sending to end
	 if(cursor_position != line_length) {
	   for(int i = cursor_position; i < line_length; i++) {
	     ch = line_buffer[cursor_position];
	     write(1, &ch, 1);
	   }
	 }
	
	 if(history_index < history_size-1 && history_index + 1 < max_history) {
	  int i = 0;
	  for (i =0; i < line_length; i++) {
	    ch = 8;
	    write(1,&ch,1);
	  }

	  //print spaces on top
          for (i =0; i < line_length; i++) {
	    ch = ' ';
	    write(1,&ch,1);
	  }

	  //print backspaces
          for (i =0; i < line_length; i++) {
	    ch = 8;
	    write(1,&ch,1);
	  } 

	  //copy line from history
	  strcpy(line_buffer, history[history_index + 1]);
	  line_length = strlen(line_buffer);
	  cursor_position = line_length;
	  history_index++;
	

	//echo line
	write(1, line_buffer, line_length);	
	}
      } 
      else if(ch1 == 91 && ch2 == 68) {
        //left arrow
	
	//go back one character
	if(cursor_position != 0) {
	  ch = 8;
	  write(1, &ch, 1);
	  cursor_position--;
	}
      } else if(ch1 == 91 && ch2 == 67) {
        //right arrow
	
        //go ahead one character by reading and printing the same character
	if(cursor_position != line_length) {
	  ch = line_buffer[cursor_position];
	  write(1, &ch, 1);
	  cursor_position++;
	}		
      }
    }
  }

  // Add eol and null char at the end of string
  line_buffer[line_length]=10;
  line_length++;
  line_buffer[line_length]=0;

  //if(max_history < history_size) {
    if(line_length != 0) {
      if(line_buffer[0] != '\n') {
        history[max_history] = (char *)malloc(line_length*sizeof(char));
        strcpy(history[max_history], line_buffer);
        *(history[max_history] + line_length-1) = '\0';	
        max_history++;
        cursor_position = line_length;
        history_index = max_history;
      }
    }
 /* } else {	  
    history_size *= 2;
    history = realloc(history, history_size*(sizeof(char *)));
    history[max_history] = (char *)malloc(line_length*sizeof(char));
    strcpy(history[max_history], line_buffer);
    *(history[max_history] + line_length-1) = '\0';
    max_history++;
    cursor_position = line_length;
    history_index = max_history;
  }
*/


  tty_term_mode();

  return line_buffer;
}

