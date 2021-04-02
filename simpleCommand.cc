
#include <cstdio>
#include <cstdlib>

#include <iostream>
#include <unistd.h>
#include <regex.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>

#include "shell.hh"

SimpleCommand::SimpleCommand() {
  _arguments = std::vector<std::string *>();
}

SimpleCommand::~SimpleCommand() {
  // iterate over all the arguments and delete them
  for (auto & arg : _arguments) {
    delete arg;
  }
}

void SimpleCommand::insertArgument( std::string * argument ) {
  // simply add the argument to the vector
  
  //checking for environment expansion 
  
  //regex to match
  std::string buf = "^.*([$][{]([^}][^}]*)[}]).*$";
  char * buffer = (char *) buf.c_str();

  //compile regex
  regex_t re;
  int result = regcomp(&re, buffer, REG_EXTENDED|REG_NOSUB);
  if(result != 0) {
    fprintf(stderr, "Bad regex\n");
    exit(-1);
  }

  //check for match in argument
  regmatch_t match;
  result = regexec(&re, (char *)argument->c_str(), 1, &match, 0);
  
  //expanding ${}
  if(result == 0) {
    //printf("yes\n");
    std::vector<std::string> expandedArg;
    char * initialArg = (char *)argument->c_str();

    //counting number of "${" and "}"
    int startCount = 0;
    int endCount = 0;
    
    for(int i = 0; i < (int)argument->length()-1; i++) {
      if(initialArg[i] == '$' && initialArg[i+1] == '{') {
        startCount++;
      } 

      if(initialArg[i+1] == '}') {
        endCount++;
      }
    }

    int count;
    if(startCount <= endCount) {
      count = startCount;
    } else {
      count = endCount;
    }

    //printf("%d\n", count);

    //finding index of env var
    std::string start = "${";
    std::string end = "}";
    int index1 = 0;
    int index2 = 0;

    for(int i = 0; i < count; i++) {

    
      int newindex1 = argument->find(start, index1);
      //printf("%d\n", index1);
      int newindex2 = argument->find(end, index2);
      //printf("%d\n", index2);

      if(newindex2 > newindex1) {
        if(newindex1 != 0 && i == 0) {
          //separating string before ${
          expandedArg.push_back(argument->substr(0, newindex1));
          //printf("%s\n", (char *)expandedArg.at(0).c_str());
        }  
     
        //expanding var and pushing in expandedArg 
        std::string varToExpand = argument->substr(newindex1+2, newindex2-newindex1-2);
	
	if(varToExpand.compare("$") == 0) {
	  //pid
	  int pid = getpid();
	  expandedArg.push_back(std::to_string(pid));
	} else if(varToExpand.compare("SHELL") == 0) {
	  //path of shell executable
	  char path[1000];
	  realpath((char *)Shell::relativePath.c_str(), path);
	  expandedArg.push_back((std::string)path);
	} else if(varToExpand.compare("?") == 0) {
	  //return code of last executed simple command
	  expandedArg.push_back(std::to_string(WEXITSTATUS(Command::status)));
	} else if(varToExpand.compare("!") == 0) {
	  //PID of the last process run in background
	  int lastProcessIndex = Shell::zombieprocess.size() - 1;
	  int pid = Shell::zombieprocess.at(lastProcessIndex);
	  expandedArg.push_back(std::to_string(pid));
	} else if(varToExpand.compare("_") == 0) {
	  //last argument in fully expanded prev command
	  expandedArg.push_back(Command::lastArg);
	}

	else {
	  expandedArg.push_back((std::string)getenv((char *)varToExpand.c_str()));
	}

        if(newindex2+1 != (int)argument->length()) {
	  if(count == 1 || i == count-1) {
            expandedArg.push_back(argument->substr(newindex2+1));
	  } else {
	    int indexNewStart = argument->find(start, newindex2+1);
	    expandedArg.push_back(argument->substr(newindex2+1, indexNewStart-newindex2-1));
	  }
        }  
      }
      index1 = newindex1+1;
      index2 = newindex2+1;
    }
  

    //combining all strings
    
    std::string *finalArg = new std::string;
    for(int i = 0; i < (int)expandedArg.size(); i++) {
      *finalArg += expandedArg.at(i);
    }

    _arguments.push_back(finalArg);

    //free regex_t
    regfree(&re);
  } else {
    //free regex_t
    regfree(&re);	  
    _arguments.push_back(argument);
  }
}


// Print out the simple command
void SimpleCommand::print() {
  for (auto & arg : _arguments) {
    std::cout << "\"" << *arg << "\" \t";
  }
  // effectively the same as printf("\n\n");
  std::cout << std::endl;
}
