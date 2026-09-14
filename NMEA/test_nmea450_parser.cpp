#include "nmea450_decoder.h"
#include <iostream>
#include <vector>

int main() {
    std::cout << "[TEST] ������������ �������������� ������ nmea450_decoder..." << std::endl;

    Nmea450Decoder decoder;

    // ������������� �� �������� ����� ��������� �����
    decoder.SetOnMsgAssembled([](const std::string& clean_msg, const std::string& src) {
        std::cout << "\n[CALLBACK SUCCESS] ��������� ������� �������!" << std::endl;
        std::cout << "   -> ��������: " << src << std::endl;
        std::cout << "   -> �������� NMEA ������: " << clean_msg << std::endl;
    });

    // 1. ���������� ����������� ���������� ������ (��������, �������)
    std::string single_packet = std::string("UdPbC\0\\c:1672531199,s:SD001*41\\$SDDPT,14.2,0.0*57", 49);
    std::cout << "\n--- ���������� ��������� ���������� ������� ---" << std::endl;
    decoder.ProcPacket(reinterpret_cast<const uint8_t*>(single_packet.data()), single_packet.length());

    // 2. ���������� ����������� �������������� ������ AIS (2 ���������, ��� 5)
    // ������ 1
    std::string packet_part1 = std::string("UdPbC\0\\c:1672531200,s:AI01,g:1-2-8888*1A\\!AIVDM,2,1,5,B,538S`v024hBl0D`G22000000000,0*3D", 84);
    // ������ 2
    std::string packet_part2 = std::string("UdPbC\0\\c:1672531200,s:AI01,g:2-2-8888*1B\\!AIVDM,2,2,5,B,0000000000000000000,0*3A", 81);

    std::cout << "\n--- ���������� �������� 1 �������������� ��������� ��� ---" << std::endl;
    decoder.ProcPacket(reinterpret_cast<const uint8_t*>(packet_part1.data()), packet_part1.length());
    std::cout << "[INFO] ����� ������������, ������� ���������� ������..." << std::endl;

    std::cout << "\n--- ���������� �������� 2 �������������� ��������� ��� ---" << std::endl;
    decoder.ProcPacket(reinterpret_cast<const uint8_t*>(packet_part2.data()), packet_part2.length());

    return 0;
}
