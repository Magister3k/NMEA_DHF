#include "geo_msg_parser.h"

#include <cmath>
#include <string>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void GeoMsgParser::SetOnReport(Callback cb) { m_cb = cb; }

void GeoMsgParser::ParseInMsg(const std::string& header, const std::vector<std::string>& fields, const std::string& msg)
{
    if (header.length() < 4 || !m_cb) return;
    NmeaReport report;
    report.msg = msg;
    report.header = header.substr(0, 6);
    if (header == "$RATTRM" && fields.size() >= 2) {
        report.object = fields[1];
    } else if (header[1] == 'P' || header[1] == 'p') {
        ParseProp(header, header, fields, report);
    } else {
        ParseStandard(header.substr(3, 3), fields, report);
    }
    if (report.has_pos || !report.object.empty()) m_cb(report);
}

void GeoMsgParser::ParseStandard(const std::string& type, const std::vector<std::string>& f, NmeaReport& r) {
    r.is_ais = false;

    if (type == "GGA" && f.size() >= 10) {
        r.lat = GeoToDec(f[2], f[3]); r.lon = GeoToDec(f[4], f[5]);
        r.quality_indicator = std::stoi(f[6]); r.alt = std::stod(f[9]);
        r.has_pos = true; r.msg_type_descr = "GNSS Fix Data";
        r.timestamp = f[1]; // ����� UTC �� ������
    }
    else if (type == "GLL" && f.size() >= 7) {
        r.lat = GeoToDec(f[1], f[2]); r.lon = GeoToDec(f[3], f[4]);
        r.has_pos = true; r.msg_type_descr = "Geographic Position Lat/Lon";
        r.timestamp = f[5];
    }
    // �������� � ���� �� RMC ������
    else if (type == "RMC" && f.size() >= 9) {
        if (f[2] != "A") return;
        r.lat = GeoToDec(f[3], f[4]); r.lon = GeoToDec(f[5], f[6]);

        // �������������� ���������� ����������� � ��������
        r.speed = f[7].empty() ? 0.0 : std::stod(f[7]);      // �������� � �����
        r.heading = f[8].empty() ? 0.0 : std::stod(f[8]);  // ������� ���� � ��������
        r.has_pos = true; r.msg_type_descr = "Recommended Minimum GNSS Data";
        r.timestamp = f[1];
    }
    else if (type == "GNS" && f.size() >= 10) {
        r.lat = GeoToDec(f[2], f[3]); r.lon = GeoToDec(f[4], f[5]);
        r.alt = f[9].empty() ? 0.0 : std::stod(f[9]);
        r.has_pos = true; r.msg_type_descr = "Fix Storage GNSS Data";
        r.timestamp = f[1];
    }
    // �������� � ����������� ����� ������ (TLL/TMM)
    else if (type == "TLL" && f.size() >= 6) {
        r.object = f[1];
        r.lat = GeoToDec(f[2], f[3]); r.lon = GeoToDec(f[4], f[5]);
        r.has_pos = true; r.msg_type_descr = "Radar Target Latitude and Longitude";
    }
    else if (type == "TTM" && f.size() >= 2) {
        r.object = f[1];
        r.msg_type_descr = "Tracked Target Message";
    }
}

void GeoMsgParser::ParseProp(const std::string& h, const std::string& t, const std::vector<std::string>& f, NmeaReport& r) {
        // $PASHR,POS (Ashtech ������������ ����������: ����, ������, ����) [3]
        if (t == "$PASHR" && f.size() >= 11 && f[1] == "POS") {
            r.heading = std::stod(f[2]); r.roll = std::stod(f[4]); r.pitch = std::stod(f[5]);
            r.has_pos = true; r.manuf_descr = "Ashtech/Magellan";
        }
        // $PFEC,TLL (Furuno Target Latitude and Longitude - ������������� ���� Furuno)
        else if (t == "$PFEC" && f.size() >= 7 && f[1] == "TLL") {
            r.object = f[2]; r.lat = GeoToDec(f[3], f[4]); r.lon = GeoToDec(f[5], f[6]);
            r.has_pos = true; r.manuf_descr = "Furuno Electric";
        }
        // $PFUG,LLQ / $PFUGDP / $PFUGP,POS / $PFUGW (������� ������������� ���� Fugro)
        else if (h.rfind("$PFUG", 0) == 0 && f.size() >= 5) {
            r.manuf_descr = "Fugro Omnistar/G2";
            if (f[1] == "LLQ" || f[1] == "GDP") { // ����� ������� ������������� ������ Fugro
                r.easting_x = std::stod(f[2]); r.northing_y = std::stod(f[3]);
                r.utm_zone = std::stoi(f[4]);
                ConvertUtmToWgs84(r.easting_x, r.northing_y, r.utm_zone, 'N', r); // ������������ � WGS84
            } else if (f[1] == "POS") {
                r.lat = std::stod(f[2]); r.lon = std::stod(f[3]); r.has_pos = true;
            }
        }
        // $PTNL,GGK / PJK / VGK (Trimble RTK ������� ������� ����������������)
        else if (h.rfind("$PTNL", 0) == 0 && f.size() >= 7) {
            r.manuf_descr = "Trimble Navigation";
            if (f[1] == "PJK") { // ������� ��������� ����� Trimble
                r.easting_x = std::stod(f[2]); r.northing_y = std::stod(f[3]);
                ConvertUtmToWgs84(r.easting_x, r.northing_y, 36, 'N', r); // ������-����
            } else if (f[1] == "GGK") {
                r.lat = GeoToDec(f[3], f[4]); r.lon = GeoToDec(f[5], f[6]);
                r.quality_indicator = std::stoi(f[7]); r.has_pos = true;
            }
        }
        // $PLCICA,GGQ ��� $GPLQ,GGQ (Leica Geo-Systems RTK ������)
        else if ((t == "$PLCICA" || t == "$GPLQ") && f.size() >= 7) {
            r.lat = GeoToDec(f[2], f[3]); r.lon = GeoToDec(f[4], f[5]);
            r.quality_indicator = std::stoi(f[6]); r.has_pos = true;
            r.manuf_descr = "Leica Geosystems";
        }
        // $PGRMF (Garmin GPS Fix Data - ���������� �������� ��������� Garmin)
        else if (t == "$PGRMF" && f.size() >= 13) {
            r.lat = GeoToDec(f[6], f[7]); r.lon = GeoToDec(f[8], f[9]);
            r.heading = f[11].empty() ? 0.0 : std::stod(f[11]); r.has_pos = true;
            r.manuf_descr = "Garmin International";
        }
        // $PGHP / $PMRNT / $PMXMIT / $PMXS,CURSOR / $PRAYA / $PRAYB / $PRBY / $PSBG,GAZ / $BSADS
        // ������������� ��������/��������� ������ ��������, ������������� � ������� ����
        else if (t == "$PSBG" && f.size() >= 5 && f[1] == "GAZ") { // ������������� ����/������ IXBLUE/SBG
            r.roll = std::stod(f[2]); r.pitch = std::stod(f[3]); r.heading = std::stod(f[4]);
            r.has_pos = true; r.manuf_descr = "SBG Systems / IXBLUE";
        }
        else {
            // ��������-������� ��� ��������� ���������� ����� �������������
            // ($PMXS,CURSOR, $PRAYA/B Raytheon, $PRBY, $PMRNT, $BSADS BaseStation)
            r.manuf_descr = "Special/Defense System Array";
            r.msg_type_descr = "Object Identification / Target Tracking Trigger";
            r.object = f.size() > 1 ? f[1] : "RAW_TRACK";
            r.has_pos = false; // ��������� ������ ��������� �������������, ������� ���
            r.has_pos = true;  // ��������� ������ � ������-���� ��� ������ �������
        }
    }

double GeoMsgParser::GeoToDec(const std::string& nmea, const std::string& hemi) {
        if (nmea.empty() || hemi.empty()) return 0.0;
        size_t dot = nmea.find('.');
        if (dot == std::string::npos) return 0.0;

        // ������������ ������ �������� ����� ��� stod(sub_str) ���������
        double degrees = std::stod(nmea.substr(0, dot - 2));
        double minutes = std::stod(nmea.substr(dot - 2));
        double dec = degrees + (minutes / 60.0);
        if (hemi == "S" || hemi == "W") dec = -dec;
        return dec;
    }

void GeoMsgParser::ConvertUtmToWgs84(double easting, double northing, int zone, char hemisphere, NmeaReport& report) {
        const double a = 6378137.0; const double f = 1.0 / 298.257223563; const double k0 = 0.9996;
        const double e2 = 2.0 * f - f * f; const double ePrime2 = e2 / (1.0 - e2);
        if (hemisphere == 'S' || hemisphere == 's') northing -= 10000000.0;
        double x = easting - 500000.0; double y = northing;
        const double n = f / (2.0 - f); const double n2 = n * n;
        const double A = a * (1.0 + n2 / 4.0); const double M = y / k0; const double mu = M / A;
        const double beta1 = 3.0 * n / 2.0 - 27.0 * n * n2 / 32.0;
        const double phi1_rad = mu + beta1 * std::sin(2.0 * mu);
        const double sin_phi1 = std::sin(phi1_rad); const double cos_phi1 = std::cos(phi1_rad);
        const double tan_phi1 = std::tan(phi1_rad); const double tan_phi12 = tan_phi1 * tan_phi1;
        const double N1 = a / std::sqrt(1.0 - e2 * sin_phi1 * sin_phi1);
        const double R1 = a * (1.0 - e2) / std::pow(1.0 - e2 * sin_phi1 * sin_phi1, 1.5);
        const double D = x / (N1 * k0); const double D2 = D * D;
        const double C1 = ePrime2 * cos_phi1 * cos_phi1; const double T1 = tan_phi12;

        double lat_rad = phi1_rad - (N1 * tan_phi1 / R1) * (D2 / 2.0 - (5.0 + 3.0 * T1 + 10.0 * C1) * D2 * D2 / 24.0);
        double lon_diff_rad = (D - (1.0 + 2.0 * T1 + C1) * D2 * D / 6.0) / cos_phi1;
        const double lon0_rad = ((zone - 1) * 6.0 - 180.0 + 3.0) * M_PI / 180.0;

        report.lat = lat_rad * 180.0 / M_PI;
        report.lon = (lon0_rad + lon_diff_rad) * 180.0 / M_PI;
        report.has_pos = true;
    }
