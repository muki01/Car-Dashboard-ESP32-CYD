#include "dtc.h"

#include <ctype.h>
#include <stdio.h>

namespace core::dtc {
namespace {

struct Entry {
  uint16_t code;
  Severity severity;
  const char* en;
  const char* tr;
};

constexpr Severity I = Severity::Info;
constexpr Severity W = Severity::Warning;
constexpr Severity C = Severity::Critical;

// Sorted by code (binary search). Descriptions follow SAE J2012 wording.
// clang-format off
const Entry kDatabase[] = {
  {0x0100, W, "Mass air flow circuit malfunction",                      "Hava akış (MAF) sensörü devre arızası"},
  {0x0101, W, "Mass air flow circuit range/performance",                "MAF sensörü aralık/performans sorunu"},
  {0x0102, W, "Mass air flow circuit low input",                        "MAF sensörü düşük sinyal"},
  {0x0103, W, "Mass air flow circuit high input",                       "MAF sensörü yüksek sinyal"},
  {0x0106, W, "Manifold pressure circuit range/performance",            "Manifold basınç (MAP) sensörü aralık/performans sorunu"},
  {0x0107, W, "Manifold pressure circuit low input",                    "MAP sensörü düşük sinyal"},
  {0x0108, W, "Manifold pressure circuit high input",                   "MAP sensörü yüksek sinyal"},
  {0x0110, W, "Intake air temperature circuit malfunction",             "Emme havası sıcaklık sensörü devre arızası"},
  {0x0112, W, "Intake air temperature circuit low input",               "Emme havası sıcaklık sensörü düşük sinyal"},
  {0x0113, W, "Intake air temperature circuit high input",              "Emme havası sıcaklık sensörü yüksek sinyal"},
  {0x0115, W, "Engine coolant temperature circuit malfunction",         "Motor suyu sıcaklık sensörü devre arızası"},
  {0x0116, W, "Engine coolant temperature circuit range/performance",   "Motor suyu sıcaklık sensörü aralık/performans sorunu"},
  {0x0117, W, "Engine coolant temperature circuit low input",           "Motor suyu sıcaklık sensörü düşük sinyal"},
  {0x0118, W, "Engine coolant temperature circuit high input",          "Motor suyu sıcaklık sensörü yüksek sinyal"},
  {0x0120, W, "Throttle position sensor A circuit malfunction",         "Gaz kelebeği konum sensörü A devre arızası"},
  {0x0121, W, "Throttle position sensor A range/performance",           "Gaz kelebeği konum sensörü A aralık/performans sorunu"},
  {0x0122, W, "Throttle position sensor A circuit low input",           "Gaz kelebeği konum sensörü A düşük sinyal"},
  {0x0123, W, "Throttle position sensor A circuit high input",          "Gaz kelebeği konum sensörü A yüksek sinyal"},
  {0x0125, I, "Insufficient coolant temperature for closed loop fuel control", "Kapalı çevrim yakıt kontrolü için motor suyu sıcaklığı yetersiz"},
  {0x0128, I, "Coolant thermostat below regulating temperature",        "Termostat: motor suyu çalışma sıcaklığının altında"},
  {0x0130, W, "O2 sensor circuit malfunction (bank 1, sensor 1)",       "Oksijen sensörü devre arızası (sıra 1, sensör 1)"},
  {0x0131, W, "O2 sensor circuit low voltage (bank 1, sensor 1)",       "Oksijen sensörü düşük voltaj (sıra 1, sensör 1)"},
  {0x0132, W, "O2 sensor circuit high voltage (bank 1, sensor 1)",      "Oksijen sensörü yüksek voltaj (sıra 1, sensör 1)"},
  {0x0133, W, "O2 sensor circuit slow response (bank 1, sensor 1)",     "Oksijen sensörü yavaş tepki (sıra 1, sensör 1)"},
  {0x0134, W, "O2 sensor circuit no activity (bank 1, sensor 1)",       "Oksijen sensöründe aktivite yok (sıra 1, sensör 1)"},
  {0x0135, W, "O2 sensor heater circuit malfunction (bank 1, sensor 1)", "Oksijen sensörü ısıtıcı devre arızası (sıra 1, sensör 1)"},
  {0x0141, W, "O2 sensor heater circuit malfunction (bank 1, sensor 2)", "Oksijen sensörü ısıtıcı devre arızası (sıra 1, sensör 2)"},
  {0x0171, W, "System too lean (bank 1)",                               "Karışım çok fakir (sıra 1)"},
  {0x0172, W, "System too rich (bank 1)",                               "Karışım çok zengin (sıra 1)"},
  {0x0174, W, "System too lean (bank 2)",                               "Karışım çok fakir (sıra 2)"},
  {0x0175, W, "System too rich (bank 2)",                               "Karışım çok zengin (sıra 2)"},
  {0x0200, W, "Injector circuit malfunction",                           "Enjektör devre arızası"},
  {0x0201, W, "Injector circuit malfunction, cylinder 1",               "Enjektör devre arızası, silindir 1"},
  {0x0202, W, "Injector circuit malfunction, cylinder 2",               "Enjektör devre arızası, silindir 2"},
  {0x0203, W, "Injector circuit malfunction, cylinder 3",               "Enjektör devre arızası, silindir 3"},
  {0x0204, W, "Injector circuit malfunction, cylinder 4",               "Enjektör devre arızası, silindir 4"},
  {0x0217, C, "Engine coolant over temperature condition",              "Motor aşırı ısınma durumu"},
  {0x0219, C, "Engine overspeed condition",                             "Motor aşırı devir durumu"},
  {0x0230, W, "Fuel pump primary circuit malfunction",                  "Yakıt pompası ana devre arızası"},
  {0x0234, C, "Turbocharger overboost condition",                       "Turbo aşırı basınç durumu"},
  {0x0299, W, "Turbocharger underboost condition",                      "Turbo basıncı yetersiz"},
  {0x0300, C, "Random/multiple cylinder misfire detected",              "Rastgele/çoklu silindir ateşleme hatası"},
  {0x0301, C, "Cylinder 1 misfire detected",                            "Silindir 1 ateşleme hatası (tekleme)"},
  {0x0302, C, "Cylinder 2 misfire detected",                            "Silindir 2 ateşleme hatası (tekleme)"},
  {0x0303, C, "Cylinder 3 misfire detected",                            "Silindir 3 ateşleme hatası (tekleme)"},
  {0x0304, C, "Cylinder 4 misfire detected",                            "Silindir 4 ateşleme hatası (tekleme)"},
  {0x0305, C, "Cylinder 5 misfire detected",                            "Silindir 5 ateşleme hatası (tekleme)"},
  {0x0306, C, "Cylinder 6 misfire detected",                            "Silindir 6 ateşleme hatası (tekleme)"},
  {0x0325, W, "Knock sensor 1 circuit malfunction",                     "Vuruntu sensörü 1 devre arızası"},
  {0x0327, W, "Knock sensor 1 circuit low input",                       "Vuruntu sensörü 1 düşük sinyal"},
  {0x0335, C, "Crankshaft position sensor A circuit malfunction",       "Krank mili konum sensörü A devre arızası"},
  {0x0340, C, "Camshaft position sensor A circuit malfunction",         "Eksantrik mili konum sensörü A devre arızası"},
  {0x0351, W, "Ignition coil A primary/secondary circuit malfunction",  "Ateşleme bobini A devre arızası"},
  {0x0352, W, "Ignition coil B primary/secondary circuit malfunction",  "Ateşleme bobini B devre arızası"},
  {0x0353, W, "Ignition coil C primary/secondary circuit malfunction",  "Ateşleme bobini C devre arızası"},
  {0x0354, W, "Ignition coil D primary/secondary circuit malfunction",  "Ateşleme bobini D devre arızası"},
  {0x0400, W, "Exhaust gas recirculation flow malfunction",             "EGR akış arızası"},
  {0x0401, W, "Exhaust gas recirculation flow insufficient",            "EGR akışı yetersiz"},
  {0x0402, W, "Exhaust gas recirculation flow excessive",               "EGR akışı aşırı"},
  {0x0420, W, "Catalyst system efficiency below threshold (bank 1)",    "Katalizör verimi eşiğin altında (sıra 1)"},
  {0x0430, W, "Catalyst system efficiency below threshold (bank 2)",    "Katalizör verimi eşiğin altında (sıra 2)"},
  {0x0440, I, "Evaporative emission system malfunction",                "Yakıt buharı (EVAP) sistemi arızası"},
  {0x0441, I, "Evaporative emission system incorrect purge flow",       "EVAP sistemi hatalı tahliye akışı"},
  {0x0442, I, "Evaporative emission system leak detected (small)",      "EVAP sisteminde küçük kaçak"},
  {0x0446, I, "Evaporative emission vent control circuit malfunction",  "EVAP havalandırma kontrol devresi arızası"},
  {0x0455, I, "Evaporative emission system leak detected (large)",      "EVAP sisteminde büyük kaçak"},
  {0x0456, I, "Evaporative emission system leak detected (very small)", "EVAP sisteminde çok küçük kaçak"},
  {0x0500, W, "Vehicle speed sensor malfunction",                       "Araç hız sensörü arızası"},
  {0x0505, W, "Idle control system malfunction",                        "Rölanti kontrol sistemi arızası"},
  {0x0506, I, "Idle control system RPM lower than expected",            "Rölanti devri beklenenden düşük"},
  {0x0507, I, "Idle control system RPM higher than expected",           "Rölanti devri beklenenden yüksek"},
  {0x0520, C, "Engine oil pressure sensor/switch circuit malfunction",  "Yağ basıncı sensörü/müşürü devre arızası"},
  {0x0524, C, "Engine oil pressure too low",                            "Motor yağ basıncı çok düşük"},
  {0x0562, W, "System voltage low",                                     "Sistem voltajı düşük"},
  {0x0563, W, "System voltage high",                                    "Sistem voltajı yüksek"},
  {0x0600, W, "Serial communication link malfunction",                  "Seri iletişim hattı arızası"},
  {0x0601, C, "Control module memory checksum error",                   "Kontrol modülü bellek sağlama hatası"},
  {0x0606, C, "Control module processor fault",                         "Kontrol modülü işlemci arızası"},
  {0x0700, W, "Transmission control system malfunction",                "Şanzıman kontrol sistemi arızası"},
  {0x0715, W, "Input/turbine speed sensor circuit malfunction",         "Şanzıman giriş/türbin hız sensörü arızası"},
  {0x0720, W, "Output speed sensor circuit malfunction",                "Şanzıman çıkış hız sensörü arızası"},
  {0x0730, W, "Incorrect gear ratio",                                   "Hatalı vites oranı"},
  {0x0741, W, "Torque converter clutch performance or stuck off",       "Tork konvertörü kavraması performans sorunu"},
  {0x4035, W, "Left front wheel speed sensor circuit",                  "Sol ön tekerlek hız sensörü devresi"},
  {0x4040, W, "Right front wheel speed sensor circuit",                 "Sağ ön tekerlek hız sensörü devresi"},
  {0x4045, W, "Left rear wheel speed sensor circuit",                   "Sol arka tekerlek hız sensörü devresi"},
  {0x4050, W, "Right rear wheel speed sensor circuit",                  "Sağ arka tekerlek hız sensörü devresi"},
  {0xC100, C, "Lost communication with ECM/PCM",                        "Motor kontrol modülü (ECM) ile iletişim kaybı"},
  {0xC101, W, "Lost communication with TCM",                            "Şanzıman kontrol modülü (TCM) ile iletişim kaybı"},
  {0xC121, W, "Lost communication with ABS control module",             "ABS kontrol modülü ile iletişim kaybı"},
  {0xC140, W, "Lost communication with body control module",            "Gövde kontrol modülü (BCM) ile iletişim kaybı"},
  {0xC155, I, "Lost communication with instrument cluster",             "Gösterge paneli ile iletişim kaybı"},
};
// clang-format on

constexpr size_t kDatabaseSize = sizeof(kDatabase) / sizeof(kDatabase[0]);

const Entry* find(uint16_t code) {
  size_t lo = 0;
  size_t hi = kDatabaseSize;
  while (lo < hi) {
    const size_t mid = (lo + hi) / 2;
    if (kDatabase[mid].code == code) return &kDatabase[mid];
    if (kDatabase[mid].code < code) {
      lo = mid + 1;
    } else {
      hi = mid;
    }
  }
  return nullptr;
}

const char* pick(const char* en, const char* tr_text) {
  return i18n::language() == Language::Turkish ? tr_text : en;
}

uint8_t first_digit(uint16_t code) { return (code >> 12) & 0x3; }
uint8_t second_digit(uint16_t code) { return (code >> 8) & 0xF; }

}  // namespace

void format(uint16_t code, char* out, size_t len) {
  static const char kSystems[] = {'P', 'C', 'B', 'U'};
  snprintf(out, len, "%c%u%03X", kSystems[code >> 14], static_cast<unsigned>(first_digit(code)),
           static_cast<unsigned>(code & 0x0FFF));
}

bool parse(const char* text, uint16_t& code) {
  if (text == nullptr) return false;
  uint16_t system;
  switch (toupper(static_cast<unsigned char>(text[0]))) {
    case 'P': system = 0; break;
    case 'C': system = 1; break;
    case 'B': system = 2; break;
    case 'U': system = 3; break;
    default: return false;
  }
  if (text[1] < '0' || text[1] > '3') return false;
  uint16_t value = static_cast<uint16_t>((system << 14) | ((text[1] - '0') << 12));
  for (int i = 2; i < 5; ++i) {
    const char c = static_cast<char>(toupper(static_cast<unsigned char>(text[i])));
    uint16_t nibble;
    if (c >= '0' && c <= '9') {
      nibble = c - '0';
    } else if (c >= 'A' && c <= 'F') {
      nibble = c - 'A' + 10;
    } else {
      return false;
    }
    value |= nibble << ((4 - i) * 4);
  }
  if (text[5] != '\0') return false;
  code = value;
  return true;
}

System system_of(uint16_t code) { return static_cast<System>(code >> 14); }

Str system_name(uint16_t code) {
  switch (system_of(code)) {
    case System::Chassis: return Str::DTC_SYS_CHASSIS;
    case System::Body: return Str::DTC_SYS_BODY;
    case System::Network: return Str::DTC_SYS_NETWORK;
    case System::Powertrain:
    default: return Str::DTC_SYS_POWERTRAIN;
  }
}

bool is_generic(uint16_t code) {
  const uint8_t d1 = first_digit(code);
  if (system_of(code) == System::Powertrain) {
    // P0xxx, P2xxx and P34xx-P39xx are SAE controlled.
    if (d1 == 0 || d1 == 2) return true;
    if (d1 == 3) return second_digit(code) >= 4;
    return false;
  }
  return d1 == 0 || d1 == 3;
}

Severity severity_of(uint16_t code) {
  if (const Entry* e = find(code)) return e->severity;
  return Severity::Warning;
}

const char* describe(uint16_t code) {
  if (const Entry* e = find(code)) return pick(e->en, e->tr);

  switch (system_of(code)) {
    case System::Chassis: return pick("Chassis system fault", "Şasi sistemi arızası");
    case System::Body: return pick("Body system fault", "Gövde sistemi arızası");
    case System::Network: return pick("Network communication fault", "Ağ iletişim arızası");
    case System::Powertrain: break;
  }

  if (!is_generic(code)) return pick("Manufacturer specific powertrain fault", "Üreticiye özel motor/şanzıman arızası");
  if (first_digit(code) != 0) return pick("Fuel, air or emission control fault", "Yakıt, hava veya emisyon kontrol arızası");

  switch (second_digit(code)) {
    case 0x0: return pick("Fuel and air metering, auxiliary emission control", "Yakıt-hava ölçümü, yardımcı emisyon kontrolü");
    case 0x1: return pick("Fuel and air metering", "Yakıt ve hava ölçümü");
    case 0x2: return pick("Fuel and air metering (injector circuit)", "Yakıt ve hava ölçümü (enjektör devresi)");
    case 0x3: return pick("Ignition system or misfire", "Ateşleme sistemi veya tekleme");
    case 0x4: return pick("Auxiliary emission controls", "Yardımcı emisyon kontrolleri");
    case 0x5: return pick("Vehicle speed, idle control or auxiliary inputs", "Araç hızı, rölanti kontrolü veya yardımcı girişler");
    case 0x6: return pick("Control module or output circuit", "Kontrol modülü veya çıkış devresi");
    case 0x7:
    case 0x8:
    case 0x9: return pick("Transmission", "Şanzıman");
    case 0xA: return pick("Hybrid propulsion system", "Hibrit tahrik sistemi");
    default: return pick("Powertrain fault", "Motor/şanzıman arızası");
  }
}

}  // namespace core::dtc
