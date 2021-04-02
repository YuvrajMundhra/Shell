#ifndef command_hh
#define command_hh

#include "simpleCommand.hh"

// Command Data Structure

struct Command {
  std::vector<SimpleCommand *> _simpleCommands;
  std::string * _outFile;
  std::string * _inFile;
  std::string * _errFile;
  std::vector<char *> args;
  bool _background;
  bool _append;

  //return code for last executed simple command
  static int status;

  //last argument in fully expanded previous command
  static std::string lastArg;

  //flag for error
  bool _error;

  Command();
  void insertSimpleCommand( SimpleCommand * simpleCommand );

  void clear();
  void print();
  void execute();

  static SimpleCommand *_currentSimpleCommand;
};

#endif
