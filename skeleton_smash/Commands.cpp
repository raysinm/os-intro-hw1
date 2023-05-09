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

void Command::_vectorize_cmdline(const char* cmd_line){
  
  char** args_parsed = new char*[COMMAND_MAX_ARGS+1];
  int num_args = _parseCommandLine(cmd_line, args_parsed);
  
  for (int i=0; i<num_args; i++){
    this->cmd_vec.push_back(string(args_parsed[i]));
    // cout<< *this->cmd_vec.begin();
    free(args_parsed[i]);
  }

  delete[] args_parsed;
  
  // cout << "Vectorize ok "; 
  return;
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
                            last_dir("NAN"),
                            jobs_list(new JobsList()){

  // if (jobs_list == nullptr){
  //   cout << "Problem in JobsList() ";
  // }
  // cout << "In smash ctor ";

  }


SmallShell::~SmallShell() {

    if (jobs_list!= nullptr){
      delete jobs_list;
    }
}

void SmallShell::setLastDir(){
  char* buf = new char[PATH_MAX];

  if(getCurrDir(buf) != 0){
    cout << "smash error: getcwd failed" << endl;
    delete[] buf;
    return;
  }
  if(chdir(last_dir.c_str()) == -1){
    cout << "smash error: chdir failed" << endl;
    delete[] buf;
    return;
  }
  
  last_dir = string(buf); 
  delete[] buf;
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command * SmallShell::CreateCommand(const char* cmd_line) {
  std::string cmd_s = _trim(string(cmd_line));  // cmd_s is a string that includes whitespace within
  string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));
  
  if (strstr(cmd_line, ">") || strstr(cmd_line, ">>")) {
    return new RedirectionCommand(cmd_line);
  }
  else if (strstr(cmd_line, "|") || strstr(cmd_line, "|&")) {
    return new PipeCommand(cmd_line);
  }
  else if (firstWord.compare("chprompt") == 0) {
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
  else{ // Currently only external commands (notice is_bg is only for external)
    return new ExternalCommand(cmd_line);

  }
  return nullptr;
}

void SmallShell::executeCommand(const char *cmd_line) {

  Command* cmd = CreateCommand(cmd_line);
  if (cmd != nullptr){
    // cout << (*(cmd->cmd_vec))[0] << endl ;
    // cout << "Cmd pid " << cmd->pid;
    this->jobs_list->removeFinishedJobs();  // Remove finished jobs
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

void SmallShell::checkJobs(){
  
}
//----------------------------------------------------------------------------------------------//


//----------------------------------- JobsList Class Methods  -----------------------------------//


// JobsList::JobsList(): jobs_list(){}

JobsList::~JobsList(){}

void JobsList::addJob(Command* cmd, bool isStopped){
  
  // Remove finished jobs
  this->removeFinishedJobs();

  int max_job_id = 0;
  if (jobs_list.size() > 0){
    for (auto job : jobs_list){
      if (job.getJobId() > max_job_id){
          max_job_id = job.getJobId();
      }
    }
  }

  // pid_t pid = cmd->pid; 
  time_t time;
  if (std::time(&time) < 0){
    cout << "time error ";
    return; //TODO: error handling. also, should we do this time thing here or outside?
  }
  jobs_list.push_back(JobEntry((max_job_id+1), time, isStopped, cmd));
  // cout << "addJob ok ";
}

void JobsList::printJobsList(){
  // Remove finished jobs

  cout << "before remove finished ";
  this->removeFinishedJobs();
  cout << "after remove finished ";
  // if (jobs_list.size() == 0){
  //   return; //empty list
  // }
  for(auto& job : jobs_list){
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

void JobsList::killAllJobs(int sig){
  // Sends 'sig' to all jobs in the list
  for (auto& job : jobs_list){
    string cmd_line = "";
    for (auto str : job.getCmdVec()){
      cmd_line += str += " ";
    }
    cout << job.getJobPid() << " " << cmd_line << endl;
    kill(job.getJobPid(), sig);
    
    // If sig -- SIGKIL updates the jobs' 'is_finished' to finished
    if (sig == SIGKILL){
      job.markFinished();
    }  
  }
}

void JobsList::removeFinishedJobs(){  
  // Checks if *non-stopped* jobs are finished
  for (auto it=jobs_list.begin(); it != jobs_list.end(); it++){
    int status;
    if(it->isBackground() && waitpid(it->getJobPid(), &status, WNOHANG)!=0){
      it->markFinished();
    }
  }
  
  // Removes form list all jobs that are finished
  for (auto it=jobs_list.begin(); it != jobs_list.end(); it++){
    if (it->isFinished()){
      jobs_list.erase(it);
    }
  }
  return;
}


//----------------------------------------------------------------------------------------------//



//----------------------------------- Command Class Methods  -----------------------------------//



Command::Command(const char* orig_cmd_line): cmd_line(new char[strlen(orig_cmd_line)+1]), pid(getpid()),
                                             cmd_vec(), is_bg(false){
  
  strcpy(this->cmd_line, orig_cmd_line);
  if(_isBackgroundComamnd(orig_cmd_line)){
    _removeBackgroundSign(this->cmd_line);
    this->is_bg = true;
  }
  
  this->_vectorize_cmdline(this->cmd_line);
        
  // cout << "In Command ";
  // cout << this;
  // cout << this->cmd_vec;
}

Command::~Command(){
  if (this->cmd_line != nullptr){
    delete[] this->cmd_line;
  }
}

//----------------------------------- BuiltInCommand Class Methods  -----------------------------------//

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
  
  int num_args = cmd_vec.size() - 1;
  SmallShell& smash = SmallShell::getInstance();
  // printf(*args_parsed);
  if (num_args >= 1){
    smash.set_prompt(cmd_vec[1]);
  } else{
    smash.set_prompt("smash");
  }

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
    char* buf = (char*) malloc(PATH_MAX * sizeof(char));
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
    
    int num_args = cmd_vec.size() - 1;
    if (num_args == 0){ //error handling
      return;
    }

    SmallShell& smash = SmallShell::getInstance();

    if (num_args > 1){
      cout << "smash error: cd: too many arguments" << endl;
      return;
    }
    const char* path = cmd_vec[1].c_str();

    char* buf = new char[PATH_MAX];
    if (buf == nullptr){  //TODO: error handling
    return; //TODO: Maybe assert?
    } 

    if (strcmp(path, "-") == 0){
      if(smash.last_dir == "NAN"){
        cout << "smash error: cd: OLDPWD not set" << endl;
        delete[] buf;
        return;
      }
      else{
        smash.setLastDir();
      }
    }
    else{
      if(getCurrDir(buf) != 0){
        cout << "smash error: getcwd failed" << endl;
        delete[] buf;
        return;
      }  
      else if(chdir(path) == -1){
        cout << "smash error: chdir failed" << endl;
        delete[] buf;
        return;
      }
    }

    smash.last_dir = string(buf);
    delete[] buf;
    
    return;
}


//fg
FgCommand::FgCommand(const char* cmd_line): BuiltInCommand(cmd_line) {}

void FgCommand::execute(){
  
    int num_args = cmd_vec.size() - 1;

    if (num_args > 1 || isAllDigits(cmd_vec[1])){
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

    int num_args = cmd_vec.size() - 1;

    if (num_args > 1 || isAllDigits(cmd_vec[1])){
      cout << "smash error: bg: invalid arguments" << endl;
      return;
    }

    SmallShell& smash = SmallShell::getInstance();
    int job_id = -1;
    JobsList::JobEntry *job;

    if(num_args == 0){
      job = smash.jobs_list->getLastStoppedJob();
      if (job == nullptr){
        cout << "smash error: bg: there is no stopped jobs to resume" << endl;
        return;
      }
    }
    else{
      job_id = std::stoi(cmd_vec[1]);
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
JobsCommand::JobsCommand(const char* cmd_line, JobsList* jobs): BuiltInCommand(cmd_line), jobs(jobs){
  cout << "In JobsCommand ctor";
}

void JobsCommand::execute(){
  cout << "in jobs ";
  if (jobs != nullptr){
    jobs->printJobsList();
  }
  // cout << "jobs print ok ";
}

//quit
QuitCommand::QuitCommand(const char* cmd_line, JobsList* jobs) : BuiltInCommand(cmd_line), jobs(jobs){
  // cout << this;
}


void QuitCommand::execute(){
  // cout << "Got to quit execute ";

  if (cmd_vec.size()>1 && cmd_vec[1] == "kill"){
    cout << "sending SIGKILL signal to " << jobs->jobs_list.size() << " jobs" << endl;
    jobs->killAllJobs(SIGKILL);
  }
  // for(auto job : jobs->jobs_list){
  //     // cout << " Job # " << job.getJobId() << endl;
  //     kill(job.getJobPid(), SIGKILL);
  // }
  // atexit(cleanup); //Maybe we should destroy everything on the way out
  exit(0);
}

//kill
KillCommand::KillCommand(const char* cmd_line, JobsList* jobs): BuiltInCommand(cmd_line), jobs(jobs){}

void KillCommand::execute(){

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

//----------------------------------- ExternalCommand Class Methods  -----------------------------------//

ExternalCommand::ExternalCommand(const char* cmd_line): Command(cmd_line){}

void ExternalCommand::execute(){
  SmallShell& smash = SmallShell::getInstance();

  // cout << "pid before fork " << this->pid << " || ";
  pid_t pid = fork(); // danger: error handling ?
  this->pid = pid;  
  if(pid == 0) // son procces
  {
    if (setpgrp() == -1) {
      perror("smash error: setpgrp failed");
      return;
    }
    // string trimmed_cmd_line = _trim(string(cmd_line));  //! danger: Doesnt trim inside spaces
    // char cmd_line_array[COMMAND_ARGS_MAX_LENGTH]; //It's not an array, its a char*
    // strcpy(cmd_line_array, trimmed_cmd_line.c_str()); 
    
    // char argv[COMMAND_ARGS_MAX_LENGTH*COMMAND_MAX_ARGS];
    string cmd_string = "";
    string cmd_name_string = cmd_vec[0];
    char* cmd_name = const_cast<char*>(cmd_name_string.c_str()); // Danger: error handling (e.g empty vec)
    for (size_t i=1; i<cmd_vec.size(); i++){
      cmd_string += cmd_vec[i];
      if (i < cmd_vec.size()-1){
        cmd_string += " ";
      }
    }
    cmd_string += "\0";

    // char *args[2];
    // args[0] = const_cast<char*>(cmd_vec[0].c_str());  // First arg - name of executable
    // args[1] = const_cast<char*>(cmd_string.c_str());  // Second arg - 

    if(strstr(cmd_line, "*") || strstr(cmd_line, "?")) // complex external command run using bash
    {
      cmd_string = cmd_name_string + cmd_string;  // In complex external, we need the command name in the beginning
      char* cmd_string_char = const_cast<char*>(cmd_string.c_str());  // Danger: conversion- string to const char* to char*
      // char bash_path[] = "/bin/bash";
      char flag[] = "-c";
      char *args_bash[] = {"/bin/bash", "-c", cmd_string_char, NULL};

      if (execv(args_bash[0], args_bash) == -1) {  
          perror("smash error: execv failed");
          return;
      }

    }
    else  //simple external command run using execv syscalls
    {
      char* cmd_string_char = const_cast<char*>(cmd_string.c_str());
      // cout << "In simple external ";
      char *args[] = {cmd_name, cmd_string_char ,NULL}; 
      if (execvp(args[0], args) == -1) {
          perror("smash error: execv failed");
          return;
      }
    }
  }
  else // father procces
  {
    if(!this->is_bg){
      int status;
      if(waitpid(pid, &status, WUNTRACED) == -1){
      perror("smash error: waitpid failed");
      }
    }
    else{
      // cout << "pid after fork " << this->pid ;
      smash.jobs_list->addJob(this);
      // smash.jobs_list->printJobsList();
      cout << "add job ok after fork ";
    }
  }
}

//----------------------------------------------------------------------------------------------//

//----------------------------------- PipeCommand Class Methods  -----------------------------------//

PipeCommand::PipeCommand(const char* cmd_line): Command(cmd_line){}

void PipeCommand::execute(){
  string s = string(cmd_line);
  string pipeType = s.find("|&") == string::npos ? "|" : "|&";
  int i = s.find(pipeType);
  string command1 = s.substr(0,i-1);
  string command2;
  if(pipeType == "|&") {  //redirect stderr
    command2 = s.substr(i+2, s.length());
  }
  else {  //redirect stdout
    command2 = s.substr(i+1, s.length());
  }
}


//----------------------------------------------------------------------------------------------//

//----------------------------------- RedirectionCommand Class Methods  -----------------------------------//

RedirectionCommand::RedirectionCommand(const char* cmd_line): Command(cmd_line){}

void RedirectionCommand::execute(){
  string s = string(cmd_line);
  string redirectionType = s.find(">>") == string::npos ? ">" : ">>";
  int i = s.find(redirectionType);
  string command = s.substr(0,i-1);
  string output_file;
  if(redirectionType == ">>") {  //append
    output_file = s.substr(i+2, s.length());
  }
  else {  //override
    output_file = s.substr(i+1, s.length());
  }
}

//----------------------------------------------------------------------------------------------//
