#ifndef NMEA450_PARSER_H
#define NMEA450_PARSER_H

#include <map>
#include <string>
#include <vector>
#include <ctime>

struct NmeaGroupAssembly {
    int totalLines;
    time_t lastUpdate;
    std::map<int, std::string> lines;

    NmeaGroupAssembly() : totalLines(0), lastUpdate(0) {}
};

class Nmea450Parser {
public:
    typedef void (*MsgAssembledCallback)(void* context, const std::string& message,
                                         const std::string& source);

    Nmea450Parser();
    void SetOnMsgAssembled(MsgAssembledCallback callback, void* context);
    void ProcPacket(const unsigned char* payload, unsigned long length);
    void CleanupTimeouts();

private:
    void HandleTagBlock(const std::string& tagBlock, const std::string& nmeaMessage);
    std::vector<std::string> SplitStr(const std::string& value, char delimiter) const;

    MsgAssembledCallback m_assembledCallback;
    void* m_callbackContext;
    std::map<std::string, NmeaGroupAssembly> m_groupPool;
};

#endif
