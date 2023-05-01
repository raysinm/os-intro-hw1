#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"
#include <limits.h>

using namespace std;

//---------------------------------------- Miscellenius Functions ----------------------------------------//

int getCurrDir(char* buf){
  if(buf != nullptr){
      if(getcwd(buf, PATH_MAX) != NULL){
        return 0;
     }
    }
  return -1;
}
//----------------------------------------------------------------------------------------------//



//---------------------------------------- String Manipulation Functions ----------------------------------------//

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

string _rtrim(const std::string& s) //returns a string (that includes WHITESPACE)
{
  size_t end = s.find_last_not_of(WHITESPACE);
  return (end == std::string::npos) ? "" : s.substr(0, end + 1);  //end==npos only if there was no non-whitespace character found.
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

// std::vector<string>& parse_cmd_line(const char** cmd_line)

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

vector<string>&  parse(const char* cmd_line){
  vector<string>* cmd_parsed = new vector<string>;
  
}

//----------------------------------------------------------------------------------------------//


//----------------------------------- SmallShell Class Methods  -----------------------------------//
SmallShell::SmallShell() :  prompt("smash"),
                            pid(getppid()),
                            last_dir(nullptr){
// TODO: add your implementation
//   this->prompt = "smash";
    
}

SmallShell::~SmallShell() {
// TODO: add your implementation
    if (last_dir != nullptr){
        free(last_dir);
    }
}

void SmallShell::setLastDir(){
  char* buf = (char*) malloc(PATH_MAX * sizeof(char));
  if(getCurrDir(buf) != 0){
    cout << "smash error: getcwd failed" << endl;
    return;
  }
  if(chdir(*last_dir) == -1){
    cout << "smash error: chdir failed" << endl;
    free(buf);
    return;
  }
  free(*last_dir);
  last_dir = &buf; 
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
  // vector<string> cmd_parsed = parse(cmd_line);

  std::string cmd_s = _trim(string(cmd_line));  // cmd_s is a string that includes whitespace within
  string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));
  
  if (firstWord.compare("chprompt") == 0) {
    return new ChangePromptCommand(cmd_line);
  }
  else if(firstWord.compare("showpid") == 0) {
    return new ShowPidCommand(cmd_line);
  }
  else if(firstWord.compare("pwd") == 0) {
    return new GetCurrDirCommand(cmd_line);
  }
  else if(firstWord.compare("cd") == 0) {
    return new ChangeDirCommand(cmd_line, this->last_dir);
  }
  else if(firstWord.compare("quit")==0){
    return new QuitCommand(cmd_line, jobs_list);
  }
  else if(firstWord.compare("fg")==0){
    return new FgCommand(cmd_line);
  }
  else if(firstWord.compare("bg")==0){
    return new BgCommand(cmd_line);
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
  if (cmd != nullptr){
    cmd->execute();
    delete cmd;
  }
  
  return;
}

std::string& SmallShell::get_prompt(){
  return this->prompt;
}

void SmallShell::set_prompt(const std::string& new_prompt){
  this->prompt = new_prompt;

}

//----------------------------------------------------------------------------------------------//


//----------------------------------- JobsList Class Methods  -----------------------------------//


JobsList::JobsList() : jobs_list(){}

void JobsList::addJob(Command* cmd, bool isStopped){
  int job_id = 0;
  
  // auto max_it = std::max(jobs_list.begin(), jobs_list.end(), 
  //       [](JobEntry& a, JobEntry& b) { return a.get_job_id() < b.get_job_id(); });
  //   if (max_it != jobs_list.end()) {
  //       job_id = max_it->get_job_id() + 1;
  //   }
  //   else{
  //     job_id += 1;
  //   }
  int max_job_id = 0;
  if (jobs_list.size() > 0){
    max_job_id = jobs_list.end()->get_job_id();
  }
  pid_t pid = cmd->pid; 
  time_t time;
  if (std::time(&time) < 0){
    return; //TODO: error handling. also, should we do this time thing here or outside?
  }
  jobs_list.push_back(JobEntry(job_id=(max_job_id+1), pid=pid, time, isStopped));
}

void JobsList::printJobsList(){

}
//----------------------------------------------------------------------------------------------//



//----------------------------------- Command Class Methods  -----------------------------------//

Command::Command(const char* cmd_line): cmd_line(cmd_line), pid(getpid()){}
BuiltInCommand::BuiltInCommand(const char* cmd_line): Command(cmd_line){}
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

//chprompt
ChangePromptCommand::ChangePromptCommand(const char* cmd_line) : BuiltInCommand(cmd_line){}

void ChangePromptCommand::execute(){
  //TODO: Consider parsing in a seperate helper function
  
  char** args_parsed = (char**) malloc((COMMAND_MAX_ARGS+1)* sizeof(char*));   //FIXME: 1. Currently takes name of command as first argument  
  if (args_parsed == nullptr){  //TODO: error handling
    return; //TODO: Maybe assert?
  }                                                                                        //2. Whats the right allocation size?
  int num_args = _parseCommandLine(cmd_line, args_parsed) - 1;

  SmallShell& smash = SmallShell::getInstance();
  // printf(*args_parsed);
  if (num_args >= 1){
    smash.set_prompt(string(args_parsed[1]));
  } else{
    smash.set_prompt("smash");
  }
  free(args_parsed);
  return;
}

//showpid
ShowPidCommand::ShowPidCommand(const char* cmd_line): BuiltInCommand(cmd_line){}

void ShowPidCommand::execute(){
    SmallShell& smash = SmallShell::getInstance();
    cout << "smash pid is " << smash.pid << endl;
    return;
}


//pwd
GetCurrDirCommand::GetCurrDirCommand(const char* cmd_line): BuiltInCommand(cmd_line){}

void GetCurrDirCommand::execute(){
    char* buf = (char*) malloc(PATH_MAX * sizeof(char));    //TODO: Change to MAX_PATH(?)
    if(getCurrDir(buf) != 0){
      cout << "smash error: getcwd failed" << endl;
    }
    cout << buf << endl;
    free(buf);
}

//cd
ChangeDirCommand::ChangeDirCommand(const char* cmd_line, char** plastPwd): BuiltInCommand(cmd_line),
                                                                            cmd_lastdir(plastPwd) {} //! Maybe better to allocate? I dont want memory leak

void ChangeDirCommand::execute(){
    char** args_parsed = (char**) malloc((COMMAND_MAX_ARGS+1)* COMMAND_ARGS_MAX_LENGTH);   //FIXME: 1. Currently takes name of command as first argument  
    if (args_parsed == nullptr){  //TODO: error handling
    return; //TODO: Maybe assert?
    }                                                                                        //2. Whats the right allocation size?
    int num_args = _parseCommandLine(this->cmd_line, args_parsed) - 1;
    
    if (num_args == 0){
      free(args_parsed);
      return;
    }

    SmallShell& smash = SmallShell::getInstance();

    if (num_args > 1){
      cout << "smash error: cd: too many arguments" << endl;
      return;
    }
    const char* path = args_parsed[1];
    char* buf = (char*) malloc(PATH_MAX * sizeof(char));
    if (buf == nullptr){  //TODO: error handling
    return; //TODO: Maybe assert?
    } 

    if (strcmp(path, "-") == 0){
      if(cmd_lastdir == nullptr){
        cout << "smash error: cd: OLDPWD not set" << endl;
        return;
      }
      else{
        smash.setLastDir();
      }
    }
    else{
      if(getCurrDir(buf) != 0){
        cout << "smash error: getcwd failed" << endl;
        free(args_parsed);
        free(buf);
        return;
      }  
      else if(chdir(path) == -1){
        cout << "smash error: chdir failed" << endl;
        free(args_parsed);
        free(buf);
        return;
      }
    }
    if(smash.last_dir != nullptr){
      free(*smash.last_dir);   ///less dangerous?
    }
    smash.last_dir = &buf;
    
    free(args_parsed);
    
    return;
}


//Fg
FgCommand::FgCommand(const char* cmd_line): BuiltInCommand(cmd_line) {}

void FgCommand::execute(){
    char** args_parsed = (char**) malloc((COMMAND_MAX_ARGS+1)* COMMAND_ARGS_MAX_LENGTH);   //FIXME: 1. Currently takes name of command as first argument  
    if (args_parsed == nullptr){  //TODO: error handling
    return; //TODO: Maybe assert?
    }                                                                                        //2. Whats the right allocation size?
    int num_args = _parseCommandLine(this->cmd_line, args_parsed) - 1;
    
    if ((num_args == 0) || (num_args > 1)){
      free(args_parsed);
      cout << "smash error: fg: invalid arguments" << endl;
      return;
    }

    SmallShell& smash = SmallShell::getInstance();
    const char* job_id = args_parsed[1];
  
}


//Bg
BgCommand::BgCommand(const char* cmd_line): BuiltInCommand(cmd_line) {}

void BgCommand::execute(){
  
}


//quit
QuitCommand::QuitCommand(const char* cmd_line, JobsList* jobs) : BuiltInCommand(cmd_line){}

void QuitCommand::execute(){
  exit(0);
  return;
}

//----------------------------------------------------------------------------------------------//
