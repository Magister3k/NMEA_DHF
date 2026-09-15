#pragma once

#include <string>
#include <vector>
#include <functional>
#include "ais_structures.h"

struct NmeaHeaderInfo {
    std::string msg;
    std::string talker_id;
    std::string msg_type;
};

class Nmea0183Parser {
public:
    // 
    using HeaderParsedCallback = std::function<void(const NmeaHeaderInfo&)>;
    using StandardMsgCallback = std::function<void(const std::string& talker, const std::string& type, const std::vector<std::string>& fields)>;
    using AisMsgCallback = std::function<void(const std::string& ais_payload)>;

    Nmea0183Parser() = default;
    ~Nmea0183Parser() = default;

    // 
    Nmea0183Parser(const Nmea0183Parser&) = delete;
    Nmea0183Parser& operator=(const Nmea0183Parser&) = delete;

    // 
    void SetOnHeaderParsed(HeaderParsedCallback cb);
    void SetOnStandardMsg(StandardMsgCallback cb);
    void SetOnAisMsg(AisMsgCallback cb);

    /**
     * @brief NMEA 0183 (NMEA-450).
     * @param msg, '$' '!'
     */
    void ParseMsg(const std::string& msg);

private:
    // 
    bool ValidateChecksum(const std::string& msg) const;
    std::vector<std::string> SplitString(const std::string& str, char delimiter) const;

    HeaderParsedCallback m_header_cb = nullptr;
    StandardMsgCallback m_standard_cb = nullptr;
    AisMsgCallback m_ais_cb = nullptr;
};
