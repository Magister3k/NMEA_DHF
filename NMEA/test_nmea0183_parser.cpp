#include "nmea_decoder.h"
#include <iostream>

int main() {
    std::cout << "[TEST] ������������ �������������� ������ nmea_decoder..." << std::endl;

    NmeaMsgParser decoder;

    // �������� �� ������� ������ ����������
    decoder.SetOnHeaderParsed([](const NmeaHeaderInfo& header) {
        std::cout << "[COLLBACK HEADER] �������� (Talker): " << header.talker_id 
                  << " | ������������: " << header.msg_type << std::endl;
    });

    // �������� �� ��������� ������������� ��������� ($GPGGA)
    decoder.SetOnStandardMsg([](const std::string& talker, const std::string& type, const std::vector<std::string>& fields) {
        std::cout << "[CALLBACK TEXT NMEA] ������: " << talker << " | ���: " << type << std::endl;
        if (type == "GGA" && fields.size() >= 4) {
            std::cout << "   -> ������ ����� UTC: " << fields[0] << std::endl;
            std::cout << "   -> ������ (�����): " << fields[1] << " " << fields[2] << std::endl;
        }
    });

    // �������� �� ��� ������ (��������������� � ais_decoder)
    decoder.SetOnAisStringDetected([](const std::string& ais_payload) {
        std::cout << "[CALLBACK AIS STREAM] ������� �������� payload: " << ais_payload << std::endl;
    });

    // ��������� ������ ����� �� �����-���� �������� ��������
    std::cout << "\n--- ��������� ������ 1 (������ ��������� GPS) ---" << std::endl;
    decoder.ParseMsg("$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47");

    std::cout << "\n--- ��������� ������ 2 (����� ����� ���) ---" << std::endl;
    decoder.ParseMsg("!AIVDM,1,1,,A,133sVf0P00PDHRGOn7@Cw?vNP000,0*7F");

    std::cout << "\n--- ��������� ����� ������ (������ �������) ---" << std::endl;
    decoder.ParseMsg("$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*99"); 
    std::cout << "���������� ���������." << std::endl;

    return 0;
}
