#pragma once
// All translations are immutable; UI language is independent of speech settings.
std::atomic<bool> englishUI{false};
struct TextPair { const char* zh; const char* en; const wchar_t* wzh; const wchar_t* wen; };
const TextPair translatedText[]={
    {"无法读取违禁词库（文件需小于 1MB），保留原词库。","Cannot read word list (maximum 1 MB). Keeping the previous list.",L"无法读取违禁词库（文件需小于 1MB），保留原词库。",L"Cannot read word list (maximum 1 MB). Keeping the previous list."},
    {"违禁词库不是有效 UTF-8 文本，保留原词库。","Word list is not valid UTF-8. Keeping the previous list.",L"违禁词库不是有效 UTF-8 文本，保留原词库。",L"Word list is not valid UTF-8. Keeping the previous list."},
    {"词库超出限制（最多 1000 个词，每词 128 个 UTF-16 单元），保留原词库。","Word list exceeds 1000 entries or 128 UTF-16 units per entry. Keeping the previous list.",L"词库超出限制（最多 1000 个词，每词 128 个 UTF-16 单元），保留原词库。",L"Word list exceeds 1000 entries or 128 UTF-16 units per entry. Keeping the previous list."},
    {"无法保存 channel_tts.ini 配置文件","Cannot save channel_tts.ini settings.",L"无法保存 channel_tts.ini 配置文件",L"Cannot save channel_tts.ini settings."},
    {"频道文字朗读：语音播放失败\n","Channel TTS: speech playback failed.\n",L"频道文字朗读：语音播放失败\n",L"Channel TTS: speech playback failed.\n"},
    {"使用 Windows 系统语音朗读当前频道文字。请在顶部“插件”菜单中的“频道文字朗读”里调整设置。","Reads current-channel text using Windows speech. Open Plugins > Channel TTS for settings.",L"使用 Windows 系统语音朗读当前频道文字。请在顶部“插件”菜单中的“频道文字朗读”里调整设置。",L"Reads current-channel text using Windows speech. Open Plugins > Channel TTS for settings."},
    {"无法初始化 Windows SAPI 语音，请检查是否安装兼容语音。","Cannot initialize Windows SAPI. Please install a compatible desktop voice.",L"无法初始化 Windows SAPI 语音，请检查是否安装兼容语音。",L"Cannot initialize Windows SAPI. Please install a compatible desktop voice."},
    {"频道文字朗读：","Channel TTS: ",L"频道文字朗读：",L"Channel TTS: "},
    {"已开启","On",L"已开启",L"On"},
    {"已关闭","Off",L"已关闭",L"Off"},
    {"；朗读自己：","; Read own messages: ",L"；朗读自己：",L"; Read own messages: "},
    {"是","Yes",L"是",L"Yes"},
    {"否","No",L"否",L"No"},
    {"；朗读昵称：","; Read sender names: ",L"；朗读昵称：",L"; Read sender names: "},
    {"；语速：","; Rate: ",L"；语速：",L"; Rate: "},
    {"；音量：","; Volume: ",L"；音量：",L"; Volume: "},
    {"；违禁词过滤：","; Word filter: ",L"；违禁词过滤：",L"; Word filter: "},
    {"；词库数量：","; Word count: ",L"；词库数量：",L"; Word count: "},
    {"；不朗读用户：","; Ignored users: ",L"；不朗读用户：",L"; Ignored users: "},
    {"已停止朗读并清空队列，后续消息仍可朗读。","Stopped speech and cleared the queue. New messages will still be read.",L"已停止朗读并清空队列，后续消息仍可朗读。",L"Stopped speech and cleared the queue. New messages will still be read."},
    {"已重新读取配置并清空朗读队列。","Settings reloaded and speech queue cleared.",L"已重新读取配置并清空朗读队列。",L"Settings reloaded and speech queue cleared."},
    {"数值超出范围：语速为 -10 至 10，音量为 0 至 100。","Out of range: rate must be -10 to 10, volume 0 to 100.",L"数值超出范围：语速为 -10 至 10，音量为 0 至 100。",L"Out of range: rate must be -10 to 10, volume 0 to 100."},
    {"频道文字朗读帮助（on=开启，off=关闭）：\n/tts on 或 /tts off：开启或关闭朗读\n/tts stop：停止当前朗读并清空队列\n/tts status：查看当前设置\n/tts reload：重新读取配置文件\n/tts self on 或 off：是否朗读自己的消息\n/tts names on 或 off：是否朗读昵称\n/tts rate 数值：设置语速（-10 至 10）\n/tts volume 数值：设置音量（0 至 100）","Channel TTS commands:\n/tts on or /tts off: enable or disable speech\n/tts stop: stop speech and clear the queue\n/tts status: show settings\n/tts reload: reload settings\n/tts self on or off: read own messages\n/tts names on or off: read sender names\n/tts rate number: rate (-10 to 10)\n/tts volume number: volume (0 to 100)",L"频道文字朗读帮助（on=开启，off=关闭）：\n/tts on 或 /tts off：开启或关闭朗读\n/tts stop：停止当前朗读并清空队列\n/tts status：查看当前设置\n/tts reload：重新读取配置文件\n/tts self on 或 off：是否朗读自己的消息\n/tts names on 或 off：是否朗读昵称\n/tts rate 数值：设置语速（-10 至 10）\n/tts volume 数值：设置音量（0 至 100）",L"Channel TTS commands:\n/tts on or /tts off: enable or disable speech\n/tts stop: stop speech and clear the queue\n/tts status: show settings\n/tts reload: reload settings\n/tts self on or off: read own messages\n/tts names on or off: read sender names\n/tts rate number: rate (-10 to 10)\n/tts volume number: volume (0 to 100)"},
    {"设置已更新，并已尝试保存到配置文件。","Settings updated. Saving to the configuration file was attempted.",L"设置已更新，并已尝试保存到配置文件。",L"Settings updated. Saving to the configuration file was attempted."},
    {"频道文字朗读：命令参数无效或发生内部错误，输入 /tts help 查看帮助。","Channel TTS: invalid argument or internal error. Use /tts help for help.",L"频道文字朗读：命令参数无效或发生内部错误，输入 /tts help 查看帮助。",L"Channel TTS: invalid argument or internal error. Use /tts help for help."},
    {"开启朗读","Enable speech",L"开启朗读",L"Enable speech"},
    {"关闭朗读","Disable speech",L"关闭朗读",L"Disable speech"},
    {"停止当前朗读并清空队列","Stop speech and clear queue",L"停止当前朗读并清空队列",L"Stop speech and clear queue"},
    {"朗读自己的消息：开启 / 关闭","Read own messages: toggle",L"朗读自己的消息：开启 / 关闭",L"Read own messages: toggle"},
    {"朗读发送者昵称：开启 / 关闭","Read sender names: toggle",L"朗读发送者昵称：开启 / 关闭",L"Read sender names: toggle"},
    {"语速加快（+1）","Increase rate (+1)",L"语速加快（+1）",L"Increase rate (+1)"},
    {"语速减慢（-1）","Decrease rate (-1)",L"语速减慢（-1）",L"Decrease rate (-1)"},
    {"音量增加（+10）","Increase volume (+10)",L"音量增加（+10）",L"Increase volume (+10)"},
    {"音量减小（-10）","Decrease volume (-10)",L"音量减小（-10）",L"Decrease volume (-10)"},
    {"查看当前设置","Show current settings",L"查看当前设置",L"Show current settings"},
    {"重新读取配置文件","Reload settings",L"重新读取配置文件",L"Reload settings"},
    {"恢复默认设置","Restore defaults",L"恢复默认设置",L"Restore defaults"},
    {"违禁词过滤：开启 / 关闭","Word filter: toggle",L"违禁词过滤：开启 / 关闭",L"Word filter: toggle"},
    {"违禁词设置…","Word filter settings...",L"违禁词设置…",L"Word filter settings..."},
    {"重新加载违禁词库","Reload word list",L"重新加载违禁词库",L"Reload word list"},
    {"查看不朗读名单","Show ignored users",L"查看不朗读名单",L"Show ignored users"},
    {"不朗读此用户的文字消息","Ignore this user for speech",L"不朗读此用户的文字消息",L"Ignore this user for speech"},
    {"恢复朗读此用户的文字消息","Allow speech for this user",L"恢复朗读此用户的文字消息",L"Allow speech for this user"},
    {"查看此用户的朗读状态","Show speech status for this user",L"查看此用户的朗读状态",L"Show speech status for this user"},
    {"无法获取该用户身份，请在其在线时重试。","Cannot get user identity. Try again while the user is online.",L"无法获取该用户身份，请在其在线时重试。",L"Cannot get user identity. Try again while the user is online."},
    {"该用户身份无效，未修改名单。","Invalid user identity. List unchanged.",L"该用户身份无效，未修改名单。",L"Invalid user identity. List unchanged."},
    {"此用户在不朗读名单中。","This user is ignored for speech.",L"此用户在不朗读名单中。",L"This user is ignored for speech."},
    {"此用户的消息允许朗读（仍受其他开关和过滤设置影响）。","Speech is allowed for this user, subject to other settings and filters.",L"此用户的消息允许朗读（仍受其他开关和过滤设置影响）。",L"Speech is allowed for this user, subject to other settings and filters."},
    {"不朗读名单已满（最多 256 个用户）。","Ignored user list is full (maximum 256).",L"不朗读名单已满（最多 256 个用户）。",L"Ignored user list is full (maximum 256)."},
    {"名单保存失败，修改已撤回，请检查配置目录是否可写。","Could not save the list. Change reverted. Check configuration folder permissions.",L"名单保存失败，修改已撤回，请检查配置目录是否可写。",L"Could not save the list. Change reverted. Check configuration folder permissions."},
    {"已保存：不再朗读此用户的文字消息。","Saved: text messages from this user will be skipped.",L"已保存：不再朗读此用户的文字消息。",L"Saved: text messages from this user will be skipped."},
    {"已保存：恢复朗读此用户的文字消息。","Saved: text messages from this user may be read again.",L"已保存：恢复朗读此用户的文字消息。",L"Saved: text messages from this user may be read again."},
    {"无法修改不朗读名单，请重试。","Cannot update ignored users. Please try again.",L"无法修改不朗读名单，请重试。",L"Cannot update ignored users. Please try again."},
    {"不朗读名单（用户唯一身份 UID）：","Ignored users (unique UID): ",L"不朗读名单（用户唯一身份 UID）：",L"Ignored users (unique UID): "},
    {" 人"," users",L" 人",L" users"},
    {"已停止朗读并清空队列。","Speech stopped and queue cleared.",L"已停止朗读并清空队列。",L"Speech stopped and queue cleared."},
    {"频道文字朗读：无法应用菜单设置，请查看客户端日志。","Channel TTS: cannot apply menu settings. See the client log.",L"频道文字朗读：无法应用菜单设置，请查看客户端日志。",L"Channel TTS: cannot apply menu settings. See the client log."},
    {"每个词最多 128 个字符，请缩短后再保存。","Each entry allows 128 UTF-16 units. Shorten it and try again.",L"每个词最多 128 个字符，请缩短后再保存。",L"Each entry allows 128 UTF-16 units. Shorten it and try again."},
    {"最多保存 1000 个词，请删减后再保存。","Maximum 1000 entries. Remove some entries and try again.",L"最多保存 1000 个词，请删减后再保存。",L"Maximum 1000 entries. Remove some entries and try again."},
    {"无法创建词库文件，请检查配置目录是否可写。","Cannot create the word list file. Check configuration folder permissions.",L"无法创建词库文件，请检查配置目录是否可写。",L"Cannot create the word list file. Check configuration folder permissions."},
    {"词库保存失败，原词库未修改。请检查文件是否被占用或目录是否可写。","Could not save the word list. Previous list unchanged. Check file locks and folder permissions.",L"词库保存失败，原词库未修改。请检查文件是否被占用或目录是否可写。",L"Could not save the word list. Previous list unchanged. Check file locks and folder permissions."},
    {"词库已保存并生效，但过滤开关保存失败，请检查配置文件权限后重试。","Word list saved and applied, but the filter switch could not be saved. Check settings permissions.",L"词库已保存并生效，但过滤开关保存失败，请检查配置文件权限后重试。",L"Word list saved and applied, but the filter switch could not be saved. Check settings permissions."},
    {"启用违禁词过滤","Enable word filter",L"启用违禁词过滤",L"Enable word filter"},
    {"每行填写一个词；匹配后跳过整条朗读。英文不区分大小写。","Enter one word per line. Matching messages are skipped. English matching ignores case.",L"每行填写一个词；匹配后跳过整条朗读。英文不区分大小写。",L"Enter one word per line. Matching messages are skipped. English matching ignores case."},
    {"最多 1000 个词，每词最多 128 个字符。保存后立即生效。","Maximum 1000 entries, 128 UTF-16 units each. Saving applies changes immediately.",L"最多 1000 个词，每词最多 128 个字符。保存后立即生效。",L"Maximum 1000 entries, 128 UTF-16 units each. Saving applies changes immediately."},
    {"保存并应用","Save and apply",L"保存并应用",L"Save and apply"},
    {"取消","Cancel",L"取消",L"Cancel"},
    {"无法应用设置，请重试。原有设置仍然保留。","Cannot apply settings. Previous settings are retained. Please try again.",L"无法应用设置，请重试。原有设置仍然保留。",L"Cannot apply settings. Previous settings are retained. Please try again."},
    {"频道文字朗读 — 违禁词设置","Channel TTS - Word filter settings",L"频道文字朗读 — 违禁词设置",L"Channel TTS - Word filter settings"},
    {"无法打开违禁词设置窗口。","Cannot open the word filter settings window.",L"无法打开违禁词设置窗口。",L"Cannot open the word filter settings window."},
};
const char* tr(const char* text) {
    if(englishUI.load())for(const auto& pair:translatedText)if(strcmp(text,pair.zh)==0)return pair.en;
    return text;
}
const wchar_t* wtr(const wchar_t* text) {
    if(englishUI.load())for(const auto& pair:translatedText)if(wcscmp(text,pair.wzh)==0)return pair.wen;
    return text;
}
void loadUILanguage() {
    std::wstring path=ini;
    if(path.empty() && ts.getConfigPath) {
        char configPath[32768]{};ts.getConfigPath(configPath,sizeof(configPath));path=wide(configPath);
        if(!path.empty() && path.back()!=L'\\' && path.back()!=L'/')path+=L'\\';
        if(!path.empty())path+=L"channel_tts.ini";
    }
    wchar_t value[32]{};
    if(!path.empty())GetPrivateProfileStringW(L"ui",L"language",L"zh-CN",value,32,path.c_str());
    englishUI.store(_wcsicmp(value,L"en")==0 || _wcsicmp(value,L"en-US")==0);
}
