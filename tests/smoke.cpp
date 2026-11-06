#include <windows.h>
#include <cassert>
#include <filesystem>
#include <iostream>
#include "ts3_functions.h"
void config(char* p,size_t n){auto dir=std::filesystem::temp_directory_path()/"channel-tts-smoke";std::filesystem::create_directories(dir);auto s=dir.u8string()+"/";strcpy_s(p,n,s.c_str());}
unsigned int logmsg(const char* m,LogLevel,const char*,uint64){std::cout<<m<<'\n';return 0;}
void print(uint64,const char* m,PluginMessageTarget){std::cout<<m<<'\n';}
uint64 current(){return 1;}
unsigned int client(uint64,anyID* id){*id=7;return 0;}
int main(){
 HMODULE dll=LoadLibraryW(L"channel_tts_win64.dll");assert(dll);
 auto version=(int(*)())GetProcAddress(dll,"ts3plugin_apiVersion");assert(version&&version()==26);
 auto set=(void(*)(TS3Functions))GetProcAddress(dll,"ts3plugin_setFunctionPointers");
 auto init=(int(*)())GetProcAddress(dll,"ts3plugin_init");
 auto stop=(void(*)())GetProcAddress(dll,"ts3plugin_shutdown");
 auto cmd=(int(*)(uint64,const char*))GetProcAddress(dll,"ts3plugin_processCommand");
 auto msg=(int(*)(uint64,anyID,anyID,anyID,const char*,const char*,const char*,int))GetProcAddress(dll,"ts3plugin_onTextMessageEvent");
 assert(set&&init&&stop&&cmd&&msg);
 TS3Functions f{};f.getConfigPath=config;f.logMessage=logmsg;f.printMessage=print;f.getCurrentServerConnectionHandlerID=current;f.getClientID=client;set(f);
 for(int i=0;i<3;i++){
  assert(init()==0);cmd(1,"volume 0");cmd(1,"on");cmd(1,"self off");
  assert(msg(1,2,0,8,"Alice","","<xml>hello</xml>",0)==0);
  assert(msg(1,1,7,8,"Alice","","private",0)==0);
  assert(msg(2,2,0,8,"Alice","","other server",0)==0);
  assert(msg(1,2,0,7,"Self","","self",0)==0);
  assert(msg(1,2,0,8,"Alice","","ignored",1)==0);
  cmd(1,"rate 99");cmd(1,"rate invalid");cmd(1,"off");cmd(1,"status");cmd(1,"reload");cmd(1,"stop");stop();
 }
 FreeLibrary(dll);std::cout<<"PASS: exports, init/shutdown x3, callbacks and commands\n";
}

