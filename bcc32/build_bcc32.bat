@echo off
setlocal

if "%BCB%"=="" (
    echo BCB must point to the C++Builder 2009 installation directory.
    exit /b 1
)

rem BCC32 6.11 / C++Builder 2009, Win32 DLL, C++03 source only.
"%BCB%\bin\bcc32.exe" -WD -tWD -6 -I.. -I..\DHF\_example\include NmeaDecoderBcc32.cpp NmeaDecoderBcc32.def
if errorlevel 1 exit /b %errorlevel%

echo Created nmea_decoder.dll (Win32) in %CD%.
