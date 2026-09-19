#ifndef NMEA_MODULE_EXT_H
#define NMEA_MODULE_EXT_H

#define __DHF_RELEASE_IFACE_MODULE_EXT_H__
#include "DHF/_example/include/IfaceModuleExt.h"

#include "NMEA/nmea0183_parser.h"
#include "NMEA/nmea450_parser.h"

#include <string>

const char MODULE_INTERFACE_VERSION[16] = "0.0";
const char MODULE_RELEASE_VERSION[16] = "1.0";

class NmeaDecoder : public IModuleExt
{
public:
    NmeaDecoder();
    ~NmeaDecoder();

    bool __stdcall Constructor(unsigned __int64 uid);
    void __stdcall Destructor();
    bool __stdcall CreateGui(IfaceCallBackGui* ig);
    bool __stdcall CreateProc(IfaceCallBackProc* ip);
    bool __stdcall Initialize();
    void __stdcall Free();

    void __stdcall CreateForm();
    void __stdcall DestroyForm();
    void __stdcall ShowForm(bool show = true);

    void __stdcall setGuiModuleSettings(SAppModuleSettings* sets);
    void __stdcall setGuiModuleOptions(SAppModuleOptions* opts);
    void __stdcall setProcModuleSettings(SAppModuleSettings* sets);
    void __stdcall setProcModuleOptions(SAppModuleOptions* opts);
    SAppModuleSettings* __stdcall getModuleParamsSetting(SAppModuleSettings* sets = NULL);
    SAppModuleOptions* __stdcall getModuleParamsOptions(SAppModuleOptions* opts = NULL);
    void __stdcall getDefaultModuleParams(SAppModuleSettings* sets, SAppModuleOptions* opts);

    void __stdcall setProcState(bool state);
    bool __stdcall procGuiData(unsigned short type, void* data, int len);
    bool __stdcall HookMessage(int wParam, int lParam);
    bool __stdcall workProc(bool rSleep);
    bool __stdcall workData(char* idsData, int idsDataLen, char* data, int len);
    void __stdcall timing();
    bool __stdcall recvGuiData(unsigned short type, char* data, int len);
    void __stdcall updateStatistic();

private:
    static void OnNmea450Message(void* context, const std::string& msg, const std::string& src);
    static void OnNmeaHeader(void* context, const NmeaHeaderInfo& header);
    void ProcMsg(const std::string& msg, const std::string& src);
    void ProcRawBytes(const char* data, int len);
    bool SendMsg(const std::string& msg);
    void SendStats(const char* msg);

    Nmea0183Parser m_decoder;
    Nmea450Parser m_nmea450;
    std::string m_lineBuffer;
    std::string m_src;
    bool m_headerParsed;
    unsigned __int64 m_validMsgs;
    unsigned __int64 m_rejectedMsgs;
};

#endif
