#ifndef NMEA0183_PARSER_H
#define NMEA0183_PARSER_H

#include <string>
#include <vector>

struct NmeaHeaderInfo {
    std::string msg;
    std::string talker_id;
    std::string msg_type;
};

class Nmea0183Parser {
public:
    typedef void (*HeaderParsedCallback)(void* context, const NmeaHeaderInfo& header);

    Nmea0183Parser();

    void SetOnHeaderParsed(HeaderParsedCallback callback, void* context);
    void ParseMsg(const std::string& msg);

private:
    bool ValidateChecksum(const std::string& msg) const;

    HeaderParsedCallback m_headerCallback;
    void* m_callbackContext;
};

#endif
