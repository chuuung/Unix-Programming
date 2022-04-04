#include <iostream>
#include <dirent.h>
#include <string>
#include <unistd.h>
#include <pwd.h>
#include <sys/stat.h>
#include <regex>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>


#define PATH_SIZE 100

using namespace std;

int c_flag = 0;
int t_flag = 0;
int f_flag = 0;

string com_con = "";
string type_con = "";
string file_con = "";

void proc_info(char*);
bool is_pid(char*);

struct pid_info {
    string cmd = "";
    string pid = "";
    string user = "";
    string fd = "";
    string type = "";
    string node = "";
    string name = "";
    // pid_t pid;
    ssize_t len;
};

bool isNumber(const string& str)
{
    for (char const &c : str) {
        if (std::isdigit(c) == 0) return false;
    }
    return true;
}


bool is_pid(char* str){
    for(int i = 0; i < int(strlen(str)); i++)
        if(isdigit(str[i]))
            return false;
    return true;
}

void do_print_inf(pid_info pid_inf){

    string final = pid_inf.cmd + "\t" + pid_inf.pid + "\t" + pid_inf.user + "\t" + pid_inf.fd + "\t" 
    + pid_inf.type + "\t" + pid_inf.node + "\t" + pid_inf.name;

    if (c_flag == 1){
        smatch m;
        regex e (com_con);
        if (regex_search(pid_inf.cmd, m, e))
            cout << final << endl;
    }
    else if (t_flag == 1){
        if (pid_inf.type == type_con)
            cout << final << endl;
    }
    else if (f_flag == 1){
        smatch m;
        regex e (file_con);
        if (regex_search(pid_inf.name, m, e))
            cout << final << endl;
    }
    else
        cout << final << endl;
}

void get_proc_command(string proc_path, pid_info &pid_inf){
    string comm_file = proc_path + "comm";
    ifstream ifs(comm_file, ios::in);
    if (!ifs.is_open()) {
        cerr << "cannot open file!!" << endl;
        return ;
    }
    stringstream ss;
    ss << ifs.rdbuf();
    string str(ss.str());
    str[str.length()-1]=0;
    pid_inf.cmd = str;

    ifs.close();
}



void which_type(string path_fd, pid_info &pid_inf){
    struct stat s;
    stat(path_fd.data(), &s);
    if (stat(path_fd.data(), &s) != 0){
        cerr << "cannot get type !!!" << endl;
        return;
    }
    if (S_ISDIR(s.st_mode)) pid_inf.type = "DIR";
    else if (S_ISREG(s.st_mode)) pid_inf.type = "REG";
    else if (S_ISCHR(s.st_mode)) pid_inf.type = "CHR";
    else if (S_ISFIFO(s.st_mode)) pid_inf.type = "FIFO";
    else if (S_ISSOCK(s.st_mode)) pid_inf.type = "SOCK";
    else if (S_ISREG(s.st_mode)) pid_inf.type = "REG";
    else pid_inf.type = "unknown";

    pid_inf.node = to_string(s.st_ino);
}

void get_proc_type(string proc_path, pid_info pid_inf){
    vector<string> FD = {"cwd", "root", "exe"};

    for (int i = 0; i < FD.size(); i++){
        int link_dest_size;
        char link_buf[200];
        memset(link_buf, '\0', 150);
        string path_fd = proc_path;
        path_fd = path_fd + FD[i];
        
        if (FD[i] == "root")
            pid_inf.fd = "rtd";
        else if (FD[i] == "exe")
            pid_inf.fd = "txt";
        else
            pid_inf.fd = FD[i];

        if((link_dest_size = readlink(path_fd.data(), link_buf, sizeof(link_buf)-1)) < 0){
            pid_inf.name = path_fd + " (Permission denied)";
            pid_inf.type = "unknown";
            // do_print_inf(pid_inf);
        }
        else{ //有權限

            pid_inf.name = string(link_buf);
            which_type(path_fd, pid_inf);

        }
        do_print_inf(pid_inf);

    }

}

const vector<string> split(const string &str, const char &delimiter) {
    vector<string> result;
    stringstream ss(str);
    string tok;

    while (getline(ss, tok, delimiter)) {
        result.push_back(tok);
    }
    return result;
}

void get_maps(string proc_path, pid_info &pid_inf){

    string maps_path = proc_path + "maps";
    ifstream ifs(maps_path, ios::in);
    unordered_map <string, int> m;

    if (!ifs.is_open()) {
        return;
    } else {
        string s;
        while (getline(ifs, s)) {

            // cout << s << endl;
            string maps_node, maps_name;
            vector<string> maps_text = split(s, ' ');
            maps_node = maps_text[4];
 
            if (m.count(maps_node) > 0 || stoi(maps_node) == 0)
                continue;
            m[maps_node]++;

            if (maps_text[maps_text.size() - 1] == "(deleted)"){
                maps_name = maps_text[maps_text.size() - 2];
                pid_inf.fd = "DEL";
                pid_inf.type = "unknown";
            }
            else{
                maps_name = maps_text[maps_text.size() - 1];
                pid_inf.fd = "mem";
                which_type(maps_name, pid_inf);
            }

    
            pid_inf.node = maps_node;
            pid_inf.name = maps_name;
            do_print_inf(pid_inf);

        }
        ifs.close();
    }
    ifs.close();

    

}

void which_fd(string path_fd_id, string fd_id,  pid_info &pid_inf){
    struct stat s;
    if(lstat(path_fd_id.data(), &s) == -1)
        return;

    if((s.st_mode & S_IREAD) && (s.st_mode & S_IWRITE))
        pid_inf.fd = fd_id + "u";
    else if(s.st_mode & S_IRUSR)
        pid_inf.fd = fd_id + "r";
    else if(s.st_mode & S_IWUSR)
        pid_inf.fd = fd_id + "w";
}

void get_fd(string proc_path, pid_info pid_inf){
    string path_fd = proc_path + "fd";
    DIR* dir = opendir(path_fd.data());

    if (!dir){ // 打不開
        pid_inf.fd = "NOFD";
        pid_inf.name = path_fd + " (Permission denied)";
        do_print_inf(pid_inf);
    }
    else{
        struct dirent* de;
        while((de = readdir(dir))){
            int link_dest_size;
            char link_dest[200];
            memset(link_dest, '\0', 150);
            string fd_id = de -> d_name;
            
            if (!isNumber(fd_id))
                continue;
            string path_fd_id = path_fd + "/" + fd_id;

            if((link_dest_size = readlink(path_fd_id.data(), link_dest, sizeof(link_dest)-1)) < 0){
                pid_inf.name = path_fd_id + strerror(errno);
                pid_inf.type = "unknown";
            }
            else{
                if (string(link_dest).find("(deleted)") != string::npos){
                    vector<string> temp = split(string(link_dest), ' ');
                    pid_inf.name = temp[0];
                }
                else
                    pid_inf.name = string(link_dest);
                which_fd(path_fd_id, fd_id, pid_inf);
                which_type(path_fd_id, pid_inf);

            }
            
            do_print_inf(pid_inf);


        }
    }
}


void get_proc_info(string proc_name, pid_info &pid_inf){

    struct stat pid_stat;
    struct passwd *pw;

    string proc_path = "/proc/" + proc_name + "/";
    pid_inf.len = proc_path.length();

    const char *p = proc_path.data(); //字串轉成 char *

// 抓process uid get user
    if (!stat(p, &pid_stat)){
        pw = getpwuid(pid_stat.st_uid);
        if(pw){
            pid_inf.user = pw -> pw_name;
            // cout << pw -> pw_name << endl;
        }
    }

    get_proc_command(proc_path, pid_inf);

    get_proc_type(proc_path, pid_inf);

    get_maps(proc_path, pid_inf);

    get_fd(proc_path, pid_inf);

}

bool check_input_type(string optarg){
    vector<string> all_types = {"REG", "CHR", "DIR", "FIFO", "SOCK", "unknown"};

    for (int i = 0; i < all_types.size(); i++){
        if ( optarg == all_types[i]) 
            return true;
    }
    return false;

}


void print_header(){
    string header = "COMMAND\t PID\t USER\t FD\t TYPE\t NODE\t NAME";
    cout << header << endl;
}

bool check_filter(int argc, char **argv){
    int arg;
    while((arg = getopt(argc, argv,"c:t:f:")) != -1){
        // cout << optind << endl;
        switch (arg) {
            case 'c':
                c_flag = 1;
                com_con = optarg;
            break;
            case 't':

                if (!check_input_type(optarg)){
                    cout << "Invalid Input Type !!" << endl;
                    return false;
                }
                t_flag = 1;
                type_con = optarg;
                
            break;
            case 'f':
                f_flag = 1;
                file_con = optarg;
            break;
        }
    }
    return true;

}


int main(int argc, char **argv){
 
    if (!check_filter(argc, argv)) return 0;

    DIR *dir = opendir("/proc");
    struct dirent* de;
    
    print_header();

    while(de = readdir(dir)){
        pid_info pid_inf;
        string proc_id = de -> d_name;

        if (!isNumber(proc_id)) //提取proc號碼
            continue;
        pid_inf.pid = proc_id;
        get_proc_info(proc_id, pid_inf);
    }
    

}