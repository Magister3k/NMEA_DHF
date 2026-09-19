#include "ais_decoder.h"

#include <algorithm>
#include <sstream>

namespace {
std::string DescribeAisMessageType(int type)
{
    switch (type) {
    case 1: case 2: case 3: return "Position Report Class A";
    case 4: return "Base Station Report";
    case 5: return "Static and Voyage Related Data";
    case 18: return "Standard Class B Position Report";
    case 19: return "Extended Class B Position Report";
    case 24: return "Class B Static Data Report";
    case 27: return "Long-Range AIS Broadcast Message";
    default: { std::ostringstream value; value << "AIS message type " << type; return value.str(); }
    }
}
}

void AisDecoder::SetOnPosReport(PosCallback cb) { m_pos_cb = cb; }
void AisDecoder::SetOnDataReport(DataCallback cb) { m_data_cb = cb; }
void AisDecoder::SetOnNmeaReport(NmeaReportCallback cb) { m_nmea_cb = cb; }

void AisDecoder::DecodePayload(const std::string& aisPayload)
{
    std::vector<uint8_t> bits = ConvertNmeaToSixBit(aisPayload);
    if (bits.size() * 6 < 38) return;

    const int type = static_cast<int>(FetchBits(bits, 0, 6));
    const uint32_t mmsi = FetchBits(bits, 6, 24);
    if ((type == 1 || type == 2 || type == 3) && bits.size() * 6 >= 168) {
        AisPosReport report;
        report.msg = aisPayload; report.msg_type = type; report.mmsi = mmsi;
        report.nav_status = static_cast<int>(FetchBits(bits, 30, 4));
        const uint32_t rawSog = FetchBits(bits, 42, 8);
        report.sog = rawSog == 1023 ? 0.0 : rawSog / 10.0;
        int32_t rawLon = static_cast<int32_t>(FetchBits(bits, 61, 28)); if (rawLon & 0x08000000) rawLon |= 0xF0000000;
        int32_t rawLat = static_cast<int32_t>(FetchBits(bits, 89, 27)); if (rawLat & 0x04000000) rawLat |= 0xF8000000;
        report.lon = DecodeAisLon(rawLon); report.lat = DecodeAisLat(rawLat);
        const uint32_t rawCog = FetchBits(bits, 116, 12); report.cog = rawCog == 3600 ? 0.0 : rawCog / 10.0;
        if (m_pos_cb) m_pos_cb(report);
        EmitNmeaReport(report);
    } else if (type == 18 || type == 19 || type == 27) {
        AisPosReport report;
        report.msg = aisPayload; report.msg_type = type; report.mmsi = mmsi;
        if (type == 27 && bits.size() * 6 >= 96) {
            const uint32_t rawSog = FetchBits(bits, 43, 6); report.sog = rawSog >= 63 ? 0.0 : rawSog;
            int32_t rawLon = static_cast<int32_t>(FetchBits(bits, 49, 18)); if (rawLon & 0x00020000) rawLon |= 0xFFFC0000;
            int32_t rawLat = static_cast<int32_t>(FetchBits(bits, 67, 17)); if (rawLat & 0x00010000) rawLat |= 0xFFFE0000;
            report.lon = rawLon == 0x1A838 ? 0.0 : rawLon / 60.0; report.lat = rawLat == 0xD548 ? 0.0 : rawLat / 60.0;
            const uint32_t rawCog = FetchBits(bits, 84, 9); report.cog = rawCog >= 511 ? 0.0 : rawCog;
        }
        if (m_pos_cb) m_pos_cb(report);
        EmitNmeaReport(report);
    } else if ((type == 5 && bits.size() * 6 >= 420) || type == 24) {
        AisDataReport report;
        report.msg = aisPayload; report.msg_type = type; report.mmsi = mmsi;
        if (type == 5) {
            report.imo_num = FetchBits(bits, 40, 30);
            report.call_sign = DecodeAisText(bits, 70, 7); report.ship_name = DecodeAisText(bits, 112, 20);
            report.ship_type = static_cast<int>(FetchBits(bits, 232, 8)); report.dest_port = DecodeAisText(bits, 302, 20);
        }
        if (m_data_cb) m_data_cb(report);
        EmitNmeaReport(report);
    }
}

std::string AisDecoder::DecodeAisText(const std::vector<uint8_t>& bitStream, size_t startBit, size_t numChars) const
{
    std::string text;
    for (size_t i = 0; i < numChars; ++i) { uint32_t value = FetchBits(bitStream, startBit + i * 6, 6); char c = value > 0 && value < 32 ? static_cast<char>(value + 64) : static_cast<char>(value); text += c == '@' ? ' ' : c; }
    while (!text.empty() && text.back() == ' ') text.pop_back();
    return text;
}
std::vector<uint8_t> AisDecoder::ConvertNmeaToSixBit(const std::string& payload) const
{
    std::vector<uint8_t> result; result.reserve(payload.size());
    for (char c : payload) { uint8_t value = static_cast<uint8_t>(c); if (value < 48 || value > 119 || (value > 87 && value < 96)) return std::vector<uint8_t>(); value -= 48; if (value > 40) value -= 8; result.push_back(value); }
    return result;
}
uint32_t AisDecoder::FetchBits(const std::vector<uint8_t>& bits, size_t startBit, size_t count) const { uint32_t result = 0; for (size_t i = 0; i < count; ++i) { const size_t index = (startBit + i) / 6; if (index >= bits.size()) return 0; result = (result << 1) | ((bits[index] >> (5 - (startBit + i) % 6)) & 1); } return result; }
double AisDecoder::DecodeAisLon(int32_t value) const { return value == 0x6791AC0 ? 0.0 : value / 600000.0; }
double AisDecoder::DecodeAisLat(int32_t value) const { return value == 0x3412140 ? 0.0 : value / 600000.0; }

void AisDecoder::EmitNmeaReport(const AisPosReport& source)
{
    if (!m_nmea_cb) return;
    NmeaReport report; report.is_ais = true; report.has_pos = true; report.ais_msg = source.msg; report.ais_mmsi = source.mmsi; report.ais_msg_type_descr = DescribeAisMessageType(source.msg_type); report.lat = source.lat; report.lon = source.lon; report.speed = source.sog; report.heading = source.cog; m_nmea_cb(report);
}
void AisDecoder::EmitNmeaReport(const AisDataReport& source)
{
    if (!m_nmea_cb) return;
    NmeaReport report; report.is_ais = true; report.ais_msg = source.msg; report.ais_mmsi = source.mmsi; report.ais_imo_num = source.imo_num; report.ais_msg_type_descr = DescribeAisMessageType(source.msg_type); report.ais_call_sign = source.call_sign; report.ais_ship_name = source.ship_name; report.ais_dest_port = source.dest_port; m_nmea_cb(report);
}
