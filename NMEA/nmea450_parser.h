#pragma once

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <chrono>

// ��������� ��� Sentence Grouping (������������� ��������� NMEA-450)
struct NmeaGroupAssembly {
    int total_lines = 0;
    std::chrono::steady_clock::time_point timestamp;
    std::map<int, std::string> lines; // ����� ������ -> ���� NMEA
};

class Nmea450Parser {
public:
    // ������� ��� �������� ���������� ������ � ��������� ������ (nmea0183_parser)
    using MsgAssembledCallback = std::function<void(const std::string& clean_msg, const std::string& src)>;

    Nmea450Parser() = default;
    ~Nmea450Parser() = default;

    // ������ �����������
    Nmea450Parser(const Nmea450Parser&) = delete;
    Nmea450Parser& operator=(const Nmea450Parser&) = delete;

    // ����������� �������
    void SetOnMsgAssembled(MsgAssembledCallback cb);

    /**
     * @brief ��������� ����� ������, ����������� �� ������������� ������.
     * @param payload ��������� �� ������ �������� ��������
     * @param len ������ �������� ��������
     */
    void ProcPacket(const uint8_t* payload, size_t len);

    /**
     * @brief ����� ������� ������������� ������������� ������� �� ��������.
    *        ���������� ������� ������������� (garbage_collection_service).
     */
    void CleanupTimeouts();

private:
    void HandleTagBlock(const std::string& tag_block, const std::string& nmea_msg);
    std::vector<std::string> SplitStr(const std::string& str, char delimiter) const;

    MsgAssembledCallback m_assembled_cb = nullptr;
    std::map<std::string, NmeaGroupAssembly> m_nmea_group_pool;
};
