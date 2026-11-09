#include "../src/plugin.cpp"
#include <cassert>
#include <iostream>
uint64 current(){return 1;}
unsigned int me(uint64,anyID* p){*p=7;return 0;}
unsigned int query(uint64,anyID id,size_t flag,char** p){assert(flag==CLIENT_UNIQUE_IDENTIFIER);if(id==99){*p=nullptr;return 1;}*p=_strdup(id==8?"botUID=":"otherUID=");return 0;}
unsigned int release(void* p){free(p);return 0;}
void print(uint64,const char*,PluginMessageTarget){}
int main(){
 ts.getCurrentServerConnectionHandlerID=current;ts.getClientID=me;ts.getClientVariableAsString=query;ts.freeMemory=release;ts.printMessage=print;
 auto dir=std::filesystem::temp_directory_path()/("tts-ignore-"+std::to_string(GetCurrentProcessId()));std::filesystem::create_directories(dir);ini=(dir/L"settings.ini").wstring();
 cfg=Settings{};quitting=false;ignoredUsers.clear();
 auto send=[](anyID from,const char* name,const char* uid){queue.clear();ts3plugin_onTextMessageEvent(1,2,0,from,name,uid,"music track",0);return queue.size();};
 assert(send(8,"Bot","botUID=")==1);
 queue.push_back({L"old message",cfg,generation});auto before=generation;
 ts3plugin_onMenuItemEvent(1,PLUGIN_MENU_TYPE_CLIENT,101,8);
 assert(ignoredUsers.count("botUID=") && queue.empty() && generation>before);
 assert(send(8,"Bot","botUID=")==0);
 assert(send(25,"Renamed Bot","botUID=")==0); // Reconnect changes session clientID, not identity.
 assert(send(9,"Bot","otherUID=")==1); // Same nickname is a different person.
 ignoredUsers.clear();loadIgnoredLocked();assert(ignoredUsers.count("botUID="));
 ts3plugin_onMenuItemEvent(1,PLUGIN_MENU_TYPE_CLIENT,101,8);assert(ignoredUsers.size()==1);
 ts3plugin_onMenuItemEvent(1,PLUGIN_MENU_TYPE_CLIENT,101,9);assert(ignoredUsers.size()==2);
 ts3plugin_onMenuItemEvent(1,PLUGIN_MENU_TYPE_CLIENT,102,8);assert(send(8,"Bot","botUID=")==1);assert(send(9,"Bot","otherUID=")==0);
 ignoredUsers.clear();loadIgnoredLocked();assert(ignoredUsers.size()==1 && ignoredUsers.count("otherUID="));
 ts3plugin_onMenuItemEvent(1,PLUGIN_MENU_TYPE_CLIENT,101,99);assert(ignoredUsers.size()==1);
 ts3plugin_onMenuItemEvent(1,PLUGIN_MENU_TYPE_CLIENT,101,65536);assert(ignoredUsers.size()==1);
 ts3plugin_onMenuItemEvent(1,PLUGIN_MENU_TYPE_GLOBAL,12,0);assert(ignoredUsers.size()==1); // Reset audio options preserves ignore list.
 ts3plugin_onMenuItemEvent(1,PLUGIN_MENU_TYPE_CLIENT,102,9);ignoredUsers.insert("stale");loadIgnoredLocked();assert(ignoredUsers.empty());
 for(int i=0;i<256;i++)ignoredUsers.insert("uid"+std::to_string(i));
 ts3plugin_onMenuItemEvent(1,PLUGIN_MENU_TYPE_CLIENT,101,8);assert(ignoredUsers.size()==256 && !ignoredUsers.count("botUID="));
 ini=(dir/L"missing"/L"settings.ini").wstring();ignoredUsers.clear();ts3plugin_onMenuItemEvent(1,PLUGIN_MENU_TYPE_CLIENT,101,8);assert(ignoredUsers.empty());
 assert(!validUID("bad\nuid") && !validUID(""));
 std::cout<<"PASS: ignored users, rename/reconnect, same-name isolation, menu add/remove, persistence/empty list, dedup, cap, query failure, save rollback, queue cancellation\n";
}

