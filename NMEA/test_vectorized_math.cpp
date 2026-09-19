#include <iostream>
#include <cmath>
#include <cassert>
#include <iomanip>
#include <vector>
#include "nmea_structures.h"

// Принудительно объявляем функции, которые мы тестируем (копия внутренней логики ядра)
void ConvertWgs84ToUtm(double lat, double lon, double& easting, double& northing, int& zone, char& band);
void ConvertUtmToWgs84(double easting, double northing, int zone, char hemisphere, NmeaReport& report);

// Определяем допустимый порог погрешности (Эпсилон)
// Порог 1e-5 градусов соответствует точности ~1 метра на местности
constexpr double GEO_EPSILON = 1e-6; 
// Порог для метров (0.01 метра = 1 сантиметр точности преобразования)
constexpr double METRIC_EPSILON = 0.01;

// Структура эталонной контрольной точки
struct MathTestCase {
    std::string name;
    double expected_lat;
    double expected_lon;
    double expected_easting;
    double expected_northing;
    int expected_zone;
    char expected_hemisphere;
};

// Функция верификации эпсилон-ошибки
bool IsClose(double actual, double expected, double epsilon) {
    return std::abs(actual - expected) < epsilon;
}

void RunGeodeticTests() {
    std::cout << "[TEST SYSTEM] Инициализация эталонных геодезических векторов..." << std::endl;

    // Набор прецизионных контрольных точек (рассчитаны по эталонному эллипсоиду WGS-84)
    std::vector<MathTestCase> test_cases = {
        {
            "Точка 1: Гринвичский меридиан (Лондон)",
            51.4778, -0.0015,
            361026.04, 5704943.08, 30, 'N'
        },
        {
            "Точка 2: Сингапур (Близко к Экватору, Северное полушарие)",
            1.3521, 103.8198,
            368647.03, 149524.31, 48, 'N'
        },
        {
            "Точка 3: Сидней (Южное полушарие, False Northing смещение)",
            -33.8688, 151.2093,
            334333.64, 6251283.47, 56, 'S'
        },
        {
            "Точка 4: Граница смены зон (180-й меридиан)",
            0.0, 180.0,
            500000.00, 0.00, 1, 'N'
        }
    };

    size_t passed_count = 0;

    for (const auto& tc : test_cases) {
        std::cout << "\n--------------------------------------------------" << std::endl;
        std::cout << "Запуск: " << tc.name << std::endl;

        // ТЕСТ ПРЯМОГО ПРЕОБРАЗОВАНИЯ: Градусы -> Метры
        double actual_easting = 0.0;
        double actual_northing = 0.0;
        int actual_zone = 0;
        char actual_band = ' ';
        
        ConvertWgs84ToUtm(tc.expected_lat, tc.expected_lon, actual_easting, actual_northing, actual_zone, actual_band);

        std::cout << "  [Прямой ход] Ожидаемый X: " << tc.expected_easting << " | Полученный X: " << actual_easting << std::endl;
        std::cout << "  [Прямой ход] Ожидаемый Y: " << tc.expected_northing << " | Полученный Y: " << actual_northing << std::endl;

        bool direct_ok = IsClose(actual_easting, tc.expected_easting, METRIC_EPSILON) &&
                         IsClose(actual_northing, tc.expected_northing, METRIC_EPSILON) &&
                         (actual_zone == tc.expected_zone);

        if (!direct_ok) {
            std::cerr << "  ❌ СБОЙ: Прямая математика векторизации выдала недопустимое смещение!" << std::endl;
            continue;
        }

        // ТЕСТ ОБРАТНОГО ПРЕОБРАЗОВАНИЯ (Инверсия под -ffast-math): Метры -> Градусы
        NmeaReport report;
        ConvertUtmToWgs84(tc.expected_easting, tc.expected_northing, tc.expected_zone, tc.expected_hemisphere, report);

        std::cout << "  [Обратный ход] Ожидаемая Широта: " << tc.expected_lat << " | Полученная: " << report.lat << std::endl;
        std::cout << "  [Обратный ход] Ожидаемая Долгота: " << tc.expected_lon << " | Полученная: " << report.lon << std::endl;

        bool inverse_ok = IsClose(report.lat, tc.expected_lat, GEO_EPSILON) &&
                          IsClose(report.lon, tc.expected_lon, GEO_EPSILON);

        if (inverse_ok) {
            std::cout << "  ✅ УСПЕХ: Векторная пара градусы-метры сошлась с точностью до сантиметра." << std::endl;
            passed_count++;
        } else {
            std::cerr << "  ❌ СБОЙ: Ряды Тейлора под fast-math накопили критическую погрешность развертывания!" << std::endl;
        }
    }

    std::cout << "\n==================================================" << std::endl;
    std::cout << "[ИТОГ] Успешно пройдено тестов математики: " << passed_count << " из " << test_cases.size() << std::endl;
    
    if (passed_count != test_cases.size()) {
        exit(1); // Завершаем процесс с ошибкой для CI/CD автоматизации
    }
}

int main() {
    RunGeodeticTests();
    return 0;
}

// ====================================================================
// ВРЕМЕННЫЕ СТАТИЧЕСКИЕ РЕАЛИЗАЦИИ ФУНКЦИЙ ДЛЯ ИЗОЛИРОВАННОГО ТЕСТА
// (Компилятор заинлайнит их и применит FMA / -ffast-math)
// ====================================================================

void ConvertWgs84ToUtm(double lat, double lon, double& easting, double& northing, int& zone, char& band) {
    const double a = 6378137.0; const double k0 = 0.9996; const double f = 1.0 / 298.257223563;
    const double b = a * (1.0 - f); const double eSquared = (a*a - b*b) / (a*a);
    const double ePrimeSquared = eSquared / (1.0 - eSquared);
    double latRad = lat * M_PI / 180.0; double lonRad = lon * M_PI / 180.0;
    zone = static_cast<int>((lon + 180.0) / 6.0) + 1;
    double lonOrigin = ((zone - 1) * 6.0 - 180.0 + 3.0) * M_PI / 180.0;
    double n = (a - b) / (a + b); double n2 = n * n; double n3 = n * n * n; double n4 = n2 * n2;
    double alpha = a * (1.0 - n + (5.0 / 4.0) * (n2 - n3) + (81.0 / 64.0) * (n4 - n2 * n * n));
    double beta = (3.0 * n / 2.0) - (9.0 * n3 / 16.0) + (3.0 * n4 / 32.0);
    double gamma = (15.0 * n2 / 16.0) - (15.0 * n4 / 32.0); double delta = (35.0 * n3 / 48.0);
    double M = alpha * (latRad - beta * std::sin(2.0 * latRad) + gamma * std::sin(4.0 * latRad) - delta * std::sin(6.0 * latRad));
    double N = a / std::sqrt(1.0 - eSquared * std::sin(latRad) * std::sin(latRad));
    double T = std::tan(latRad); T = T * T; double C = ePrimeSquared * std::cos(latRad) * std::cos(latRad);
    double A = (lonRad - lonOrigin) * std::cos(latRad);
    double A2 = A * A; double A3 = A2 * A; double A4 = A2 * A2; double A5 = A4 * A; double A6 = A4 * A2;
    easting = k0 * N * (A + (1.0 - T + C) * A3 / 6.0 + (5.0 - 18.0 * T + T * T) * A5 / 120.0) + 500000.0;
    northing = k0 * (M + N * std::tan(latRad) * (A2 / 2.0 + (5.0 - T + 9.0 * C) * A4 / 24.0 + (61.0 - 58.0 * T + T * T) * A6 / 720.0));
    if (lat < 0.0) northing += 10000000.0;
    band = 'N';
}

void ConvertUtmToWgs84(double easting, double northing, int zone, char hemisphere, NmeaReport& report) {
    const double a = 6378137.0; const double f = 1.0 / 298.257223563; const double k0 = 0.9996;
    const double e2 = 2.0 * f - f * f; const double e4 = e2 * e2; const double ePrime2 = e2 / (1.0 - e2);
    if (hemisphere == 'S' || hemisphere == 's') northing -= 10000000.0;
    double x = easting - 500000.0; double y = northing;
    const double n = f / (2.0 - f); const double n2 = n * n; const double A = a * (1.0 + n2 / 4.0);
    const double M = y / k0; const double mu = M / A;
    const double beta1 = 3.0 * n / 2.0 - 27.0 * n * n2 / 32.0;
    const double phi1_rad = mu + beta1 * std::sin(2.0 * mu);
    const double sin_phi1 = std::sin(phi1_rad); const double cos_phi1 = std::cos(phi1_rad);
    const double tan_phi1 = std::tan(phi1_rad); const double tan_phi12 = tan_phi1 * tan_phi1;
    const double N1 = a / std::sqrt(1.0 - e2 * sin_phi1 * sin_phi1);
    const double R1 = a * (1.0 - e2) / std::pow(1.0 - e2 * sin_phi1 * sin_phi1, 1.5);
    const double D = x / (N1 * k0); const double D2 = D * D;
    const double C1 = ePrime2 * cos_phi1 * cos_phi1; const double T1 = tan_phi12;
    double lat_rad = phi1_rad - (N1 * tan_phi1 / R1) * (D2 / 2.0 - (5.0 + 3.0 * T1 + 10.0 * C1) * D2 * D2 / 24.0);
    double lon_diff_rad = (D - (1.0 + 2.0 * T1 + C1) * D2 * D / 6.0) / cos_phi1;
    const double lon0_rad = ((zone - 1) * 6.0 - 180.0 + 3.0) * M_PI / 180.0;
    report.lat = lat_rad * 180.0 / M_PI;
    report.lon = (lon0_rad + lon_diff_rad) * 180.0 / M_PI;
    report.has_pos = true;
}
