#include <windows.h>
#include <cassert>
#include <filesystem>
#include <iostream>
#include "ts3_functions.h"
void config(char* p,size_t n){auto dir=std::filesystem::temp_directory_path()/"channel-tts-smoke";std::filesystem::create_directories(dir);auto s=dir.u8string()+"/";strcpy_s(p,n,s.c_str());}
unsigned int logmsg(const char* m,LogLevel,const char*,uint64){std::cout<<m<<'\n';return 0;}
std::string lastMessage;
void print(uint64,const char* m,PluginMessageTarget){lastMessage=m;std::cout<<m<<'\n';}
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
  auto reg=(void(*)(const char*))GetProcAddress(dll,"ts3plugin_registerPluginID");
 auto menus=(void(*)(PluginMenuItem***,char**))GetProcAddress(dll,"ts3plugin_initMenus");
 auto freeMem=(void(*)(void*))GetProcAddress(dll,"ts3plugin_freeMemory");
 auto click=(void(*)(uint64,PluginMenuType,int,uint64))GetProcAddress(dll,"ts3plugin_onMenuItemEvent");
 assert(set&&init&&stop&&cmd&&msg&&reg&&menus&&freeMem&&click);
 TS3Functions f{};f.getConfigPath=config;f.logMessage=logmsg;f.printMessage=print;f.getCurrentServerConnectionHandlerID=current;f.getClientID=client;set(f);
 for(int i=0;i<3;i++){
    assert(init()==0); reg("test-channel-tts");
  PluginMenuItem** items=nullptr; char* icon=nullptr; menus(&items,&icon);
  assert(items && !icon);
  for(int j=0;j<20;j++){assert(items[j] && items[j]->id==(j<17?j+1:101+j-17) && items[j]->type==(j<17?PLUGIN_MENU_TYPE_GLOBAL:PLUGIN_MENU_TYPE_CLIENT));assert(MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,items[j]->text,-1,nullptr,0)>0);freeMem(items[j]);}
  assert(items[20]==nullptr);freeMem(items);
  click(1,PLUGIN_MENU_TYPE_GLOBAL,12,0);
  assert(lastMessage.find("语速：0；音量：80")!=std::string::npos);
  click(1,PLUGIN_MENU_TYPE_GLOBAL,2,0);assert(lastMessage.find("已关闭")!=std::string::npos);
  click(1,PLUGIN_MENU_TYPE_GLOBAL,1,0);assert(lastMessage.find("已开启")!=std::string::npos);
  click(1,PLUGIN_MENU_TYPE_GLOBAL,4,0);assert(lastMessage.find("朗读自己：是")!=std::string::npos);
  click(1,PLUGIN_MENU_TYPE_GLOBAL,5,0);assert(lastMessage.find("朗读昵称：否")!=std::string::npos);
  for(int j=0;j<25;j++)click(1,PLUGIN_MENU_TYPE_GLOBAL,6,0);
  assert(lastMessage.find("语速：10")!=std::string::npos);
  for(int j=0;j<25;j++)click(1,PLUGIN_MENU_TYPE_GLOBAL,7,0);
  assert(lastMessage.find("语速：-10")!=std::string::npos);
  for(int j=0;j<20;j++)click(1,PLUGIN_MENU_TYPE_GLOBAL,8,0);
  assert(lastMessage.find("音量：100")!=std::string::npos);
  for(int j=0;j<20;j++)click(1,PLUGIN_MENU_TYPE_GLOBAL,9,0);
  assert(lastMessage.find("音量：0")!=std::string::npos);
  click(1,PLUGIN_MENU_TYPE_GLOBAL,11,0);assert(lastMessage.find("音量：0")!=std::string::npos);
  click(1,PLUGIN_MENU_TYPE_GLOBAL,3,0);click(1,PLUGIN_MENU_TYPE_GLOBAL,10,0);
  cmd(1,"volume 0");cmd(1,"on");cmd(1,"self off");
  assert(msg(1,2,0,8,"Alice","","<xml>hello</xml>",0)==0);
  assert(msg(1,1,7,8,"Alice","","private",0)==0);
  assert(msg(2,2,0,8,"Alice","","other server",0)==0);
  assert(msg(1,2,0,7,"Self","","self",0)==0);
  assert(msg(1,2,0,8,"Alice","","ignored",1)==0);
  cmd(1,"rate 99");cmd(1,"rate invalid");cmd(1,"off");cmd(1,"status");cmd(1,"reload");cmd(1,"stop");stop();
 }
 FreeLibrary(dll);std::cout<<"PASS: exports, init/shutdown x3, callbacks, commands, menus, UTF-8 labels, bounds and saved settings\n";
}




