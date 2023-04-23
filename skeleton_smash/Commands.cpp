#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"

using namespace std;

const std::string WHITESPACE = " \n\r\t\f\v";

#if 0
#define FUNC_ENTRY()  \
  cout << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cout << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

string _ltrim(const std::string& s)
{
  size_t start = s.find_first_not_of(WHITESPACE);
  return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string& s)
{
  size_t end = s.find_last_not_of(WHITESPACE);
  return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string& s)
{
  return _rtrim(_ltrim(s));
}

int _parseCommandLine(const char* cmd_line, char** args) {
  FUNC_ENTRY()
  int i = 0;
  std::istringstream iss(_trim(string(cmd_line)).c_str());
  for(std::string s; iss >> s; ) {
    args[i] = (char*)malloc(s.length()+1);
    memset(args[i], 0, s.length()+1);
    strcpy(args[i], s.c_str());
    args[++i] = NULL;
  }
  return i;

  FUNC_EXIT()
}

bool _isBackgroundComamnd(const char* cmd_line) {
  const string str(cmd_line);
  return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(char* cmd_line) {
  const string str(cmd_line);
  // find last character other than spaces
  unsigned int idx = str.find_last_not_of(WHITESPACE);
  // if all characters are spaces then return
  if (idx == string::npos) {
    return;
  }
  // if the command line does not end with & then return
  if (cmd_line[idx] != '&') {
    return;
  }
  // replace the & (background sign) with space and then remove all tailing spaces.
  cmd_line[idx] = ' ';
  // truncate the command line string up to the last non-space character
  cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}

// TODO: Add your implementation for classes in Commands.h 



SmallShell::SmallShell() {
// TODO: add your implementation
  this->prompt = "smash";
}

SmallShell::~SmallShell() {
// TODO: add your implementation
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command * SmallShell::CreateCommand(const char* cmd_line) {
	// For example:
/*
  string cmd_s = _trim(string(cmd_line));
  string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));

  if (firstWord.compare("pwd") == 0) {
    return new GetCurrDirCommand(cmd_line);
  }
  else if (firstWord.compare("showpid") == 0) {
    return new ShowPidCommand(cmd_line);
  }
  else if ...
  .....
  else {
    return new ExternalCommand(cmd_line);
  }
  */

 //TODO: parse command!
  
  std::string cmd_s = _trim(string(cmd_line));
  string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));
  
  if (firstWord.compare("chprompt") == 0) {
    return new ChangePromptCommand(cmd_line);
  }
  return nullptr;
}

void SmallShell::executeCommand(const char *cmd_line) {
  // TODO: Add your implementation here
  // for example:
  // Command* cmd = CreateCommand(cmd_line);
  // cmd->execute();
  // Please note that you must fork smash process for some commands (e.g., external commands....)
  
  Command* cmd = CreateCommand(cmd_line);
  cmd->execute();
  return;
}

void SmallShell::change_prompt(std::string& new_prompt) {
  this->prompt = new_prompt;
  return;
}


/*****  Command Implementations  ****/
/*
What we know:
1. Each command gets cmd_line as is
2. Each command has to know:
    a. number of args
    b. what are the args
3. ExecuteCommand calls CreateCommand that initializes a certain command based on the first word in cmd_line
4. Each command gets an instance of the shell (singleton method!).
5. Don't forget freeing lists of args allocated.
*/
void ChangePromptCommand::execute(){
  //TODO: Consider parsing in a seperate helper function
  
  char** args_parsed = (char**) malloc((COMMAND_MAX_ARGS+1)* COMMAND_ARGS_MAX_LENGTH);   //FIXME: 1. Currently takes name of command as first argument  
  if (args_parsed == nullptr)
  {
    return; //TODO: Maybe assert?
  }                                                                                        //2. Whats the right allocation size?
  int num_args = _parseCommandLine(this->cmd_line, args_parsed) - 1;


  SmallShell& smash = SmallShell::getInstance();
  printf(*args_parsed);
  if (num_args >= 1){
    smash.change_prompt(string(args_parsed[1]));
  } else{
    smash.change_prompt(string("smash"));
  }
  // smash.change_prompt(args_parsed[1]);
  free(args_parsed);
  return;

}