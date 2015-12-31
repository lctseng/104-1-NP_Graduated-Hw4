#include <iostream>
#include <regex>
#include <vector>

//#define CONSOLE
#include "nonblock_client.hpp"

#define HTML_BG_COLOR "#FFFFFF" //336699

using std::cout;
using std::cin;
using std::endl;
using std::smatch;
using std::stoi;

vector<BatchInfo> batch_list;

void print_header(){
  // commom
  cout << "Content-Type: text/html" << endl;
  cout << "Cache-Control: max-age=0" << endl;
  cout << endl;
  cout << "<html>\n <head>\n <meta http-equiv='Content-Type' content='text/html; charset=big5' />\n <title>Network Programming Homework 3</title>\n </head>\n <body bgcolor=" << HTML_BG_COLOR << ">\n <font face='Courier New' size=2 color=#FFFF99>\n" << endl;
  cout << "<table width=\"800\" border=\"1\">\n<tr>\n";
  // each column 1
  for(const BatchInfo& info : batch_list){
    cout << "<td>" << info.hostname << "</td>";
  }
  cout << "</tr><tr>";
  // each column 2
  for(int i=0;i<batch_list.size();i++){
    cout << "<td valign=\"top\" id=\"m" << i << "\"></td>";
  }
  cout << "</table>" << endl;
  
}

void print_footer(){
  cout << "</font></body></html>" << endl;
}

void parse_batch_info(){
    string remain = getenv("QUERY_STRING");
    smatch match; 
    while(regex_search(remain,match,regex("h\\d+=([a-zA-Z0-9.-]*)&p\\d+=([a-zA-Z0-9.-]*)&f\\d+=([a-zA-Z0-9.-]*)&sh\\d+=([a-zA-Z0-9.-]*)&sp\\d+=([a-zA-Z0-9.-]*)&?"))){
      if(!string(match[1]).empty()){
        batch_list.emplace_back(match[1],stoi(match[2]),match[3],match[4],stoi(match[5]));
      }
      remain = match.suffix();
    }
}

int main(){
  // fill batch list
  parse_batch_info();
  // setup client
  NonblockClientCollection clients(batch_list);
#ifndef CONSOLE
  // print header
  print_header();
#endif
#ifndef CONSOLE
  // print footer
  print_footer();
#endif
  clients.fetch_output();
  return 0;
}


