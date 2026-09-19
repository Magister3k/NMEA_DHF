#include "nmea_decoder.h"

#ifdef _WIN32
#include <commctrl.h>
#include <windows.h>
#endif

#include <cstdio>
#include <cstring>
#include <sstream>
#include <vector>

namespace
{
const char kShortName[] = "NMEA decoder";
const char kLongName[] = "NMEA/AIS message decoder (0183/61162-450)";
const char kGroupName[] = "PineCode Lab";
const char kSettingsName[] = "NMEA decoder settings";
const char kSettingMessageType[] = "message_type";
const char kSettingCoordinatesOnly[] = "coordinates_only";

const int kIdFilterAll = 1001;
const int kIdFilterNmea = 1002;
const int kIdFilterAis = 1003;
const int kIdCoordinatesOnly = 1004;

bool GetBool(const char* value)
{
    return value != NULL && (_stricmp(value, "true") == 0 || std::strcmp(value, "1") == 0);
}

std::vector<std::string> Split(const std::string& value, char delimiter)
{
    std::vector<std::string> fields;
    std::string field;
    std::istringstream stream(value);
    while (std::getline(stream, field, delimiter)) fields.push_back(field);
    if (!value.empty() && value.back() == delimiter) fields.push_back("");
    return fields;
}

bool HasNmeaPosition(const std::string& type, const std::vector<std::string>& fields)
{
    if (type == "GGA" || type == "GNS") return fields.size() >= 6 && !fields[2].empty() && !fields[4].empty();
    if (type == "GLL") return fields.size() >= 5 && !fields[1].empty() && !fields[3].empty();
    if (type == "RMC") return fields.size() >= 7 && fields[2] == "A" && !fields[3].empty() && !fields[5].empty();
    if (type == "TLL") return fields.size() >= 5 && !fields[2].empty() && !fields[4].empty();
    return false;
}

bool DecodeAisInfo(const std::vector<std::string>& fields, unsigned int& type, bool& hasPos)
{
    if (fields.size() < 6 || fields[5].empty()) return false;
    const unsigned char first = static_cast<unsigned char>(fields[5][0]);
    if (first < '0' || first > 'w' || (first > 'W' && first < '`')) return false;
    unsigned int sixBit = first - '0';
    if (sixBit > 40) sixBit -= 8;
    type = sixBit;
    // Position reports defined by AIS include 1/2/3, 4, 9, 11, 18, 19, 21 and 27.
    hasPos = type == 1 || type == 2 || type == 3 || type == 4 || type == 9 || type == 11 ||
             type == 18 || type == 19 || type == 21 || type == 27;
    return true;
}



#ifdef _WIN32
BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
{
    (void)instance; (void)reason; (void)reserved;
    return TRUE;
}
#endif

NmeaDecoder::NmeaDecoder()
    : m_filter(FilterAll), m_coordinatesOnly(false), m_processedMsgs(0), m_rejectedMsgs(0),
      m_aisDecodedMsgs(0), m_posMsgs(0), m_nmeaPosMsgs(0), m_aisPosMsgs(0)
{
    std::memset(aShortName, 0, sizeof(aShortName));
    std::memset(aLongName, 0, sizeof(aLongName));
    std::memset(aGroupName, 0, sizeof(aGroupName));
    std::strncpy(aShortName, kShortName, sizeof(aShortName) - 1);
    std::strncpy(aLongName, kLongName, sizeof(aLongName) - 1);
    std::strncpy(aGroupName, kGroupName, sizeof(aGroupName) - 1);
    std::snprintf(aVersion, sizeof(aVersion), "%s.%s", MODULE_INTERFACE_VERSION, MODULE_RELEASE_VERSION);
    bIsInputModule = false;
    byteTypeData = true;
    typeData = '\n';
    hSettingForm = NULL;
    hBitmap = NULL;
}

NmeaDecoder::~NmeaDecoder() { DestroyForm(); Free(); Destructor(); }
bool __stdcall NmeaDecoder::Constructor(unsigned __int64 uid) { moduleUid = static_cast<unsigned short>(uid); return true; }
void __stdcall NmeaDecoder::Destructor() { m_lineBuffer.clear(); m_src.clear(); }
bool __stdcall NmeaDecoder::CreateGui(IfaceCallBackGui* ig) { ifaceGui = ig; return true; }
bool __stdcall NmeaDecoder::CreateProc(IfaceCallBackProc* ip) { ifaceProc = ip; return ip != NULL; }

bool __stdcall NmeaDecoder::Initialize()
{
    recv = send = 0;
    m_processedMsgs = m_rejectedMsgs = m_aisDecodedMsgs = 0;
    m_posMsgs = m_nmeaPosMsgs = m_aisPosMsgs = 0;
    m_nmeaMsgCounts.clear();
    m_aisTypeCounts.clear();
    m_lineBuffer.clear();
    m_nmea450.SetOnMsgAssembled([this](const std::string& msg, const std::string& src) { ProcMsg(msg, src); });
    return true;
}
void __stdcall NmeaDecoder::Free() { m_lineBuffer.clear(); }

void __stdcall NmeaDecoder::CreateForm()
{
#ifdef _WIN32
    if (hSettingForm != NULL) return;
    INITCOMMONCONTROLSEX controls = { sizeof(controls), ICC_TAB_CLASSES };
    InitCommonControlsEx(&controls);
    static const char kClassName[] = "NmeaDecoderSettings";
    static bool registered = false;
    if (!registered) {
        WNDCLASS windowClass = {};
        windowClass.lpfnWndProc = SettingsWndProc;
        windowClass.hInstance = GetModuleHandle(NULL);
        windowClass.lpszClassName = kClassName;
        windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
        registered = RegisterClass(&windowClass) != 0 || GetLastError() == ERROR_CLASS_ALREADY_EXISTS;
    }
    hSettingForm = CreateWindowEx(0, kClassName, "NMEA decoder", WS_CHILD | WS_CLIPCHILDREN,
                                  0, 0, 440, 290, NULL, NULL, GetModuleHandle(NULL), this);
    if (hSettingForm != NULL) CreateSettingsControls(hSettingForm);
#endif
}
void __stdcall NmeaDecoder::DestroyForm()
{
#ifdef _WIN32
    if (hSettingForm != NULL) DestroyWindow(hSettingForm);
#endif
    hSettingForm = NULL;
}
void __stdcall NmeaDecoder::ShowForm(bool show)
{
#ifdef _WIN32
    if (hSettingForm != NULL) ShowWindow(hSettingForm, show ? SW_SHOW : SW_HIDE);
#else
    (void)show;
#endif
}

void NmeaDecoder::ApplySettings(SAppModuleSettings* sets, bool updateGui)
{
    if (sets == NULL) return;
    for (int i = 0; i < sets->cnt_params; ++i) {
        const SAppModuleParam& param = sets->params[i];
        if (std::strcmp(param.name, kSettingMessageType) == 0) {
            if (_stricmp(param.value, "nmea") == 0) m_filter = FilterNmea;
            else if (_stricmp(param.value, "ais") == 0) m_filter = FilterAis;
            else m_filter = FilterAll;
        } else if (std::strcmp(param.name, kSettingCoordinatesOnly) == 0) {
            m_coordinatesOnly = GetBool(param.value);
        }
    }
    if (updateGui) UpdateGuiSettings();
}
void __stdcall NmeaDecoder::setGuiModuleSettings(SAppModuleSettings* sets) { ApplySettings(sets, true); }
void __stdcall NmeaDecoder::setGuiModuleOptions(SAppModuleOptions* opts) { (void)opts; }
void __stdcall NmeaDecoder::setProcModuleSettings(SAppModuleSettings* sets) { ApplySettings(sets, false); if (sets != NULL) clearModulesSettings(sets); }
void __stdcall NmeaDecoder::setProcModuleOptions(SAppModuleOptions* opts) { if (opts != NULL) clearModulesOptions(opts); }

SAppModuleSettings* __stdcall NmeaDecoder::getModuleParamsSetting(SAppModuleSettings* sets)
{
    if (sets == NULL) sets = new SAppModuleSettings;
    clearModulesSettings(sets);
    std::strncpy(sets->name, kSettingsName, sizeof(sets->name) - 1);
    sets->name[sizeof(sets->name) - 1] = '\0';
    sets->cnt_params = 2;
    sets->params = new SAppModuleParam[sets->cnt_params];
    std::strncpy(sets->params[0].name, kSettingMessageType, sizeof(sets->params[0].name) - 1);
    std::strncpy(sets->params[0].value, m_filter == FilterNmea ? "nmea" : (m_filter == FilterAis ? "ais" : "all"), sizeof(sets->params[0].value) - 1);
    std::strncpy(sets->params[1].name, kSettingCoordinatesOnly, sizeof(sets->params[1].name) - 1);
    std::strncpy(sets->params[1].value, m_coordinatesOnly ? "true" : "false", sizeof(sets->params[1].value) - 1);
    return sets;
}
SAppModuleOptions* __stdcall NmeaDecoder::getModuleParamsOptions(SAppModuleOptions* opts)
{
    if (opts == NULL) opts = new SAppModuleOptions;
    clearModulesOptions(opts); opts->cnt_arg = 0; opts->args = NULL;
    return opts;
}
void __stdcall NmeaDecoder::getDefaultModuleParams(SAppModuleSettings* sets, SAppModuleOptions* opts)
{
    const MessageFilter previousFilter = m_filter; const bool previousCoordinatesOnly = m_coordinatesOnly;
    m_filter = FilterAll; m_coordinatesOnly = false;
    if (sets != NULL) getModuleParamsSetting(sets);
    m_filter = previousFilter; m_coordinatesOnly = previousCoordinatesOnly;
    if (opts != NULL) { clearModulesOptions(opts); opts->cnt_arg = 0; opts->args = NULL; }
}

void __stdcall NmeaDecoder::setProcState(bool state) { IModuleExt::setProcState(state); UpdateGuiEnabled(); }
bool __stdcall NmeaDecoder::procGuiData(unsigned short type, void* data, int len) { (void)type; (void)data; (void)len; return true; }
bool __stdcall NmeaDecoder::HookMessage(int wParam, int lParam) { (void)wParam; (void)lParam; return true; }
bool __stdcall NmeaDecoder::workProc(bool rSleep) { (void)rSleep; return true; }

bool __stdcall NmeaDecoder::workData(char* idsData, int idsDataLen, char* data, int len)
{
    if (data == NULL || len <= 0 || ifaceProc == NULL) return false;
    recv += static_cast<unsigned __int64>(len);
    if (idsData != NULL && idsDataLen > 0) ifaceProc->PutPrevIdentify(idsData, idsDataLen);
    if (len >= 6 && std::memcmp(data, "UdPbC\0", 6) == 0) m_nmea450.ProcPacket(reinterpret_cast<const uint8_t*>(data), static_cast<size_t>(len));
    else ProcRawBytes(data, len);
    return true;
}
void __stdcall NmeaDecoder::timing() { m_nmea450.CleanupTimeouts(); }
bool __stdcall NmeaDecoder::recvGuiData(unsigned short type, char* data, int len) { if (ifaceProc != NULL) ifaceProc->SendGuiData(type, data, len); delete[] data; return true; }

void __stdcall NmeaDecoder::updateStatistic()
{
    char message[256];
    std::snprintf(message, sizeof(message), "Обработано сообщений NMEA\t%llu", static_cast<unsigned long long>(m_processedMsgs)); SendStats(message);
    SendStats("-------------------------------------------");
    for (std::map<std::string, unsigned __int64>::const_iterator it = m_nmeaMsgCounts.begin(); it != m_nmeaMsgCounts.end(); ++it) { std::snprintf(message, sizeof(message), "%s\t%llu", it->first.c_str(), static_cast<unsigned long long>(it->second)); SendStats(message); }
    std::snprintf(message, sizeof(message), "Декодировано сообщений AIS\t%llu", static_cast<unsigned long long>(m_aisDecodedMsgs)); SendStats(message);
    SendStats("-------------------------------------------");
    for (std::map<unsigned int, unsigned __int64>::const_iterator it = m_aisTypeCounts.begin(); it != m_aisTypeCounts.end(); ++it) { std::snprintf(message, sizeof(message), "Тип %u\t%llu", it->first, static_cast<unsigned long long>(it->second)); SendStats(message); }
    std::snprintf(message, sizeof(message), "Сообщений с координатами (всего/NMEA/AIS)\t%llu/%llu/%llu", static_cast<unsigned long long>(m_posMsgs), static_cast<unsigned long long>(m_nmeaPosMsgs), static_cast<unsigned long long>(m_aisPosMsgs)); SendStats(message);
}

void NmeaDecoder::ProcRawBytes(const char* data, int len)
{
    m_lineBuffer.append(data, static_cast<size_t>(len)); size_t lineEnd;
    while ((lineEnd = m_lineBuffer.find_first_of("\r\n")) != std::string::npos) {
        const std::string line = m_lineBuffer.substr(0, lineEnd); size_t consume = lineEnd;
        while (consume < m_lineBuffer.size() && (m_lineBuffer[consume] == '\r' || m_lineBuffer[consume] == '\n')) ++consume;
        m_lineBuffer.erase(0, consume); if (!line.empty()) ProcMsg(line, m_src);
    }
}

void NmeaDecoder::ProcMsg(const std::string& msg, const std::string& src)
{
    std::string clean = msg; while (!clean.empty() && (clean.back() == '\r' || clean.back() == '\n')) clean.pop_back(); if (clean.empty()) return;
    NmeaHeaderInfo header; bool parsed = false;
    m_decoder.SetOnHeaderParsed([&header, &parsed](const NmeaHeaderInfo& value) { header = value; parsed = true; });
    m_decoder.SetOnStandardMsg([](const std::string&, const std::string&, const std::vector<std::string>&) {});
    m_decoder.SetOnAisMsg([](const std::string&) {});
    m_decoder.ParseMsg(clean);
    if (!parsed) { ++m_rejectedMsgs; return; }

    const std::vector<std::string> fields = Split(clean.substr(0, clean.find('*')), ',');
    const bool isAis = clean[0] == '!' && (header.msg_type == "VDM" || header.msg_type == "VDO");
    bool hasPos = false; unsigned int aisType = 0;
    if (isAis) {
        if (!DecodeAisInfo(fields, aisType, hasPos)) { ++m_rejectedMsgs; return; }
        ++m_aisDecodedMsgs; ++m_aisTypeCounts[aisType];
    } else hasPos = HasNmeaPosition(header.msg_type, fields);

    ++m_processedMsgs;
    const std::string sentenceType = clean.substr(0, clean.find(','));
    ++m_nmeaMsgCounts[sentenceType];
    if (hasPos) { ++m_posMsgs; if (isAis) ++m_aisPosMsgs; else ++m_nmeaPosMsgs; }
    if (IsAllowed(isAis, hasPos) && !SendMsg(clean)) ++m_rejectedMsgs;
    m_src = src;
}

bool NmeaDecoder::IsAllowed(bool isAis, bool hasPos) const { return (!m_coordinatesOnly || hasPos) && (m_filter == FilterAll || (m_filter == FilterAis && isAis) || (m_filter == FilterNmea && !isAis)); }
bool NmeaDecoder::SendMsg(const std::string& msg) { if (ifaceProc == NULL) return false; std::string packet = msg + "\r\n"; const int result = ifaceProc->SendDataProc(&packet[0], static_cast<int>(packet.size())); if (result > 0) send += static_cast<unsigned __int64>(result); return result == static_cast<int>(packet.size()); }
void NmeaDecoder::SendStats(const char* msg) { if (ifaceProc != NULL) ifaceProc->SendStatString(const_cast<char*>(msg)); }

void NmeaDecoder::UpdateGuiSettings()
{
#ifdef _WIN32
    if (hSettingForm == NULL) return;
    CheckRadioButton(hSettingForm, kIdFilterAll, kIdFilterAis, kIdFilterAll + static_cast<int>(m_filter));
    SendDlgItemMessage(hSettingForm, kIdCoordinatesOnly, BM_SETCHECK, m_coordinatesOnly ? BST_CHECKED : BST_UNCHECKED, 0);
#endif
}
void NmeaDecoder::UpdateGuiEnabled()
{
#ifdef _WIN32
    if (hSettingForm == NULL) return;
    EnableWindow(GetDlgItem(hSettingForm, kIdFilterAll), !bStateProc); EnableWindow(GetDlgItem(hSettingForm, kIdFilterNmea), !bStateProc); EnableWindow(GetDlgItem(hSettingForm, kIdFilterAis), !bStateProc); EnableWindow(GetDlgItem(hSettingForm, kIdCoordinatesOnly), !bStateProc);
#endif
}

#ifdef _WIN32
LRESULT CALLBACK NmeaDecoder::SettingsWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    NmeaDecoder* decoder = reinterpret_cast<NmeaDecoder*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    if (message == WM_NCCREATE) { decoder = reinterpret_cast<NmeaDecoder*>(reinterpret_cast<CREATESTRUCT*>(lParam)->lpCreateParams); SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(decoder)); }
    if (decoder != NULL && message == WM_COMMAND && HIWORD(wParam) == BN_CLICKED) { const int id = LOWORD(wParam); if (id >= kIdFilterAll && id <= kIdFilterAis) { decoder->m_filter = static_cast<MessageFilter>(id - kIdFilterAll); decoder->UpdateGuiSettings(); } else if (id == kIdCoordinatesOnly) decoder->m_coordinatesOnly = IsDlgButtonChecked(hwnd, id) == BST_CHECKED; return 0; }
    return DefWindowProc(hwnd, message, wParam, lParam);
}
void NmeaDecoder::CreateSettingsControls(HWND hwnd)
{
    CreateWindow("STATIC", "Настройки", WS_CHILD | WS_VISIBLE, 12, 12, 100, 18, hwnd, NULL, GetModuleHandle(NULL), NULL);
    CreateWindow("STATIC", "Типы обрабатываемых сообщений:", WS_CHILD | WS_VISIBLE, 24, 44, 220, 18, hwnd, NULL, GetModuleHandle(NULL), NULL);
    CreateWindow("BUTTON", "Все", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON, 42, 68, 120, 20, hwnd, reinterpret_cast<HMENU>(kIdFilterAll), GetModuleHandle(NULL), NULL);
    CreateWindow("BUTTON", "NMEA", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON, 42, 92, 120, 20, hwnd, reinterpret_cast<HMENU>(kIdFilterNmea), GetModuleHandle(NULL), NULL);
    CreateWindow("BUTTON", "AIS", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON, 42, 116, 120, 20, hwnd, reinterpret_cast<HMENU>(kIdFilterAis), GetModuleHandle(NULL), NULL);
    CreateWindow("BUTTON", "Только с координатами", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 24, 150, 200, 20, hwnd, reinterpret_cast<HMENU>(kIdCoordinatesOnly), GetModuleHandle(NULL), NULL);
    CreateWindow("STATIC", "Статистика отображается на вкладке статистики модуля.", WS_CHILD | WS_VISIBLE, 24, 210, 360, 18, hwnd, NULL, GetModuleHandle(NULL), NULL);
    UpdateGuiSettings(); UpdateGuiEnabled();
}
#endif

extern "C" DHF_IFACE_MODULE_EXT_DLL IModuleExt* __stdcall getIModuleExt() { return new NmeaDecoder(); }
extern "C" DHF_IFACE_MODULE_EXT_DLL void __stdcall removeIModuleExt(IModuleExt* module) { delete module; }
