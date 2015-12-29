#include <iostream>
#include <regex>
#include <fstream>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>

#define DEFAULT_PORT 18080
#define MAX_BUF_SIZE 1024

#include "lib.hpp"

using std::string;
using std::getline;
using std::cout;
using std::endl;
using std::regex;
using std::smatch;
using std::regex_search;
using std::ifstream;

void process_client(int clientfd){
  ifdstream fin(clientfd);
  ofdstream fout(clientfd);
  // get req
  string line,req;
  getline(fin,req);
  while(getline(fin,line)){
    if(string_strip(line).empty()){
      break;
    }
  }
  // parse GET req
  cout << "Request:" << req << endl;
  smatch match;
  if(regex_search(req,match,regex("GET /(.*?)(\\?.*)? (HTTP/\\d\\.\\d)?"))){
    string filename = match[1];
    string raw_query = match[2];
    if(regex_search(filename,match,regex(".*\\.cgi$"))){
      fout << "HTTP/1.0 200 OK" << endl;
      // CGI
      string query;
      if(raw_query.empty()){
        query = "";  
      }
      else{
        query = raw_query.substr(1);
      }
      // clear env
      clearenv();
      // QUERY_STRING
      setenv("QUERY_STRING",query.c_str(),1);
      // basic http env
      setenv("CONTENT_LENGTH","content length",1);
      setenv("REQUEST_METHOD","GET",1);
      setenv("SCRIPT_NAME",filename.c_str(),1);
      setenv("REMOTE_HOST","lctseng.cs",1);
      setenv("REMOTE_ADDR","remote addr",1);
      setenv("AUTH_TYPE","auth type",1);
      setenv("REMOTE_USER","remote user",1);
      setenv("REMOTE_IDENT","remote ident",1);

      // reopen
      fd_reopen(FD_STDOUT,clientfd);
      // Exec
      if(!exec_cmd(string("./") + filename,"")){
        cout << endl << "Exec Error for " << filename << endl;
      }
    }
    else{
      // normal file
      // try to open
      ifstream file_in(filename);
      if(file_in.is_open()){
        fout << "HTTP/1.0 200 OK" << endl;
        fout << "Content-Type: text/html; charset=UTF-8" << endl  << endl;
        while(getline(file_in,line)){
          fout << line << endl;
        }
      }
      else{
        fout << "HTTP/1.0 404 Not Found" << endl << endl;
        fout << "I can't find " << filename << " for you T___T" << endl;
      }
    }
  }
  else{
    fout << "HTTP/1.0 400 Bad Request" << endl << endl;
    fout << "I Don't know what to do =___=" << endl;
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
      process_client(clientfd); 
      close(clientfd);
      exit(0);
    }
  }
  

  return 0;
}
