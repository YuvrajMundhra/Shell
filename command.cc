/*
 * CS252: Shell project
 *
 * Template file.
 * You will need to add more code here to execute the command table.
 *
 * NOTE: You are responsible for fixing any bugs this code may have!
 *
 * DO NOT PUT THIS PROJECT IN A PUBLIC REPOSITORY LIKE GIT. IF YOU WANT 
 * TO MAKE IT PUBLICALLY AVAILABLE YOU NEED TO REMOVE ANY SKELETON CODE 
 * AND REWRITE YOUR PROJECT SO IT IMPLEMENTS FUNCTIONALITY DIFFERENT THAN
 * WHAT IS SPECIFIED IN THE HANDOUT. WE OFTEN REUSE PART OF THE PROJECTS FROM  
 * SEMESTER TO SEMESTER AND PUTTING YOUR CODE IN A PUBLIC REPOSITORY
 * MAY FACILITATE ACADEMIC DISHONESTY.
 */

#include <cstdio>
#include <cstdlib>

#include <iostream>

#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>

#include <sys/stat.h>
#include <fcntl.h>

#include <string.h>
#include <stdlib.h>
#include <termios.h>

#include "command.hh"
#include "shell.hh"

int putenv(char *string);
int setenv(const char *envname, const char *envval, int overwrite);
int unsetenv(const char *name);
int chdir(const char *path);

int Command::status;
std::string Command::lastArg;

Command::Command() {
    // Initialize a new vector of Simple Commands
    _simpleCommands = std::vector<SimpleCommand *>();

    _outFile = NULL;
    _inFile = NULL;
    _errFile = NULL;
    _background = false;
    _error = false;
    _append = false;

}

void Command::insertSimpleCommand( SimpleCommand * simpleCommand ) {
    // add the simple command to the vector
    _simpleCommands.push_back(simpleCommand);
}

void Command::clear() {
    // deallocate all the simple commands in the command vector
    for (auto simpleCommand : _simpleCommands) {
        delete simpleCommand;
    }

    // remove all references to the simple commands we've deallocated
    // (basically just sets the size to 0)
    _simpleCommands.clear();

    //check if outfile and errfile point to same file
    if(_outFile == _errFile && _outFile != NULL) {
      delete _outFile;
    } else {
        if ( _outFile ) {
          delete _outFile;
        }

	if(_errFile) {
	  delete _errFile;
	}
    }
    
    _outFile = NULL;
    _errFile = NULL;

    if ( _inFile ) {
        delete _inFile;
    }
    _inFile = NULL;

    _background = false;

    _error = false;

    _append = false;
}

void Command::print() {
    printf("\n\n");
    printf("              COMMAND TABLE                \n");
    printf("\n");
    printf("  #   Simple Commands\n");
    printf("  --- ----------------------------------------------------------\n");

    int i = 0;
    // iterate over the simple commands and print them nicely
    for ( auto & simpleCommand : _simpleCommands ) {
        printf("  %-3d ", i++ );
        simpleCommand->print();
    }

    printf( "\n\n" );
    printf( "  Output       Input        Error        Background\n" );
    printf( "  ------------ ------------ ------------ ------------\n" );
    printf( "  %-12s %-12s %-12s %-12s\n",
            _outFile?_outFile->c_str():"default",
            _inFile?_inFile->c_str():"default",
            _errFile?_errFile->c_str():"default",
            _background?"YES":"NO");
    printf( "\n\n" );
}

void Command::execute() {
    // Don't do anything if there are no simple commands
    if ( _simpleCommands.size() == 0 ) {
        Shell::prompt();
        return;
    }

    //checking if error in syntax
    if (_error) {
      printf("Ambiguous output redirect.\n");
      Shell::prompt();
      return;
    }

    //checking if command was exit
    if (_simpleCommands.size() == 1 && _simpleCommands[0]->_arguments.size() == 1) {
      if(strcmp(_simpleCommands[0]->_arguments[0]->c_str(), "exit") == 0) {
	printf("Good bye!!\n");
	exit(1);
      }
    }

    // Print contents of Command data structure
    //print();

    // Add execution here
    
    //save in, out, error initial file pointers
    int tempin = dup(0);
    int tempout = dup(1);
    int temperr = dup(2);

    //setting initial input
    int fdin;
    if(_inFile != NULL) {
      //provided file
      fdin = open((char *)_inFile->c_str(), O_RDONLY, 0600);
      if(fdin < 0) {
        //perror("No such file or directory\n");
      } 
    } else {
      //terminal
      fdin = dup(tempin); 
    }

    dup2(fdin, 0);
    close(fdin);

    //setting stderr for every process
    int fderr;
    if(_errFile != NULL) {
      //provided file
      fderr = open((char *)_errFile->c_str(), O_CREAT|O_APPEND|O_RDWR, 0600);
    } else {
      //terminal
      fderr = dup(temperr);
    }

    dup2(fderr, 2);
    close(fderr);    //close fderr since 2 has been redirected

    //creating child process for every simple command
    int ret;
    int fdout;
    int numberOfCommands = (int) _simpleCommands.size();

    //looping through every simple command
    for(int i = 0; i < numberOfCommands; i++) {
      //redirecting input
      dup2(fdin, 0);
      close(fdin);     //since 0 has been redirected to fdin

      //setting up output depending on pipes and file 
      if(i == numberOfCommands - 1) {
        //get last arg
	int argSize = _simpleCommands[i]->_arguments.size();
	Command::lastArg = (std::string)_simpleCommands[i]->_arguments[argSize-1]->c_str();

        //last command
	if(_outFile != NULL) {
	  if(_append) {
	    //open file in append mode
	    fdout = open((char *)_outFile->c_str(), O_WRONLY|O_CREAT|O_APPEND, 0666); 
	  } else {
	    //not opening in append mode
	    fdout = open((char *)_outFile->c_str(), O_WRONLY|O_CREAT|O_TRUNC, 0666);
	  }
	} else {
	  //output to terminal
	  fdout = dup(tempout);
	}
      } else {
        //creating pipe
	int fdpipe[2];
	pipe(fdpipe);
	fdout = fdpipe[1];
	fdin = fdpipe[0];
      }

      //calling dup2 to redirect ouput
      dup2(fdout, 1);
      close(fdout);   //since 1 has been redirected to fdout
      

    //checking for setenv  
    if(strcmp((char *)_simpleCommands[i]->_arguments[0]->c_str(), "setenv") == 0) {
      setenv((char *)_simpleCommands[i]->_arguments[1]->c_str(), (char *)_simpleCommands[i]->_arguments[2]->c_str(), 1);
      continue;
    }

    //checking for unsetenv
    if(strcmp((char *)_simpleCommands[i]->_arguments[0]->c_str(), "unsetenv") == 0) {
      unsetenv((char *)_simpleCommands[i]->_arguments[1]->c_str());
      continue;
    } 

    //checking for cd
    if(strcmp((char *)_simpleCommands[i]->_arguments[0]->c_str(), "cd") == 0) {
      if(_simpleCommands[i]->_arguments.size() == 1) {
        chdir(getenv("HOME"));
      } else if (strcmp((char *)_simpleCommands[i]->_arguments[1]->c_str(), "${HOME}") == 0) { 
        chdir(getenv("HOME"));
      } else {
        int val = chdir((char *)_simpleCommands[i]->_arguments[1]->c_str());
	if(val == -1) {
	  fprintf(stderr, "cd: can't cd to %s\n", (char *)_simpleCommands[i]->_arguments[1]->c_str());
	}
      }
      continue;
    }
  
        
      //creating child process
      ret = fork();
      
      //checking for fork error
      if(ret < 0) {
        perror("fork");
	exit(1);
      } 


      if(ret == 0) {
        close(tempin);
	close(tempout);
	close(temperr);

        //checking if command is printenv        
        if(strcmp(_simpleCommands[i]->_arguments[0]->c_str(), "printenv") == 0) {
	  char **p = environ;
          while(*p != NULL) {
      	    printf("%s\n", *p);
	    p++;  
          }
	  exit(0);
	}
        
  



        //child process
	int j;
        
        //getting number of arguments for a command, +1 for NULL
	int argSize = _simpleCommands[i]->_arguments.size() + 1;
        
     	//making char array to store arguments
	std::vector<char *> args;

	//copying all the arguments of simpleCommand into array
	for(j = 0; j < argSize - 1; j++) {
	  
	  args.push_back((char *)_simpleCommands[i]->_arguments[j]->c_str());
	}

	//setting last element of args to NULL
	args[j] = NULL;

	//calling execvp
	char * arg1 = (char *)args.at(0);
	execvp(arg1, args.data());

	//printing and exiting if error
	perror("execvp");
	_exit(1);	
      }
    }
   
    //restoring in/out/error defaults
    dup2(tempin, 0);
    dup2(tempout, 1);
    dup2(temperr, 2);
    
    //closing temp file pointers since all redirected
    close(tempin);
    close(tempout);
    close(temperr);

    //calling waipid if no background_opt
    if(!_background) {
      //wait for last command to complete
      waitpid(ret, &Command::status, 0);
    } else {  //adding the processid to the list of background processes
      Shell::zombieprocess.push_back(ret);
    } 
     
    
    // For every simple command fork a new process
    // Setup i/o redirection
    // and call exec

    //ON_ERROR
    if(getenv("ON_ERROR") != NULL && strcmp(_simpleCommands[0]->_arguments[0]->c_str(), "setenv") != 0) {
      if(WEXITSTATUS(Command::status) != 0) {
        printf("%s\n", getenv("ON_ERROR"));
      }
    }

    //clear to prepare for next command
    clear();

    // Print new prompt
    Shell::prompt();
}

SimpleCommand * Command::_currentSimpleCommand;
