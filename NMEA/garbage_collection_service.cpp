#include "nmea_service.h"
#include "nmea450_decoder.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#include <sys/resource.h>
#endif

nmea_service::nmea_service() {}

nmea_service::~nmea_service() {
    StopTimeoutCleaner();
}

void nmea_service::StartTimeoutCleaner(std::shared_ptr<Nmea450Decoder> net_meta_decoder) {
    std::lock_guard<std::mutex> lock(m_cv_mutex);
    if (m_cleaner_thread.joinable()) return; // Защита от повторного запуска потока

    // Сохраняем слабую/общую ссылку на прикладной декодер для его очистки
    m_net_meta_decoder = net_meta_decoder;
    m_shutdown_requested = false;
    
    m_cleaner_thread = std::thread(&nmea_service::CleanerWorker, this);
}

void nmea_service::StopTimeoutCleaner() {
    {
        std::lock_guard<std::mutex> lock(m_cv_mutex);
        m_shutdown_requested = true;
    }
    
    m_cv.notify_all(); // Мгновенно пробуждаем поток, если он находился в состоянии ожидания

    if (m_cleaner_thread.joinable()) {
        m_cleaner_thread.join();
    }
}

void nmea_service::CleanerWorker() {
    // 🧠 Сеньорское архитектурное решение: Адаптивное понижение приоритета фонового потока.
    // Гарантирует, что тяжелое сканирование хэш-таблиц никогда не создаст задержек (jitter)
    // для основных сетевых потоков, обрабатывающих прерывания сокетов на уровне ядра.
#ifdef _WIN32
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
#else
    // Для Linux (POSIX) выставляем значение nice (10 из 19, где 19 - самый низкий приоритет)
    setpriority(PRIO_PROCESS, 0, 10); 
#endif

    // Интервал утилизации мертвых сессий (каждые 2 секунды)
    const auto interval = std::chrono::seconds(2);

    while (!m_shutdown_requested.load()) {
        std::unique_lock<std::mutex> lock(m_cv_mutex);
        
        // Вместо блокирующего std::this_thread::sleep_for используется condition_variable.
        // Это предотвращает зависание процесса в памяти при экстренном закрытии программы.
        if (m_cv.wait_for(lock, interval, [this]() { return m_shutdown_requested.load(); })) {
            break; // Атомарный триггер сработал -> немедленно выходим из рабочего цикла
        }

        // ШАГ 1: Очистка таймаутов транспортного уровня L4 (базовый класс nmea_processor).
        // Мьютекс внутри CleanupTimeouts заблокируется на микросекунды для удаления мертвых TCP-сессий.
        CleanupTimeouts();

        // ШАГ 2: Агрегированная очистка таймаутов уровня сетевых метаданных L5 (nmea450_decoder).
        // Если указатель на декодер был передан при инициализации, чистим его многострочные пулы (g: теги).
        if (m_net_meta_decoder) {
            m_net_meta_decoder->CleanupTimeouts();
        }
    }
}
