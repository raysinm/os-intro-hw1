//*** RUN TEST: tests/runner/runner.sh <test_name> (or test_name* to run many tests from the same category)


#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"

#include <limits.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <algorithm>

using namespace std;

//---------------------------------------- Miscellenius Functions ----------------------------------------//

int getCurrDir(char* buf){
  if(getcwd(buf, PATH_MAX) == NULL){
    perror("smash error: getcwd failed");
    return -1;
  }
  return 0;
}
//----------------------------------------------------------------------------------------------//



//---------------------------------------- String Manipulation Functions ----------------------------------------//

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
    free(args_parsed[i]);
  }

  delete[] args_parsed;
  
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
                            jobs_list(new JobsList()),
                            fg_cmd(nullptr){

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
    // cout << "smash error: getcwd failed" << endl;
    delete[] buf;
    return;
  }
  if(chdir(last_dir.c_str()) == -1){
   perror("smash error: chdir failed");
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
  else if(firstWord.compare("kill")==0){
    return new KillCommand(cmd_line, jobs_list);
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
    return new JobsCommand(cmd_line);
  }
  else if(firstWord.compare("setcore")==0){
    return new SetcoreCommand(cmd_line);
  }
  else if(firstWord.compare("getfiletype")==0){
    return new GetFileTypeCommand(cmd_line);
  }
  else if(firstWord.compare("chmod")==0){
    return new ChmodCommand(cmd_line);
  }
  else{ // Currently only external commands (notice is_bg is only for external)
    ExternalCommand* ext_cmd = new ExternalCommand(cmd_line);
    this->fg_cmd = ext_cmd;
    return ext_cmd;

  }
  return nullptr;
}

void SmallShell::executeCommand(const char *cmd_line) {

  Command* cmd = CreateCommand(cmd_line);
  this->jobs_list->removeFinishedJobs();  // Remove finished jobs
  if (cmd != nullptr){
    // cout << "DEBUG: cmd_vec:";
    // for (auto& elm : cmd->cmd_vec){
    //   cout << elm << " | ";
    // }
    // cout << "DEBUG: current command:" << cmd->cmd_line_str << "." <<endl;
    cmd->execute();
    delete cmd;
    this->fg_cmd = nullptr;
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


JobsList::JobsList(): jobs_list(), fg_job(nullptr){}

JobsList::JobEntry::JobEntry(const int job_id, pid_t pid, const time_t init_time, bool is_stopped, bool is_bg, vector<string> cmd_vec, string cmd_line) : job_id(job_id), 
                                      pid(pid),
                                      init_time(init_time),
                                      is_stopped(is_stopped),
                                      is_background(is_bg),
                                      is_finished(false),
                                      cmd_vec(cmd_vec),
                                      cmd_line(cmd_line){
  this->cmd_line_nobg = this->cmd_line;
  size_t pos = this->cmd_line.find_last_not_of(WHITESPACE);
  if (pos != std::string::npos) {
    cmd_line_nobg.erase(pos, 1);
  }
                                      }

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

  time_t time_now;
  if (std::time(&time_now) < 0){
    perror("smash error: time failed");
    return; 
  }

  JobEntry new_job = JobEntry((max_job_id+1), cmd->pid, time_now, isStopped, cmd->is_bg, cmd->cmd_vec, cmd->cmd_line_str);
  this->jobs_list.push_back(new_job);
}

void JobsList::addFgJob(){
  /** Useful when doing fg and then ctrlZ. need to put the job back to the list with same id! **/
  auto it=jobs_list.begin();
  for (; (it!=jobs_list.end()) && (it->getJobId() < fg_job->getJobId()); ++it){
    // Do nothing, just move iterator until we find plasce for the job
  }
  time_t time_now;
  if (std::time(&time_now) < 0){
    perror("smash error: time failed");
    return; 
  }

  this->fg_job->setInitTime(time_now);
  this->fg_job->stopJob();

  jobs_list.insert(it, *fg_job);
  this->fg_job = nullptr;
}

void JobsList::printJobsList(){
  // Remove finished jobs
  this->removeFinishedJobs();

  time_t time_now;
  std::time(&time_now);
  for(auto& job : jobs_list){
    cout << "[" << job.getJobId() << "] ";
    cout << job.getCmdLine() << " : ";
    cout << job.getJobPid() << " ";
    cout << difftime(time_now,job.getInitTime()) << " secs";
    if (job.isStopped()){
      cout << " (stopped)";
    }
    cout << endl;
  }
}
JobsList::JobEntry * JobsList::getJobById(int jobId){

  int target_id = jobId;
  auto it = std::find_if(jobs_list.begin(), jobs_list.end(), 
    [&target_id](JobsList::JobEntry& job) { return job.getJobId() == target_id; });

  if (it != jobs_list.end()) {
    return &(*it);    // Job found
  } else {
    return nullptr;   // Job NOT found
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
  if (jobId < 1){
    return;
  }
  for (auto it=jobs_list.begin(); it != jobs_list.end(); it++){
    if (it->getJobId() == jobId){
      //Found the job
      // this->fg_job = new JobEntry(*it);
      jobs_list.erase(it);  //shifts rest of items to fill gap
      return;
    }
  }
  return;
}

void JobsList::killAllJobs(int sig){
  // Sends 'sig' to all jobs in the list
  for (auto& job : jobs_list){
    // string cmd_line = "";
    // for (auto str : job.getCmdVec()){
    //   cmd_line += str += " ";
    // }
    cout << job.getJobPid() << ": " << job.getCmdLine() << endl;
    kill(job.getJobPid(), sig);
    
    // If sig -- SIGKIL updates the jobs' 'is_finished' to finished
    if (sig == SIGKILL){
      job.markFinished();
    }  
  }
}

void JobsList::removeFinishedJobs(){  
  // Checks if *non-stopped* jobs are finished
  for (auto it=jobs_list.begin(); it != jobs_list.end(); ++it){
    auto job = *it;
    if(job.isFinished() || (job.isBackground() && waitpid(job.getJobPid(), NULL, WNOHANG)!=0)){
      jobs_list.erase(it);
      --it;
    }
  }

}


//----------------------------------------------------------------------------------------------//



//----------------------------------- Command Class Methods  -----------------------------------//



Command::Command(const char* orig_cmd_line): cmd_line_str(string(orig_cmd_line)), pid(getpid()),
                                             cmd_vec(), is_bg(false){
  

  this->cmd_line = new char[strlen(orig_cmd_line)];
  strcpy(this->cmd_line, orig_cmd_line);
  if(_isBackgroundComamnd(orig_cmd_line)){
    _removeBackgroundSign(this->cmd_line);
    this->is_bg = true;
  }
  
  this->_vectorize_cmdline(this->cmd_line);

        
}

Command::~Command(){

}

//----------------------------------- BuiltInCommand Class Methods  -----------------------------------//

BuiltInCommand::BuiltInCommand(const char* cmd_line): Command(cmd_line){}


//chprompt
ChangePromptCommand::ChangePromptCommand(const char* cmd_line) : BuiltInCommand(cmd_line){}

void ChangePromptCommand::execute(){
  

  int num_args = cmd_vec.size() - 1;
  SmallShell& smash = SmallShell::getInstance();
  
  
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
      cerr << "smash error: cd: too many arguments" << endl;
      return;
    }
    const char* path = cmd_vec[1].c_str();

    char* buf = new char[PATH_MAX];

    if (strcmp(path, "-") == 0){
      if(smash.last_dir == "NAN"){
        cerr << "smash error: cd: OLDPWD not set" << endl;
        delete[] buf;
        return;
      }
      else{
        smash.setLastDir();
        delete[] buf;
        return; 
      }
    }
    else{
      if(getCurrDir(buf) != 0){
        delete[] buf;
        return;
      }  
      else if(chdir(path) == -1){
        perror("smash error: chdir failed");
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

    if (num_args > 1 || (num_args!=0 && !isAllDigits(cmd_vec[1]))){
      cerr << "smash error: fg: invalid arguments" << endl;
      return;
    }

    SmallShell& smash = SmallShell::getInstance();
    JobsList::JobEntry *job = nullptr;
    int job_id;

    if(num_args == 0){  // Get maximal jobid job
      job = smash.jobs_list->getLastJob();
      if (job == nullptr){
        cerr << "smash error: fg: jobs list is empty" << endl;
        return;
      }
      job_id = job ->getJobId();
    }
    else{
      job_id = std::stoi(cmd_vec[1]);
      job = smash.jobs_list->getJobById(job_id);
      if(job == nullptr){
        cerr << "smash error: fg: job-id " << job_id << " does not exist" << endl;
        return;
      }
    }
    
    cout << job->getCmdLine() << " : " << job->getJobPid() << endl;


    ExternalCommand* cont_cmd = new ExternalCommand((job->getCmdLine(true)).c_str());  
    cont_cmd->pid = job->getJobPid();
    smash.fg_cmd = cont_cmd;
  

    if(job->isStopped()){
      if (kill(cont_cmd->pid, SIGCONT) == -1) {
        perror("smash error: kill failed");
        return;
      }
    }
    int status;
    smash.jobs_list->fg_job = new JobsList::JobEntry(*job);
    smash.jobs_list->removeJobById(job_id);
    if (waitpid(cont_cmd->pid, &status, WUNTRACED) == -1) {
      perror("smash error: waitpid failed");
    }
    delete cont_cmd;
    smash.fg_cmd = nullptr;
}


//bg
BgCommand::BgCommand(const char* cmd_line): BuiltInCommand(cmd_line) {}

void BgCommand::execute(){

    int num_args = cmd_vec.size() - 1;

    if (num_args > 1 || (num_args!=0 && !isAllDigits(cmd_vec[1]))){
      cerr << "smash error: bg: invalid arguments" << endl;
      return;
    }

    SmallShell& smash = SmallShell::getInstance();
    int job_id = -1;
    JobsList::JobEntry *job;

    if(num_args == 0){
      job = smash.jobs_list->getLastStoppedJob();
      if (job == nullptr){
        cerr << "smash error: bg: there is no stopped jobs to resume" << endl;
        return;
      }
    }
    else{
      job_id = std::stoi(cmd_vec[1]);
      job = smash.jobs_list->getJobById(job_id);
      if(job == nullptr){
        cerr << "smash error: bg: job-id " << job_id << " does not exist" << endl;
        return;
      }
      if(!job->isStopped()){
        cerr << "smash error: bg: job-id " << job_id << " is already running in the background" << endl;
        return;
      }
    }

    //Printing job
    cout << job->getCmdLine() << " : " << job->getJobPid() << endl;

    if (kill(job->getJobPid(), SIGCONT) == -1) {
        perror("smash error: kill failed");
        return;
    }

    job->continueJob();
  
}

//jobs
JobsCommand::JobsCommand(const char* cmd_line): BuiltInCommand(cmd_line){}

void JobsCommand::execute(){
 
  SmallShell& smash = SmallShell::getInstance();
  JobsList* jobs = smash.jobs_list;
  
  if (jobs != nullptr){
    jobs->printJobsList();
  }
}

//quit
QuitCommand::QuitCommand(const char* cmd_line, JobsList* jobs) : BuiltInCommand(cmd_line), jobs(jobs){}


void QuitCommand::execute(){
  if (cmd_vec.size()>1 && cmd_vec[1] == "kill"){
    cout << "smash: sending SIGKILL signal to " << jobs->jobs_list.size() << " jobs:" << endl;
    jobs->killAllJobs(SIGKILL);
  }

  exit(0);
}

//kill
KillCommand::KillCommand(const char* cmd_line, JobsList* jobs): BuiltInCommand(cmd_line), jobs(jobs){}

void KillCommand::execute(){
  int num_args = cmd_vec.size() - 1;
  if (num_args != 2 || !isAllDigits(cmd_vec[2]) || cmd_vec[1][0]!='-'){
    cerr << "smash error: kill: invalid arguments" << endl;
    return; //error handling
  }

  string sig = cmd_vec[1].substr(1);
  
  if (!isAllDigits(sig)){
    cerr << "smash error: kill: invalid arguments" << endl;
    return; //error handling
  }
  string job_id = cmd_vec[2];
  
  auto job = jobs->getJobById(std::stoi(job_id));
  if (job == nullptr){
    // job not found
    cerr << "smash error: kill: job-id " << job_id << " does not exist" << std::endl;
    return;
  }
  else{
    int sig_num = std::stoi(sig);
    if (kill(job->getJobPid(), sig_num) != 0){
      perror("smash error: kill failed");
      return;
    }
    if (sig_num == SIGKILL){
      job->markFinished();
    }
    if (sig_num == SIGSTOP){
      job->stopJob();
    }
    if(job->isStopped() && sig_num == SIGCONT){
      job->continueJob();
    }
    cout << "signal number " << sig_num << " was sent to pid " << job->getJobPid() << endl;
  }
}

//setcore
SetcoreCommand::SetcoreCommand(const char* cmd_line) : BuiltInCommand(cmd_line){}

void SetcoreCommand::execute(){
  int num_args = cmd_vec.size() - 1;

  if (num_args != 2 || !isAllDigits(cmd_vec[1]) || !isAllDigits(cmd_vec[2])){
      cerr << "smash error: setcore: invalid arguments" << endl;
      return;
  }
  int job_id = std::stoi(cmd_vec[1]);

  int core_num = std::stoi(cmd_vec[2]);
  long num_cores = sysconf(_SC_NPROCESSORS_ONLN);

  if (core_num < 0 || core_num > num_cores){
    cerr << "smash error: setcore: invalid core number";
  }

  SmallShell& smash = SmallShell::getInstance();
  auto job = smash.jobs_list->getJobById(job_id);
  if (job == nullptr){
    cerr << "smash error: setcore: job-id " << job_id << " does not exist" << std::endl;
  }

  // We are going to use sched_setaffinity() syscall for setting the core of the job.
  pid_t job_pid = job->getJobPid();
  cpu_set_t cpu_mask;
  CPU_ZERO(&cpu_mask);
  CPU_SET(core_num, &cpu_mask); 

  if(sched_setaffinity(job_pid, sizeof(cpu_set_t), &cpu_mask) !=0){
    perror("smash error: sched_setaffinity failed");
  }
}


//getfiletype
GetFileTypeCommand::GetFileTypeCommand(const char* cmd_line): BuiltInCommand(cmd_line){}

void GetFileTypeCommand::execute(){
  int num_args = cmd_vec.size() - 1;
  if (num_args != 1 || access(cmd_vec[1].c_str(), F_OK)!=0){  //Checks if file exists and can be accessed.
    cerr << "smash error: getfiletype: invalid arguments" << endl;
    return;
  }
  const char* filepath = cmd_vec[1].c_str();

  struct stat file_info;
  if(lstat(filepath, &file_info) != 0){
    perror("smash error: lstat failed");
    return;
  }

  off_t file_size = file_info.st_size;
  mode_t file_type = file_info.st_mode & S_IFMT;
  string file_type_res;
  switch (file_type){
    case S_IFREG:
      file_type_res = "regular file";
      break;
    case S_IFDIR:
      file_type_res = "directory";
      break;
    case S_IFCHR:
      file_type_res = "character device";
      break;
    case S_IFBLK:
      file_type_res = "block device";
      break;
    case S_IFIFO:
      file_type_res = "FIFO";
      break;
    case S_IFLNK:
      file_type_res = "symbolic link";
      break;
    case S_IFSOCK:
      file_type_res = "socket";
      break;
    default:
      break;
  }

  cout << filepath << "'s type is \"" << file_type_res << "\" and takes up " << file_size << " bytes" << endl;

}

//chmod
ChmodCommand::ChmodCommand(const char* cmd_line): BuiltInCommand(cmd_line){}

void ChmodCommand::execute(){
  int num_args = cmd_vec.size() - 1;
  if (num_args != 2 || access(cmd_vec[2].c_str(), F_OK)!=0 || !isAllDigits(cmd_vec[1]) || cmd_vec[1].size() > 4){  //Checks if file exists and can be accessed, and if new_mode is digits
    cerr << "smash error: chmod: invalid arguments" << endl;
    return;
  }
  const char* filepath = cmd_vec[2].c_str();

  long int new_mode = strtol(cmd_vec[1].c_str(), NULL, 8);  // Converting to octal base
 
  if (chmod(filepath, new_mode)!=0){
    perror("smash error: chmod failed");
  }


}


//----------------------------------------------------------------------------------------------//

//----------------------------------- ExternalCommand Class Methods  -----------------------------------//

ExternalCommand::ExternalCommand(const char* cmd_line): Command(cmd_line){}

void ExternalCommand::execute(){
  SmallShell& smash = SmallShell::getInstance();

  pid_t pid = fork(); 
  this->pid = pid;
  if (pid < 0 ){
    perror("smash error: fork failed");
    return;
  }  
  else if(pid == 0) // son procces
  {
    if (setpgrp() == -1) {
      perror("smash error: setpgrp failed");
      return;
    }

    string cmd_string = "";
    char *argv[COMMAND_MAX_ARGS+2];
    string cmd_name_string = cmd_vec[0];
    for (size_t i=0; i<cmd_vec.size(); ++i){
      cmd_string += cmd_vec[i];
      if (i < cmd_vec.size()-1){
        cmd_string += " ";
      }
      argv[i] = const_cast<char*>(cmd_vec[i].c_str());
    }
    argv[cmd_vec.size()] = NULL;

    if(strstr(cmd_line, "*") || strstr(cmd_line, "?")) // complex external command run using bash
    {
      char* cmd_string_char = const_cast<char*>(cmd_string.c_str());  // Danger: conversion- string to const char* to char*
      char exec[] = "/bin/bash";
      char c_flag[] = "-c";
      char *args_bash[] = {exec, c_flag, cmd_string_char, NULL};

      if (execv(args_bash[0], args_bash) == -1) {  
          perror("smash error: execv failed");
          return;
      }

    }
    else  //simple external command run using execv syscalls
    {

      if (execvp(argv[0], argv) == -1) {
          perror("smash error: execv failed");
          return;
      }
    }
  }
  else // father procces
  {
    if(!this->is_bg){
      smash.fg_cmd->pid = this->pid;
      int status;
      if(waitpid(pid, &status, WUNTRACED) == -1){
        perror("smash error: waitpid failed");
        return;
      }
    }
    else{ // background command
      smash.jobs_list->addJob(this);
    }
  }
}

//----------------------------------------------------------------------------------------------//

//----------------------------------- PipeCommand Class Methods  -----------------------------------//

PipeCommand::PipeCommand(const char* cmd_line): Command(cmd_line){}

void PipeCommand::execute(){

  SmallShell& smash = SmallShell::getInstance();
  
  string s = string(cmd_line);
  
  string pipeType = s.find("|&") == string::npos ? "|" : "|&";

  int i = s.find(pipeType);
  
  string command1 = s.substr(0,i);  //command to redirect its output (first one before | or |&)
  string command2; //command to redirect its output (one before | or |&)
  if(pipeType == "|&") {  //command to redirect the output to (one after | or |&)
        command2 = s.substr(i+2);
    }
    else {  //redirect stdout
        command2 = s.substr(i+1);
    }
  
  int fd[2];
  
  // int* pipe(fd);
  if(pipe(fd) == -1){
    perror("smash error: pipe failed");
    return;
  }

  pid_t pid = fork();

  if(pid == -1){
    perror("smash error: fork failed");
    return;
  }
  else if(pid > 0){ // father procces
    
    if(close(fd[1]) == -1){ //closing writing fd for fateher
        perror("smash error: close failed");
        return;
      }
    int status;
		waitpid(pid, &status, WUNTRACED);

    int stdin_copy = dup(0); //copy of standard input file descriptor

    if(stdin_copy == -1){
        perror("smash error: dup failed");
        return;
      }
    if(dup2(fd[0], 0) == -1) { //duplicate fd and replace standard input fd with it
				perror("smash error: dup2 failed");
        return;
			}
    
    
    // FIRST, we wait for the child process to finish executing command1 (so we can be sure output is written to channel)

    if(close(fd[0]) == -1) { 
        perror("smash error: close failed");
        return;
    }
    smash.executeCommand(command2.c_str());
    if(dup2(stdin_copy, 0) == -1){ // Restoring stdin fd
      perror("smash error: dup2 failed");
      return;      
		}

    if(close(stdin_copy) == -1) {
        perror("smash error: close failed");
        return; 
    }
  }
  else{ // son procces
    if (setpgrp() == -1) {
          perror("smash error: setpgrp failed");
          return;
        }
    if(close(fd[0]) == -1){ //closing reading fd for son
        perror("smash error: close failed");
        return; 
      }
    if(pipeType == "|&") {  //redirect stderr
        int stderr_copy = dup(2); //copy of standard error file descriptor
			  if(stderr_copy == -1){
				    perror("smash error: dup failed");
            return;
			  }
        if(dup2(fd[1], 2) == -1){ //duplicate fd and replace standard error fd with it
				    perror("smash error: dup2 failed");
            return;
			  }
        if(close(fd[1]) == -1){ 
				    perror("smash error: close failed");
            return;
			  }
        
        smash.executeCommand(command1.c_str());

        if(dup2(stderr_copy, 2) == -1){ // Restoring stderr fd
				    perror("smash error: dup2 failed");
            return;
			  }
			  if(close(stderr_copy) == -1){
				    perror("smash error: close failed");
            return;
			  }
    }
    else {  //redirect stdout
        int stdout_copy = dup(1); //copy of standard output file descriptor
			  if(stdout_copy == -1){
				    perror("smash error: dup failed");
			  }
        if(dup2(fd[1], 1) == -1){ //duplicate fd and replace standard output fd with it
				    perror("smash error: dup2 failed");
			  }
        smash.executeCommand(command1.c_str());

        if(close(fd[1]) == -1){ //? why do we close fd? -because we already duplicated it to stdout fd (2)
				    perror("smash error: close failed");
			  }
        if(dup2(stdout_copy, 1) == -1){ // Restoring stdout fd
				    perror("smash error: dup2 failed");
			  }
			  if(close(stdout_copy) == -1){
				    perror("smash error: close failed");
			  }
    }
    exit(0);
  }
}


//----------------------------------------------------------------------------------------------//

//----------------------------------- RedirectionCommand Class Methods  -----------------------------------//

RedirectionCommand::RedirectionCommand(const char* cmd_line): Command(cmd_line){}

void RedirectionCommand::execute(){
  SmallShell& smash = SmallShell::getInstance();
  
  string s = string(cmd_line);
  
  string redirectionType = s.find(">>") == string::npos ? ">" : ">>";
  
  int i = s.find(redirectionType);
  string command = s.substr(0,i);
  string output_file;
  int fd;
  int stdout_copy = dup(1); //copy of standard output file descriptor
	if(stdout_copy == -1){
			perror("smash error: dup failed");
      return;
	}
  if(redirectionType == ">>") {  //append
    output_file = s.substr(i+2);
    size_t pos = output_file.find_first_not_of(' ');
    output_file = output_file.substr(pos);
    fd = open(output_file.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0655);
    if(fd == -1){
        perror("smash error: open failed");
        return;
    }
  }
  else {  //override
    output_file = s.substr(i+1);
    size_t pos = output_file.find_first_not_of(' ');
    output_file = output_file.substr(pos);
    
    fd = open(output_file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0655);
    if(fd == -1){
        perror("smash error: open failed");
        return;
    }
  }
  if(dup2(fd, 1) == -1){
      perror("smash error: dup2 failed");
      return;
    }

  smash.executeCommand(command.c_str());

  if(close(fd)==-1){
    perror("smash error: close failed");
    return;
  }
  if(dup2(stdout_copy, 1) == -1){ //restore original stdout
      perror("smash error: dup2 failed");
      return;
    }
   if(close(stdout_copy)==-1){
    perror("smash error: close failed");
    return;
  } 
  
}

//----------------------------------------------------------------------------------------------//
