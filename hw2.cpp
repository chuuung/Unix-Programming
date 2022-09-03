#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

using namespace std;

char *exec_arg[50];
int redir_flag = 0;
string redir_file = "";
char ld_preload[200] = "./logger.so";

int main(int argc, char **argv){
    int index = 0;
    

    if (argc < 2){
        cout << "no command given." << endl;
        return 0;
    }
    int arg;
    while((arg = getopt(argc, argv,"o:p:")) != -1){
        switch (arg) {
            case 'o':
                redir_flag = 1;
                redir_file = optarg;
                break;
            case 'p':
                strcpy(ld_preload, optarg);
                break;
            default:
                cout << "usage: ./logger [-o file] [-p sopath] [--] cmd [cmd args ...]" << endl;
                cout << "    -p: set the path to logger.so, default = ./logger.so" << endl;
                cout << "    -o: print output to file, print to \"stderr\" if no file specified" << endl;
                cout << "    --: separate the arguments for logger and for the command" << endl;
                break;

        }
    }

    for(int i = optind; i < argc; i++) {
        exec_arg[index] = argv[i];
        index++;
    }

    int redir_fd;
    if (redir_flag == 1){
        redir_fd = open(redir_file.c_str(), O_RDWR | O_CREAT | O_TRUNC, 00700);
        dup2(redir_fd, 3);
    }
    else{
        dup2(2, 3);
    }

    // dup2(2, 3); //將2指到與3指的相同的檔案
    setenv("OUTPUT_FD", to_string(3).c_str(), true);
    setenv("LD_PRELOAD", "./logger.so", true);
    execvp(argv[optind], exec_arg);
     if (redir_flag == 1) close(redir_fd);
    return 0;
}