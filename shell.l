
/*
 *
 * CS-252 
 * shell.l: lexical analyzer for shell
 * You have to extend it.
 *
 */

%{

#include <cstring>
#include <string>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <vector>
#include <sys/types.h>
#include "y.tab.hh"

int fork(void);

static  void yyunput (int c,char *buf_ptr  );

void myunputc(int c) {
  unput(c);
}

extern "C" char * read_line();

int mygetc(FILE *f) {
  static char *p;
  char ch;

  if(!isatty(0)) {
    return getc(f);
  }

  if(p==NULL || *p==0) {
    char *s = read_line();
    p = s;
  }

  ch = *p;
  p++;

  return ch;
}

#undef getc
#define getc(f) mygetc(f)

%}

%option noyywrap

%%

\n {
  return NEWLINE;
}

[ \t] {
  /* Discard spaces and tabs */
}

">" {
  return GREAT;
}

"|" {
  return PIPE;
}

">>" {
  return GREATGREAT;
}

"<" {
  return LESS;
}

"&" {
  return AMPERSAND;
}

">&" {
  return GREATAMPERSAND;
}


">>&" {
  return GREATGREATAMPERSAND;
}

"2>" {
  return TWOGREAT;
}

source[ ][^ \t\n][^ \t\n]* {
  yylval.cpp_string = new std::string(yytext);

  //getting filename
  *yylval.cpp_string = yylval.cpp_string->substr(7);
  char *filename = (char *)yylval.cpp_string->c_str();
  

  //open file to read
  FILE *fp = fopen(filename, "r");
  yypush_buffer_state(yy_create_buffer(fp, YY_BUF_SIZE));
  yyparse();
  yypop_buffer_state();
}



\$\([^\n]*\) {
  //subshell
  
  //store originial stdout stdin
  int tempin = dup(0);
  int tempout = dup(1);

  //create two pipe
  int pin[2];
  int pout[2];

  //call pipe
  pipe(pin);
  pipe(pout);

   
  //getting the required command substring
  char command[strlen(yytext)-2];

  int j = 0;
  for(int i = 2; i < strlen(yytext)-1; i++) {
    command[j++] = yytext[i];
  }
  command[j] = '\0';

  
  //writing commands into pin[1] and closing
  write(pin[1], command, strlen(command));
  write(pin[1], "\n", 1);
  //writing exit was failing testcase
  close(pin[1]);
  
  //getting args for execvp
  int size = strlen("/proc/self/exe");
  char shellfile[size] = "/proc/self/exe";
 
  std::vector<char *> d;
  d.push_back(shellfile);
  d.push_back(NULL);

  //file redirections for child
  dup2(pin[0], 0);
  dup2(pout[1], 1);
  close(pin[0]);
  close(pout[1]);
  
  //calling fork
  int ret;
  ret = fork();

  //child
  if(ret == 0) {
    //execvp
    close(pout[0]);
    execvp("/proc/self/exe", d.data());
    perror("execvp");
    exit(1);
  }

  //restoring initial stdin stdout
  dup2(tempin, 0);
  dup2(tempout, 1);
  close(tempin);
  close(tempout);
  

//buffer to read in characters
  std::vector<char> buffer;
  
  char c;

  //read in characters
  while(read(pout[0], &c, 1)) {
    //checking if buffer has '\n'
    if(c == '\n') {
      buffer.push_back(' ');
    } else {
      buffer.push_back(c);
    }
  }

  //closing not required file pointers
  close(pout[0]);
  
  //calling myunputc to pass back to lex
  for(int i = buffer.size()-1; i>=0; i--) {
    myunputc(buffer.at(i));
  }

}

~[^ \t\n]* {
  //for tilde expansion
  if(strlen(yytext) == 1) {
    //~
    char * home = getenv("HOME");
    yylval.cpp_string = new std::string(home);
    return WORD;
  }
 
  //getting / index
  yylval.cpp_string = new std::string(yytext);
  int index = yylval.cpp_string->find("/");

  if(index == 1) {
    //~/dir
    std::string home = (std::string)getenv("HOME");
    std::string str1 = yylval.cpp_string->substr(1);
    std::string *finalstr = new std::string;
    *finalstr = home + str1;
    yylval.cpp_string = finalstr;
    return WORD;
  } else if(index != std::string::npos) {
    //~george/dir
    std::string home = "/homes/";
    std::string str1 = yylval.cpp_string->substr(1, index-1);
    std::string str2 = yylval.cpp_string->substr(index);
    std::string *finalstr = new std::string;
    *finalstr = home + str1 + str2;
    yylval.cpp_string = finalstr;
    return WORD; 
  } else {
    //~george 
    std::string home = "/homes/";
    std::string str1 = yylval.cpp_string->substr(1);
    std::string *finalstr = new std::string;
    *finalstr = home + str1;
    yylval.cpp_string = finalstr;
    return WORD;
  }
}




([^ \t\n]*\\[^ \t\n][^ \t\n]*)([^ \t\n]*\\[^ \t\n][^ \t\n])* {
  //for strings having \ escape

  char temp[strlen(yytext)]; 

  //copying the string into temp leaving \.
  int j = 0;
  for(int i = 0; i < strlen(yytext); i++) {
    if(yytext[i] == '\\') {
      
      temp[j] = (yytext[i+1]);
      j++;
      i++;
    } else {
      temp[j] = (yytext[i]);
      j++;
    }
  }
  
  temp[j] = '\0';
  //printf("%s\n", temp);

  yylval.cpp_string = new std::string(temp);
  return WORD;
  
}

\".*\" {
  /* for string surrounded by quotes */
  /*initializing c string*/
  yylval.cpp_string = new std::string(yytext);

  //removing quotes
  *yylval.cpp_string = yylval.cpp_string->substr(1, yylval.cpp_string->length()-2);    //changing the string
 
  return WORD;   
}

[^ \t\n|<&>][^ \t\n|<&>]*  {
  /* Assume that file names have only alpha chars */
  yylval.cpp_string = new std::string(yytext);
  return WORD;
}
