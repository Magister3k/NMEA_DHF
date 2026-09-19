cmake_minimum_required(VERSION 3.15)
project(NmeaDecoder CXX)

# The production module is intentionally limited to the C++98 language level
# supported by the classic Borland C++Builder 6 compiler.
set(CMAKE_CXX_STANDARD 98)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(WIN32)
    add_library(nmea_decoder SHARED
        nmea_decoder.cpp
        NMEA/nmea0183_parser.cpp
        NMEA/nmea450_parser.cpp
    )

    target_include_directories(nmea_decoder PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/NMEA
        ${CMAKE_CURRENT_SOURCE_DIR}/DHF/_example/include
    )

    set_target_properties(nmea_decoder PROPERTIES
        OUTPUT_NAME "NmeaDecoderExt"
    )
endif()
