#include <stdio.h>
#include <iostream>
#include <iomanip>
#include <elf.h>
#include <errno.h>
#include <string.h>
#include <vector>
#include <sstream>
#include <string>
#include <sys/wait.h>
#include <sys/ptrace.h>
#include <unistd.h>
#include <assert.h>
#include <fstream>
#include <sstream>
#include <sys/user.h>
#include <unordered_map>
#include <algorithm>
#include <capstone/capstone.h>

using namespace std;

#define NONLOAD 0
#define LOADED  1
#define RUNNING 2
#define DISASM_MAX 10
#define DUMP_MAX 5


struct elf_inf{
    FILE *fp;

    Elf64_Ehdr  ehdr;           //  ELF header
    Elf64_Shdr  str_shdr;       //  string section header
    char        *str_tab;       //  string tables
    Elf64_Shdr  text_shdr;      //  text section header
};

struct instruction1 {
   unsigned char bytes[16];
   int size;
   string opr, opnd;
};

struct bp_inf{
    long addr;
    long orig_code;
};

string exec_path = "";
int state = NONLOAD;
pid_t pid = 0;
elf_inf elf;
vector<bp_inf> bp_list;
static unordered_map<long long, instruction1> instructions;
char *code = NULL;
int code_size = 0;
struct user_regs_struct regs;
bool flag_s = false;

bool intext(const unsigned long addr){
    return elf.text_shdr.sh_addr <= addr && addr <= (elf.text_shdr.sh_addr + elf.text_shdr.sh_size);
}

void errquit(const char* func, int error_num){
    cerr << "** The function ["<< func << "] has ERROR:"<< strerror(error_num);
    exit(-1);
}

vector<string> split(string &str, const char &delimiter) {
    vector<string> result;
    stringstream ss(str);
    string tok;

    while (getline(ss, tok, delimiter)) {
        result.push_back(tok);
    }
    return result;
}

void print8bytes(long code){
    for(int i=0; i<8; i++)
        cerr<<hex<<setw(2)<<setfill('0')<<(int)((unsigned char *) (&code))[i]<<" ";
    cerr<<setfill(' ');
}

void print8ascii(unsigned long code){
    for(int i=0; i<8; i++){
        if(isprint((int)((char *) (&code))[i]))
            cerr<<((char *) (&code))[i];
        else
            cerr<<".";
    }
}
void load(string exec_path){
    if(state==LOADED){
        cerr<<"** The state is already LOADED!\n";
        return;
    }
    FILE *fp;
    elf.str_tab = NULL;
    int str_shdr_offset;
    if((fp=fopen(exec_path.c_str(), "rb"))==NULL) errquit("fopen", errno);
    elf.fp = fp;
    fread(&(elf.ehdr), 1, sizeof(Elf64_Ehdr), elf.fp);
    if(elf.ehdr.e_ident[EI_MAG0]==0x7f &&
        elf.ehdr.e_ident[EI_MAG1]=='E' &&
        elf.ehdr.e_ident[EI_MAG2]=='L' &&
        elf.ehdr.e_ident[EI_MAG3]=='F'){
        
        if(elf.ehdr.e_ident[EI_CLASS]==ELFCLASS64){
            /*  Find String Section Header  */

            str_shdr_offset = elf.ehdr.e_shoff + (elf.ehdr.e_shstrndx)*sizeof(Elf64_Shdr);
            fseek(elf.fp, str_shdr_offset, SEEK_SET);
            fread(&(elf.str_shdr), 1, sizeof(Elf64_Shdr), elf.fp);
            elf.str_tab = (char*)malloc(sizeof(char)*elf.str_shdr.sh_size);
            fseek(elf.fp, elf.str_shdr.sh_offset, SEEK_SET);
            fread(elf.str_tab, elf.str_shdr.sh_size, sizeof(char), elf.fp);

            /*  Find Text Section Header    */
            Elf64_Shdr tmp_shdr;
            fseek(elf.fp, elf.ehdr.e_shoff, SEEK_SET);
            for(int i=0; i<elf.ehdr.e_shnum; i++){
                fread(&(tmp_shdr), 1, sizeof(Elf64_Shdr), elf.fp);
                if(strcmp((elf.str_tab+tmp_shdr.sh_name), ".text")==0){
                    elf.text_shdr = tmp_shdr;
                    break;
                }
            }
        }else{
            cerr<<"** Not 64-bits program!\n";
            return;
        }
    }else{
        cerr<<"** Not ELF file!\n";
        return;
    }
    cerr<<"** program '"<<exec_path<<"' loaded. entry point 0x"<<hex<<elf.ehdr.e_entry<<"\n";
    state = LOADED;
}

void start(){
    
    if (state == RUNNING){
        cerr << "** program ["<< exec_path <<"] is running" << endl;
        return;
    }
    if (state != LOADED){
        cerr << "** program ["<< exec_path <<"] didn't loaded" << endl;
        return;
    }
    
    if (pid > 0){
        cerr << "** The program has already running!" << endl;
        return;
    }

    if ((pid = fork()) < 0){
        errquit("fork", errno);
        return;
    }
    else if (pid == 0){
        if (ptrace(PTRACE_TRACEME, 0, 0, 0) < 0) errquit("ptrace", errno);
        char **arg = {NULL};
        if ((execvp(exec_path.c_str(), arg) < 0)) errquit("execvp", errno);
        
    }
    else{
        int status;
        if (waitpid(pid, &status, 0) < 0) errquit("wait", errno);
        assert(WIFSTOPPED(status));
        ptrace(PTRACE_SETOPTIONS, pid, 0, PTRACE_O_EXITKILL);
        cerr << "** pid " <<dec<< pid << endl;
        state = RUNNING;
    }
}

void print_maps(vector<string> maps_inf){
    vector<string> addr_range = split(maps_inf[0], '-');
    vector<string> perm = split(maps_inf[1], 'p');
    cerr << setw(16)<<setfill('0')<<right << addr_range[0]<< "-" << setw(16)<<setfill('0')<<right << addr_range[1] << " " 
    << setfill(' ') << perm[0] << " " << hex << stoul(maps_inf[2]) << "\t" << maps_inf[maps_inf.size()-1] << endl;
}

void vmmap(){
    if (state != RUNNING){
        cerr << "** Program ["<< exec_path <<"] is not running" << endl;
        return;
    }

    string maps_path = "/proc/" + to_string(pid) + "/maps";
    ifstream ifs(maps_path, ios::in);

    if (!ifs.is_open()) return;
    else{
        string s;
        while (getline(ifs, s)) {
            vector<string> maps_inf = split(s, ' ');
            print_maps(maps_inf);
        }
    }

}

void help(){
    cerr<<"- break {instruction-address}: add a break point" << endl;
    cerr<<"- cont: continue execution" << endl;
    cerr<<"- delete {break-point-id}: remove a break point" << endl;
    cerr<<"- disasm addr: disassemble instructions in a file or a memory region" << endl;
    cerr<<"- dump addr [length]: dump memory content" << endl;
    cerr<<"- exit: terminate the debugger" << endl;
    cerr<<"- get reg: get a single value from a register" << endl;
    cerr<<"- getregs: show registers" << endl;
    cerr<<"- help: show this message" << endl;
    cerr<<"- list: list break points" << endl;
    cerr<<"- load {path/to/a/program}: load a program" << endl;
    cerr<<"- run: run the program" << endl;
    cerr<<"- vmmap: show memory layout" << endl;
    cerr<<"- set reg val: get a single value to a register" << endl;
    cerr<<"- si: step into instruction" << endl;
    cerr<<"- start: start the program and stop at the first instruction" << endl;
}

vector<pair<string,unsigned long long *>> get_all_reg(){   
    vector<pair<string,unsigned long long *>> regs_map;
    if(ptrace(PTRACE_GETREGS, pid, 0, &regs) < 0) errquit("ptrace(PTRACE_GETREGS)", errno);

    regs_map.push_back(make_pair("rax", &regs.rax));
    regs_map.push_back(make_pair("rbx", &regs.rbx));
    regs_map.push_back(make_pair("rcx", &regs.rcx));
    regs_map.push_back(make_pair("rdx", &regs.rdx));
    regs_map.push_back(make_pair("r8", &regs.r8));
    regs_map.push_back(make_pair("r9", &regs.r9));
    regs_map.push_back(make_pair("r10", &regs.r10));
    regs_map.push_back(make_pair("r11", &regs.r11));
    regs_map.push_back(make_pair("r12", &regs.r12));
    regs_map.push_back(make_pair("r13", &regs.r13));
    regs_map.push_back(make_pair("r14", &regs.r14));
    regs_map.push_back(make_pair("r15", &regs.r15));
    regs_map.push_back(make_pair("rdi", &regs.rdi));
    regs_map.push_back(make_pair("rsi", &regs.rsi));
    regs_map.push_back(make_pair("rbp", &regs.rbp));
    regs_map.push_back(make_pair("rsp", &regs.rsp));
    regs_map.push_back(make_pair("rip", &regs.rip));
    regs_map.push_back(make_pair("flags", &regs.eflags));

    return regs_map;
}

void get(string reg){
    long ret;
    bool reg_flag = false;
    vector<pair<string, unsigned long long *>> regs_map = get_all_reg();

    for(unsigned i = 0; i < regs_map.size(); i++){
        if (regs_map[i].first == reg){
            reg_flag = true;
            ret = *(regs_map[i].second);
            break;
        }
    }

    if (!reg_flag){
        cerr <<"** REG [" << reg << "] is not found!" << endl;
        return;
    }

    cerr << reg << " = "<<dec<<ret<<" (0x"<<hex<<ret<<")" << endl;
}


void getregs(){
    vector<pair<string, unsigned long long* >> regs_map = get_all_reg();
    for (unsigned i = 0; i < regs_map.size(); i++){
        string reg_name = regs_map[i].first;
        string big_reg_name;
        transform(reg_name.begin(), reg_name.end(), back_inserter(big_reg_name), ::toupper);
        if(i == regs_map.size()-1)
            cerr << hex << left << setw(7) << big_reg_name << " "  <<left<<setw(16)<<setfill('0')<<right<< *regs_map[i].second << endl;
        else if (i%4 == 3)
            cerr << hex << left << setw(7) << big_reg_name << " " << setw(10) << *regs_map[i].second << endl;
        else
            cerr << hex << left << setw(7) << big_reg_name << " " << setw(10) << *regs_map[i].second << "\t";
    }

}

int getcode(){
    ifstream f(exec_path.c_str(), ios::in | ios::binary);
    f.seekg(0, f.end);
    int size = f.tellg();
    f.seekg(0, f.beg);
    code = (char*)malloc(sizeof(char)*size);
    f.read(code, size);
    f.close();
    return size;
}

void disasm(unsigned long dis_addr, int size){
    if(state!=RUNNING){
        cerr<<"** State should be RUNNING!"<<endl;
        return;
    }
    if(!dis_addr){
        cerr << "** No addr is given!" << endl;
        return;
    }
    
    if(!intext(dis_addr)){
        cerr << "** Addr out of text region!" << endl;
        return;
    }

    if(code==NULL){
        code_size = getcode();
    }
    long long offset = elf.text_shdr.sh_offset + (dis_addr - elf.text_shdr.sh_addr);
    char *cur_code = code + offset;

    csh handle;
    cs_insn *insn;
    size_t count;
    uint64_t cur_addr = (uint64_t)dis_addr;
    if(cs_open(CS_ARCH_X86, CS_MODE_64, &handle)!=CS_ERR_OK) errquit("cs_open", errno);
    if((count=cs_disasm(handle, (uint8_t*)cur_code, (size_t)code_size, cur_addr, (size_t)size, &insn))>0){
        for(int i=0; i<(int)count; i++){
            unsigned char bytes[16];
            char bits[128] = "";
            memcpy(bytes, insn[i].bytes, insn[i].size);
            for(int j=0; j<insn[i].size; j++)   //bytes to bits
                snprintf(&bits[j*3], 4, "%2.2x ", bytes[j]);
            if (insn[i].address < elf.text_shdr.sh_addr + elf.text_shdr.sh_size){
                cerr<<hex<<right<<setw(12)<<insn[i].address<<":  "
                <<left<<setw(32)<<bits
                <<left<<setw(7)<<insn[i].mnemonic
                <<left<<setw(7)<<insn[i].op_str<<endl;
            }
            else{
                cerr << "** the address is out of the range of the text segment" << endl;
                break;
            }
            
        }
        cs_free(insn, count);
    }else
        cerr << "** Can not disassemble code!" << endl;
    cs_close(&handle);
}

void dump(unsigned long dump_addr){
    if(state!=RUNNING){
        cerr<<"** State should be RUNNING!"<<endl;
        return;
    }
    if(!dump_addr){
        cerr<<"** No addr is given!\n";
        return;
    }
    if(!intext(dump_addr)){
        cerr<<"** Addr out of text region!\n";
        return;
    }

    unsigned long code1, code2;
    for(int i=0; i<DUMP_MAX; i++, dump_addr+=16){
        code1 = ptrace(PTRACE_PEEKTEXT, pid, dump_addr, 0);
        code2 = ptrace(PTRACE_PEEKTEXT, pid, dump_addr+8, 0);
        cerr<<hex<<setw(12)<<setfill(' ')<<right<<dump_addr<<": ";
        print8bytes(code1);
        print8bytes(code2);
        cerr << "|";
        print8ascii(code1);
        print8ascii(code2);
        cerr << "|" << endl;
    }
}

int bp_in_list(long bp_addr){

    for (unsigned i = 0; i < bp_list.size(); i++){
        if (bp_list[i].addr == bp_addr) return i;
    }
    return -1;
}

void set_bp(long bp_addr){
    if (state != RUNNING){
        cerr << "** PROG [" << exec_path << "] should be loaded and running!" << endl;
    }

    long code = ptrace(PTRACE_PEEKTEXT, pid, bp_addr, 0);
    if(ptrace(PTRACE_POKETEXT, pid, bp_addr, (code & 0xffffffffffffff00) | 0xcc)!=0) errquit("PTRACE_POKETEXT)", errno);

    if (bp_in_list(bp_addr) == -1){ //代表 list 中沒有
        bp_inf temp;
        temp.addr = bp_addr;
        temp.orig_code = code;
        bp_list.push_back(temp);
    }

}

void check_bp(){
    struct user_regs_struct regs;
    if(ptrace(PTRACE_GETREGS, pid, 0, &regs) != 0) errquit("ptrace(PTRACE_GETREGS)", errno);

    for (unsigned i = 0; i < bp_list.size(); i++){
        if (bp_list[i].addr > regs.rip) set_bp(bp_list[i].addr);
    }
}


void check_termin(){
    int status;
    if ((waitpid(pid, &status, 0)) < 0) errquit("waitpid", errno);

    if (WIFEXITED(status)){
        cerr << "** child process " <<dec<< pid <<" terminiated normally (code " << status << ")" << endl;
        state = LOADED;
        pid = 0;
    }

    else if (WIFSTOPPED(status)){
        if (WSTOPSIG(status)==SIGTRAP){
            struct user_regs_struct regs;
            if(ptrace(PTRACE_GETREGS, pid, 0, &regs) != 0) errquit("PTRACE_GETREGS", errno);
            int bp_idx = bp_in_list(regs.rip - 1);
            if (bp_idx > -1){
                cerr << "** breakpoint @" << "\t";
                disasm(regs.rip - 1, 1);
                if(ptrace(PTRACE_POKETEXT, pid, regs.rip - 1, bp_list[bp_idx].orig_code) != 0) errquit("ptrace(PTRACE_POKETEXT)", errno);
                
                regs.rip--;
                if(ptrace(PTRACE_SETREGS, pid, 0, &regs) != 0) errquit("ptrace(PTRACE_SETREGS)", errno);
                check_bp();
            }
            
        }
        else{
            cerr <<"** child process "<<pid<<" terminiated by signal (code"<<WSTOPSIG(status)<<")"<<endl;
        }
    }
}


void run(){
    if (state == NONLOAD){
        cerr << "** Program ["<< exec_path <<"] is not loaded" << endl;
        return;
    }
    else if (state == RUNNING){
        cerr << "** Program ["<< exec_path <<"] is running" << endl;
    }
    else{
        start();
    }

    check_bp();
    if((ptrace(PTRACE_CONT, pid, 0, 0)) < 0) errquit("ptrace(PTRACE_CONT)", errno);
    check_termin();
    
}

void cont(){
    if(state!=RUNNING){
        cerr<<"** The state is not RUNNING!" << endl;
        return;
    }
    check_bp();
    if(ptrace(PTRACE_CONT, pid, 0, 0)<0) errquit("ptrace(PTRACE_CONT)", errno);
    check_termin();
}

void si(){
    if(state!=RUNNING){
        cerr<<"** The state is not RUNNING!\n";
        return;
    }
    if(ptrace(PTRACE_SINGLESTEP, pid, 0, 0)<0) errquit("ptrace(PTRACE_SINGLESTEP)", errno);
    check_termin();
}

void set(string reg, unsigned long long val){
    if(state!=RUNNING){
        cerr<<"** The state is not RUNNING!\n";
        return;
    }

    bool reg_flag = false;
    vector<pair<string, unsigned long long* >> regs_map = get_all_reg();
    for(unsigned i = 0; i < regs_map.size(); i++){
        if (regs_map[i].first == reg){
            reg_flag = true;
            *(regs_map[i].second) = val;
            break;
        }
    }
    if (!reg_flag){
        cerr<<"** REG [" << reg << "] is not found!" << endl;
        return;
    }

    if(ptrace(PTRACE_SETREGS, pid, 0, &regs) != 0) errquit("ptrace(PTRACE_SETREGS)", errno);
}

void list(){
    for (unsigned i = 0; i < bp_list.size(); i++){
        cerr<< "\t" << i << ":\t" <<hex<< bp_list[i].addr<<endl;
    }
}

void del(unsigned del_idx){
    if (bp_list.size() == 0){
        cerr << "no breakpoint!" << endl;
        return;
    }
    
    if (del_idx > (bp_list.size() - 1)){
        cerr << "** breakpoint " <<  del_idx << " does not exist" << endl;
    }
    else{
        if(ptrace(PTRACE_POKETEXT, pid, bp_list[del_idx].addr, bp_list[del_idx].orig_code) != 0) errquit("ptrace(PTRACE_POKETEXT)", errno);
        bp_list.erase(bp_list.begin() + del_idx);
        cerr << "** breakpoint " << del_idx <<" deleted" << endl;
    }
}

void parse(string cmd){
    vector<string> cmd_line = split(cmd, ' ');
    
    if (cmd_line[0] == "load"){
        exec_path = cmd_line[1];
        load(exec_path);
    }
    else if (cmd_line[0] == "start"){
        start();
    }
    else if (cmd_line[0] == "vmmap" || cmd_line[0] == "m"){
        vmmap();
    }
    else if (cmd_line[0] == "get" || cmd_line[0] == "g"){
        get(cmd_line[1]);
    }
    else if (cmd_line[0] == "run" || cmd_line[0] == "r"){
        run();
    }
    else if (cmd_line[0] == "getregs"){
        getregs();
    }
    else if (cmd_line[0] == "cont"){
        cont();
    }
    else if (cmd_line[0] == "list" || cmd_line[0] == "l"){
        list();
    }
    else if (cmd_line[0] == "si"){
        si();
        si();
    }
    else if (cmd_line[0] == "help" || cmd_line[0] == "h"){
        help();
    } 
    else if (cmd_line[0] == "break" || cmd_line[0] == "b"){
        if(cmd_line.size() < 2){
            cerr << "** no addr is given." << endl;
            return;
        }
        set_bp(strtol(cmd_line[1].c_str(), NULL, 16));
    }
    else if (cmd_line[0] == "disasm" || cmd_line[0] == "d"){
        if (cmd_line.size() < 2){
            cerr << "** no addr is given." << endl;
            return;
        }
        disasm(strtoul(cmd_line[1].c_str(), NULL, 16), DISASM_MAX);
    }
    else if (cmd_line[0] == "dump" || cmd_line[0] == "x"){
        if (cmd_line.size() < 2){
            cerr << "** no addr is given." << endl;
            return;
        }
        dump(strtoul(cmd_line[1].c_str(), NULL, 16));
    }
    
    else if (cmd_line[0] == "delete"){
        if (cmd_line.size() < 2){
            cerr << "** no id is given." << endl;
            return;
        }
        del(stoi(cmd_line[1]));
    }
    else if (cmd_line[0] == "set" || cmd_line[0] == "s"){
        set(cmd_line[1], strtoull(cmd_line[2].c_str(), NULL, 16));
    }

}

vector<string> get_script(string script_path){
    vector<string> script;
    ifstream ifs(script_path, ios::in);
    if (!ifs.is_open()){
        cerr << "open fail" << endl;
    }
    else{
        std::string s;
        while (std::getline(ifs, s)) {
            script.push_back(s);
        }
    }
    return script;
}

int main(int argc, char *argv[]){
    string notion = "sdb> ";
    string cmd;
    if (argc > 1){
        string par_s = argv[1];
        if (par_s.find("-s") != string::npos){
            flag_s = true;
        }
    }
    
    if (argc > 1 && flag_s == false){
        exec_path = argv[1];
        load(exec_path);
    }

    if (flag_s){
        if (argc > 3){
            exec_path = argv[3];
            load(exec_path);
        }
        vector<string> script = get_script(argv[2]);
        for (unsigned i = 0; i < script.size(); i++){
            cmd = script[i];
            parse(cmd);
        }
    }
    else{
    
        while(1){
            cerr << notion;
            getline(cin, cmd);
            parse(cmd); 
        }
    }
    
}