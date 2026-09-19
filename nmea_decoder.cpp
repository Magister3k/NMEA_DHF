#include "nmea_decoder.h"

#ifdef _WIN32
#include <windows.h>
#endif

#include <algorithm>
#include <cstdio>
#include <cstring>

namespace
{
const char kShortName[] = "NMEA decoder";
const char kLongName[] = "NMEA/AIS decoder (0183/61162-450)";
const char kGroupName[] = "PineCode Lab";
}

#ifdef _WIN32
#if defined(__BORLANDC__)
int WINAPI DllEntryPoint(HINSTANCE instance, unsigned long reason, void* reserved)
#else
BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
#endif
{
    (void)instance;
    (void)reason;
    (void)reserved;
    return TRUE;
}
#endif

NmeaDecoder::NmeaDecoder()
    : m_headerParsed(false), m_validMsgs(0), m_rejectedMsgs(0)
{
    ::strncpy(aShortName, kShortName, sizeof(aShortName) - 1);
    ::strncpy(aLongName, kLongName, sizeof(aLongName) - 1);
    ::strncpy(aGroupName, kGroupName, sizeof(aGroupName) - 1);
    ::sprintf(aVersion, "%s.%s", MODULE_INTERFACE_VERSION, MODULE_RELEASE_VERSION);
    bIsInputModule = false;
    byteTypeData = true;
    typeData = '\n';
}

NmeaDecoder::~NmeaDecoder()
{
    Free();
    Destructor();
}

bool __stdcall NmeaDecoder::Constructor(unsigned __int64 uid)
{
    moduleUid = static_cast<unsigned short>(uid);
    return true;
}

void __stdcall NmeaDecoder::Destructor()
{
    m_lineBuffer.clear();
    m_src.clear();
}

bool __stdcall NmeaDecoder::CreateGui(IfaceCallBackGui* ig)
{
    ifaceGui = ig;
    return true;
}

bool __stdcall NmeaDecoder::CreateProc(IfaceCallBackProc* ip)
{
    ifaceProc = ip;
    return ip != NULL;
}

bool __stdcall NmeaDecoder::Initialize()
{
    recv = 0;
    send = 0;
    m_validMsgs = 0;
    m_rejectedMsgs = 0;
    m_headerParsed = false;
    m_lineBuffer.clear();

    m_nmea450.SetOnMsgAssembled(&NmeaDecoder::OnNmea450Message, this);
    return true;
}

void __stdcall NmeaDecoder::Free()
{
    m_lineBuffer.clear();
}

void __stdcall NmeaDecoder::CreateForm() {}
void __stdcall NmeaDecoder::DestroyForm() {}
void __stdcall NmeaDecoder::ShowForm(bool show) { (void)show; }
void __stdcall NmeaDecoder::setGuiModuleSettings(SAppModuleSettings* sets) { (void)sets; }
void __stdcall NmeaDecoder::setGuiModuleOptions(SAppModuleOptions* opts) { (void)opts; }
void __stdcall NmeaDecoder::setProcModuleSettings(SAppModuleSettings* sets) { (void)sets; }
void __stdcall NmeaDecoder::setProcModuleOptions(SAppModuleOptions* opts) { (void)opts; }

SAppModuleSettings* __stdcall NmeaDecoder::getModuleParamsSetting(SAppModuleSettings* sets)
{
    if (sets == NULL) sets = new SAppModuleSettings;
    sets->cnt_params = 0;
    sets->params = NULL;
    return sets;
}

SAppModuleOptions* __stdcall NmeaDecoder::getModuleParamsOptions(SAppModuleOptions* opts)
{
    if (opts == NULL) opts = new SAppModuleOptions;
    opts->cnt_arg = 0;
    opts->args = NULL;
    return opts;
}

void __stdcall NmeaDecoder::getDefaultModuleParams(SAppModuleSettings* sets, SAppModuleOptions* opts)
{
    if (sets != NULL) {
        ::strncpy(sets->name, "NMEA decoder", sizeof(sets->name) - 1);
        sets->cnt_params = 0;
        sets->params = NULL;
    }
    if (opts != NULL) {
        opts->cnt_arg = 0;
        opts->args = NULL;
    }
}

void __stdcall NmeaDecoder::setProcState(bool state)
{
    IModuleExt::setProcState(state);
}

bool __stdcall NmeaDecoder::procGuiData(unsigned short type, void* data, int len)
{
    (void)type;
    (void)data;
    (void)len;
    return true;
}

bool __stdcall NmeaDecoder::HookMessage(int wParam, int lParam)
{
    (void)wParam;
    (void)lParam;
    return true;
}

bool __stdcall NmeaDecoder::workProc(bool rSleep)
{
    (void)rSleep;
    return true;
}

bool __stdcall NmeaDecoder::workData(char* idsData, int idsDataLen, char* data, int len)
{
    if (data == NULL || len <= 0 || ifaceProc == NULL) return false;

    recv += static_cast<unsigned __int64>(len);
    if (idsData != NULL && idsDataLen > 0) {
        ifaceProc->PutPrevIdentify(idsData, idsDataLen);
    }

    if (len >= 6 && ::memcmp(data, "UdPbC\0", 6) == 0) {
        m_nmea450.ProcPacket(reinterpret_cast<const unsigned char*>(data), static_cast<unsigned long>(len));
    } else {
        ProcRawBytes(data, len);
    }
    return true;
}

void __stdcall NmeaDecoder::timing()
{
    m_nmea450.CleanupTimeouts();
}

bool __stdcall NmeaDecoder::recvGuiData(unsigned short type, char* data, int len)
{
    if (ifaceProc != NULL) ifaceProc->SendGuiData(type, data, len);
    delete[] data;
    return true;
}

void __stdcall NmeaDecoder::updateStatistic()
{
    char msg[128];
    ::sprintf(msg, "valid=%I64u rejected=%I64u", m_validMsgs, m_rejectedMsgs);
    SendStats(msg);
}

void NmeaDecoder::ProcRawBytes(const char* data, int len)
{
    m_lineBuffer.append(data, static_cast<size_t>(len));
    size_t lineEnd = std::string::npos;
    while ((lineEnd = m_lineBuffer.find_first_of("\r\n")) != std::string::npos) {
        std::string line = m_lineBuffer.substr(0, lineEnd);
        size_t consume = lineEnd;
        while (consume < m_lineBuffer.size() &&
               (m_lineBuffer[consume] == '\r' || m_lineBuffer[consume] == '\n')) {
            ++consume;
        }
        m_lineBuffer.erase(0, consume);
        if (!line.empty()) ProcMsg(line, m_src);
    }
}

void NmeaDecoder::ProcMsg(const std::string& msg, const std::string& src)
{
    std::string clean = msg;
    while (!clean.empty() && (clean[clean.length() - 1] == '\r' || clean[clean.length() - 1] == '\n')) clean.erase(clean.length() - 1);
    if (clean.empty()) return;
    m_src = src;

    m_headerParsed = false;
    m_decoder.SetOnHeaderParsed(&NmeaDecoder::OnNmeaHeader, this);
    m_decoder.ParseMsg(clean);

    if (m_headerParsed && SendMsg(clean)) {
        ++m_validMsgs;
    } else {
        ++m_rejectedMsgs;
    }
}

void NmeaDecoder::OnNmea450Message(void* context, const std::string& msg, const std::string& src)
{
    NmeaDecoder* decoder = static_cast<NmeaDecoder*>(context);
    if (decoder != NULL) decoder->ProcMsg(msg, src);
}

void NmeaDecoder::OnNmeaHeader(void* context, const NmeaHeaderInfo& header)
{
    NmeaDecoder* decoder = static_cast<NmeaDecoder*>(context);
    if (decoder != NULL && !header.msg.empty()) decoder->m_headerParsed = true;
}

bool NmeaDecoder::SendMsg(const std::string& msg)
{
    if (ifaceProc == NULL) return false;
    std::string packet = msg + "\r\n";
    int result = ifaceProc->SendDataProc(&packet[0], static_cast<int>(packet.size()));
    if (result > 0) send += static_cast<unsigned __int64>(result);
    return result == static_cast<int>(packet.size());
}

void NmeaDecoder::SendStats(const char* msg)
{
    if (ifaceProc != NULL) ifaceProc->SendStatString(const_cast<char*>(msg));
}

extern "C" DHF_IFACE_MODULE_EXT_DLL IModuleExt* __stdcall getIModuleExt()
{
    return new NmeaDecoder();
}

extern "C" DHF_IFACE_MODULE_EXT_DLL void __stdcall removeIModuleExt(IModuleExt* module)
{
    delete module;
}
