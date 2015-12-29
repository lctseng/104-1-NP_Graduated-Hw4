#include <unistd.h>


#define CD_CONNECT 1
#define CD_BIND 2
#define CD_GRANTED 90
#define CD_REJECTED 91

#define SOCKS_REQ_FIELD_LENGTH  8

struct socks4_data{
  uint8_t vn;
  uint8_t cd;
  uint16_t dst_port;
  uint32_t dst_ip;
  char user_id[1024];

  char* data(){
    return (char*)this;
  }

  void fill_ip_from_string(const char* ip_str){

  }

  void fill_port(int port){
    
  }

  void read_from_fd(int fd){
    int id_sz = 0;
    // read req field
    read(fd,data(),SOCKS_REQ_FIELD_LENGTH);
    // read user id
    char c;
    while(true){
      read(fd,&c,1);
      user_id[id_sz++] = c;
      if(c == 0){
        break;
        // terminate
      }
    }
  }

  void write_to_fd(int fd){
    // required field
    write(fd,data(),SOCKS_REQ_FIELD_LENGTH);
  }


  socks4_data()
    :vn(0),
     user_id(""),
     cd(0),
     dst_port(0),
     dst_ip(0)
  {}
};
