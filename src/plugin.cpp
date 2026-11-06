#include <windows.h>
#include <sapi.h>
#include <algorithm>
#include <condition_variable>
#include <deque>
#include <future>
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
struct Settings { bool enabled=true, self=false, names=true; int rate=0, volume=80; } cfg;
struct Job { std::wstring text; Settings settings; unsigned long long generation; };
std::deque<Job> queue;
bool quitting=false;
unsigned long long generation=0;
std::wstring ini;
std::wstring wide(const char* s) {
    if(!s) return {};
    int n=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,s,-1,nullptr,0);
    if(n<=1) return {};
    std::wstring out(n,L'\0');
    MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,s,-1,out.data(),n);
    out.pop_back(); return out;
}
void clearLocked() { queue.clear(); ++generation; cv.notify_all(); }
void saveLocked() {
    auto put=[](const wchar_t* k,int v){ auto s=std::to_wstring(v); return WritePrivateProfileStringW(L"tts",k,s.c_str(),ini.c_str()); };
    bool ok=put(L"enabled",cfg.enabled)!=0;
    ok=(put(L"self",cfg.self)!=0)&&ok; ok=(put(L"names",cfg.names)!=0)&&ok;
    ok=(put(L"rate",cfg.rate)!=0)&&ok; ok=(put(L"volume",cfg.volume)!=0)&&ok;
    if(!ok) ts.logMessage("Could not save channel_tts.ini",LogLevel_WARNING,"Channel TTS",0);
}
void loadLocked() {
    auto get=[](const wchar_t* k,int d){ return static_cast<int>(GetPrivateProfileIntW(L"tts",k,d,ini.c_str())); };
    cfg.enabled=get(L"enabled",1)!=0; cfg.self=get(L"self",0)!=0; cfg.names=get(L"names",1)!=0;
    cfg.rate=std::clamp(get(L"rate",0),-10,10); cfg.volume=std::clamp(get(L"volume",80),0,100);
}
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
            if(FAILED(hr)) OutputDebugStringW(L"Channel TTS: speech failed\n");
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
API const char* ts3plugin_name(){return "Channel TTS";}
API const char* ts3plugin_version(){return "1.0.0";}
API int ts3plugin_apiVersion(){return 26;}
API const char* ts3plugin_author(){return "Channel TTS contributors";}
API const char* ts3plugin_description(){return "Read current channel chat locally using Windows SAPI. /tts help";}
API void ts3plugin_setFunctionPointers(const TS3Functions funcs){ts=funcs;}
API int ts3plugin_init(){
    try {
        char path[32768]{}; ts.getConfigPath(path,sizeof(path)); ini=wide(path);
        if(ini.empty()) return 1;
        if(ini.back()!=L'\\' && ini.back()!=L'/') ini+=L'\\';
        ini+=L"channel_tts.ini";
        {std::lock_guard<std::mutex> l(mu); quitting=false; clearLocked(); loadLocked();}
        std::promise<HRESULT> p; auto result=p.get_future(); worker=std::thread(speech,std::move(p));
        HRESULT hr=result.get();
        if(FAILED(hr)){worker.join(); ts.logMessage("Cannot initialize Windows SAPI voice",LogLevel_ERROR,"Channel TTS",0);return 1;}
        return 0;
    } catch(...) { if(worker.joinable()){ {std::lock_guard<std::mutex> l(mu); quitting=true; cv.notify_all();} worker.join(); } return 1; }
}
API void ts3plugin_shutdown(){
    {std::lock_guard<std::mutex> l(mu); quitting=true; clearLocked();}
    if(worker.joinable()) worker.join();
}
API const char* ts3plugin_commandKeyword(){return "tts";}
API int ts3plugin_processCommand(uint64 s,const char* command){
    try {
        std::istringstream in(command?command:""); std::string cmd,arg,extra; in>>cmd>>arg>>extra;
        std::lock_guard<std::mutex> l(mu);
        if(cmd=="status") {reply(s,std::string("Channel TTS: ")+(cfg.enabled?"on":"off")+", self="+std::to_string(cfg.self)+", names="+std::to_string(cfg.names)+", rate="+std::to_string(cfg.rate)+", volume="+std::to_string(cfg.volume));return 0;}
        if(cmd=="stop"){clearLocked();return 0;}
        if(cmd=="reload"){loadLocked();clearLocked();return 0;}
        if((cmd=="on"||cmd=="off") && arg.empty()) cfg.enabled=cmd=="on";
        else if((cmd=="self"||cmd=="names") && (arg=="on"||arg=="off") && extra.empty()) {if(cmd=="self")cfg.self=arg=="on";else cfg.names=arg=="on";}
        else if((cmd=="rate"||cmd=="volume") && !arg.empty() && extra.empty()) {
            size_t used=0; int v=std::stoi(arg,&used);
            if(used!=arg.size() || (cmd=="rate"?(v< -10||v>10):(v<0||v>100))) {reply(s,"Invalid value: rate -10..10, volume 0..100");return 0;}
            if(cmd=="rate")cfg.rate=v;else cfg.volume=v;
        } else {reply(s,"Channel TTS commands: /tts on|off|stop|status|reload; /tts self on|off; /tts names on|off; /tts rate -10..10; /tts volume 0..100");return 0;}
        clearLocked();saveLocked(); reply(s,"Channel TTS settings saved.");
    }catch(...){reply(s,"Channel TTS: invalid command or internal error.");}
    return 0;
}
API int ts3plugin_onTextMessageEvent(uint64 s,anyID mode,anyID /*to*/,anyID from,const char* name,const char*,const char* message,int ignored){
    try {
        if(ignored || mode!=TextMessageTarget_CHANNEL || s!=ts.getCurrentServerConnectionHandlerID())return 0;
        anyID me=0; if(ts.getClientID(s,&me)!=ERROR_ok)return 0;
        std::lock_guard<std::mutex> l(mu);
        if(quitting||!cfg.enabled||(!cfg.self && me==from))return 0;
        auto text=wide(message); if(text.empty())return 0;
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
