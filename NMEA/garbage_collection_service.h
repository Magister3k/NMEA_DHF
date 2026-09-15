#pragma once

#include "net_processor.h"
#include <thread>
#include <atomic>
#include <condition_variable>
#include <memory>

// Опережающее объявление, чтобы не раздувать инклуды в заголовке
class Nmea450Parser;

class GcService : public NetProcessor {
public:
    GcService();
    ~GcService();

    // Запрет копирования семантики (RAII-поток должен быть уникальным)
    GcService(const GcService&) = delete;
    GcService& operator=(const GcService&) = delete;

    /**
     * @brief Запуск фонового низкоприоритетного потока очистки таймаутов
     * @param net_meta_decoder Указатель на декодер L5, чьи таймауты сборки предложений (g:) тоже нужно чистить
     */
    void StartTimeoutCleaner(std::shared_ptr<Nmea450Parser> net_meta_decoder = nullptr);
    
    /**
     * @brief Принудительный останов фонового потока (вызывается также автоматически в деструкторе)
     */
    void StopTimeoutCleaner();

private:
    // Фоновое рабочее тело потока
    void CleanerWorker();

    std::thread m_cleaner_thread;
    std::atomic<bool> m_shutdown_requested{false};
    
    std::mutex m_cv_mutex;
    std::condition_variable m_cv;

    // Слабая ссылка на декодер NMEA-450 для безопасной очистки его пула из фонового потока
    std::shared_ptr<Nmea450Parser> m_net_meta_decoder = nullptr;
};
