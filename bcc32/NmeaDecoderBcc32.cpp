// Win32 DLL implementation for Embarcadero C++Builder 2009 (BCC32).
//
// This translation unit intentionally uses only C++03 facilities.  Do not add
// C++11 syntax here: BCC32 6.11 is the compiler supplied with C++Builder 2009.

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cstddef>
#include <map>
#include <string>

#define __DHF_RELEASE_IFACE_MODULE_EXT_H__
#include "../DHF/_example/include/IfaceModuleExt.h"

namespace {

const char kInterfaceVersion[] = "0.0";
const char kReleaseVersion[] = "1.0";
const char kShortName[] = "NMEA";
const char kLongName[] = "NMEA/AIS decoder (0183/61162-450)";
const char kGroupName[] = "PineCode Lab";

bool IsValidNmea(const std::string& message)
{
    const std::string::size_type star = message.find('*');
    if (message.size() < 6 || (message[0] != '$' && message[0] != '!') ||
        star == std::string::npos || star + 3 > message.size()) {
        return false;
    }

    unsigned char checksum = 0;
    std::string::size_type i;
    for (i = 1; i < star; ++i) {
        checksum ^= static_cast<unsigned char>(message[i]);
    }

    char checksumText[3];
    checksumText[0] = message[star + 1];
    checksumText[1] = message[star + 2];
    checksumText[2] = '\0';
    char* end = NULL;
    const unsigned long expected = std::strtoul(checksumText, &end, 16);
    return end == checksumText + 2 && expected <= 0xffUL &&
           checksum == static_cast<unsigned char>(expected);
}

int ParsePositiveInt(const std::string& value)
{
    char* end = NULL;
    const long parsed = std::strtol(value.c_str(), &end, 10);
    return end != value.c_str() && *end == '\0' && parsed > 0 ?
        static_cast<int>(parsed) : 0;
}

struct NmeaGroup {
    int totalLines;
    unsigned long updatedAt;
    std::map<int, std::string> lines;

    NmeaGroup() : totalLines(0), updatedAt(0) {}
};

} // namespace

class NmeaDecoderBcc32 : public IModuleExt
{
public:
    NmeaDecoderBcc32();
    virtual ~NmeaDecoderBcc32();

    virtual bool __stdcall Constructor(unsigned __int64 uid);
    virtual void __stdcall Destructor();
    virtual bool __stdcall CreateGui(IfaceCallBackGui* ig);
    virtual bool __stdcall CreateProc(IfaceCallBackProc* ip);
    virtual bool __stdcall Initialize();
    virtual void __stdcall Free();
    virtual void __stdcall CreateForm();
    virtual void __stdcall DestroyForm();
    virtual void __stdcall ShowForm(bool show = true);
    virtual void __stdcall setGuiModuleSettings(SAppModuleSettings* sets);
    virtual void __stdcall setGuiModuleOptions(SAppModuleOptions* opts);
    virtual void __stdcall setProcModuleSettings(SAppModuleSettings* sets);
    virtual void __stdcall setProcModuleOptions(SAppModuleOptions* opts);
    virtual SAppModuleSettings* __stdcall getModuleParamsSetting(SAppModuleSettings* sets = NULL);
    virtual SAppModuleOptions* __stdcall getModuleParamsOptions(SAppModuleOptions* opts = NULL);
    virtual void __stdcall getDefaultModuleParams(SAppModuleSettings* sets, SAppModuleOptions* opts);
    virtual void __stdcall setProcState(bool state);
    virtual bool __stdcall procGuiData(unsigned short type, void* data, int len);
    virtual bool __stdcall HookMessage(int wParam, int lParam);
    virtual bool __stdcall workProc(bool rSleep);
    virtual bool __stdcall workData(char* idsData, int idsDataLen, char* data, int len);
    virtual void __stdcall timing();
    virtual bool __stdcall recvGuiData(unsigned short type, char* data, int len);
    virtual void __stdcall updateStatistic();

private:
    void ProcessRawBytes(const char* data, int len);
    void Process450Packet(const char* data, int len);
    void ProcessMessage(const std::string& message);
    bool SendMessage(const std::string& message);
    void CleanupGroups();

    std::string lineBuffer;
    std::map<std::string, NmeaGroup> groups;
    unsigned __int64 validMessages;
    unsigned __int64 rejectedMessages;
};

NmeaDecoderBcc32::NmeaDecoderBcc32() : validMessages(0), rejectedMessages(0)
{
    std::memset(aShortName, 0, sizeof(aShortName));
    std::memset(aLongName, 0, sizeof(aLongName));
    std::memset(aGroupName, 0, sizeof(aGroupName));
    std::strncpy(aShortName, kShortName, sizeof(aShortName) - 1);
    std::strncpy(aLongName, kLongName, sizeof(aLongName) - 1);
    std::strncpy(aGroupName, kGroupName, sizeof(aGroupName) - 1);
    std::sprintf(aVersion, "%s.%s", kInterfaceVersion, kReleaseVersion);
    bIsInputModule = false;
    byteTypeData = true;
    typeData = '\n';
}

NmeaDecoderBcc32::~NmeaDecoderBcc32() { Free(); }
bool __stdcall NmeaDecoderBcc32::Constructor(unsigned __int64 uid) { moduleUid = static_cast<unsigned short>(uid); return true; }
void __stdcall NmeaDecoderBcc32::Destructor() { Free(); }
bool __stdcall NmeaDecoderBcc32::CreateGui(IfaceCallBackGui* ig) { ifaceGui = ig; return true; }
bool __stdcall NmeaDecoderBcc32::CreateProc(IfaceCallBackProc* ip) { ifaceProc = ip; return ip != NULL; }
bool __stdcall NmeaDecoderBcc32::Initialize() { recv = send = validMessages = rejectedMessages = 0; Free(); return true; }
void __stdcall NmeaDecoderBcc32::Free() { lineBuffer.erase(); groups.clear(); }
void __stdcall NmeaDecoderBcc32::CreateForm() {}
void __stdcall NmeaDecoderBcc32::DestroyForm() {}
void __stdcall NmeaDecoderBcc32::ShowForm(bool show) { (void)show; }
void __stdcall NmeaDecoderBcc32::setGuiModuleSettings(SAppModuleSettings* sets) { (void)sets; }
void __stdcall NmeaDecoderBcc32::setGuiModuleOptions(SAppModuleOptions* opts) { (void)opts; }
void __stdcall NmeaDecoderBcc32::setProcModuleSettings(SAppModuleSettings* sets) { (void)sets; }
void __stdcall NmeaDecoderBcc32::setProcModuleOptions(SAppModuleOptions* opts) { (void)opts; }
void __stdcall NmeaDecoderBcc32::setProcState(bool state) { IModuleExt::setProcState(state); }
bool __stdcall NmeaDecoderBcc32::procGuiData(unsigned short type, void* data, int len) { (void)type; (void)data; (void)len; return true; }
bool __stdcall NmeaDecoderBcc32::HookMessage(int wParam, int lParam) { (void)wParam; (void)lParam; return true; }
bool __stdcall NmeaDecoderBcc32::workProc(bool rSleep) { (void)rSleep; return true; }

SAppModuleSettings* __stdcall NmeaDecoderBcc32::getModuleParamsSetting(SAppModuleSettings* sets)
{
    if (sets == NULL) sets = new SAppModuleSettings;
    sets->cnt_params = 0; sets->params = NULL;
    return sets;
}

SAppModuleOptions* __stdcall NmeaDecoderBcc32::getModuleParamsOptions(SAppModuleOptions* opts)
{
    if (opts == NULL) opts = new SAppModuleOptions;
    opts->cnt_arg = 0; opts->args = NULL;
    return opts;
}

void __stdcall NmeaDecoderBcc32::getDefaultModuleParams(SAppModuleSettings* sets, SAppModuleOptions* opts)
{
    if (sets != NULL) { std::memset(sets->name, 0, sizeof(sets->name)); std::strncpy(sets->name, "NMEA decoder", sizeof(sets->name) - 1); sets->cnt_params = 0; sets->params = NULL; }
    if (opts != NULL) { opts->cnt_arg = 0; opts->args = NULL; }
}

bool __stdcall NmeaDecoderBcc32::workData(char* idsData, int idsDataLen, char* data, int len)
{
    if (data == NULL || len <= 0 || ifaceProc == NULL) return false;
    recv += static_cast<unsigned __int64>(len);
    if (idsData != NULL && idsDataLen > 0) ifaceProc->PutPrevIdentify(idsData, idsDataLen);
    if (len >= 6 && std::memcmp(data, "UdPbC\0", 6) == 0) Process450Packet(data, len);
    else ProcessRawBytes(data, len);
    return true;
}

void __stdcall NmeaDecoderBcc32::timing() { CleanupGroups(); }
bool __stdcall NmeaDecoderBcc32::recvGuiData(unsigned short type, char* data, int len) { if (ifaceProc != NULL) ifaceProc->SendGuiData(type, data, len); delete[] data; return true; }
void __stdcall NmeaDecoderBcc32::updateStatistic()
{
    char text[128];
#ifdef __BORLANDC__
    std::sprintf(text, "valid=%I64u rejected=%I64u", validMessages, rejectedMessages);
#else
    std::sprintf(text, "valid=%llu rejected=%llu",
                 static_cast<unsigned long long>(validMessages),
                 static_cast<unsigned long long>(rejectedMessages));
#endif
    if (ifaceProc != NULL) ifaceProc->SendStatString(text);
}

void NmeaDecoderBcc32::ProcessRawBytes(const char* data, int len)
{
    lineBuffer.append(data, static_cast<std::string::size_type>(len));
    std::string::size_type end;
    while ((end = lineBuffer.find_first_of("\r\n")) != std::string::npos) {
        const std::string line = lineBuffer.substr(0, end);
        while (end < lineBuffer.size() && (lineBuffer[end] == '\r' || lineBuffer[end] == '\n')) ++end;
        lineBuffer.erase(0, end);
        if (!line.empty()) ProcessMessage(line);
    }
}

void NmeaDecoderBcc32::Process450Packet(const char* data, int len)
{
    const std::string payload(data + 6, static_cast<std::string::size_type>(len - 6));
    if (payload.empty() || payload[0] != '\\') return;
    const std::string::size_type close = payload.find('\\', 1);
    if (close == std::string::npos) return;
    const std::string tags = payload.substr(1, close - 1);
    std::string message = payload.substr(close + 1);
    while (!message.empty() && (message[message.size() - 1] == '\r' || message[message.size() - 1] == '\n')) message.erase(message.size() - 1);

    std::string source, group;
    std::string::size_type pos = 0;
    while (pos < tags.size()) {
        const std::string::size_type next = tags.find(',', pos);
        const std::string tag = tags.substr(pos, next == std::string::npos ? std::string::npos : next - pos);
        if (tag.compare(0, 2, "s:") == 0) source = tag.substr(2);
        if (tag.compare(0, 2, "g:") == 0) group = tag.substr(2);
        if (next == std::string::npos) break;
        pos = next + 1;
    }
    if (group.empty()) { ProcessMessage(message); return; }

    const std::string::size_type first = group.find('-');
    const std::string::size_type second = first == std::string::npos ? std::string::npos : group.find('-', first + 1);
    if (first == std::string::npos || second == std::string::npos) return;
    const int index = ParsePositiveInt(group.substr(0, first));
    const int total = ParsePositiveInt(group.substr(first + 1, second - first - 1));
    if (index == 0 || total == 0 || index > total) return;
    NmeaGroup& assembled = groups[source + "_" + group.substr(second + 1)];
    assembled.totalLines = total; assembled.updatedAt = GetTickCount(); assembled.lines[index] = message;
    if (static_cast<int>(assembled.lines.size()) != total) return;
    std::string complete;
    for (int i = 1; i <= total; ++i) {
        const std::string part = assembled.lines[i];
        if (part.empty()) return;
        if (i == 1) complete = part;
        else { const std::string::size_type comma = part.find(','); const std::string::size_type star = part.find('*'); const std::string::size_type insertAt = complete.find('*'); if (comma == std::string::npos || star == std::string::npos || insertAt == std::string::npos) return; complete.insert(insertAt, part.substr(comma, star - comma)); }
    }
    groups.erase(source + "_" + group.substr(second + 1));
    ProcessMessage(complete);
}

void NmeaDecoderBcc32::ProcessMessage(const std::string& message)
{
    if (IsValidNmea(message) && SendMessage(message)) ++validMessages;
    else ++rejectedMessages;
}

bool NmeaDecoderBcc32::SendMessage(const std::string& message)
{
    const std::string packet = message + "\r\n";
    const int sent = ifaceProc->SendDataProc(const_cast<char*>(packet.c_str()), static_cast<int>(packet.size()));
    if (sent > 0) send += static_cast<unsigned __int64>(sent);
    return sent == static_cast<int>(packet.size());
}

void NmeaDecoderBcc32::CleanupGroups()
{
    const unsigned long now = GetTickCount();
    std::map<std::string, NmeaGroup>::iterator it = groups.begin();
    while (it != groups.end()) {
        if (now - it->second.updatedAt > 3000UL) groups.erase(it++);
        else ++it;
    }
}

extern "C" DHF_IFACE_MODULE_EXT_DLL IModuleExt* __stdcall getIModuleExt() { return new NmeaDecoderBcc32; }
extern "C" DHF_IFACE_MODULE_EXT_DLL void __stdcall removeIModuleExt(IModuleExt* module)
{
    // IModuleExt has no virtual destructor in the host ABI.  The factory only
    // returns this concrete implementation, so delete it as that type.
    delete static_cast<NmeaDecoderBcc32*>(module);
}
