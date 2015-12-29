#ifndef __FDSTREAM_HPP_INCLUDED
#define __FDSTREAM_HPP_INCLUDED
#include <iostream>
#include <ext/stdio_filebuf.h>
using std::basic_istream;
using std::basic_ostream;

class ifdstream : public basic_istream<char>
{
  public:
    typedef __gnu_cxx::stdio_filebuf<char> buf_type;
    ifdstream(int fd):
      buf(fd,std::ios::in),
      basic_istream<char>(&buf)
    {
    }
  private:
    buf_type buf;
};
class ofdstream : public basic_ostream<char>
{
  public:
    typedef __gnu_cxx::stdio_filebuf<char> buf_type;
    ofdstream(int fd):
      buf(fd,std::ios::out),
      basic_ostream<char>(&buf)
    {
    }
  private:
    buf_type buf;
};
#endif
