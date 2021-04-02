
/*
 * CS-252
 * shell.y: parser for shell
 *
 * This parser compiles the following grammar:
 *
 *	cmd [arg]* [> filename]
 *
 * you must extend it to understand the complete shell grammar
 *
 */

%code requires 
{
#include <string>

#if __cplusplus > 199711L
#define register      // Deprecated in C++11 so remove the keyword
#endif
}

%union
{
  char        *string_val;
  // Example of using a c++ type in yacc
  std::string *cpp_string;
}

%token <cpp_string> WORD
%token NOTOKEN GREAT NEWLINE PIPE GREATGREAT LESS AMPERSAND GREATAMPERSAND GREATGREATAMPERSAND TWOGREAT 

%{
//#define yylex yylex
#include <cstdio>
#include "shell.hh"

void yyerror(const char * s);
int yylex();
void expandWildcardsIfNecessary(std::string * arg);
void expandWildcard(char *prefix, char *suffix);

%}

%%

goal: 
  command_list
  ;

command_list:
  command_line
  | command_list command_line
  ;

command_line:
  pipe_list io_modifier_list background_opt NEWLINE {
    /*printf("   Yacc: Execute command\n");*/
    Shell::_currentCommand.execute();
  }
  | NEWLINE {
      Shell::prompt();
  }
  | error NEWLINE { 
      yyerrok;
      printf("\n");
      Shell::prompt(); 
  }
  ;

pipe_list:
  pipe_list PIPE cmd_and_args
  | cmd_and_args
  ;

cmd_and_args:
  WORD {
    /*printf("   Yacc: insert command \"%s\"\n", $1->c_str());*/
    Command::_currentSimpleCommand = new SimpleCommand();
    Command::_currentSimpleCommand->insertArgument( $1 );
  } arglist {
      Shell::_currentCommand.insertSimpleCommand( Command::_currentSimpleCommand );
  } 
  ;

arglist:
  arglist WORD {
    /*printf("   Yacc: insert argument \"%s\"\n", $2->c_str());*/
    /*Command::_currentSimpleCommand->insertArgument( $2 );*/
    expandWildcardsIfNecessary( $2 );
    if(Shell::newMatchedFiles.size()) {
      delete($2);
    }

    /*clearin global variable for matched files*/
    Shell::newMatchedFiles.clear();
  }
  | /* empty string */
  ;

io_modifier_list:
  io_modifier_list io_modifier
  | /* empty string */
  ;

io_modifier: 
  GREATGREAT WORD {
    if(Shell::_currentCommand._outFile != NULL) {
      Shell::_currentCommand._error = true;
    } else {
        Shell::_currentCommand._outFile = $2;
	Shell::_currentCommand._append = true;
    }
  }
  | GREAT WORD {
    if(Shell::_currentCommand._outFile != NULL) {
      Shell::_currentCommand._error = true;
    } else {
        Shell::_currentCommand._outFile = $2;
    }

  }
  | GREATAMPERSAND WORD {
    if(Shell::_currentCommand._outFile != NULL || Shell::_currentCommand._errFile != NULL) {
      Shell::_currentCommand._error = true;
    } else {
        Shell::_currentCommand._outFile = $2;
        Shell::_currentCommand._errFile = $2;
    }
  }
  | GREATGREATAMPERSAND WORD {
    if(Shell::_currentCommand._outFile != NULL || Shell::_currentCommand._errFile != NULL) {
      Shell::_currentCommand._error = true;
    } else {
        Shell::_currentCommand._outFile = $2;
        Shell::_currentCommand._errFile = $2;
	Shell::_currentCommand._append = true;
    }
  }
  | TWOGREAT WORD {
    if(Shell::_currentCommand._errFile != NULL) {
      Shell::_currentCommand._error = true;
    } else {
        Shell::_currentCommand._errFile = $2;
    }
  }
  | LESS WORD {
      if(Shell::_currentCommand._inFile != NULL) {
      Shell::_currentCommand._error = true;
    } else {
        Shell::_currentCommand._inFile = $2;
    }
  }
  ;

background_opt: 
  AMPERSAND {
    if(Shell::_currentCommand._background) {
      Shell::_currentCommand._error = true;
    } else {
      Shell::_currentCommand._background = true;
    }
  }
  | /* empty */
  ;



%%

void
yyerror(const char * s)
{
  fprintf(stderr,"%s", s);
}

#if 0
main()
{
  yyparse();
}
#endif
