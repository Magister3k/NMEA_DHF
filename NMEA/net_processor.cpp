#include "nmea_processor.h"

constexpr size_t DDOS_PROTECTION_BUFFER_LIMIT = 4096; // 4Кб лимит на пачку без символа \n
constexpr auto TCP_INACTIVITY_TIMEOUT = std::chrono::minutes(5);

NmeaProcessor::NmeaProcessor() {}

void NmeaProcessor::SetOnRawDataChunkReady(RawDataChunkCallback cb) {
    m_raw_chunk_cb = cb;
}

void NmeaProcessor::ProcUdpDatagram(const uint8_t* udp_payload, size_t udp_len) {
    if (udp_len == 0 || !m_raw_chunk_cb) return;

    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Поскольку UDP по своей природе атомарен, мы просто мгновенно 
    // перенаправляем датаграмму на следующий уровень абстракции (L5/L7)
    m_raw_chunk_cb(udp_payload, udp_len);
}

void NmeaProcessor::ProcTcpSegment(uint32_t src_ip, uint32_t dst_ip, uint16_t src_port, uint16_t dst_port,
                                    const uint8_t* tcp_payload, size_t tcp_len) 
{
    if (tcp_len == 0 || !m_raw_chunk_cb) return;

    std::lock_guard<std::mutex> lock(m_mutex);

    TcpSessionKey key{ src_ip, dst_ip, src_port, dst_port };
    auto& session = m_tcp_pool[key];
    session.last_activity = std::chrono::steady_clock::now();

    // Записываем пришедший сегмент в конец скользящего буфера накопления
    session.data_accumulator.append(reinterpret_cast<const char*>(tcp_payload), tcp_len);

    // Вырезаем из байтового потока готовые строки по маркеру '\n'
    size_t newline_pos;
    while ((newline_pos = session.data_accumulator.find('\n')) != std::string::npos) {
        // Извлекаем законченный пакет данных (включая потенциальный \r в конце)
        std::string line = session.data_accumulator.substr(0, newline_pos + 1);
        session.data_accumulator.erase(0, newline_pos + 1);

        // Передаем сырую строку дальше по конвейеру обработки
        m_raw_chunk_cb(reinterpret_cast<const uint8_t*>(line.data()), line.length());
    }

    // 🛡️ Защита ядра: если в сокет гонят бесконечный мусор без разделителей строк, 
    // очищаем буфер сессии во избежание утечки и переполнения оперативной памяти кучи
    if (session.data_accumulator.length() > DDOS_PROTECTION_BUFFER_LIMIT) {
        session.data_accumulator.clear(); 
    }
}

void NmeaProcessor::TerminateTcpSession(uint32_t src_ip, uint32_t dst_ip, uint16_t src_port, uint16_t dst_port) {
    std::lock_guard<std::mutex> lock(m_mutex);
    TcpSessionKey key{ src_ip, dst_ip, src_port, dst_port };
    m_tcp_pool.erase(key);
}

void NmeaProcessor::CleanupTimeouts() {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto now = std::chrono::steady_clock::now();

    // Итерируемся по пулу сессий и вычищаем клиентов, оборвавших связь без FIN/RST пакетов
    for (auto it = m_tcp_pool.begin(); it != m_tcp_pool.end();) {
        if (now - it->second.last_activity > TCP_INACTIVITY_TIMEOUT) {
            it = m_tcp_pool.erase(it); // Эффективное удаление из unordered_map за O(1)
        } else {
            ++it;
        }
    }
}
