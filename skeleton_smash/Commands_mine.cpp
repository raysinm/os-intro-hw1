#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"
#include <limits.h>

#include <algorithm>

using namespace std;

//---------------------------------------- Miscellenius Functions ----------------------------------------//

int getCurrDir(char* buf){
  if(getcwd(buf, PATH_MAX) != NULL){
    return 0;
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

vector<string>& _vectorize_cmdline(const char* cmd_line){
  
  std::vector<string>* cmd_vec = new std::vector<string>();
   
  char** args_parsed = (char**) malloc((COMMAND_MAX_ARGS+1)* sizeof(char*));   //FIXME: 1. Currently takes name of command as first argument  
  // if (args_parsed == nullptr){  //TODO: error handling
  //   delete cmd_vec;
  //   return nullptr; //TODO: Maybe assert?
  // }    

  int num_args = _parseCommandLine(cmd_line, args_parsed);
  // if ((num_args-1)>COMMAND_MAX_ARGS){ //TODO: MAKE THIS CHECK SOMEWHERE ELSE
  //   delete cmd_vec;
  //   free(args_parsed);
  //   return nullptr; //error handling
  // }
  for (int i=0; i<num_args; i++){
    cmd_vec->push_back(args_parsed[i]);
  }
  free(args_parsed);
  // cout << "Vectorize ok "; 
  return *cmd_vec; 

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

// vector<string>&  parse(const char* cmd_line){
//   vector<string>* cmd_parsed = new vector<string>;
  
// }


bool isAllDigits(string& s){
  for (char c : s) {
        if (!std::isdigit(c)) {
          return false;
        }
    }
    return true;
}

//----------------------------------------------------------------------------------------------//


//----------------------------------- SmallShell Class Methods  -----------------------------------//
SmallShell::SmallShell() :  prompt("smash"),
                            pid(getpid()),
                            last_dir(nullptr),
                            jobs_list(new JobsList()){
                              if (jobs_list == nullptr){
                                cout << "Problem in JobsList() ";
                              }
                              cout << "In smash ctor ";
                            }
// TODO: add your implementation
//   this->prompt = "smash";



SmallShell::~SmallShell() {
// TODO: add your implementation
    if (last_dir != nullptr){
        free(last_dir);
    }
    if (jobs_list!= nullptr){
      delete jobs_list;
    }
}

void SmallShell::setLastDir(){
  char* buf = (char*) malloc(PATH_MAX * sizeof(char));
  if(buf == nullptr){
    return; //error handling
  }
  if(getCurrDir(buf) != 0){
    cout << "smash error: getcwd failed" << endl;
    free(buf);
    return;
  }
  if(chdir(last_dir) == -1){
    cout << "smash error: chdir failed" << endl;
    free(buf);
    return;
  }
  if(last_dir!=nullptr){
    free(last_dir);
  }
  
  last_dir = buf; 
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
  
  // if (strstr(cmd_line, ">") || strstr(cmd_line, ">>")) {
  //   return new RedirectionCommand(cmd_line);
  // }
  // else if (strstr(cmd_line, "|") || strstr(cmd_line, "|&")) {
  //   return new PipeCommand(cmd_line);
  // }
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
    return new ChangeDirCommand(cmd_line);
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
  else if(firstWord.compare("jobs")==0){
    return new JobsCommand(cmd_line, jobs_list);
  }
  // else{
  //   return new ExternalCommand(cmd_line);
  // }
  return nullptr;
}

void SmallShell::executeCommand(const char *cmd_line) {
  // TODO: Add your implementation here
  // for example:
  // Command* cmd = CreateCommand(cmd_line);
  // cmd->execute();
  // Please note that you must fork smash process for some commands (e.g., external commands....)
  // cout << "Before CreateCommand " << cmd_line;
  Command* cmd = CreateCommand(cmd_line);
  if (cmd != nullptr){
    // cout << (*(cmd->cmd_vec))[0] << endl ;
    // cout << "Cmd pid " << cmd->pid;
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


// JobsList::JobsList(): jobs_list(){}

JobsList::~JobsList(){}

void JobsList::addJob(Command* cmd, bool isStopped){
  int job_id = 0;
  
  // auto max_it = std::max(jobs_list.begin(), jobs_list.end(), 
  //       [](JobEntry& a, JobEntry& b) { return a.getJobId() < b.getJobId(); });
  //   if (max_it != jobs_list.end()) {
  //       job_id = max_it->getJobId() + 1;
  //   }
  //   else{
  //     job_id += 1;
  //   }
  int max_job_id = 0;
  for (auto job : jobs_list){
    if (job.getJobId() > max_job_id){
        max_job_id = job.getJobId();
    }
  }

  pid_t pid = cmd->pid; 
  time_t time;
  // string cmd_name = (*(cmd->cmd_vec))[0]; //might be prone to bugs
  if (std::time(&time) < 0){
    return; //TODO: error handling. also, should we do this time thing here or outside?
  }
  jobs_list.push_back(JobEntry(job_id=(max_job_id+1), pid=pid, time, isStopped, (cmd->cmd_vec)));
}

void JobsList::printJobsList(){
  // bool wtf = jobs_list
  // cout << wtf;
  if (jobs_list.size() == 0){
    return; //empty list
  }
  for(auto& job : jobs_list){
    // cout << job;
    cout << "[" << job.getJobId() << "] ";
    cout << job.getCmdName() << " : ";
    cout << job.getJobPid() << " ";
    cout << job.getTimeElapsed() << " ";
    if (job.isStopped()){
      cout << "(stopped)";
    }
    cout << endl;

  }
}
JobsList::JobEntry * JobsList::getJobById(int jobId){

  // int target_id = std::stoi(cmd_vec[2]);   ///TODO: check the format (stoi will fail if thereare chars)
  int target_id = jobId;
  auto it = std::find_if(jobs_list.begin(), jobs_list.end(), 
    [&target_id](JobsList::JobEntry& job) { return job.getJobId() == target_id; });
  
  if (it != jobs_list.end()) {
    return &(*it);
  } else {
    return nullptr;
  }
}

JobsList::JobEntry* JobsList::getLastJob(){  //Used to be getLastJob(int* lastJobId) but don't need the int
  // Returns job with max id or nullptr if job_list is empty
  if (jobs_list.size() == 0){
    return nullptr;
  }
  return &(jobs_list.back());
}

JobsList::JobEntry* JobsList::getLastStoppedJob(){
  // Return job with max id which is also stopped, or nullptr if list empty/no stopped jobs
  if (jobs_list.size() == 0){
    return nullptr;
  }
  int max_jobid = 0 ;
  JobEntry* ret = nullptr;
  for(auto& job : jobs_list){
    if (job.isStopped() && job.getJobId()>max_jobid){
      max_jobid = job.getJobId();
      ret = &job;
    }
  }
  return ret;
}

void JobsList::removeJobById(int jobId){
  for (auto it=jobs_list.begin(); it != jobs_list.end(); it++){
    if (it->getJobId() == jobId){
      //Found the job
      jobs_list.erase(it);  //shifts rest of items to fill gap
      return;
    }
  }
  return;
}

void JobsList::killAllJobs(){
  // Sends a SIGKILL to all jobs in the list and updates their 'is_finished'
  for (auto& job : jobs_list){
    string cmd_line = "";
    for (auto str : job.getCmdVec()){
      cmd_line += str;
    }
    cout << job.getJobPid() << cmd_line;
    kill(job.getJobPid(), SIGKILL);
    job.markFinished();
  }
}

void JobsList::removeFinishedJobs(){
  // Removes form list jobs that are finished
  for (auto it=jobs_list.begin(); it != jobs_list.end(); it++){
    if (it->isFinished()){
      jobs_list.erase(it);
    }
  }
  return;
}

//----------------------------------------------------------------------------------------------//



//----------------------------------- Command Class Methods  -----------------------------------//

// Command::Command(const char* orig_cmd_line): cmd_line(new char[strlen(orig_cmd_line)+1]), pid(getpid()), cmd_vec(_vectorize_cmdline(orig_cmd_line)){
//   // cout << "In Command ";
//   // cout << this;
//   // cout << this->cmd_vec;
//   strcpy(this->cmd_line, orig_cmd_line);
// }

Command::Command(const char* orig_cmd_line): cmd_line(new char[strlen(orig_cmd_line)+1]), pid(getpid()){
  cmd_vec = _vectorize_cmdline(orig_cmd_line);
  cout << "In Command ";
  // cout << this;
  // cout << this->cmd_vec;
  strcpy(this->cmd_line, orig_cmd_line);
}

Command::~Command(){
  if (this->cmd_line != nullptr){
    delete this->cmd_line;
  }
}

BuiltInCommand::BuiltInCommand(const char* cmd_line): Command(cmd_line){
  // cout << "In BuiltInCommand ";
  // cout << this;
}
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
  
  // char** args_parsed = (char**) malloc((COMMAND_MAX_ARGS+1)* sizeof(char*));   //FIXME: 1. Currently takes name of command as first argument  
  // if (args_parsed == nullptr){  //TODO: error handling
  //   return; //TODO: Maybe assert?
  // }                                                                                        //2. Whats the right allocation size?
  // // int num_args = _parseCommandLine(cmd_line, args_parsed) - 1;
  // if(this->cmd_vec==nullptr){
  //   return; //error handling
  // }
  int num_args = cmd_vec.size() - 1;
  SmallShell& smash = SmallShell::getInstance();
  // printf(*args_parsed);
  if (num_args >= 1){
    smash.set_prompt(cmd_vec[1]);
  } else{
    smash.set_prompt("smash");
  }
  //No need to free anything, cmd_vec will det deleted in cmd destroyer
  // free(args_parsed);
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
    if(buf == NULL){
      return; //error handling
    }
    if(getCurrDir(buf) != 0){
      cout << "smash error: getcwd failed" << endl;
      free(buf);
      return;
    }
    cout << buf << endl;
    free(buf);
}

//cd
ChangeDirCommand::ChangeDirCommand(const char* cmd_line): BuiltInCommand(cmd_line){}
                                                                           
void ChangeDirCommand::execute(){
    // char** args_parsed = (char**) malloc((COMMAND_MAX_ARGS+1)* COMMAND_ARGS_MAX_LENGTH);   //FIXME: 1. Currently takes name of command as first argument  
    // if (args_parsed == nullptr){  //TODO: error handling
    // return; //TODO: Maybe assert?
    // }                                                                                        //2. Whats the right allocation size?
    // int num_args = _parseCommandLine(this->cmd_line, args_parsed) - 1;
    
    // if(this->cmd_vec==nullptr){
    //   return; //error handling
    // }
    int num_args = cmd_vec.size() - 1;

    if (num_args == 0){
      // free(args_parsed);
      return;
    }

    SmallShell& smash = SmallShell::getInstance();

    if (num_args > 1){
      cout << "smash error: cd: too many arguments" << endl;
      return;
    }
    const char* path = cmd_vec[1].c_str();

    char* buf = (char*) malloc(PATH_MAX * sizeof(char));
    if (buf == nullptr){  //TODO: error handling
    return; //TODO: Maybe assert?
    } 

    if (strcmp(path, "-") == 0){
      if(smash.last_dir == nullptr){
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
        // free(args_parsed);
        free(buf);
        return;
      }  
      else if(chdir(path) == -1){
        cout << "smash error: chdir failed" << endl;
        // free(args_parsed);
        free(buf);
        return;
      }
    }
    if(smash.last_dir != nullptr){
      free(smash.last_dir);   ///less dangerous?
    }
    smash.last_dir = buf;
    
    // free(args_parsed);
    
    return;
}


//fg
FgCommand::FgCommand(const char* cmd_line): BuiltInCommand(cmd_line) {}

void FgCommand::execute(){
    // char** args_parsed = (char**) malloc((COMMAND_MAX_ARGS+1)* COMMAND_ARGS_MAX_LENGTH);   //FIXME: 1. Currently takes name of command as first argument  
    // if (args_parsed == nullptr){  //TODO: error handling
    // return; //TODO: Maybe assert?
    // }                                                                                        //2. Whats the right allocation size?
    // int num_args = _parseCommandLine(this->cmd_line, args_parsed) - 1;
    
    // if(this->cmd_vec==nullptr){
    //   return; //error handling
    // }
    int num_args = cmd_vec.size() - 1;

    if (num_args > 1 || isAllDigits(cmd_vec[1])){
      // free(args_parsed);
      cout << "smash error: fg: invalid arguments" << endl;
      return;
    }

    SmallShell& smash = SmallShell::getInstance();
    int job_id = std::stoi(cmd_vec[1]);
    JobsList::JobEntry *job;

    if(num_args == 0){
      job = smash.jobs_list->getLastJob();
      if (job == nullptr){
        cout << "smash error: fg: jobs list is empty" << endl;
        return;
      }
    }
    else{
      job = smash.jobs_list->getJobById(job_id);
      if(job == nullptr){
        cout << "smash error: fg: job-id " << job_id << " does not exist" << endl;
        return;
      }
    }
    
    cout << job->getCmdName() << ":" << job->getJobPid() << endl;

    if(job->isStopped()){
      if (kill(job->getJobPid(), SIGCONT) == -1) {
        perror("smash error: kill failed");
        return;
      }
    }

    int status;
    smash.jobs_list->removeJobById(job_id);
    if (waitpid(job->getJobPid(), &status, WUNTRACED) == -1) {
      perror("smash error: waitpid failed");
      return;
    }
}


//bg
BgCommand::BgCommand(const char* cmd_line): BuiltInCommand(cmd_line) {}

void BgCommand::execute(){
  // if(this->cmd_vec==nullptr){
  //     return; //error handling
  //   }
    int num_args = cmd_vec.size() - 1;

    if (num_args > 1 || isAllDigits(cmd_vec[1])){
      // free(args_parsed);
      cout << "smash error: bg: invalid arguments" << endl;
      return;
    }

    SmallShell& smash = SmallShell::getInstance();
    int job_id = std::stoi(cmd_vec[1]);
    JobsList::JobEntry *job;

    if(num_args == 0){
      job = smash.jobs_list->getLastStoppedJob();
      if (job == nullptr){
        cout << "smash error: bg: there is no stopped jobs to resume" << endl;
        return;
      }
    }
    else{
      job = smash.jobs_list->getJobById(job_id);
      if(job == nullptr){
        cout << "smash error: bg: job-id " << job_id << " does not exist" << endl;
        return;
      }
      if(!job->isStopped()){
        cout << "smash error: bg: job-id " << job_id << " is already running in the background" << endl;
      }
    }

    cout << job->getCmdName() << ":" << job->getJobPid() << endl;

    if (kill(job->getJobPid(), SIGCONT) == -1) {
        perror("smash error: kill failed");
        return;
    }

    job->continueJob();
  
}

//jobs
JobsCommand::JobsCommand(const char* cmd_line, JobsList* jobs): BuiltInCommand(cmd_line){
  this->jobs = jobs; 
}

void JobsCommand::execute(){
  if (jobs != nullptr){
    jobs->printJobsList();
  }
  // cout << "jobs print ok ";
}

//quit
QuitCommand::QuitCommand(const char* cmd_line, JobsList* jobs) : BuiltInCommand(cmd_line), jobs(jobs){
  cout << this;
}


void QuitCommand::execute(){
  cout << "Got to quit execute ";
  if (cmd_vec[1] == "kill"){
    cout << "sending SIGKILL signal to " << jobs->jobs_list.size() << "jobs" << endl;
    jobs->killAllJobs();
  }
  for(auto job : jobs->jobs_list){
      cout << "Job #" << job.getJobId() << endl;
      kill(job.getJobPid(), SIGKILL);
  }
  
  exit(0);
  return;
}

//kill
KillCommand::KillCommand(const char* cmd_line, JobsList* jobs): BuiltInCommand(cmd_line), jobs(jobs){}

void KillCommand::execute(){
  
  // if(cmd_vec == nullptr){
  //   return; //error handling
  // }
  int num_args = cmd_vec.size() - 1;
  string job_id = cmd_vec[2];
  string& sig = cmd_vec[1];  //TODO: check format!!
  
  if (num_args != 2 || !isAllDigits(job_id) || sig[0]!='-'){
    cout << "smash error: kill: invalid arguments" << endl;
    return; //error handling
  }
  
  auto job = jobs->getJobById(std::stoi(job_id));
  if (job == nullptr){
    // job not found
    std::cout << "smash error: kill: job-id " << job_id << " does not exist" << std::endl;
  }
  else{
    int sig_num = std::stoi(sig.substr(1));
    if (kill(job->getJobPid(), sig_num) != 0){
      cout << "TODO : Handle with perror" << endl;
    }
    cout << "signal number " << sig_num << " was sent to pid " << job->getJobPid() << endl;
  }
  
}

//----------------------------------------------------------------------------------------------//
