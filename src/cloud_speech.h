#pragma once
std::wstring xmlText(const std::wstring& text) {
    std::wstring result;
    for(wchar_t ch:text){switch(ch){case L'&':result+=L"&amp;";break;case L'<':result+=L"&lt;";break;case L'>':result+=L"&gt;";break;case L'\"':result+=L"&quot;";break;case L'\'':result+=L"&apos;";break;default:if(ch>=32 || ch==L'\n' || ch==L'\r' || ch==L'\t')result+=ch;}}
    return result;
}
std::string cloudSSML(const Settings& c,const std::wstring& text) {
    int rate=std::clamp(c.rate*10,-90,100);
    auto ssml=L"<speak version='1.0' xmlns='http://www.w3.org/2001/10/synthesis' xml:lang='en-US'><voice name='"+xmlText(c.cloudVoice)+L"'><prosody rate='"+(rate>=0?std::wstring(L"+"):std::wstring{})+std::to_wstring(rate)+L"%' volume='"+std::to_wstring(c.volume-100)+L"%'>"+xmlText(text)+L"</prosody></voice></speak>";
    return utf8(ssml.c_str());
}
bool waveDuration(const std::vector<unsigned char>& audio,DWORD& duration) {
    if(audio.size()<44 || memcmp(audio.data(),"RIFF",4) || memcmp(audio.data()+8,"WAVE",4))return false;
    auto u32=[&](size_t offset){DWORD value=0;memcpy(&value,audio.data()+offset,4);return value;};
    bool format=false;DWORD data=0;
    for(size_t p=12;p+8<=audio.size();){DWORD size=u32(p+4);if(size>audio.size()-p-8)return false;
        if(!memcmp(audio.data()+p,"fmt ",4)){if(size<16)return false;WORD type=0,channels=0,bits=0;memcpy(&type,audio.data()+p+8,2);memcpy(&channels,audio.data()+p+10,2);memcpy(&bits,audio.data()+p+22,2);format=type==1 && channels==1 && u32(p+12)==24000 && u32(p+16)==48000 && bits==16;}
        if(!memcmp(audio.data()+p,"data",4))data=size;
        p+=8+size+(size%2);
    }
    if(!format || !data || data%2 || data>48000*180)return false;
    duration=static_cast<DWORD>((static_cast<unsigned long long>(data)*1000)/48000);return true;
}
struct InternetHandle {HINTERNET value=nullptr;~InternetHandle(){if(value)WinHttpCloseHandle(value);}operator HINTERNET()const{return value;}};
template<class Cancelled>
bool fetchCloudAudio(const Settings& c,const std::wstring& text,std::vector<unsigned char>& audio,std::wstring& error,Cancelled cancelled) {
    audio.clear();
    auto fail=[&](const wchar_t* zh,const wchar_t* en){error=englishUI.load()?en:zh;return false;};
    if(!validCloudSettings(c))return fail(L"在线语音未配置：请填写 Azure 区域、声音和密钥。",L"Configure Azure region, voice and key first.");
    if(cancelled())return false;
    InternetHandle session{WinHttpOpen(L"ChannelTTS/1.7",WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,WINHTTP_NO_PROXY_NAME,WINHTTP_NO_PROXY_BYPASS,0)};
    if(!session.value)return fail(L"无法初始化在线语音连接。",L"Cannot initialize online speech connection.");
    WinHttpSetTimeouts(session,3000,4000,4000,4000);
    auto host=c.region+L".tts.speech.microsoft.com";
    InternetHandle connection{WinHttpConnect(session,host.c_str(),INTERNET_DEFAULT_HTTPS_PORT,0)};
    InternetHandle request{connection.value?WinHttpOpenRequest(connection,L"POST",L"/cognitiveservices/v1",nullptr,WINHTTP_NO_REFERER,WINHTTP_DEFAULT_ACCEPT_TYPES,WINHTTP_FLAG_SECURE):nullptr};
    if(!request.value)return fail(L"无法创建在线语音请求。",L"Cannot create online speech request.");
    DWORD redirects=WINHTTP_OPTION_REDIRECT_POLICY_NEVER;WinHttpSetOption(request,WINHTTP_OPTION_REDIRECT_POLICY,&redirects,sizeof(redirects));
    auto headers=L"Content-Type: application/ssml+xml; charset=utf-8\r\nX-Microsoft-OutputFormat: riff-24khz-16bit-mono-pcm\r\nOcp-Apim-Subscription-Key: "+c.cloudKey+L"\r\n";
    auto body=cloudSSML(c,text);auto start=GetTickCount64();
    if(!WinHttpSendRequest(request,headers.c_str(),static_cast<DWORD>(headers.size()),body.data(),static_cast<DWORD>(body.size()),static_cast<DWORD>(body.size()),0) || !WinHttpReceiveResponse(request,nullptr))return fail(L"在线语音网络失败或超时，请检查网络和区域。",L"Online speech network failure or timeout. Check network and region.");
    DWORD status=0,size=sizeof(status);if(!WinHttpQueryHeaders(request,WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER,nullptr,&status,&size,nullptr))return fail(L"无法读取语音服务响应。",L"Cannot read speech service response.");
    if(status!=200){error=(englishUI.load()?L"Azure speech HTTP ":L"Azure 语音 HTTP ")+std::to_wstring(status)+(status==401||status==403?(englishUI.load()?L": check key and region.":L"：请检查密钥和区域。"):status==429?(englishUI.load()?L": quota or rate limit.":L"：额度或调用频率受限。"):(englishUI.load()?L": check voice/service settings.":L"：请检查声音或服务设置。"));return false;}
    while(!cancelled() && GetTickCount64()-start<30000){
        unsigned char buffer[8192];DWORD read=0;
        if(!WinHttpReadData(request,buffer,sizeof(buffer),&read))return fail(L"在线音频下载失败。",L"Online audio download failed.");
        if(!read){DWORD duration=0;if(waveDuration(audio,duration))return true;return fail(L"服务返回的音频格式无效。",L"Service returned invalid audio.");}
        if(audio.size()+read>8*1024*1024)return fail(L"在线音频超过长度限制。",L"Online audio exceeds size limit.");
        audio.insert(audio.end(),buffer,buffer+read);
    }
    return fail(L"在线语音已取消或超时。",L"Online speech cancelled or timed out.");
}
