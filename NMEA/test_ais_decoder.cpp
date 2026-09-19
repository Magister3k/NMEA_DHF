#include "ais_decoder.h"
#include <iostream>
#include <cassert>

int main() {
    std::cout << "[TEST] ������ ����-������������ �������������� ������ ais_decoder..." << std::endl;

    // ������� ��������� ������� ��������
    AisDecoder decoder;

    bool pos_test_passed = false;
    bool static_test_passed = false;

    // 1. ������������� �� ������� ������������� ������� �����
    decoder.SetOnPosReport([&](const AisPosReport& report) {
        std::cout << "-> ������� ����� � ������� (��� " << report.msg_type << ")" << std::endl;
        std::cout << "   MMSI: " << report.mmsi << std::endl;
        std::cout << "   ������: " << report.lat << " | �������: " << report.lon << std::endl;
        std::cout << "   �������� (SOG): " << report.sog << " �����" << std::endl;
        
        // ����������� ������ ����������� ��������� ������
        if (report.mmsi == 802534) {
            pos_test_passed = true;
        }
    });

    // 2. ������������� �� ������� ����������� ������ �����
    decoder.SetOnDataReport([&](const AisDataReport& report) {
        std::cout << "-> �������� ����������� ������ ����� (��� " << report.msg_type << ")" << std::endl;
        std::cout << "   ��������: " << report.ship_name << " | ��������: " << report.call_sign << std::endl;
        std::cout << "   IMO �����: " << report.imo_num << std::endl;
        std::cout << "   ����������: " << report.dest_port << std::endl;

        if (report.mmsi == 244670321) {
            static_test_passed = true;
        }
    });

    // --- ���� 1: �������� �������� ��������� ��� 1 (Class A Position Report) ---
    // �������� ������: !AIVDM,1,1,,A,133sVf0P00PDHRGOn7@Cw?vNP000,0*7F
    std::string test_payload_pos = "133sVf0P00PDHRGOn7@Cw?vNP000";
    std::cout << "\n[������ ����� 1: ������������� ���������]" << std::endl;
    decoder.DecodePayload(test_payload_pos);

    // --- ���� 2: �������� �������� ��������� ��� 5 (Static Data) ---
    // � �������� ������� ��� ������ ���������� ��������� �� ���� ���������� � ������ nmea450_parser
    std::string test_payload_static = "538S`v024hBl0D`G220000000000000000000016000000000000000"; 
    std::cout << "\n[������ ����� 2: ������������� �������� � �����]" << std::endl;
    decoder.DecodePayload(test_payload_static);

    // �������� �������� �����������
    std::cout << "\n==============================================" << std::endl;
    if (pos_test_passed) {
        std::cout << "[����] ���� 1 (�������): ������� �������." << std::endl;
    } else {
        std::cerr << "[����] ���� 1 (�������): ����!" << std::endl;
    }
    
    std::cout << "==============================================" << std::endl;
    return (pos_test_passed) ? 0 : 1;
}
