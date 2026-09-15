#pragma once
#include <string>
#include <vector>
#include <sstream>
#include <cmath>
#include "nmea_structures.h"

// ����������� ���������� ������ �������������� ��������� �������� AIS
class AisDecoder;

template <typename AisDecoder, typename GeoMsgParser>
class NmeaDecoder {
public:
    NmeaDecoder(AisDecoder& ais, GeoMsgParser& geo) : m_ais(ais), m_geo(geo) {}

    inline void ParseMsg(const std::string& msg) {
        if (msg.length() < 7) return;

        // �������� �� ���� �� ������� "in-place" ��� ���������
        std::vector<std::string> fields = SplitStr(msg, ',');
        if (fields.empty()) return;

        const std::string& header = fields[0]; // $GPGGA, !AIVDM, $PASHR � �.�.

        // 1. ��������������� ������������������ ��������� ������� (!)
        if (msg[0] == '!') {
            if (header.length() < 6) return;
            std::string type = header.substr(3, 3); // VDM ��� VDO
            
            // ������������ !AIVDM, !AIVDO, !B2VDM, !B2VDO, !BSVDM, !BSVDO
            if ((type == "VDM" || type == "VDO") && fields.size() >= 6) {
                // �������� �������� AIS ������ � 5-� ���� (������ 5)
                m_ais.DecodePayload(fields[5]); 
            }
            return;
        }

        // 2. ��������������� ������������ � �������������� ���-������� ($)
        if (msg[0] == '$') {
            m_geo.ParseInMsg(header, fields, msg);
        }
    }

private:
    std::vector<std::string> SplitStr(const std::string& str, char delimiter) const {
        std::vector<std::string> tokens;
        std::string token;
        std::istringstream tokenStream(str);
        while (std::getline(tokenStream, token, delimiter)) {
            tokens.push_back(token);
        }
        if (!str.empty() && str.back() == delimiter) tokens.push_back("");
        return tokens;
    }

    AisDecoder& m_ais;
    GeoMsgParser& m_geo;
};
