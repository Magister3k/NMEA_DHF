#pragma once

#include <cstdint>
#include <string>

struct NmeaReport {
    std::string msg;
    std::string src;
    std::string dst;
    std::string header;
    std::string device_descr;
    std::string manuf_descr;
    std::string msg_type_descr;
    std::string comment;

    // NMEA time fields may not contain a date or timezone.  Preserve their
    // source representation instead of inventing an incorrect time_point.
    std::string timestamp;
    std::string object;
    double lat = 0.0;
    double lon = 0.0;
    double heading = 0.0;
    double speed = 0.0;
    double alt = 0.0;
    int quality_indicator = 0;
    double roll = 0.0;
    double pitch = 0.0;
    double easting_x = 0.0;
    double northing_y = 0.0;
    int utm_zone = 0;

    std::string ais_msg;
    std::string ais_msg_type_descr;
    uint32_t ais_mmsi = 0;
    std::string ais_nav_status_descr;
    std::string ais_device_type_descr;
    std::string ais_device_status_descr;
    std::string ais_pos_accuracy_descr;
    uint32_t ais_imo_num = 0;
    std::string ais_call_sign;
    std::string ais_ship_name;
    std::string ais_ship_type_descr;
    std::string ais_cargo_type_descr;
    std::string ais_dest_port;

    bool is_ais = false;
    bool has_pos = false;
    bool is_srv_time = false;
};
