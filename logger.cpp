#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <dirent.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/stat.h>
#include <pwd.h>
#include <vector>
#include <dlfcn.h>
#include <sys/types.h>
#include <cstdarg>
#include <fcntl.h>


using namespace std;

#define LIBC "libc.so.6"

void* handle;
char real_path_buf[200];

FILE* output_file = NULL;
int (* old_chmod)(const char*, mode_t) = NULL;
int (* old_chown)(const char *pathname, uid_t owner, gid_t group) = NULL;
int (* old_close)(int fd) = NULL;
int (* old_creat)(const char *path, mode_t mode) = NULL;
int (* old_creat64)(const char *path, mode_t mode) = NULL;
FILE* (*old_fopen)(const char *,  const char *) = NULL;
FILE* (*old_fopen64)(const char *,  const char *) = NULL;
int (*old_fclose)(FILE *stream) = NULL;
size_t (*old_fread)(void *ptr, size_t size, size_t nmemb,FILE *stream) = NULL;
size_t (*old_fwrite)(const void *ptr, size_t size, size_t nitems, FILE *stream) = NULL;
int (*old_open)(const char *path, int oflag, ...) = NULL;
int (*old_open64)(const char *path, int oflag, ...) = NULL;
ssize_t (*old_read)(int fildes, void *buf, size_t nbyte) = NULL;
int (*old_remove)(const char *pathname) = NULL;
int (*old_rename)(const char *old, const char *new_one) = NULL;
FILE* (*old_tmpfile)(void) = NULL;
FILE* (*old_tmpfile64)(void) = NULL;
ssize_t (*old_write)(int fd, const void *buf, size_t count) = NULL;


void get_fun(void** old_fun, const char* libc_fun) {
    handle = dlopen(LIBC, RTLD_LAZY);

    if (!handle){
        cerr << dlerror() << endl;
    }
    else{
        *old_fun = dlsym(handle, libc_fun);
    }
}

void sget_real_path(const char * path, char* real_path_buf){
    memset(real_path_buf, '\0', 200);

    realpath(path, real_path_buf);

    if(real_path_buf == NULL)//轉換失敗
        strcpy(real_path_buf, path);
}

string pget_real_path(FILE* stream){

    char link_dest[200] = {'\0'};
    int fd_no = fileno(stream);
    int pid = getpid();
    string fd_path = "/proc/" + to_string(pid) + "/fd/" + to_string(fd_no);
    readlink(fd_path.c_str(), link_dest, sizeof(link_dest)-1);

    return string(link_dest);
}

void check_character(char *ptr, char *output_char){

    int index = 0;
    while(ptr[index] != '\0' && index <= 31){
        if (isprint(ptr[index])){
            output_char[index] = ptr[index];
        }
        else{
            output_char[index] = '.';
        }
        index++;
    }
    output_char[index] = '\0';
}

extern "C"{
    
    int chmod(const char *pathname, mode_t mode) {
        if (old_chmod == NULL){
            get_fun((void**)&old_chmod, "chmod");
        }
        
        int re;
        if (old_chmod != NULL){
            re = old_chmod(pathname, mode);
            sget_real_path(pathname, real_path_buf);
            dprintf(atoi(getenv("OUTPUT_FD")), "[logger] chmod(\"%s\", \"%o\") = %d\n",  real_path_buf, mode, re);
        }
        return re;
    }
    int chown(const char *pathname, uid_t owner, gid_t group){
        if (old_chown == NULL){
            get_fun((void**)&old_chown, "chown");
        }

        int re;
        if (old_chown != NULL){
            re = old_chown(pathname, owner, group);
            sget_real_path(pathname, real_path_buf);
            dprintf(atoi(getenv("OUTPUT_FD")), "[logger] chown(\"%s\", \"%o\", \"%o\") = %d\n",  real_path_buf, owner, group, re);
        }
        return re;

    }

    int close(int fd){
        if (old_close == NULL){
            get_fun((void**)&old_close, "close");
        }

        int re;
        if (old_close != NULL){
            char link_dest[200];
            memset(link_dest, '\0', 200);
            int pid = getpid();
            string fd_path = "/proc/" + to_string(pid) + "/fd/" + to_string(fd);
            readlink(fd_path.c_str(), link_dest, sizeof(link_dest)-1);

            re = old_close(fd);
            dprintf(atoi(getenv("OUTPUT_FD")), "[logger] close(\"%s\") = %d\n",  link_dest, re);
        }
        dlclose(handle);
        return re;
    }

    int creat(const char *path, mode_t mode){
        if (old_creat == NULL){
            get_fun((void**)&old_creat, "creat");
        }

        int re;
        if (old_creat != NULL){

            re = old_creat(path, mode);
            sget_real_path(path, real_path_buf);
            dprintf(atoi(getenv("OUTPUT_FD")), "[logger] creat(\"%s\", \"%o\") = %d\n",  real_path_buf, mode, re);
            
        }

        dlclose(handle);
        return re;
    }

    int creat64(const char *path, mode_t mode){
        if (old_creat64 == NULL){
            get_fun((void**)&old_creat64, "creat64");
        }

        int re;
        if (old_creat64 != NULL){

            re = old_creat64(path, mode);
            sget_real_path(path, real_path_buf);
            dprintf(atoi(getenv("OUTPUT_FD")), "[logger] creat64(\"%s\", \"%o\") = %d\n",  real_path_buf, mode, re);
            
        }

        dlclose(handle);
        return re;
    }

    
    FILE* fopen(const char * pathname, const char * mode)
    {
        
        if (old_fopen == NULL){
            get_fun((void**)&old_fopen, "fopen");
        }

        FILE* f_addr;

        if (old_fopen != NULL){
            f_addr = old_fopen(pathname, mode);
            sget_real_path(pathname, real_path_buf);
            dprintf(atoi(getenv("OUTPUT_FD")), "[logger] fopen(\"%s\", \"%s\") = %p\n",  real_path_buf, mode, f_addr);
        }
        
        dlclose(handle);
        return f_addr;
    }

    FILE* fopen64(const char * pathname, const char * mode)
    {
        
        if (old_fopen64 == NULL){
            get_fun((void**)&old_fopen64, "fopen64");
        }

        FILE* f_addr;

        if (old_fopen64 != NULL){
            f_addr = old_fopen64(pathname, mode);
            sget_real_path(pathname, real_path_buf);
            dprintf(atoi(getenv("OUTPUT_FD")), "[logger] fopen64(\"%s\", \"%s\") = %p\n",  real_path_buf, mode, f_addr);
        }
        
        dlclose(handle);
        return f_addr;
    }

    int fclose(FILE *stream)
    {
        if (old_fclose == NULL){
            get_fun((void**)&old_fclose, "fclose");
        }

        int re;
        if (old_fclose != NULL){

            string link_dest = pget_real_path(stream);
            re = (*old_fclose)(stream);
            dprintf(atoi(getenv("OUTPUT_FD")), "[logger] fclose(\"%s\") = %d\n",  link_dest.c_str(), re);
        }
        dlclose(handle);

        return re;
    }

    size_t fread(void *ptr, size_t size, size_t nmemb,FILE *stream){
        if (old_fread == NULL){
            get_fun((void**)&old_fread, "fread");
        }

        int re;
        if (old_fread != NULL){

            string link_dest = pget_real_path(stream);
            char output_char[32] = {'\0'};
            check_character((char*)ptr, output_char);

            re = old_fread(ptr, size, nmemb, stream);
            dprintf(atoi(getenv("OUTPUT_FD")), "[logger] fread(\"%s\", %lu, %lu, \"%s\") = %d\n",  output_char, size, nmemb, link_dest.c_str(), re);

        }
        dlclose(handle);
        return re;

    }


    size_t fwrite(const void *ptr, size_t size, size_t nitems, FILE *stream){
        if (old_fwrite == NULL){
            get_fun((void**)&old_fwrite, "fwrite");
        }

        int re;
        if (old_fwrite != NULL){

            string link_dest = pget_real_path(stream);
            char output_char[32] = {'\0'};
            check_character((char*)ptr, output_char);

            re = old_fwrite(ptr, size, nitems, stream);
            dprintf(atoi(getenv("OUTPUT_FD")), "[logger] fwrite(\"%s\", %lu, %lu, \"%s\") = %d\n",  output_char, size, nitems, link_dest.c_str(), re);

        }
        dlclose(handle);
        return re;
    }

    int open(const char *path, int oflag, ...){
        if (old_open == NULL){
            get_fun((void**)&old_open, "open");
        }

        mode_t mode = 0;
        if (__OPEN_NEEDS_MODE (oflag)){
            va_list arg;
            va_start (arg, oflag);
            mode = va_arg (arg, int);
            va_end (arg);
        }

        int re;
        if (old_open != NULL){

            re = old_open(path, oflag, mode);
            sget_real_path(path, real_path_buf);
            dprintf(atoi(getenv("OUTPUT_FD")), "[logger] open(\"%s\", \"%d\", \"%o\") = %d\n",  real_path_buf, oflag, mode, re);
            
        }

        dlclose(handle);
        return re;
    }

    int open64(const char *path, int oflag, ...){
        if (old_open64 == NULL){
            get_fun((void**)&old_open64, "open64");
        }

        mode_t mode = 0;
        if (__OPEN_NEEDS_MODE (oflag)){
            va_list arg;
            va_start (arg, oflag);
            mode = va_arg (arg, int);
            va_end (arg);
        }

        int re;
        if (old_open64 != NULL){

            re = old_open64(path, oflag, mode);
            sget_real_path(path, real_path_buf);
            dprintf(atoi(getenv("OUTPUT_FD")), "[logger] open64(\"%s\", \"%d\", \"%o\") = %d\n",  real_path_buf, oflag, mode, re);
            
        }

        dlclose(handle);
        return re;
    }

    ssize_t read(int fildes, void *buf, size_t nbyte){
        if (old_read == NULL){
            get_fun((void**)&old_read, "read");
        }

        int re;
        if (old_read != NULL){

            char link_dest[200];
            memset(link_dest, '\0', 200);
            int pid = getpid();
            string fd_path = "/proc/" + to_string(pid) + "/fd/" + to_string(fildes);
            readlink(fd_path.c_str(), link_dest, sizeof(link_dest)-1);

            re = old_read(fildes, buf, nbyte);
            char output_char[32] = {'\0'};
            check_character((char*)buf, output_char);
            dprintf(atoi(getenv("OUTPUT_FD")), "[logger] read(\"%s\", \"%s\",  \"%lu\") = %d\n",  link_dest, output_char, nbyte, re);
        }

        dlclose(handle);
        return re;
    }

    int remove(const char *pathname){
        if (old_remove == NULL){
            get_fun((void**)&old_remove, "remove");
        }

        int re;
        if (old_remove != NULL){
            re = old_remove(pathname);
            memset(real_path_buf, '\0', 200);
            sget_real_path(pathname, real_path_buf);
            dprintf(atoi(getenv("OUTPUT_FD")), "[logger] remove(\"%s\") = %d\n",  real_path_buf, re);
        }

        dlclose(handle);
        return re;
    }

    int rename(const char *old, const char *new_one){
        if (old_rename == NULL){
            get_fun((void**)&old_rename, "rename");
        }

        int re;
        if (old_rename != NULL){
            re = old_rename(old, new_one);
            char old_path[200] = {'\0'};
            char new_path[200] = {'\0'};

            realpath(old, old_path);
            realpath(new_one, new_path);

            dprintf(atoi(getenv("OUTPUT_FD")), "[logger] rename(\"%s\", \"%s\") = %d\n",  old_path, new_path, re);
        }

        dlclose(handle);
        return re;
    }

    FILE *tmpfile(void){
        if (old_tmpfile == NULL){
            get_fun((void**)&old_tmpfile, "tmpfile");
        }

        FILE* f_addr;
        if (old_tmpfile != NULL){
            f_addr = old_tmpfile();

            dprintf(atoi(getenv("OUTPUT_FD")), "[logger] tmpfile() = %p\n", f_addr);
        }

        dlclose(handle);
        return f_addr;
    }

    FILE *tmpfile64(void){
        if (old_tmpfile64 == NULL){
            get_fun((void**)&old_tmpfile64, "tmpfile64");
        }

        FILE* f_addr;
        if (old_tmpfile64 != NULL){
            f_addr = old_tmpfile64();

            dprintf(atoi(getenv("OUTPUT_FD")), "[logger] tmpfile64() = %p\n", f_addr);
        }

        dlclose(handle);
        return f_addr;
    }


    ssize_t write(int fd, const void *buf, size_t count){
        if (old_write == NULL){
            get_fun((void**)&old_write, "write");
        }

        int re;
        if (old_write != NULL){

            char link_dest[200];
            memset(link_dest, '\0', 200);
            int pid = getpid();
            string fd_path = "/proc/" + to_string(pid) + "/fd/" + to_string(fd);
            readlink(fd_path.c_str(), link_dest, sizeof(link_dest)-1);

            re = old_write(fd, buf, count);
            char output_char[32] = {'\0'};
            check_character((char*)buf, output_char);
            dprintf(atoi(getenv("OUTPUT_FD")), "[logger] write(\"%s\", \"%s\",  \"%lu\") = %d\n",  link_dest, output_char, count, re);
        }

        dlclose(handle);
        return re;
    }

}