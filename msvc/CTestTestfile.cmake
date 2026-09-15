# CMake generated Testfile for 
# Source directory: D:/Projects/C++/NMEA_DHF
# Build directory: D:/Projects/C++/NMEA_DHF/msvc
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
if(CTEST_CONFIGURATION_TYPE MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test(GeodeticMathVerification "D:/Projects/C++/NMEA_DHF/msvc/msvc/release/test_math.exe")
  set_tests_properties(GeodeticMathVerification PROPERTIES  _BACKTRACE_TRIPLES "D:/Projects/C++/NMEA_DHF/CMakeLists.txt;107;add_test;D:/Projects/C++/NMEA_DHF/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
  add_test(GeodeticMathVerification "D:/Projects/C++/NMEA_DHF/msvc/msvc/release/test_math.exe")
  set_tests_properties(GeodeticMathVerification PROPERTIES  _BACKTRACE_TRIPLES "D:/Projects/C++/NMEA_DHF/CMakeLists.txt;107;add_test;D:/Projects/C++/NMEA_DHF/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
  add_test(GeodeticMathVerification "D:/Projects/C++/NMEA_DHF/msvc/msvc/release/test_math.exe")
  set_tests_properties(GeodeticMathVerification PROPERTIES  _BACKTRACE_TRIPLES "D:/Projects/C++/NMEA_DHF/CMakeLists.txt;107;add_test;D:/Projects/C++/NMEA_DHF/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
  add_test(GeodeticMathVerification "D:/Projects/C++/NMEA_DHF/msvc/msvc/release/test_math.exe")
  set_tests_properties(GeodeticMathVerification PROPERTIES  _BACKTRACE_TRIPLES "D:/Projects/C++/NMEA_DHF/CMakeLists.txt;107;add_test;D:/Projects/C++/NMEA_DHF/CMakeLists.txt;0;")
else()
  add_test(GeodeticMathVerification NOT_AVAILABLE)
endif()
