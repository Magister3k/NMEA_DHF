#include "nmea450_parser.h"

#include <cstdlib>
#include <cstring>

Nmea450Parser::Nmea450Parser()
    : m_assembledCallback(0), m_callbackContext(0)
{
}

void Nmea450Parser::SetOnMsgAssembled(MsgAssembledCallback callback, void* context)
{
    m_assembledCallback = callback;
    m_callbackContext = context;
}

void Nmea450Parser::ProcPacket(const unsigned char* payload, unsigned long length)
{
    std::string packet;
    std::string::size_type closeTag;

    if (payload == 0 || length < 8 || std::memcmp(payload, "UdPbC\0", 6) != 0) return;
    packet.assign(reinterpret_cast<const char*>(payload + 6), length - 6);
    if (packet.empty() || packet[0] != '\\') return;

    closeTag = packet.find('\\', 1);
    if (closeTag == std::string::npos) return;

    std::string message = packet.substr(closeTag + 1);
    while (!message.empty() && (message[message.length() - 1] == '\r' ||
                                message[message.length() - 1] == '\n')) {
        message.erase(message.length() - 1);
    }
    HandleTagBlock(packet.substr(1, closeTag - 1), message);
}

void Nmea450Parser::HandleTagBlock(const std::string& tagBlock, const std::string& nmeaMessage)
{
    const std::string::size_type star = tagBlock.find('*');
    std::vector<std::string> tags;
    std::vector<std::string> groupParts;
    std::string source;
    std::string groupTag;
    std::string groupId;
    std::string complete;
    std::string::size_type firstComma;
    std::string::size_type messageStar;
    int currentLine;
    int line;
    int totalLines;
    std::vector<std::string>::size_type i;

    if (star == std::string::npos) return;
    tags = SplitStr(tagBlock.substr(0, star), ',');
    for (i = 0; i < tags.size(); ++i) {
        if (tags[i].compare(0, 2, "s:") == 0) source = tags[i].substr(2);
        else if (tags[i].compare(0, 2, "g:") == 0) groupTag = tags[i].substr(2);
    }

    if (groupTag.empty()) {
        if (m_assembledCallback != 0) m_assembledCallback(m_callbackContext, nmeaMessage, source);
        return;
    }

    groupParts = SplitStr(groupTag, '-');
    if (groupParts.size() != 3) return;
    currentLine = std::atoi(groupParts[0].c_str());
    totalLines = std::atoi(groupParts[1].c_str());
    if (currentLine < 1 || totalLines < 1 || currentLine > totalLines) return;

    groupId = source + "_" + groupParts[2];
    NmeaGroupAssembly& group = m_groupPool[groupId];
    group.lastUpdate = std::time(0);
    group.totalLines = totalLines;
    group.lines[currentLine] = nmeaMessage;
    if (group.lines.size() != static_cast<std::map<int, std::string>::size_type>(totalLines)) return;

    for (line = 1; line <= totalLines; ++line) {
        if (group.lines.find(line) == group.lines.end()) return;
        if (line == 1) complete = group.lines[line];
        else {
            firstComma = group.lines[line].find(',');
            messageStar = group.lines[line].find('*');
            if (firstComma == std::string::npos || messageStar == std::string::npos || messageStar <= firstComma) return;
            complete.insert(complete.find('*'), group.lines[line].substr(firstComma, messageStar - firstComma));
        }
    }
    m_groupPool.erase(groupId);
    if (m_assembledCallback != 0) m_assembledCallback(m_callbackContext, complete, source);
}

void Nmea450Parser::CleanupTimeouts()
{
    const time_t now = std::time(0);
    std::map<std::string, NmeaGroupAssembly>::iterator it = m_groupPool.begin();
    while (it != m_groupPool.end()) {
        if (now - it->second.lastUpdate > 3) m_groupPool.erase(it++);
        else ++it;
    }
}

std::vector<std::string> Nmea450Parser::SplitStr(const std::string& value, char delimiter) const
{
    std::vector<std::string> result;
    std::string::size_type start = 0;
    std::string::size_type end;
    do {
        end = value.find(delimiter, start);
        result.push_back(value.substr(start, end == std::string::npos ? end : end - start));
        start = end + 1;
    } while (end != std::string::npos);
    return result;
}
