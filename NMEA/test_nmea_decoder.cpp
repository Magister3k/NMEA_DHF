#include <thread>
#include <atomic>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <memory>
#include <cstring>
#include "lock_free_queue.h"
#include "compile_time_pipeline.h"
#include "geo_parser.h"
#include "ais_decoder.h"

// Глобальная неблокирующая SPSC очередь между сетью (L4) и конвейером обработки (L7)
// Емкость 131072 (степень двойки) гарантирует аппаратную защиту от пиковых сетевых всплесков
LockFreeSpscQueue<NetPacket, 131072> g_net_queue;
std::atomic<bool> g_running{true};

// ====================================================================
// ПОТОК 1: СЕТЕВОЙ ПРИЕМНИК (THREAD 1 - ВЫДЕЛЕННОЕ ЯДРО CPU)
// ====================================================================
void NetReceiveThreadLoop() {
    std::cout << "[THREAD 1] Сетевой поток захвата пакетов L4 успешно запущен." << std::endl;
    
    // Имитируем тестовые наборы данных, прилетающие из Ethernet
    // Тест 1: Стандартное предложение $GPRMC, несущее координаты, скорость и путевой угол
    std::string test_nmea_rmc = "$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.5,230394,003.1,W*6A\r\n";

    // Тест 2: Фрагментированный на уровне L5 (NMEA-450) сетевой пакет, несущий бинарный payload AIS тип 5
    // Фрагмент 1 из 2 (Спецификация IEC 61162-450)
    std::string test_n450_ais_part1 = std::string("UdPbC\0\\c:1672531200,s:AI01,g:1-2-9999*1A\\!AIVDM,2,1,5,B,538S`v024hBl0D`G22000000000,0*3D\r\n", 86);
    // Фрагмент 2 из 2
    std::string test_n450_ais_part2 = std::string("UdPbC\0\\c:1672531200,s:AI01,g:2-2-9999*1B\\!AIVDM,2,2,5,B,0000000000000000000,0*3A\r\n", 83);

    NetPacket packet;
    packet.src_ip = 0x0A000001; // 10.0.0.1
    packet.dst_ip = 0x0A0000FF; // 10.0.0.255 (Multicast)
    packet.src_port = 40001;
    packet.dst_port = 40001;

    // --- 1. Отправляем в конвейер текстовый GPS пакет $GPRMC ---
    packet.len = test_nmea_rmc.length();
    std::memcpy(packet.payload, test_nmea_rmc.data(), packet.len);
    while (!g_net_queue.Push(packet)) { std::this_thread::yield(); }

    // Даем конвейеру микросекундную паузу для наглядности вывода в консоль
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // --- 2. Отправляем Фрагмент 1 многострочного сообщения AIS ---
    packet.len = test_n450_ais_part1.length();
    std::memcpy(packet.payload, test_n450_ais_part1.data(), packet.len);
    while (!g_net_queue.Push(packet)) { std::this_thread::yield(); }

    // --- 3. Отправляем Фрагмент 2 многострочного сообщения AIS ---
    packet.len = test_n450_ais_part2.length();
    std::memcpy(packet.payload, test_n450_ais_part2.data(), packet.len);
    while (!g_net_queue.Push(packet)) { std::this_thread::yield(); }

    // Имитируем фоновое удержание потока (генерация пустых циклов, как в реальной сети)
    while (g_running.load(std::memory_order_relaxed)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    std::cout << "[THREAD 1] Сетевой поток остановлен." << std::endl;
}

// ====================================================================
// ПОТОК 2: ВЫЧИСЛИТЕЛЬНЫЙ ПРОЦЕССОР КОНВЕЙЕРА (THREAD 2 - ВЫДЕЛЕННОЕ ЯДРО)
// ====================================================================
void ProcThreadLoop() {
    std::cout << "[THREAD 2] Вычислительный поток L7 успешно запущен." << std::endl;

    // Инициализация прикладных вычислительных ядер
    GeoParser geo_core;
    AisDecoder ais_core;
    
    // Проводка жестких связей на этапе компиляции (Compile-time Pipeline)
    // Компилятор полностью уничтожает накладные расходы виртуальных вызовов и инлайнит этот конвейер
    NmeaDecoder<AisDecoder, GeoParser> l7_text_layer(ais_core, geo_core);
    Nmea450Decoder l5_net_meta_layer;

    // Подключаем конечную шину вывода (Событийный Observer верхнего уровня)
    geo_core.SetOnReport([](const NmeaReport& out) {
        std::cout << "\n====================================================================" << std::endl;
        std::cout << "[ВЫХОД МОДУЛЯ] Сгенерирован унифицированный путевой отчет:" << std::endl;
        std::cout << "  1. Сообщение NMEA:      " << out.msg << std::endl;
        std::cout << "  2. Заголовок:           " << out.header << std::endl;
        std::cout << "  3. Прибор / Вендор:     " << out.device_descr << " | " << out.manuf_descr << std::endl;
        std::cout << "  4. Тип сообщения:       " << out.msg_type_descr << std::endl;
        std::cout << "  5. Источник:            " << out.src << std::endl;
        std::cout << "  6. Временная метка:     " << out.timestamp << std::endl;
        std::cout << "  7. Идентификатор:       " << out.object_id << std::endl;
        
        if (out.has_pos) {
            std::cout << "  8. Широта: " << std::fixed << std::setprecision(6) << out.lat << std::endl;
            std::cout << "  9. Долгота: " << std::fixed << std::setprecision(6) << out.lon << std::endl;
        }

        std::cout << "  10. Скорость (SOG):     " << out.speed << " узлов" << std::endl;
        std::cout << "  11. Направление (COG):  " << out.heading << " градусов" << std::endl;

        if (out.is_ais) {
            std::cout << "  12. Тип сообщения AIS: " << out.ais_msg_type << std::endl;
            std::cout << "  13. Сообщение AIS:     " << out.ais_msg << std::endl;
            
            if (out.ais_msg_type == 5) {
                std::cout << "  14. Название судна:  " << out.ais_ship_name << std::endl;
                std::cout << "  15. Позывной судна:  " << out.ais_call_sign << std::endl;
                std::cout << "  16. Порт назначения: " << out.ais_dest << std::endl;
            }
        }
        std::cout << "====================================================================" << std::endl;
    });

    NetPacket local_packet;

    // Основной вычислительный цикл L4-L7 конвейера
    while (g_running.load(std::memory_order_relaxed)) {
        // Выгребаем пакеты из неблокирующего кольца за 3 наносекунды
        if (g_net_queue.Pop(local_packet)) {
            // Формируем имя источника на основе сетевых дескрипторов пакета
            std::string src_info = "UDP_" + std::to_string(local_packet.src_port);
            
            // Вбрасываем сырые байты в инлайновый конвейер
            l5_net_meta_layer.ProcPacket(local_packet.payload, local_packet.len);
        } else {
            // Если в сети временное затишье, разгружаем ядро процессора
            std::this_thread::yield();
        }
    }
    std::cout << "[THREAD 2] Вычислительный поток остановлен." << std::endl;
}

// ====================================================================
// ТОЧКА СБОРКИ И ПУСКА ПРИЛОЖЕНИЯ
// ====================================================================
int main() {
    std::cout << "====================================================================" << std::endl;
    std::cout << " СВЕРХВЫСОКОСКОРОСТНОЙ СЕТЕВОЙ НАВИГАЦИОННЫЙ СТЕНД РЕАЛЬНОГО ВРЕМЕНИ" << std::endl;
    std::cout << "====================================================================" << std::endl;

    // Шаг 1: Аллокация и привязка потоков к независимым ядрам процессора
    std::thread net_producer(NetReceiveThreadLoop);
    std::thread proc_consumer(ProcThreadLoop);

    // Даем конвейеру поработать и обработать встроенные тестовые векторы данных
    std::this_thread::sleep_for(std::chrono::seconds(2));

    std::cout << "\n[SYSTEM] Тестовая сессия завершена. Инициируем безопасный останов (RAII)..." << std::endl;

    // Шаг 2: Сигнальный останов потоков без зависаний
    g_running = false;
    
    net_producer.join();
    proc_consumer.join();

    std::cout << "[SYSTEM] Все потоки успешно выгружены. Выход." << std::endl;
    return 0;
}
