#include <windows.h>
#include <sapi.h>
#include <commctrl.h>
#pragma comment(lib,"comctl32.lib")
#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <deque>
#include <cstdlib>
#include <cstring>
#include <future>
#include <filesystem>
#include <fstream>
#include <vector>
#include <set>
#include "word_filter.h"
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include "ts3_functions.h"
#include "teamspeak/public_errors.h"
#define API extern "C" __declspec(dllexport)
namespace {
TS3Functions ts{};
std::mutex mu;
std::condition_variable cv;
std::thread worker;
struct Settings { bool enabled=true, self=false, names=true, filter=false; int rate=0, volume=80; } cfg;
struct Job { std::wstring text; Settings settings; unsigned long long generation; };
std::deque<Job> queue;
bool quitting=false;
unsigned long long generation=0;
std::wstring ini, wordPath;
std::vector<std::wstring> blockedWords;
std::string pluginID;
std::set<std::string> ignoredUsers;
std::wstring wide(const char* s) {
    if(!s) return {};
    int n=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,s,-1,nullptr,0);
    if(n<=1) return {};
    std::wstring out(n,L'\0');
    MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,s,-1,out.data(),n);
    out.pop_back(); return out;
}
#include "localization.h"
void loadWordsLocked() {
    std::ifstream file(std::filesystem::path(wordPath),std::ios::binary|std::ios::ate);
    if(!file || file.tellg()<0 || file.tellg()>1024*1024) {
        ts.logMessage(tr("无法读取违禁词库（文件需小于 1MB），保留原词库。"),LogLevel_WARNING,"Channel TTS",0); return;
    }
    const auto size=static_cast<size_t>(file.tellg()); file.seekg(0);
    std::string content(size,'\0');
    if(size && !file.read(content.data(),static_cast<std::streamsize>(size))) return;
    if(content.compare(0,3,"\xEF\xBB\xBF")==0) content.erase(0,3);
    if(content.find('\0')!=std::string::npos || (!content.empty() && wide(content.c_str()).empty())) {
        ts.logMessage(tr("违禁词库不是有效 UTF-8 文本，保留原词库。"),LogLevel_WARNING,"Channel TTS",0);return;
    }
    std::vector<std::wstring> words;
    std::istringstream lines(content); std::string line;
    while(std::getline(lines,line)) {
        auto word=wide(line.c_str());
        const auto first=word.find_first_not_of(L" \t\r");
        if(first==std::wstring::npos || word[first]==L'#') continue;
        word=word.substr(first,word.find_last_not_of(L" \t\r")-first+1);
        if(word.size()>128 || words.size()>=1000) {
            ts.logMessage(tr("词库超出限制（最多 1000 个词，每词 128 个 UTF-16 单元），保留原词库。"),LogLevel_WARNING,"Channel TTS",0);return;
        }
        words.push_back(std::move(word));
    }
    blockedWords=std::move(words);
}
void createWordsFile() {
    HANDLE file=CreateFileW(wordPath.c_str(),GENERIC_WRITE,FILE_SHARE_READ,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(file==INVALID_HANDLE_VALUE) return; // Never overwrite the user's dictionary.
    const char initial[]="\xEF\xBB\xBF# 违禁词库：UTF-8 编码，每行一个词；# 开头为注释。\r\n# 命中正文或正在朗读的昵称时跳过整条语音；英文不区分大小写。\r\n# 修改保存后，点击插件菜单中的“重新加载违禁词库”。\r\n傻逼\r\n操你妈\r\nfuck\r\n";
    DWORD written=0; WriteFile(file,initial,sizeof(initial)-1,&written,nullptr); CloseHandle(file);
}
void clearLocked() { queue.clear(); ++generation; cv.notify_all(); }
void loadLocked() {
    auto get=[](const wchar_t* k,int d){ return static_cast<int>(GetPrivateProfileIntW(L"tts",k,d,ini.c_str())); };
    cfg.enabled=get(L"enabled",1)!=0; cfg.self=get(L"self",0)!=0; cfg.names=get(L"names",1)!=0;
    cfg.filter=get(L"filter",0)!=0;
    cfg.rate=std::clamp(get(L"rate",0),-10,10); cfg.volume=std::clamp(get(L"volume",80),0,100);
}

std::string utf8(const wchar_t* value) {
    int n=WideCharToMultiByte(CP_UTF8,0,value,-1,nullptr,0,nullptr,nullptr);
    if(n<=1) return {};
    std::string text(n,'\0'); WideCharToMultiByte(CP_UTF8,0,value,-1,text.data(),n,nullptr,nullptr);
    text.pop_back();return text;
}
#include "words_dialog.h"
bool validUID(const std::string& uid) {
    return !uid.empty() && uid.size()<=128 && uid.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=_-")==std::string::npos;
}
void loadIgnoredLocked() {
    std::vector<wchar_t> buffer(65536);
    DWORD n=GetPrivateProfileSectionW(L"ignored_users",buffer.data(),static_cast<DWORD>(buffer.size()),ini.c_str());
    if(n>=buffer.size()-2) return;
    std::set<std::string> next;
    for(const wchar_t* row=buffer.data();*row;row+=wcslen(row)+1) {
        const wchar_t* equals=wcschr(row,L'=');if(!equals)continue;
        auto uid=utf8(equals+1);if(validUID(uid) && next.size()<256)next.insert(uid);
    }
    ignoredUsers=std::move(next);
}
bool saveIgnoredLocked() {
    std::wstring section;size_t i=0;
    for(const auto& uid:ignoredUsers) {
        section+=L"user"+std::to_wstring(i++)+L"="+wide(uid.c_str());section.push_back(L'\0');
    }
    section.push_back(L'\0');section.push_back(L'\0');
    return WritePrivateProfileSectionW(L"ignored_users",section.c_str(),ini.c_str())!=0;
}
#include "settings_store.h"
#include "settings_dialog.h"
void speech(std::promise<HRESULT> ready) {
    HRESULT hr=CoInitializeEx(nullptr,COINIT_MULTITHREADED);
    if(FAILED(hr)) { ready.set_value(hr); return; }
    ISpVoice* voice=nullptr;
    hr=CoCreateInstance(CLSID_SpVoice,nullptr,CLSCTX_INPROC_SERVER,IID_ISpVoice,reinterpret_cast<void**>(&voice));
    ready.set_value(hr);
    if(SUCCEEDED(hr)) {
        std::unique_lock<std::mutex> lock(mu);
        while(!quitting) {
            cv.wait(lock,[]{return quitting || !queue.empty();});
            if(quitting) break;
            Job job=std::move(queue.front()); queue.pop_front();
            lock.unlock();
            voice->SetRate(job.settings.rate); voice->SetVolume(static_cast<USHORT>(job.settings.volume));
            hr=voice->Speak(job.text.c_str(),SPF_ASYNC|SPF_IS_NOT_XML,nullptr);
            lock.lock();
            while(SUCCEEDED(hr) && !quitting && generation==job.generation) {
                lock.unlock(); hr=voice->WaitUntilDone(40); lock.lock();
                if(hr!=S_FALSE) break;
            }
            const bool cancel=quitting || generation!=job.generation;
            lock.unlock();
            if(cancel) voice->Speak(nullptr,SPF_ASYNC|SPF_PURGEBEFORESPEAK,nullptr);
            if(FAILED(hr)) OutputDebugStringW(wtr(L"频道文字朗读：语音播放失败\n"));
            lock.lock();
        }
        lock.unlock();
        voice->Speak(nullptr,SPF_ASYNC|SPF_PURGEBEFORESPEAK,nullptr);
        voice->Release();
    }
    CoUninitialize();
}
void clear() { std::lock_guard<std::mutex> l(mu); clearLocked(); }
void moved(uint64 s,anyID c) { anyID me=0; if(ts.getClientID(s,&me)==ERROR_ok && me==c) clear(); }
void reply(uint64 s,const std::string& text) { ts.printMessage(s,text.c_str(),PLUGIN_MESSAGE_TARGET_SERVER); }
}
API const char* ts3plugin_name(){return "频道文字朗读 (Channel TTS)";}
API const char* ts3plugin_version(){return "1.6.0";}
API int ts3plugin_apiVersion(){return 26;}
API const char* ts3plugin_author(){return "Channel TTS contributors";}
API const char* ts3plugin_description(){return tr("使用 Windows 系统语音朗读当前频道文字。请在顶部“插件”菜单中的“频道文字朗读”里调整设置。");}
API void ts3plugin_setFunctionPointers(const TS3Functions funcs){ts=funcs;}
API int ts3plugin_init(){
    try {
        char path[32768]{}; ts.getConfigPath(path,sizeof(path)); ini=wide(path);
        if(ini.empty()) return 1;
        if(ini.back()!=L'\\' && ini.back()!=L'/') ini+=L'\\';
        wordPath=ini+L"channel_tts_blocked_words.txt";
        ini+=L"channel_tts.ini"; createWordsFile();
        {std::lock_guard<std::mutex> l(mu); quitting=false; clearLocked(); loadLocked(); loadUILanguage(); loadWordsLocked(); loadIgnoredLocked();}
        std::promise<HRESULT> p; auto result=p.get_future(); worker=std::thread(speech,std::move(p));
        HRESULT hr=result.get();
        if(FAILED(hr)){worker.join(); ts.logMessage(tr("无法初始化 Windows SAPI 语音，请检查是否安装兼容语音。"),LogLevel_ERROR,"Channel TTS",0);return 1;}
        return 0;
    } catch(...) { if(worker.joinable()){ {std::lock_guard<std::mutex> l(mu); quitting=true; cv.notify_all();} worker.join(); } return 1; }
}
API void ts3plugin_shutdown(){
    if(settingsDialog) SendMessageW(settingsDialog,WM_CLOSE,0,0);
    if(wordsDialog) SendMessageW(wordsDialog,WM_CLOSE,0,0);
    {std::lock_guard<std::mutex> l(mu); quitting=true; clearLocked();}
    if(worker.joinable()) worker.join();
    pluginID.clear();
}
API int ts3plugin_offersConfigure(){return PLUGIN_OFFERS_CONFIGURE_QT_THREAD;}
API void ts3plugin_configure(void*,void*){openSettings();}
API const char* ts3plugin_commandKeyword(){return "tts";}
API int ts3plugin_processCommand(uint64 s,const char* command){
    try {
        std::istringstream in(command?command:"");std::string cmd,arg,extra;in>>cmd>>arg>>extra;
        auto draft=currentDraft();auto& c=draft.settings;
        if(cmd=="status") {
            reply(s,std::string(tr("频道文字朗读："))+(c.enabled?tr("已开启"):tr("已关闭"))+tr("；朗读自己：")+(c.self?tr("是"):tr("否"))+tr("；朗读昵称：")+(c.names?tr("是"):tr("否"))+tr("；语速：")+std::to_string(c.rate)+tr("；音量：")+std::to_string(c.volume)+tr("；违禁词过滤：")+(c.filter?tr("已开启"):tr("已关闭"))+tr("；词库数量：")+std::to_string(draft.words.size())+tr("；不朗读用户：")+std::to_string(draft.ignored.size()));return 0;
        }
        if(cmd=="stop"){clear();reply(s,tr("已停止朗读并清空队列，后续消息仍可朗读。"));return 0;}
        if(cmd=="reload"){
            {std::lock_guard<std::mutex> lock(mu);loadLocked();loadUILanguage();loadIgnoredLocked();loadWordsLocked();clearLocked();}
            reply(s,tr("已重新读取配置并清空朗读队列。"));return 0;
        }
        if((cmd=="on"||cmd=="off") && arg.empty())c.enabled=cmd=="on";
        else if((cmd=="self"||cmd=="names") && (arg=="on"||arg=="off") && extra.empty()){if(cmd=="self")c.self=arg=="on";else c.names=arg=="on";}
        else if((cmd=="rate"||cmd=="volume") && !arg.empty() && extra.empty()){
            size_t used=0;int value=std::stoi(arg,&used);
            if(used!=arg.size() || (cmd=="rate"?(value < -10 || value>10):(value<0 || value>100))){reply(s,tr("数值超出范围：语速为 -10 至 10，音量为 0 至 100。"));return 0;}
            if(cmd=="rate")c.rate=value;else c.volume=value;
        }else{
            reply(s,englishUI.load()?"Open Plugins > Channel TTS > Settings for all options. Optional commands: /tts on, off, stop, status, reload, self on|off, names on|off, rate -10..10, volume 0..100.":"在 插件 → 频道文字朗读 → 设置 中调整全部选项。可选命令：/tts on、off、stop、status、reload、self on|off、names on|off、rate -10..10、volume 0..100。");return 0;
        }
        std::wstring error;
        if(commitDraft(draft,error))reply(s,englishUI.load()?"Saved and applied.":"已保存并应用。");else reply(s,utf8(error.c_str()));
    }catch(...){reply(s,tr("频道文字朗读：命令参数无效或发生内部错误，输入 /tts help 查看帮助。"));}
    return 0;
}
API int ts3plugin_onTextMessageEvent(uint64 s,anyID mode,anyID /*to*/,anyID from,const char* name,const char* fromUID,const char* message,int ignored){
    try {
        if(ignored || mode!=TextMessageTarget_CHANNEL || s!=ts.getCurrentServerConnectionHandlerID())return 0;
        anyID me=0; if(ts.getClientID(s,&me)!=ERROR_ok)return 0;
        std::lock_guard<std::mutex> l(mu);
        if(quitting||!cfg.enabled||(!cfg.self && me==from))return 0;
        if(fromUID && ignoredUsers.count(fromUID))return 0;
        auto text=wide(message); if(text.empty())return 0;
        if(cfg.filter && (word_filter::matches(text,blockedWords) || (cfg.names && word_filter::matches(wide(name),blockedWords))))return 0;
        // Limit UTF-16 length without leaving a dangling high surrogate.
        if(text.size()>500) {text.resize(500);if(text.back()>=0xD800 && text.back()<=0xDBFF)text.pop_back();}
        if(cfg.names){auto n=wide(name);if(n.size()>80)n.resize(80);text=n+L": "+text;}
        if(queue.size()>=20)queue.pop_front();
        queue.push_back({std::move(text),cfg,generation});cv.notify_one();
    }catch(...){ /* Never propagate exceptions into the host. */ }
    return 0; // Preserve normal chat display.
}
API void ts3plugin_currentServerConnectionChanged(uint64){clear();}
API void ts3plugin_onConnectStatusChangeEvent(uint64,int,unsigned int){clear();}
API void ts3plugin_onClientMoveEvent(uint64 s,anyID c,uint64,uint64,int,const char*){moved(s,c);}
API void ts3plugin_onClientMoveMovedEvent(uint64 s,anyID c,uint64,uint64,int,anyID,const char*,const char*,const char*){moved(s,c);}
API void ts3plugin_onClientKickFromChannelEvent(uint64 s,anyID c,uint64,uint64,int,anyID,const char*,const char*,const char*){moved(s,c);}
API void ts3plugin_onClientKickFromServerEvent(uint64 s,anyID c,uint64,uint64,int,anyID,const char*,const char*,const char*){moved(s,c);}



// Global menu entries are displayed by TS3 under this plugin's submenu.
API void ts3plugin_registerPluginID(const char* id) {
    try { pluginID=id?id:""; } catch(...) { pluginID.clear(); }
}
API void ts3plugin_freeMemory(void* data) { std::free(data); }
API void ts3plugin_initMenus(PluginMenuItem*** items,char** icon) {
    *items=nullptr; *icon=nullptr;
    loadUILanguage();
    const char* labels[]={
        "设置 / Settings…", "朗读：开启 / 关闭 | Speech on/off", "停止朗读 / Stop speech",
        tr("不朗读此用户的文字消息"), tr("恢复朗读此用户的文字消息")
    };
    const int ids[]={17,18,3,101,102};
    constexpr size_t count=sizeof(labels)/sizeof(labels[0]);
    auto list=static_cast<PluginMenuItem**>(std::calloc(count+1,sizeof(PluginMenuItem*)));
    if(!list) return;
    for(size_t i=0;i<count;++i) {
        list[i]=static_cast<PluginMenuItem*>(std::calloc(1,sizeof(PluginMenuItem)));
        if(!list[i]) {
            for(size_t j=0;j<i;++j) std::free(list[j]);
            std::free(list); return;
        }
        list[i]->type=i<3?PLUGIN_MENU_TYPE_GLOBAL:PLUGIN_MENU_TYPE_CLIENT;
        list[i]->id=ids[i];
        strcpy_s(list[i]->text,PLUGIN_MENU_BUFSZ,labels[i]);
    }
    *items=list;
}
API void ts3plugin_onMenuItemEvent(uint64 s,PluginMenuType type,int id,uint64 selected) {
    
    if(type==PLUGIN_MENU_TYPE_CLIENT && id>=101 && id<=103) {
        try {
            if(selected==0 || selected>65535)return;
            char* allocated=nullptr;
            if(ts.getClientVariableAsString(s,static_cast<anyID>(selected),CLIENT_UNIQUE_IDENTIFIER,&allocated)!=ERROR_ok || !allocated) {
                reply(s,tr("无法获取该用户身份，请在其在线时重试。"));return;
            }
            std::string uid;
            try {uid=allocated;} catch(...) {ts.freeMemory(allocated);throw;}
            ts.freeMemory(allocated);
            if(!validUID(uid)){reply(s,tr("该用户身份无效，未修改名单。"));return;}
            std::lock_guard<std::mutex> l(mu);
            if(quitting)return;
            const bool existed=ignoredUsers.count(uid)!=0;
            if(id==103){reply(s,existed?tr("此用户在不朗读名单中。"):tr("此用户的消息允许朗读（仍受其他开关和过滤设置影响）。"));return;}
            if(id==101 && !existed && ignoredUsers.size()>=256){reply(s,tr("不朗读名单已满（最多 256 个用户）。"));return;}
            if(id==101)ignoredUsers.insert(uid);else ignoredUsers.erase(uid);
            if(!saveIgnoredLocked()) {
                if(existed)ignoredUsers.insert(uid);else ignoredUsers.erase(uid);
                reply(s,tr("名单保存失败，修改已撤回，请检查配置目录是否可写。"));return;
            }
            clearLocked();
            reply(s,id==101?tr("已保存：不再朗读此用户的文字消息。"):tr("已保存：恢复朗读此用户的文字消息。"));
        } catch(...) {reply(s,tr("无法修改不朗读名单，请重试。"));}
        return;
    }
    if(type!=PLUGIN_MENU_TYPE_GLOBAL || id<1 || id>18) return;
    if(id==16) { openSettings(2); return; }
    if(id==17) { openSettings(); return; }
    if(id==14) { openSettings(1); return; }
    try {
        if(id==3){clear();reply(s,tr("已停止朗读并清空队列。"));return;}
        if(id==10){ts3plugin_processCommand(s,"status");return;}
        if(id==11){ts3plugin_processCommand(s,"reload");return;}
        if(id==15){std::lock_guard<std::mutex> lock(mu);loadWordsLocked();clearLocked();return;}
        auto draft=currentDraft();auto& c=draft.settings;
        switch(id){
            case 18:c.enabled=!c.enabled;break;
            case 1:c.enabled=true;break;case 2:c.enabled=false;break;
            case 4:c.self=!c.self;break;case 5:c.names=!c.names;break;
            case 6:c.rate=std::min(10,c.rate+1);break;case 7:c.rate=std::max(-10,c.rate-1);break;
            case 8:c.volume=std::min(100,c.volume+10);break;case 9:c.volume=std::max(0,c.volume-10);break;
            case 12:c=Settings{};break;case 13:c.filter=!c.filter;break;
            default:return;
        }
        std::wstring error;
        if(!commitDraft(draft,error)){reply(s,utf8(error.c_str()));return;}
        ts3plugin_processCommand(s,"status");
    }catch(...){reply(s,tr("频道文字朗读：无法应用菜单设置，请查看客户端日志。"));}
}
