#include "nmea0183_parser.h"

Nmea0183Parser::Nmea0183Parser()
    : m_headerCallback(0), m_callbackContext(0)
{
}

void Nmea0183Parser::SetOnHeaderParsed(HeaderParsedCallback callback, void* context)
{
    m_headerCallback = callback;
    m_callbackContext = context;
}

void Nmea0183Parser::ParseMsg(const std::string& msg)
{
    NmeaHeaderInfo header;

    if (msg.length() < 6 || (msg[0] != '$' && msg[0] != '!')) return;
    if (!ValidateChecksum(msg)) return;

    header.msg = msg;
    header.talker_id = msg.substr(1, 2);
    header.msg_type = msg.substr(3, 3);

    if (m_headerCallback != 0) m_headerCallback(m_callbackContext, header);
}

bool Nmea0183Parser::ValidateChecksum(const std::string& msg) const
{
    const std::string::size_type star = msg.find('*');
    unsigned char checksum = 0;
    unsigned int expected = 0;
    char digit;
    std::string::size_type i;

    if (star == std::string::npos || star + 3 != msg.length()) return false;

    for (i = 1; i < star; ++i) checksum ^= static_cast<unsigned char>(msg[i]);

    for (i = star + 1; i < star + 3; ++i) {
        digit = msg[i];
        expected <<= 4;
        if (digit >= '0' && digit <= '9') expected |= digit - '0';
        else if (digit >= 'A' && digit <= 'F') expected |= digit - 'A' + 10;
        else if (digit >= 'a' && digit <= 'f') expected |= digit - 'a' + 10;
        else return false;
    }
    return checksum == expected;
}
