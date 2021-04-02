#ifndef shell_hh
#define shell_hh

#include "command.hh"

struct Shell {

  static void prompt();

  static Command _currentCommand;

  static std::vector<int> zombieprocess;

  static std::string relativePath;

  static int numberofArgs;

  static std::vector<std::string> newMatchedFiles;

};

#endif
