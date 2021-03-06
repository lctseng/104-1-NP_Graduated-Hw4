#ifndef __LIB_HPP_INCLUDED
#define __LIB_HPP_INCLUDED
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <cstring>
#include <cstdlib>

#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

#include "pipe.hpp"

//#define DEBUG

#define FD_STDIN 0
#define FD_STDOUT 1
#define FD_STDERR 2

#define L_SIGMSG SIGUSR1
#define L_SIGPIPE SIGUSR2

using std::cerr;
using std::endl;
using std::string;
using std::vector;
using std::stringstream;
using std::strncpy;


typedef void (*Sigfunc)(int);


Sigfunc old_handler;

template<class T>
int debug(const T& val,bool nl = true){
  if(nl){
    cerr << ":::DEBUG:::";
  }
  cerr << val;
  if(nl){
    cerr << endl;  
  }
}

void err_abort(const string& msg){
  debug(msg);
  abort();
}

int fd_reopen(int old_fd,int target_fd){
  if(old_fd != target_fd){
    close(old_fd);
    dup2(target_fd,old_fd);
  }
  return old_fd;
}

// strip while spaces
string string_strip(const string& org){
  if(org.empty()){
    return org;
  }
  else{
    string::size_type front = org.find_first_not_of(" \t\n\r");
    if(front==string::npos){
      return "";
    }
    string::size_type end = org.find_last_not_of(" \t\n\r");
    return org.substr(front,end - front + 1);
  }
}

// wait for children pid vector and clear them
void wait_for_children(vector<pid_t>& children){
    for(const pid_t& pid : children){
      waitpid(pid,nullptr,0);
    }
    children.clear();
}


// convert string to argv
char** convert_string_to_argv(const string& arg_list){ 
  stringstream ss(arg_list);
  string arg;
  vector<string> arg_array;
  while(ss >> arg){
    arg_array.push_back(arg);
  }
  char** argv = new char*[arg_array.size()+1];
  for(int i=0;i<arg_array.size();i++){
    string& arg = arg_array[i];
    argv[i] = new char[arg.length()+1];
    strncpy(argv[i],arg.c_str(),arg.length()+1);
  }
  argv[arg_array.size()] = nullptr;
#ifdef DEBUG
  debug("ARGV:");
  char **ptr = argv;
  while(*ptr){
    debug(*ptr);
    ++ptr;
  }
#endif
  return argv;
}
// release argv space
void delete_argv(char** argv){
  char **ptr = argv;
  while(*ptr){
    delete[] *ptr;
    ++ptr;
  }
  delete[] argv;
    
}


// execute program with arg string
bool exec_cmd(const string& filename,const string& arg_list){
  char** argv = convert_string_to_argv(filename + " " + arg_list);
  if(execvp(filename.c_str(),argv)){
    delete_argv(argv);
    return false;
  }
  return true;
}

// pipe-related
void exit_error(UnixPipe& pipe,const char* msg){
  pipe.writeline(msg);
  pipe.close_write();
  abort();
}



// Signal handler for SIGCHLD
void handle_sigchld(int sig) {
  while (waitpid((pid_t)(-1), 0, WNOHANG) > 0) {}
}

void register_sigchld(){
  old_handler = signal(SIGCHLD,handle_sigchld);
}
void restore_sigchld(){
  signal(SIGCHLD,old_handler);
}

uint16_t getsockport(int sockfd){
    struct sockaddr_in localAddress;
    socklen_t addressLength = sizeof(localAddress);;
    getsockname(sockfd, (struct sockaddr*)&localAddress, &addressLength);
    return ntohs(localAddress.sin_port);
}


#endif
