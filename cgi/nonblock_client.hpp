#ifndef __NONBLOCK_CLIENT_HPP_INCLUDED
#define __NONBLOCK_CLIENT_HPP_INCLUDED
#include <iostream>
#include <sstream>
#include <fstream>
#include <regex>

#include "lib.hpp"
#include "socks.hpp"


#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>

#define S_CONNECT 0
#define S_WAIT_SOCKS 1
#define S_READ 2
#define S_WRITE 3
#define S_DONE 4

#define MAX_BUF_SIZE 16000

using std::string;
using std::ifstream;
using std::regex;
using std::regex_replace;


// append script, transform tag
string html_format(const string& src_str,int index,bool bold = false){
  // transform < and >
  // <
  string temp = regex_replace(src_str, regex("<"), "&lt;");
  temp = regex_replace(temp, regex(">"), "&gt;");
  
  if(bold){
    temp = string("% <b>") + temp + string("</b>");
  }
  // append header and footer
  stringstream ss;
#ifndef CONSOLE
  ss << "<script>document.all['m" << index << "'].innerHTML += \"" << temp << "<br>\";</script>";
#else
  ss << "client[" << index <<"]:" << temp;
#endif
  return ss.str();
}

struct BatchInfo{
  string hostname;
  int port;
  string filename;
  string sock_host;
  int sock_port;
  BatchInfo(const string& host,int port,const string& filename,const string& sock_host,int sock_port)
    :hostname(host),port(port),filename(filename),sock_host(sock_host),sock_port(sock_port)
  {}
};


class NonblockClient{
public:
  NonblockClient(int index,const BatchInfo& info)
    :index(index),
    info(info),
    state(S_CONNECT),
    fin(info.filename),
    need_write(0),
    close_read_command(false)

  {
    connect_server();
  }
  void connect_server(){
    if((peer_host=gethostbyname(info.hostname.c_str())) == NULL) {
      err_abort("Find remote hostname error");
    }
    if((sock_host=gethostbyname(info.sock_host.c_str())) == NULL) {
      err_abort("Find socks hostname error");
    }


    // create connection to sock 
    fd = socket(AF_INET,SOCK_STREAM,0);
    memset(&sock_addr, 0, sizeof(sock_addr)); 
    sock_addr.sin_family = AF_INET;
    sock_addr.sin_addr = *((struct in_addr *)sock_host->h_addr); 
    sock_addr.sin_port = htons(info.sock_port);

    int flag = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flag | O_NONBLOCK);
    if ( connect(fd,(struct sockaddr *)&sock_addr,sizeof(sock_addr)) < 0) {
      if (errno != EINPROGRESS){
        err_abort("Connect socks error");
      };
    }

    pfin = new ifdstream(fd);
    pfout = new ofdstream(fd);
  }

  void close(){
    delete pfin;
    delete pfout;
    ::close(fd);
  }

  void inject_string(const char* str){
    const char* ptr = str;
    const char* start = str;
    char buf[MAX_BUF_SIZE];
    while(*ptr){
      if(*ptr == '\n'){
        int diff = ptr - start;
        strncpy(buf,start,diff);
        buf[diff] = '\0';
        ss << buf;
        cout << html_format(regex_replace(ss.str(), regex("\r"), ""),index) << endl;
        ss.str("");
        start = ptr + 1;

      }
      ++ptr;
    }
    if(ptr != start){
      ss << start;
    }
  }

  int index;
  BatchInfo info;
  int state;
  int fd;
  stringstream ss;
  sockaddr_in  peer_addr;
  sockaddr_in  sock_addr;
  hostent      *peer_host;
  hostent      *sock_host;
  ifdstream *pfin;
  ofdstream *pfout;
  ifstream fin;
  char write_buf[MAX_BUF_SIZE+2];
  char* write_ptr;
  int need_write;
  bool close_read_command;
};

class NonblockClientCollection{
public:
  NonblockClientCollection(const vector<BatchInfo>& batch_list)
  {
    nfds = 0;
    FD_ZERO(&rs);
    FD_ZERO(&ws);
    conn = batch_list.size();
    for(int i=0;i<conn;i++){
      clients.emplace_back(i,batch_list[i]);    
      int new_fd = clients[i].fd;
      FD_SET(new_fd,&rs);
      FD_SET(new_fd,&ws);
      nfds = std::max(nfds,new_fd + 1);
    } 
  }

  void fetch_output(){
    fd_set rfds,wfds;
    socklen_t n;
    int n_byte;
    char buf[MAX_BUF_SIZE+1];
    while(conn > 0){
      memcpy(&rfds,&rs,sizeof(rfds));
      memcpy(&wfds,&ws,sizeof(wfds));
      if((select(nfds,&rfds,&wfds,NULL,NULL))<0){
        err_abort("Select Error!");
      }
      // check each client
      for(NonblockClient& client : clients){
        switch(client.state){
        case S_CONNECT:
          if(FD_ISSET(client.fd,&rfds)||FD_ISSET(client.fd,&wfds)){
            // error?
            int error;
            n = sizeof(int);
            if(getsockopt(client.fd,SOL_SOCKET,SO_ERROR,&error,&n) < 0 || error != 0){
              // non-block fail!
              cout << client.info.port << ";";
              err_abort("Nonblock connect error!");
            } 
            //cout << "Sock connected: " << client.info.sock_port << endl;
            // SOCKS connected
            // send reqest
            socks4_data socks_req;
            socks_req.vn = 4;
            socks_req.cd = CD_CONNECT;
            socks_req.dst_port = htons(client.info.port); 
            socks_req.dst_ip = ((struct in_addr *)client.sock_host->h_addr)->s_addr;
            socks_req.write_to_fd(client.fd,true); // with user id
            client.state = S_WAIT_SOCKS;
          }
          break;
        case S_WAIT_SOCKS:
          if(FD_ISSET(client.fd,&rfds)){
            // read request from sock
            socks4_data socks_rep;
            if(socks_rep.read_from_fd(client.fd,false)){ // no read id
              // check for cd
              if(socks_rep.cd == CD_GRANTED){
                // every thing is ready!
                client.state = S_READ;
              }
              else{
                // failure!
                err_abort("Socks access error! Request rejected!");
              }
            }
            else{
              // failure!
              err_abort("Socks access error! Server terminated!");
            }

          }
          break;
        case S_READ:
          if(FD_ISSET(client.fd,&rfds)){
            bzero(buf,MAX_BUF_SIZE);
            n_byte = read(client.fd,buf,MAX_BUF_SIZE);
            if(n_byte > 0){

              buf[n_byte] = '\0';
              if(!client.close_read_command){
                // when get %, go to write mode
                for(int i=0;i<n_byte;i++){
                  if(buf[i]=='%'){
                    buf[i] = '\0';
                    client.state = S_WRITE;
                    break;
                  }
                } 
              }
              client.inject_string(buf);
            }
            else{

              //cout << "Client:" << client.info.port << " Disconnected" << endl; 
              // disconnected
              client.state = S_DONE;
              --conn;
              FD_CLR(client.fd,&rs);
              FD_CLR(client.fd,&ws);
              client.close();
            }
          }
          break;
        case S_WRITE:
          if(FD_ISSET(client.fd,&wfds)){
            if(client.need_write > 0){
              int w_byte = write(client.fd,client.write_ptr,client.need_write);
              if(w_byte <= 0){
                // cannot write...
                cout << "Cannot write!" << endl;
              }
              else{
                // write it!
                if(w_byte == client.need_write){
                  // all written
                  client.need_write = 0;
                  // back to read mode
                  client.state = S_READ;
                }
                else{
                  client.need_write -= w_byte;
                  client.write_ptr += w_byte;
                }
              }

            }
            else{
              // need write == 0 : fetch new
              if(client.need_write == 0 && !client.close_read_command){
                string line;  
                if(getline(client.fin,line)){
                  line = regex_replace(line, regex("\r"), "");
                  // store into buffer
                  int len = line.size();
                  strncpy(client.write_buf,line.c_str(),MAX_BUF_SIZE);
                  client.write_buf[len] = '\n';
                  client.write_buf[len+1] = '\0';
                  client.write_ptr = client.write_buf;
                  client.need_write = len+1;
                  cout << html_format(line,client.index,true) << endl;
                  // judge exit
                  if(regex_search(line, regex("^\\s*exit(\\s+|$)"))){
                    client.close_read_command = true;
                  }

                }
                else{
                  // EOF
                  client.need_write = -1;
                }
              }
              else{
                // < 0: write closed
              }
            }
          }
          break;
        }
      }

    }
  }

  /*

     void fetch_output(){
     for(int i=0;i<3;i++)
     for(NonblockClient& client : clients){
     cout << html_format("testHTML",client.index);
     sleep(1);
     }
     }
   */

  vector<NonblockClient> clients;
  int nfds;
  fd_set rs;
  fd_set ws;
  int conn;

};


#endif


