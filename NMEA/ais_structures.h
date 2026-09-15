#pragma once
#include <string>
#include <cstdint>

/**
 * @brief Промежуточная структура динамических данных позиционирования судна AIS
 *        Применяется для декодирования сообщений типов 1, 2, 3 (Class A Position Report),
 *        типа 4 (Base Station), типов 18/19 (Class B) и типа 27 (Satellite Long-Range).
 */
struct AisPosReport {
    std::string msg;                // Сообщение AIS (6-битный ASCII код)
    int         msg_type = 0;       // Тип сообщения
    uint32_t    mmsi = 0;           // MMSI
    int         nav_status = 15;    // Навигационный статус (на ходу, на якоре и др.)
    int         device_type = 0;    // Тип навигационного оборудования (GPS, GLONASS и др.)
    int         device_status = 0;  // Статус навигационного оборудования (специальные значения временной метки UTC)
    int         pos_accuracy = 0;   // Точность позиционирования (высокая, низкая)
    double      lon = 0.0;          // Долгота в десятичных градусах (WGS-84)
    double      lat = 0.0;          // Широта в десятичных градусах (WGS-84)
    double      cog = 0.0;          // Курс (Course Over Ground) в градусах
    double      sog = 0.0;          // Скорость (Speed Over Ground) в узлах
};

/**
 * @brief Промежуточная структура статических и рейсовых данных судна AIS
 *        Применяется для декодирования тяжелых сообщений типа 5 (Static and Voyage Related Data)
 *        и типа 24 (Class B Static Data Report),
 *        которые обычно приходят фрагментированными на уровне L5 (NMEA-450).
 */
struct AisDataReport {
    std::string msg;                // Сообщение AIS (6-битный ASCII код)
    int         msg_type = 0;       // Тип сообщения
    uint32_t    mmsi = 0;           // MMSI
    uint32_t    imo_num = 0;        // Номер IMO
    std::string call_sign;          // Позывной судна
    std::string ship_name;          // Название судна
    int         ship_type = 0;      // Тип судна
    int         cargo_type = 0;     // Характер перевозимого груза
    int         device_type = 0;    // Тип навигационного оборудования (GPS, GLONASS и др.)
    std::string dest_port;          // Порт назначения
};
