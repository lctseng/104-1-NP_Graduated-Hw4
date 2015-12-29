#include <iostream>
#include <regex>
#include <fstream>
#include <iomanip>
#include <cstdio>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

#define DEFAULT_PORT 18090
#define MAX_BUF_SIZE 10240

#include "socks.hpp"
#include "lib.hpp"

using std::string;
using std::cout;
using std::endl;
using std::regex;
using std::smatch;
using std::regex_search;
using std::ifstream;


int nfds;
fd_set rs;
fd_set ws;
fd_set rfds,wfds;
char buf[MAX_BUF_SIZE];

// return value: whether to close connection
bool read_a_to_b(int from_fd,int to_fd){
  bzero(buf,MAX_BUF_SIZE);
  int read_n = read(from_fd,buf,MAX_BUF_SIZE); 
  if(read_n <= 0){
    // client closed, check if there data from remote
    if(FD_ISSET(to_fd,&rfds)){
      int read_n = read(to_fd,buf,MAX_BUF_SIZE); 
      if(read_n <= 0){
        // remote closed, too
        // do nothing
      }
      else{
        // remote has data
        // send these data back to client if writable
        //if(FD_ISSET(from_fd,&wfds)){
          write(to_fd,buf,read_n);
        //}
      }
    }
    // remote may have no data, close both
    return false;
  }
  else{
    cout << read_n << " bytes (";
    // dump up to 10 bytes
    int dump_max = std::min(read_n,10);
    for(int i=0;i<dump_max;i++){
      printf("%02hhx ", buf[i]);
    }
    cout  << ")" <<endl;
    // write to remote
    write(to_fd,buf,read_n);
    return true;
  }
}


void process_client(sockaddr_in* p_client_addr,int client_fd){
  sockaddr_in  remote_addr;
  char src_ip[20],dst_ip[20];
  uint16_t src_port,dst_port;
  src_port = ntohs(p_client_addr->sin_port);
  strncpy(src_ip,inet_ntoa(p_client_addr->sin_addr),20);
  // read sock request
  socks4_data socks_req;
  socks_req.read_from_fd(client_fd);
  // set remote data
  struct in_addr in_addr_dst_ip;
  in_addr_dst_ip.s_addr = socks_req.dst_ip;
  strncpy(dst_ip,inet_ntoa(in_addr_dst_ip),20);
  dst_port = ntohs(socks_req.dst_port);
  if (socks_req.cd == CD_CONNECT){
    cout << "[INFO] SOCKS CONNECT Request" 
         << " [User ID] \"" << socks_req.user_id
         << "\" [Src] " << src_ip << ":" << src_port << " [Dst] " 
         << dst_ip << ":" << dst_port << endl;
    // connect to remote
    // create socket and fill addr data
    int remote_fd = socket(AF_INET,SOCK_STREAM,0);
    memset(&remote_addr, 0, sizeof(remote_addr)); 
    remote_addr.sin_family = AF_INET;
    remote_addr.sin_addr.s_addr = socks_req.dst_ip;
    remote_addr.sin_port = socks_req.dst_port;
    // connect remote
    if(connect(remote_fd,(struct sockaddr *)&remote_addr,sizeof(remote_addr)) == -1) {
      err_abort("Sock Connect Error");
    }
    // reply to client: socks ready
    socks4_data socks_rep = socks_req;
    socks_rep.vn = 0x0;
    socks_rep.cd = CD_GRANTED;
    socks_rep.write_to_fd(client_fd);
    cout << "[INFO] SOCKS Request granted " 
         << "[Src] " << src_ip << ":" << src_port << " [Dst] " 
         << dst_ip << ":" << dst_port << endl;
    // start ot relay
    // using select
    // init fds
    nfds = std::max(client_fd,remote_fd) + 1;
    FD_ZERO(&rs);
    FD_ZERO(&ws);
    FD_SET(client_fd,&rs);
    FD_SET(client_fd,&ws);
    FD_SET(remote_fd,&rs);
    FD_SET(remote_fd,&ws);
    
    // select loop
    while(true){
      memcpy(&rfds,&rs,sizeof(rfds));
      memcpy(&wfds,&ws,sizeof(wfds));
      if((select(nfds,&rfds,&wfds,NULL,NULL))<0){
        err_abort("Select Error!");
      }
      // client read
      if(FD_ISSET(client_fd,&rfds)){
        cout << "[INFO] Data from " 
             << src_ip << ":" << src_port << " to " 
             << dst_ip << ":" << dst_port << ", ";
        if(!read_a_to_b(client_fd,remote_fd)){
          cout << " client closed" << endl;
          break;
        }
      }
      // remote read
      if(FD_ISSET(remote_fd,&rfds)){
        cout << "[INFO] Data from " 
             << dst_ip << ":" << dst_port << " to " 
             << src_ip << ":" << src_port << ", ";
        if(!read_a_to_b(remote_fd,client_fd)){
          cout << " remote closed" << endl;
          break;
        }
      }
    }

  }
  else{
    cout << "[INFO] SOCKS BIND Request" 
         << " [User ID] \"" << socks_req.user_id
         << "\" [Src] " << src_ip << ":" << src_port << " [Dst] " 
         << dst_ip << ":" << dst_port << endl;
  }

  
  
}


int main(int argc,char** argv){
  // get port
  int port;
  if(argc < 2){
    port = DEFAULT_PORT; 
  }
  else{
    port = atoi(argv[1]);
  }
  cout << "Listen to port:" << port << endl;
  // start program
  int sockfd,clientfd;
  socklen_t cli_addr_len;
  sockaddr_in cli_addr,serv_addr;
  pid_t pid;
  
  // socket
  if((sockfd=socket(AF_INET,SOCK_STREAM,0))<0){
    err_abort("can't create server socket");
  }
  // fill addr
  bzero((char *) &serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port = htons(port);
  // bind
  if(bind(sockfd,(sockaddr*)&serv_addr,sizeof(serv_addr))<0){
    err_abort("can't bind local address");
  } 
  // listen
  listen(sockfd,0);
  // accept client
  while(true){
    cli_addr_len = sizeof(cli_addr);
    clientfd = accept(sockfd,(sockaddr*)&cli_addr,&cli_addr_len);
    if(clientfd<0){
      debug("Accept error!");
    }
    if((pid=fork())<0){
      err_abort("Fork error!");
    }
    else if(pid>0){
      // parent
      close(clientfd);
    }
    else{
      // child
      close(sockfd); 
      process_client(&cli_addr,clientfd); 
      close(clientfd);
      exit(0);
    }
  }
  

  return 0;
}
