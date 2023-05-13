#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include "Commands.h"
#include "signals.h"

int main(int argc, char* argv[]) {
    if(signal(SIGTSTP , ctrlZHandler)==SIG_ERR) {
        perror("smash error: failed to set ctrl-Z handler");
    }
    if(signal(SIGINT , ctrlCHandler)==SIG_ERR) {
        perror("smash error: failed to set ctrl-C handler");
    }

    //TODO: setup sig alarm handler

    SmallShell& smash = SmallShell::getInstance();
    while(true) {
        std::cout << smash.get_prompt() << "> ";
        std::string cmd_line;
        // std::cout << "BEFORE getline ";
        // std::cout << std::cin.good();
        std::getline(std::cin, cmd_line);
        // std::cout << "AFTER getline ";
        smash.executeCommand(cmd_line.c_str());

        // if (std::getline(std::cin, cmd_line)) {
        //     std::cout << "AFTER getline ";
        //     smash.executeCommand(cmd_line.c_str());
        // } else {
        //     std::cout << "Failed to read from input stream." << std::endl;
        // }
    }
    return 0;
}

