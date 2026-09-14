#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <functional>

// Структура уникальной идентификации TCP-сессии (IP:Port -> IP:Port)
struct TcpSessionKey {
    uint32_t src_ip;
    uint32_t dst_ip;
    uint16_t src_port;
    uint16_t dst_port;

    bool operator==(const TcpSessionKey& other) const {
        return src_ip == other.src_ip && dst_ip == other.dst_ip && 
               src_port == other.src_port && dst_port == other.dst_port;
    }
};

namespace std {
    template <> struct hash<TcpSessionKey> {
        size_t operator()(const TcpSessionKey& k) const {
            return ((hash<uint32_t>()(k.src_ip) ^ (hash<uint32_t>()(k.dst_ip) << 1)) ^ 
                    (hash<uint16_t>()(k.src_port) << 2)) ^ hash<uint16_t>()(k.dst_port);
        }
    };
}

// Скользящий кольцевой буфер накопления для TCP-сессий
struct TcpStreamBuffer {
    std::string data_accumulator;
    std::chrono::steady_clock::time_point last_activity;
};

class NmeaProcessor {
public:
    // Сигнатуры колбэков для трансляции сырых пакетов на следующий уровень (nmea450_decoder)
    using RawDataChunkCallback = std::function<void(const uint8_t* payload, size_t len)>;

    NmeaProcessor();
    virtual ~NmeaProcessor() = default;

    // Запрет копирования семантики (RAII / Безопасность многопоточности)
    NmeaProcessor(const NmeaProcessor&) = delete;
    NmeaProcessor& operator=(const NmeaProcessor&) = delete;

    // Регистрация конвейерного обработчика данных
    void SetOnRawDataChunkReady(RawDataChunkCallback cb);

    /**
     * @brief Прием монолитной UDP-датаграммы (после внешней IP-дефрагментации)
     */
    void ProcUdpDatagram(const uint8_t* udp_payload, size_t udp_len);

    /**
     * @brief Прием куска байтового потока из сетевого TCP-сокета
     */
    void ProcTcpSegment(uint32_t src_ip, uint32_t dst_ip, uint16_t src_port, uint16_t dst_port,
                        const uint8_t* tcp_payload, size_t tcp_len);

    /**
     * @brief Принудительное закрытие сессии TCP при разрыве соединения (FIN/RST пакеты)
     */
    void TerminateTcpSession(uint32_t src_ip, uint32_t dst_ip, uint16_t src_port, uint16_t dst_port);

    /**
     * @brief Сканирование и очистка зависших в памяти пустых/мертвых TCP-сессий
     */
    void CleanupTimeouts();

private:
    std::mutex m_mutex;
    RawDataChunkCallback m_raw_chunk_cb = nullptr;
    std::unordered_map<TcpSessionKey, TcpStreamBuffer> m_tcp_pool;
};
