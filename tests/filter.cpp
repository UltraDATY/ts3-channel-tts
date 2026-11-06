#include "../src/plugin.cpp"
#include <cassert>
#include <iostream>
uint64 testServer(){return 1;}
unsigned int testClient(uint64,anyID* id){*id=7;return 0;}
unsigned int testLog(const char*,LogLevel,const char*,uint64){return 0;}
void testPrint(uint64,const char*,PluginMessageTarget){}
int main(){
    ts.getCurrentServerConnectionHandlerID=testServer;ts.getClientID=testClient;
    ts.logMessage=testLog;ts.printMessage=testPrint;
    auto dir=std::filesystem::temp_directory_path()/("channel-tts-filter-"+std::to_string(GetCurrentProcessId()));
    std::filesystem::create_directories(dir);
    ini=(dir/L"channel_tts.ini").wstring();wordPath=(dir/L"words.txt").wstring();
    createWordsFile();loadWordsLocked();assert(blockedWords.size()==3);
    {std::ofstream f(std::filesystem::path(wordPath),std::ios::binary);f<<"\xEF\xBB\xBF# comment\r\n  测试禁词 \r\nBaDwOrD\r\n\r\n";}
    createWordsFile();loadWordsLocked();assert(blockedWords.size()==2);
    cfg=Settings{};cfg.filter=true;cfg.names=true;quitting=false;
    auto send=[](const char* text,const char* name="Alice"){
        queue.clear(); assert(ts3plugin_onTextMessageEvent(1,2,0,8,name,"",text,0)==0); return queue.size();
    };
    assert(send("正常中文消息")==1);
    assert(send("包含测试禁词的消息")==0);
    assert(send("has BADword inside")==0);
    assert(send("正常内容","测试禁词用户")==0);
    cfg.names=false;assert(send("正常内容","测试禁词用户")==1);
    cfg.filter=false;assert(send("测试禁词")==1);
    cfg.filter=true;assert(send((std::string(600,'a')+"测试禁词").c_str())==0);
    saveLocked();cfg.filter=false;loadLocked();assert(cfg.filter);
    ts3plugin_onMenuItemEvent(1,PLUGIN_MENU_TYPE_GLOBAL,13,0);assert(!cfg.filter);
    loadLocked();assert(!cfg.filter);
    ts3plugin_onMenuItemEvent(1,PLUGIN_MENU_TYPE_GLOBAL,13,0);assert(cfg.filter);
    {std::ofstream f(std::filesystem::path(wordPath),std::ios::binary);f<<"newword\n";}
    ts3plugin_onMenuItemEvent(1,PLUGIN_MENU_TYPE_GLOBAL,15,0);
    assert(send("newWORD")==0);assert(send("测试禁词")==1);
    {std::ofstream f(std::filesystem::path(wordPath),std::ios::binary);f<<"\xff";}
    loadWordsLocked();assert(blockedWords.size()==1 && blockedWords[0]==L"newword");
    blockedWords.clear();assert(send("newword")==1);
    std::cout<<"PASS: dictionary UTF-8/BOM/comments, no overwrite, body/name filtering, case, full message, disabled bypass, empty list, persistence, toggle and reload, invalid UTF-8 fallback\n";
}
