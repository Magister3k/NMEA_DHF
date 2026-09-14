#pragma once

#include <string>
#include <vector>
#include <functional>
#include "nmea_structures.h"

class GeoParser {
public:
    // ������ �������� ������ ��� ������ ���������� � ������-������ (Observer)
    using Callback = std::function<void(const NmeaReport&)>;

    GeoParser() = default;
    ~GeoParser() = default;

    // ������ ����������� � ����������� (RAII / ������ ��������� ������ ���� ����������)
    GeoParser(const GeoParser&) = delete;
    GeoParser& operator=(const GeoParser&) = delete;

    /**
     * @brief ����������� ��������� ����������� ��������������� ���-�������
     */
    void SetOnReport(Callback cb);

    /**
     * @brief ������� ���������� ����� ����� ���������� ��������� (L7)
     * @param header ������ �� ������������ ���� (��������, "$GPGGA")
     * @param fields ������ ���������������� ����� ���������
     * @param msg ������ �������� ����� ������ NMEA
     */
    void ParseInMsg(const std::string& header, const std::vector<std::string>& fields, const std::string& msg);

private:
    // ������ ����������� ������������ NMEA 0183
    void ParseStandard(const std::string& type, const std::vector<std::string>& f, NmeaReport& r);

    // ������ ���������� � ������� ������������� ���������� ($P...)
    void ParseProp(const std::string& h, const std::string& t, const std::vector<std::string>& f, NmeaReport& r);

    /**
     * @brief ������������ ������ � ����������� ������� ����� NMEA ��� heap-���������.
     *        ������������� ��� ��������� � ������������ -ffast-math.
     */
    double GeoToDec(const std::string& nmea, const std::string& hemi);

    /**
     * @brief �������� ������������ ������������� ������: ����� UTM -> ������� WGS-84
     *        ���������� �� ����� ������� 6-� ������� � ���������� ����������� FMA.
     */
    void ConvertUtmToWgs84(double easting, double northing, int zone, char hemisphere, NmeaReport& report);

    Callback m_cb = nullptr;
};
