#pragma once
#include <chrono>
#include <cstdint>
#include <string>

struct MsgTimestamp {
    // Единая точка времени для всей бизнес-логики и картографии
    std::chrono::system_clock::time_point utc_time;
    
    // Быстрый экспорт в число (Unix Timestamp в секундах)
    [[nodiscard]] int64_t to_unix_sec() const noexcept {
        return std::chrono::duration_cast<std::chrono::seconds>(
            utc_time.time_since_epoch()
        ).count();
    }
};

struct NmeaReport {
    // Базовый контур
    std::string msg;                      // Сообщение NMEA
    std::string src;                      // Источник сообщения (из NMEA-450 тега \s:)
    std::string dst;                      // Получатель сообщения (из NMEA-450 тега \d:)
    std::string header;                   // 5-буквенный заголовок с 1-м символом (например, "$GPRMC", "!AIVDM")
    std::string device_descr;             // Talker ID (текстовое описание из файла)
    std::string manuf_descr;              // Производитель (текстовое описание из файла)
    std::string msg_type_descr;           // Тип сообщения (текстовое описание из файла)
    std::string comment;                  // Комментарий (из NMEA-450 тега \t:)
    MsgTimestamp timestamp;               // Временная метка сообщения (из NMEA-450 тега \c:, полей времени сообщений NMEA и AIS)
    double      lat = 0.0;                // Широта в десятичных градусах (WGS-84)
    double      lon = 0.0;                // Долгота в десятичных градусах (WGS-84)
    double      heading = 0.0;            // Направление в градусах
    double      speed = 0.0;              // Скорость в узлах
    
    // Параметры сообщения AIS
    std::string ais_msg;                  // Сообщение AIS (66-битный ASCII код)
    std::string ais_msg_type_descr        // Тип сообщения
    uint32_t    ais_mmsi = 0;             // MMSI
    std::string ais_nav_status_descr;     // Навигационный статус (на ходу, на якоре и др.)
    std::string ais_device_type_descr;    // Тип навигационного оборудования (GPS, GLONASS и др.)
    std::string ais_device_status_descr;  // Статус навигационного оборудования (специальные значения временной метки UTC)
    std::string ais_pos_accuracy_descr;   // Точность позиционирования (высокая, низкая)
    // Рейсовые данные (заполняются только для сообщения типов 5 и 24)
    uint32_t    ais_imo_num = 0;          // Номер IMO
    std::string ais_call_sign;            // Позывной судна
    std::string ais_ship_name;            // Название судна
    std::string ais_ship_type_descr;      // Тип судна
    std::string ais_cargo_type_descr;     // Характер перевозимого груза
    std::string ais_dest_port;            // Порт назначения

    // Флаги
    bool        is_ais = false;           // Флаг наличия AIS-трафика
    bool        has_pos = false;          // Флаг наличия координат
    bool        is_srv_time = false;      // Флаг использования времени сервера
};
