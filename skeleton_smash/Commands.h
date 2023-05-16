#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>

#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)

const std::string WHITESPACE = " \n\r\t\f\v";


enum class DirErrors{
  BUF_NULLPTR,
  FAILED_GETCWD
};

class Command {
 protected:
 public:  // Data
  char* cmd_line;
  std::string cmd_line_str;
  pid_t pid;
  std::vector<std::string> cmd_vec;
  bool is_bg;
 public:  // Methods
  Command(const char* orig_cmd_line);
  virtual ~Command();
  virtual void execute() = 0;
  void _vectorize_cmdline(const char* cmd_line);
};

class BuiltInCommand : public Command {
 public:
  BuiltInCommand(const char* cmd_line);
  virtual ~BuiltInCommand() {}
};

class ExternalCommand : public Command {
 public:
  ExternalCommand(const char* cmd_line);
  virtual ~ExternalCommand() {}
  void execute() override;
};

class PipeCommand : public Command {
 public:
  PipeCommand(const char* cmd_line);
  virtual ~PipeCommand() {}
  void execute() override;
};

class RedirectionCommand : public Command {
 public:
  explicit RedirectionCommand(const char* cmd_line);
  virtual ~RedirectionCommand() {}
  void execute() override;
};

class ChangeDirCommand : public BuiltInCommand {
public:
  ChangeDirCommand(const char* cmd_line);
  virtual ~ChangeDirCommand() {}
  void execute() override;
};

class GetCurrDirCommand : public BuiltInCommand {
 public:
  GetCurrDirCommand(const char* cmd_line);
  virtual ~GetCurrDirCommand() {}
  void execute() override;
};

class ShowPidCommand : public BuiltInCommand {
 public:
  ShowPidCommand(const char* cmd_line);
  virtual ~ShowPidCommand() {}
  void execute() override;
};

class ChangePromptCommand : public BuiltInCommand {
  public:
    ChangePromptCommand(const char* cmd_line);
    virtual ~ChangePromptCommand() {}
    void execute() override;
};

class FgCommand : public BuiltInCommand {
  public:
    FgCommand(const char* cmd_line);
    virtual ~FgCommand() {}
    void execute() override;
};

class BgCommand : public BuiltInCommand {
  public:
    BgCommand(const char* cmd_line);
    virtual ~BgCommand() {}
    void execute() override;
};

class JobsList;
class QuitCommand : public BuiltInCommand {
  JobsList* jobs;
public:
  QuitCommand(const char* cmd_line, JobsList* jobs);
  virtual ~QuitCommand() {
    return;
  }
  void execute() override;
};


class JobsList {
 public:
  class JobEntry {
    int job_id;
    pid_t pid;
    time_t init_time;
    bool is_stopped;
    bool is_background;
    bool is_finished;
    std::vector<std::string> cmd_vec; 
    std::string cmd_line;
    std::string cmd_line_nobg;
  public:
    JobEntry() = delete;
    JobEntry(const int job_id, pid_t pid, const time_t init_time, bool is_stopped, bool is_bg, std::vector<std::string> cmd_vec, std::string cmd_line);
    ~JobEntry() = default;
    const int& getJobId(){ return this->job_id;}
    
    const int& getJobPid(){ return this->pid;}
    
    time_t getInitTime(){ return this->init_time;}
    void setInitTime(time_t new_time){ this->init_time=new_time;}
    bool isStopped(){ return this->is_stopped;}
    bool isBackground(){ return this->is_background;}
    bool isFinished(){ return this->is_finished;}
    void markFinished(){ this->is_finished = true;}
    void stopJob(){ this->is_stopped = true;}
    void continueJob(){ this->is_stopped = false;}  
    std::vector<std::string>& getCmdVec(){ return this->cmd_vec;}
    std::string& getCmdName(){ return this->cmd_vec[0];}
    std::string& getCmdLine(bool no_bg=false){
      if (no_bg){
        return this->cmd_line_nobg;
      }
      return this->cmd_line;
    }
  
  };
 public:
  std::vector<JobEntry> jobs_list;
  JobEntry* fg_job;
  JobsList();
  ~JobsList();
  void addJob(Command* cmd, bool isStopped = false);
  void addFgJob();
  void printJobsList();
  void killAllJobs(int sig);
  void removeFinishedJobs();
  JobEntry * getJobById(int jobId);
  void removeJobById(int jobId);
  JobEntry * getLastJob();
  JobEntry *getLastStoppedJob();
};

class JobsCommand : public BuiltInCommand {
 public:
  JobsCommand(const char* cmd_line);
  virtual ~JobsCommand() {}
  void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
 public:
  ForegroundCommand(const char* cmd_line, JobsList* jobs);
  virtual ~ForegroundCommand() {}
  void execute() override;
};

class BackgroundCommand : public BuiltInCommand {
 public:
  BackgroundCommand(const char* cmd_line, JobsList* jobs);
  virtual ~BackgroundCommand() {}
  void execute() override;
};

class TimeoutCommand : public BuiltInCommand {
/* Bonus */
// TODO: Add your data members
 public:
  explicit TimeoutCommand(const char* cmd_line);
  virtual ~TimeoutCommand() {}
  void execute() override;
};

class ChmodCommand : public BuiltInCommand {
 public:
  ChmodCommand(const char* cmd_line);
  virtual ~ChmodCommand() {}
  void execute() override;
};

class GetFileTypeCommand : public BuiltInCommand {
 public:
  GetFileTypeCommand(const char* cmd_line);
  virtual ~GetFileTypeCommand() {}
  void execute() override;
};

class SetcoreCommand : public BuiltInCommand {
 public:
  SetcoreCommand(const char* cmd_line);
  virtual ~SetcoreCommand() {}
  void execute() override;
};

class KillCommand : public BuiltInCommand {
 JobsList* jobs;
 public:
  KillCommand(const char* cmd_line, JobsList* jobs);
  virtual ~KillCommand() {}
  void execute() override;
};

class SmallShell {
 private:
  SmallShell();
 public:  // Data memebers
  std::string prompt;
  pid_t pid;
  std::string last_dir;
  JobsList* jobs_list;
  ExternalCommand* fg_cmd;

public:   // Methods
  SmallShell(SmallShell const&)      = delete; // disable copy ctor
  void operator=(SmallShell const&)  = delete; // disable = operator
  static SmallShell& getInstance() // make SmallShell singleton
  {
    static SmallShell instance; // Guaranteed to be destroyed.
    // Instantiated on first use.
    return instance;
  }
  void setLastDir();
  ~SmallShell();
  
  void executeCommand(const char* cmd_line);
  Command *CreateCommand(const char* cmd_line);
  //----OUR METHODS
  std::string& get_prompt();
  void set_prompt(const std::string& new_prompt);
  void checkJobs();
};

#endif //SMASH_COMMAND_H_
