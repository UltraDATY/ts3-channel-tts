#pragma once
bool cloudIdentifier(const std::wstring& value,size_t maximum) {
    return !value.empty() && value.size()<=maximum && value.find_first_not_of(L"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-")==std::wstring::npos;
}
bool validCloudSettings(const Settings& c) {
    return cloudIdentifier(c.region,48) && cloudIdentifier(c.cloudVoice,128) && c.cloudKey.size()>=16 && c.cloudKey.size()<=256 && c.cloudKey.find_first_not_of(L"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789+/=_-")==std::wstring::npos;
}
bool protectCloudKey(const std::wstring& key,std::wstring& encoded) {
    encoded.clear();if(key.empty())return true;
    DATA_BLOB input{static_cast<DWORD>(key.size()*sizeof(wchar_t)),reinterpret_cast<BYTE*>(const_cast<wchar_t*>(key.data()))},output{};
    if(!CryptProtectData(&input,L"ChannelTTS Azure key",nullptr,nullptr,nullptr,CRYPTPROTECT_UI_FORBIDDEN,&output))return false;
    const wchar_t hex[]=L"0123456789abcdef";
    for(DWORD i=0;i<output.cbData;i++){encoded+=hex[output.pbData[i]>>4];encoded+=hex[output.pbData[i]&15];}
    SecureZeroMemory(output.pbData,output.cbData);LocalFree(output.pbData);return true;
}
std::wstring unprotectCloudKey(const std::wstring& encoded) {
    if(encoded.empty() || encoded.size()%2 || encoded.size()>8192)return {};
    std::vector<BYTE> bytes;
    auto digit=[](wchar_t ch)->int{if(ch>=L'0' && ch<=L'9')return ch-L'0';if(ch>=L'a' && ch<=L'f')return ch-L'a'+10;return -1;};
    for(size_t i=0;i<encoded.size();i+=2){int a=digit(encoded[i]),b=digit(encoded[i+1]);if(a<0 || b<0)return {};bytes.push_back(static_cast<BYTE>(a*16+b));}
    DATA_BLOB input{static_cast<DWORD>(bytes.size()),bytes.data()},output{};
    if(!CryptUnprotectData(&input,nullptr,nullptr,nullptr,nullptr,CRYPTPROTECT_UI_FORBIDDEN,&output))return {};
    std::wstring key;
    if(output.cbData%sizeof(wchar_t)==0 && output.cbData<=512)key.assign(reinterpret_cast<wchar_t*>(output.pbData),output.cbData/sizeof(wchar_t));
    SecureZeroMemory(output.pbData,output.cbData);LocalFree(output.pbData);return key;
}
void loadCloudSettings(Settings& c) {
    wchar_t buffer[8193]{};
    c.online=GetPrivateProfileIntW(L"cloud",L"enabled",0,ini.c_str())!=0;
    GetPrivateProfileStringW(L"cloud",L"region",L"eastasia",buffer,8193,ini.c_str());c.region=buffer;
    GetPrivateProfileStringW(L"cloud",L"voice",L"en-US-EmmaMultilingualNeural",buffer,8193,ini.c_str());c.cloudVoice=buffer;
    GetPrivateProfileStringW(L"cloud",L"key_dpapi",L"",buffer,8193,ini.c_str());c.cloudKey=unprotectCloudKey(buffer);
}
