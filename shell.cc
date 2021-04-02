#include <cstdio>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <regex.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>

#include "shell.hh"

int yyparse(void);
int isatty(int fd);
extern "C" void disp(int sig);
extern "C" void killzombie(int sig);
void expandWildcard(char *prefix, char *suffix);

//global var for environment var
extern char ** environ;

//function to print when sigint interrupt caught
extern "C" void disp(int sig) {
  sig = sig;
  printf("\n");
  Shell::prompt();
  fflush(stdout);
  //fprintf(stderr, "\nsig: %d  Ouch!\n", sig);
}

//function to handle zombieprocess
extern "C" void killzombie(int sig) {
  sig = sig;
  //calling waitpid to kill zombie process
  int pid = waitpid(-1, NULL, WNOHANG);
  
  //checking if the process was background or pipe
  while(pid > 0) {
    bool flag = false;
    for(int i = 0; i < (int)Shell::zombieprocess.size(); i++) {
      if(Shell::zombieprocess.at(i) == pid) {
        flag = true;
	break;
      }
    }

    if(flag) {
      printf("%d exited.\n", pid);
    }
   
    pid = waitpid(-1, NULL, WNOHANG);
  }
}



void Shell::prompt() {
  if(isatty(0)) {
    std::string newPrompt; 
    if(getenv("PROMPT")) {
      newPrompt = (std::string)getenv("PROMPT");
      printf("%s", (char *)newPrompt.c_str());
      fflush(stdout);
    } else {
      printf("myshell>");
      fflush(stdout);
    }
  }
}

int main(int argc, char **argv) {

  //Assigning global vars 
  Shell::numberofArgs = argc;
  Shell::relativePath = (std::string)argv[0];  

  //code to catch interrupt using sigaction
  struct sigaction sa;
  sa.sa_handler = disp;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
    
  //handles ctrl-c
  if(sigaction(SIGINT, &sa, NULL)) {
    perror("sigaction");
    exit(2);
  }

  //handles zombie process
  sa.sa_handler = killzombie;
  if(sigaction(SIGCHLD, &sa, NULL)) {
    perror("sigaction");
    exit(2);
  }  

  Shell::prompt();
  yyparse();

}

//expand wildcards if necessary
void expandWildcardsIfNecessary(std::string * arg) {
  //if no wildcard
  if((arg->find("*") == std::string::npos && arg->find("?") == std::string::npos) || (arg->find("*") == std::string::npos && arg->find("{?}") != std::string::npos)) {
    Command::_currentSimpleCommand->insertArgument(arg); 
  } else {
    //setting args for expandWildcard
    char * prefix;
    std::string temp = "";
    prefix = (char *)temp.c_str();

    char * suffix = (char *)arg->c_str();

    //calling expandWildcard
    expandWildcard(prefix, suffix);

    //if no match
    if(Shell::newMatchedFiles.size() == 0) {
      Command::_currentSimpleCommand->insertArgument(arg);
    }

    //sorting files alphabetically
    for(int i = 0; i < (int)Shell::newMatchedFiles.size()-1; i++) {
      for(int j = i + 1; j > 0; j--) {
        if(Shell::newMatchedFiles.at(j) < Shell::newMatchedFiles.at(j-1)) {
	  std::string temp = Shell::newMatchedFiles.at(j);
	  Shell::newMatchedFiles.at(j) = Shell::newMatchedFiles.at(j-1);
	  Shell::newMatchedFiles.at(j-1) = temp;  
	}
      } 
    }

    for(int i = 0; i < (int)Shell::newMatchedFiles.size(); i++) {
      //printf("%s\n", (char *)matchedFiles.at(i).c_str());
      Command::_currentSimpleCommand->insertArgument(new std::string(Shell::newMatchedFiles.at(i)));
    }
  }
  return;
}


//wildcard subdirectory
void expandWildcard(char *prefix, char *suffix) {
  //base case when suffix = ""
  if(strcmp(suffix, "") == 0) {
    //insert files in vector
    char * file = strdup(prefix);
    std::string temp = (std::string) file;
    Shell::newMatchedFiles.push_back(temp);
    free(file);
    return;
  }

  //creating component and new suffix
  std::string component;
  std::string *suf = new std::string(suffix);

  //index of /
  int index = suf->find("/");

  if(index != (int)std::string::npos) {
    //make required changes to new prefix and suffix
    component = suf->substr(0, index);
    *suf = suf->substr(index+1);
  } else {
    //suffix empty since last filename
    component = *suf;
    *suf = "";
  }

//printf("%s\n", component.c_str());
  //checking if no wildcard
  if((component.find("*") == std::string::npos && component.find("?") == std::string::npos) || (component.find("*") == std::string::npos && component.find("{?}") != std::string::npos)) {
	  std::string newPrefix;
	  //if prefix = /, we donot want to add an extra /
	  if(strcmp(prefix, "/") == 0) {
	    newPrefix = (std::string)prefix + component;
	  } else {
	    newPrefix = (std::string)prefix + "/" + component;
	  }
	  std::string newSuffix = *suf;
	  free(suf);
	  expandWildcard((char *)newPrefix.c_str(), (char *)newSuffix.c_str());
	  return;
  }

  //expanding component for wildcard
  std::string *expandedComponent = new std::string;
  *expandedComponent = "^";

  char *tempComponent = (char *)component.c_str();

  //getting first character
  char firstChar = tempComponent[0];

  while(*tempComponent != '\0') {
    if(*tempComponent == '*') {
      *expandedComponent += ".*";
    } else if(*tempComponent == '?') {
      *expandedComponent += ".";
    } else if(*tempComponent == '.') {
      *expandedComponent += "\\.";
    } else {
      std::string temp(1, *tempComponent);
      *expandedComponent += temp;
    }
    tempComponent++;
  }

  *expandedComponent += "$";
  std::string expandedComp = *expandedComponent;
  //free expandedComponent
  //printf("%s\n", (char *)expandedComp.c_str());
  free(expandedComponent);

  //compiling regex
  regex_t re;
  int result = regcomp(&re, (char *)expandedComp.c_str(), REG_EXTENDED|REG_NOSUB);
  if(result != 0) {
    fprintf(stderr, "Bad regex\n");
    exit(-1);
  }

  //opening the required directory
  std::string d;
  
  //if prefix is empty, open current dir else use prefix
  if(strcmp(prefix, "") == 0) {
    d = ".";
  } else {
    d = (std::string)prefix;
  }
  
  

  //checking if it is directory
  struct stat sb;
  if(stat((char *)d.c_str(), &sb) == -1) {
    perror("stat");
    exit(-1);
  }

  DIR * dir;
  if(S_ISDIR(sb.st_mode)) {
    dir = opendir((char *)d.c_str());
    if(dir == NULL) {
      perror("opendir");
      return;
    }
  } else {
    return;
  }


  //check for matches
  regmatch_t match;

  struct dirent * ent;

  while((ent = readdir(dir)) != NULL) {
    int res = regexec(&re, ent->d_name, 1, &match, 0);
    if(res == 0) {
      //for hidden file
      if(ent->d_name[0] == '.') {
        if(firstChar == '.') {
          //modify prefix and call expandWildcard
          char * file = strdup(ent->d_name);
	  std::string newFile = (std::string)file;
	  std::string newPrefix;
      
	  //to add "/" at beginning depends on whether prefix empty or not
          if(strcmp(prefix, "") && strcmp(prefix, "/") != 0) {
	    newPrefix = (std::string)prefix + "/" + newFile;
	  } else {
	    newPrefix = (std::string)prefix + newFile;
	  }

	  free(file);
	  expandWildcard((char *)newPrefix.c_str(), (char *)suf->c_str());                                                                            	  
	}  
      } else {
        //modify prefix and call expandWildcard
        char * file = strdup(ent->d_name);
        std::string newFile = (std::string)file;
        std::string newPrefix;

        //to add "/" at beginning depends on whether prefix empty or not
        if(strcmp(prefix, "") && strcmp(prefix, "/") != 0) {
          newPrefix = (std::string)prefix + "/" + newFile;
        } else {
          newPrefix = (std::string)prefix + newFile;
        }

        free(file);
        expandWildcard((char *)newPrefix.c_str(), (char *)suf->c_str());      
      }
    }
  }

  //calling free
  free(suf); 
  closedir(dir);
  regfree(&re);
}



Command Shell::_currentCommand;

std::vector<int> Shell::zombieprocess;

std::string Shell::relativePath;

int Shell::numberofArgs;

std::vector<std::string> Shell::newMatchedFiles;

