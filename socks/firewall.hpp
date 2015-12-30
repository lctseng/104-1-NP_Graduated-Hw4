#ifndef __FIREWALL_HPP_INCLUDED
#define __FIREWALL_HPP_INCLUDED

#include <regex>
#include <iostream>
#include <vector>
#include <fstream>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

#include "lib.hpp"

#define FIREWALL_NOT_MATCH 0
#define FIREWALL_ALLOW 1
#define FIREWALL_DENY 2

using std::cout;
using std::endl;
using std::getline;
using std::regex;
using std::smatch;
using std::regex_search;
using std::ifstream;
using std::vector;
using std::string;
using std::stoi;

class FireWallRule{
public:
  // Allow X.X.X.X
  FireWallRule(const string& rule){
    smatch match;
    string remain = rule;
    if(regex_search(remain,match,regex("^((Allow)|(Deny))\\s+"))){
      string action_str = string_strip(match[1]);
      if(action_str == "Allow"){
        action = FIREWALL_ALLOW;
      }
      else if(action_str == "Deny"){
        action = FIREWALL_DENY;
      }
      else{
        cout << "[ERROR] Unknow firewall action: " << action_str << endl;
        action = FIREWALL_NOT_MATCH;
        return;
      }
      // split ip
      remain = match.suffix();
      while(regex_search(remain,match,regex("(\\d+)\\.?"))){
        ip[field_count++] = stoi(match[1]);
        remain = match.suffix();
        if(field_count >= 4){
          cout << "[WARNING] Ignore un-used ip fields: " << remain << endl;
          break;
        }
      }
      cout << "[INFO] Firewall rule loaded: " <<rule << endl;
    }
    else{
      cout << "[ERROR] Unknow firewall rule: " << remain << endl;
      action = FIREWALL_NOT_MATCH;
    }
  }
  int check(uint32_t check_ip){
    unsigned char* check_ip_ptr = (unsigned char*)&check_ip;
    for(int i=0;i<field_count;i++){
      // check field, if not match, return not match
      if(ip[i] != *(check_ip_ptr+i)){
        return FIREWALL_NOT_MATCH;
      }
    }
    return action;
  }

  int action = FIREWALL_NOT_MATCH;
  int field_count = 0;
  unsigned char ip[4];
};

class FireWallRules{
public:
  FireWallRules(){
    ifstream fin("socks.conf");
    if(fin.is_open()){
      string raw;
      while(getline(fin,raw)){
        string line = string_strip(raw);
        if(!line.empty()){
          rules.emplace_back(line);
        }
      }
      if(rules.empty()){
        cout << "[NOTICE] No firewall rule found, permit all connection." << endl;
        permit_all = true;
      }
    }
    else{
      cout << "[NOTICE] No firewall conf found, permit all connection." << endl;
      permit_all = true;
    }

  }

  bool check_rule(uint32_t ip){
    if(permit_all){
      return true;
    }
    else{
      for(auto& rule : rules){
        switch(rule.check(ip)){
        case FIREWALL_ALLOW:
          return true;
          break;
        case FIREWALL_DENY:
          return false;
          break;
        default:
          // continue to next
          break;
        }
      }
      // No match, deny all
      return false;
    }
  }

private:
  vector<FireWallRule> rules;
  bool permit_all = false;
};


#endif
