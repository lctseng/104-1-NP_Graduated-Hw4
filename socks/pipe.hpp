#ifndef __PIPE_HPP_INCLUDED
#define __PIPE_HPP_INCLUDED
#include <string>
#include <deque>
#include <iostream>

#include <unistd.h>


#include "lib.hpp"
#include "fdstream.hpp"

using std::string;
using std::getline;
using std::deque;
using std::cout;
using std::endl;

#define PIPE_MAX_READ 65535


class UnixPipe;
// Gobal Variables
deque<UnixPipe> pipe_pool;

class UnixPipe{
public:
  UnixPipe(bool real = true){
    if(!real){
      read_fd = -1;
      write_fd = -1;
    }
    else{
      int fd[2];
      pipe(fd);
      read_fd = fd[0];
      write_fd = fd[1];
    }
  } 
  bool is_write_closed()const{
    return write_fd < 0;
  }
  bool is_read_closed()const{
    return read_fd < 0;
  }
  int write(const string& str)const{
    if(!is_write_closed()){
      return ::write(write_fd,str.c_str(),str.length()+1);
    }
    else{
      throw "Write end is closed!";
      return 0;
    }
  }
  int writeline(const string& str)const{
    return write(str+"\r\n");
  }
  string getline()const{
    if(!is_read_closed()){
      string str;
      ifdstream is(read_fd);
      ::getline(is,str);
      return str;
    }
    else{
      throw "Read end is closed!";
      return "";
    }
  }
  bool close_read(){
    if(!is_read_closed()){
      ::close(read_fd);
      read_fd = -1;
      return true;
    }
    return false;
  }
  bool close_write(){
    if(!is_write_closed()){
      ::close(write_fd);
      write_fd = -1;
      return true;
    }
    return false;
  }
  bool is_alive() const{
    return !is_write_closed() || !is_read_closed();
  }
  void close(){
    close_read();
    close_write();
  }

  // member variables

  int read_fd,write_fd;

};

void init_pipe_pool(){
  pipe_pool = deque<UnixPipe>();
  pipe_pool.push_back(UnixPipe(false));
}

UnixPipe create_numbered_pipe(int count){
  int current_max = (int)pipe_pool.size() - 1;
  if(current_max < count){ // new pipe needed
    return UnixPipe();
  }
  else{
    // check of that pipe is alive
    if(pipe_pool[count].is_alive()){
      return pipe_pool[count];
    }
    else{
      return UnixPipe();
    }
  }
}
void record_pipe_info(int count,const UnixPipe& pipe){
  if(count >= 0){
    int current_max = (int)pipe_pool.size() - 1;
    if(current_max < count){
      // fill pipe pool
      for(int i=current_max+1;i<count;i++){

        pipe_pool.push_back(UnixPipe(false));
      }
      // create on that
      pipe_pool.push_back(pipe);
    }
    else{
      // check of that pipe is alive
      if(!pipe_pool[count].is_alive()){
        pipe_pool[count] = pipe;
      }
    }
  }
}

void close_pipe_larger_than(int count){
  if(pipe_pool.size()>1){
    for(int i=count+1;i<pipe_pool.size();i++){
      pipe_pool[i].close();
    }
  }
}

void decrease_pipe_counter_and_close(){
  if(!pipe_pool.empty()){
    pipe_pool.pop_back();
    if(!pipe_pool.empty()){
      pipe_pool[0].close();
    }
  }
}

#endif
