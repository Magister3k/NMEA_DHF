#include "nmea_decoder.h"
#include <sstream>

void NmeaMsgParser::SetOnHeaderParsed(HeaderParsedCallback cb) { m_header_cb = cb; }
void NmeaMsgParser::SetOnStandardMsg(StandardMsgCallback cb) { m_standard_cb = cb; }
void NmeaMsgParser::SetOnAisMsg(AisMsgCallback cb) { m_ais_cb = cb; }

void NmeaMsgParser::ParseMsg(const std::string& msg) {
    // 1. ��������� ��������� ��������� � �����
    if (msg.length() < 6) return;
    if (msg[0] != '$' && msg[0] != '!') return;

    // 2. �������� ���������� ����������� ����� (XOR)
    if (!ValidateChecksum(msg)) return;

    // 3. �������� ���������� ��������� (Talker ID � Message Type)
    NmeaHeaderInfo header;
    header.msg = msg;
    header.talker_id = msg.substr(1, 2);   // ��������, "GP" ��� "AI"
    header.msg_type = msg.substr(3, 3); // ��������, "GGA" ��� "VDM"

    // ������� ������� ������� ��������� ��� �������� ��� ����
    if (m_header_cb) {
        m_header_cb(header);
    }

    // 4. �������� ����������� ����� � ����� ������ (���, ��� ����� '*' ������� ���� '*')
    size_t star_pos = msg.find('*');
    std::string body_without_checksum = msg.substr(0, star_pos);

    // 5. �����������: ��������� ������ �� ������� �� ������ �����
    std::vector<std::string> fields = SplitString(body_without_checksum, ',');

    // 6. ������������� �� ������ ���������
    if (msg[0] == '!') {
        // �������� ����������������� ������ (������� AIS: !AIVDM, !AIVDO)
        if ((header.msg_type == "VDM" || header.msg_type == "VDO") && fields.size() >= 7) {
            if (m_ais_cb) {
                // �������� ������ 5-� ����, ���������� 6-������ ������
                m_ais_cb(fields[5]);
            }
        }
    } else {
        // ����������� ��������� �������� (NMEA 0183: $GPGGA, $GPRMC, $HEHDT)
        if (m_standard_cb) {
            // �������� Talker ID, ��� � ������ ����� (������� � ������� 1, ��� ��� 0 � ��� ���������)
            std::vector<std::string> data_fields(fields.begin() + 1, fields.end());
            m_standard_cb(header.talker_id, header.msg_type, data_fields);
        }
    }
}

bool NmeaMsgParser::ValidateChecksum(const std::string& msg) const {
    size_t star = msg.find('*');
    if (star == std::string::npos || star + 3 > msg.length()) return false;

    uint8_t checksum = 0;
    // ������� XOR ���� �������� ������ ����� '$'/'!' � '*'
    for (size_t i = 1; i < star; ++i) {
        checksum ^= static_cast<uint8_t>(msg[i]);
    }

    std::string hex_str = msg.substr(star + 1, 2);
    try {
        unsigned long target_checksum = std::stoul(hex_str, nullptr, 16);
        return checksum == static_cast<uint8_t>(target_checksum);
    } catch (...) {
        return false;
    }
}

std::vector<std::string> NmeaMsgParser::SplitString(const std::string& str, char delimiter) const {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(str);
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    // ���� ������ ������������� ������������ (��������, ",,"), ��������� ������ ����� � �����
    if (!str.empty() && str.back() == delimiter) {
        tokens.push_back("");
    }
    return tokens;
}
