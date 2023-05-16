#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"

using namespace std;

void ctrlZHandler(int sig_num) {
  cout << "smash: got ctrl-Z" << endl;

  SmallShell& smash = SmallShell::getInstance();
  if (smash.fg_cmd==nullptr){
    return;
  }
  if(kill(smash.fg_cmd->pid, SIGSTOP)!=0){ //error handling
    cout << "smash: kill failed";
  }
  if (smash.jobs_list->fg_job != nullptr){
    smash.jobs_list->addFgJob();
  }else{
    smash.jobs_list->addJob(smash.fg_cmd, true);
  }
  cout << "smash: process " << smash.fg_cmd->pid << " was stopped" << endl;

}

void ctrlCHandler(int sig_num) {
  cout << "smash: got ctrl-C" << endl;

  SmallShell& smash = SmallShell::getInstance();
  if (smash.fg_cmd==nullptr){
    return;
  }

  if(kill(smash.fg_cmd->pid, SIGKILL)!=0){ //error handling
    cout << "smash: kill error in C handler";
  }
  cout << "smash: process " << smash.fg_cmd->pid << " was killed" << endl;
  smash.fg_cmd = nullptr;
}

void alarmHandler(int sig_num) {
  // TODO: Add your implementation
}

