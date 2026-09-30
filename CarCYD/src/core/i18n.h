/**
 * @file i18n.h
 * User interface strings (English / Turkish).
 *
 * Strings are declared once in I18N_STRINGS as X(id, english, turkish), which
 * keeps both languages in lock-step at compile time. Source files are UTF-8;
 * the UI fonts contain the Turkish glyphs (see tools/fonts/generate_fonts.py).
 */
#pragma once

#include <stdint.h>

#include "types.h"

// clang-format off
#define I18N_STRINGS(X) \
  /* ---- Common ---------------------------------------------------------- */ \
  X(APP_TAGLINE,          "Vehicle Dashboard",              "Araç Gösterge Paneli") \
  X(OK,                   "OK",                             "Tamam") \
  X(CANCEL,               "Cancel",                         "İptal") \
  X(CLOSE,                "Close",                          "Kapat") \
  X(ON,                   "On",                             "Açık") \
  X(OFF,                  "Off",                            "Kapalı") \
  X(RESET,                "Reset",                          "Sıfırla") \
  X(AUTO,                 "Auto",                           "Oto") \
  X(JUST_NOW,             "just now",                       "az önce") \
  X(FMT_SECONDS_AGO,      "%lu s ago",                      "%lu sn önce") \
  X(FMT_MINUTES_AGO,      "%lu min ago",                    "%lu dk önce") \
  X(FMT_HOURS_AGO,        "%lu h ago",                      "%lu sa önce") \
  /* ---- Boot ------------------------------------------------------------ */ \
  X(BOOT_SETTINGS,        "Loading settings",               "Ayarlar yükleniyor") \
  X(BOOT_HARDWARE,        "Starting hardware",              "Donanım başlatılıyor") \
  X(BOOT_VEHICLE,         "Connecting to vehicle",          "Araca bağlanılıyor") \
  X(BOOT_READY,           "Ready",                          "Hazır") \
  /* ---- Pages ----------------------------------------------------------- */ \
  X(PAGE_DASHBOARD,       "Dashboard",                      "Gösterge") \
  X(PAGE_LIVE,            "Live Data",                      "Canlı Veri") \
  X(PAGE_DIAG,            "Diagnostics",                    "Arıza Teşhis") \
  X(PAGE_TRIP,            "Trip",                           "Yolculuk") \
  X(PAGE_SETTINGS,        "Settings",                       "Ayarlar") \
  /* ---- Signals: full names -------------------------------------------- */ \
  X(SIG_SPEED,            "Vehicle speed",                  "Araç hızı") \
  X(SIG_RPM,              "Engine speed",                   "Motor devri") \
  X(SIG_COOLANT,          "Coolant temperature",            "Motor suyu sıcaklığı") \
  X(SIG_OIL_TEMP,         "Oil temperature",                "Yağ sıcaklığı") \
  X(SIG_INTAKE_TEMP,      "Intake air temperature",         "Emme havası sıcaklığı") \
  X(SIG_AMBIENT_TEMP,     "Outside temperature",            "Dış sıcaklık") \
  X(SIG_BATTERY,          "Battery voltage",                "Akü voltajı") \
  X(SIG_FUEL_LEVEL,       "Fuel level",                     "Yakıt seviyesi") \
  X(SIG_LOAD,             "Engine load",                    "Motor yükü") \
  X(SIG_THROTTLE,         "Throttle position",              "Gaz kelebeği konumu") \
  X(SIG_MAP,              "Manifold pressure",              "Manifold basıncı") \
  X(SIG_MAF,              "Mass air flow",                  "Hava akış miktarı") \
  X(SIG_TIMING,           "Ignition timing",                "Ateşleme avansı") \
  X(SIG_FUEL_RATE,        "Fuel rate",                      "Anlık yakıt tüketimi") \
  X(SIG_STFT,             "Short term fuel trim",           "Kısa dönem yakıt düzeltme") \
  X(SIG_LTFT,             "Long term fuel trim",            "Uzun dönem yakıt düzeltme") \
  X(SIG_RUN_TIME,         "Engine run time",                "Motor çalışma süresi") \
  X(SIG_GEAR,             "Gear",                           "Vites") \
  /* ---- Signals: short names (tiles) ----------------------------------- */ \
  X(SHORT_SPEED,          "SPEED",                          "HIZ") \
  X(SHORT_RPM,            "RPM",                            "DEVİR") \
  X(SHORT_COOLANT,        "COOLANT",                        "SU") \
  X(SHORT_OIL_TEMP,       "OIL",                            "YAĞ") \
  X(SHORT_INTAKE_TEMP,    "INTAKE",                         "EMME") \
  X(SHORT_AMBIENT_TEMP,   "OUTSIDE",                        "DIŞ") \
  X(SHORT_BATTERY,        "BATTERY",                        "AKÜ") \
  X(SHORT_FUEL_LEVEL,     "FUEL",                           "YAKIT") \
  X(SHORT_LOAD,           "LOAD",                           "YÜK") \
  X(SHORT_THROTTLE,       "THROTTLE",                       "GAZ") \
  X(SHORT_MAP,            "MAP",                            "MAP") \
  X(SHORT_MAF,            "MAF",                            "MAF") \
  X(SHORT_TIMING,         "TIMING",                         "AVANS") \
  X(SHORT_FUEL_RATE,      "FUEL RATE",                      "TÜKETİM") \
  X(SHORT_STFT,           "STFT",                           "STFT") \
  X(SHORT_LTFT,           "LTFT",                           "LTFT") \
  X(SHORT_RUN_TIME,       "RUN TIME",                       "SÜRE") \
  X(SHORT_GEAR,           "GEAR",                           "VİTES") \
  /* ---- Dashboard ------------------------------------------------------- */ \
  X(DASH_PICK_TITLE,      "Show on this gauge",             "Bu göstergede göster") \
  /* ---- Live data ------------------------------------------------------- */ \
  X(LIVE_MIN,             "Min",                            "Min") \
  X(LIVE_AVG,             "Avg",                            "Ort") \
  X(LIVE_MAX,             "Max",                            "Maks") \
  X(LIVE_NO_DATA,         "No data",                        "Veri yok") \
  /* ---- Diagnostics ----------------------------------------------------- */ \
  X(DIAG_MIL_CAPTION,     "Check engine",                   "Arıza lambası") \
  X(DIAG_MIL_ON,          "ON",                             "YANIYOR") \
  X(DIAG_MIL_OFF,         "OFF",                            "SÖNÜK") \
  X(DIAG_MIL_UNKNOWN,     "Unknown",                        "Bilinmiyor") \
  X(DIAG_FMT_CODE_ONE,    "%u fault code",                  "%u arıza kodu") \
  X(DIAG_FMT_CODE_MANY,   "%u fault codes",                 "%u arıza kodu") \
  X(DIAG_FMT_LAST_SCAN,   "Last scan %s",                   "Son tarama %s") \
  X(DIAG_NEVER_SCANNED,   "Not scanned yet",                "Henüz taranmadı") \
  X(DIAG_SCAN,            "Scan",                           "Tara") \
  X(DIAG_CLEAR,           "Clear",                          "Sil") \
  X(DIAG_SCANNING,        "Reading fault codes…",           "Arıza kodları okunuyor…") \
  X(DIAG_CLEARING,        "Clearing fault codes…",          "Arıza kodları siliniyor…") \
  X(DIAG_EMPTY_TITLE,     "No fault codes",                 "Arıza kodu yok") \
  X(DIAG_EMPTY_TEXT,      "The ECU reports no stored or pending codes.", "ECU kayıtlı veya bekleyen kod bildirmedi.") \
  X(DIAG_IDLE_TITLE,      "Ready to scan",                  "Taramaya hazır") \
  X(DIAG_IDLE_TEXT,       "Tap Scan to read the fault codes from the ECU.", "ECU'daki arıza kodlarını okumak için Tara'ya dokunun.") \
  X(DIAG_CLEAR_TITLE,     "Clear fault codes?",             "Arıza kodları silinsin mi?") \
  X(DIAG_CLEAR_TEXT,      "Stored codes and freeze frame data will be erased and readiness monitors reset. Ignition on, engine off.", "Kayıtlı kodlar ve donmuş veri silinir, hazırlık monitörleri sıfırlanır. Kontak açık, motor kapalı olmalı.") \
  X(DIAG_CLEARED,         "Fault codes cleared",            "Arıza kodları silindi") \
  X(DIAG_SCAN_DONE,       "Scan complete",                  "Tarama tamamlandı") \
  X(DIAG_FAILED,          "The ECU did not respond",        "ECU yanıt vermedi") \
  X(DIAG_NOT_CONNECTED,   "Not connected to the vehicle",   "Araca bağlı değil") \
  X(DTC_STORED,           "Stored",                         "Kayıtlı") \
  X(DTC_PENDING,          "Pending",                        "Bekleyen") \
  X(DTC_PERMANENT,        "Permanent",                      "Kalıcı") \
  X(DTC_SYS_POWERTRAIN,   "Powertrain",                     "Motor / şanzıman") \
  X(DTC_SYS_CHASSIS,      "Chassis",                        "Şasi") \
  X(DTC_SYS_BODY,         "Body",                           "Gövde") \
  X(DTC_SYS_NETWORK,      "Network",                        "Ağ iletişimi") \
  X(DTC_GENERIC,          "Generic (SAE J2012)",            "Genel (SAE J2012)") \
  X(DTC_MANUFACTURER,     "Manufacturer specific",          "Üreticiye özel") \
  X(DTC_SYSTEM,           "System",                         "Sistem") \
  X(DTC_TYPE,             "Code type",                      "Kod tipi") \
  X(DTC_STATUS,           "Status",                         "Durum") \
  X(DTC_ADVICE_CRITICAL,  "Critical: stop safely and have the vehicle inspected.", "Kritik: güvenli şekilde durun ve aracı kontrol ettirin.") \
  X(DTC_ADVICE_WARNING,   "Have the vehicle inspected soon.", "Aracı en kısa sürede kontrol ettirin.") \
  X(DTC_ADVICE_INFO,      "Minor fault. Monitor and check at the next service.", "Küçük arıza. Takip edin, ilk bakımda kontrol ettirin.") \
  /* ---- Trip ------------------------------------------------------------ */ \
  X(TRIP_DISTANCE,        "Distance",                       "Mesafe") \
  X(TRIP_TIME,            "Drive time",                     "Sürüş süresi") \
  X(TRIP_AVG_SPEED,       "Avg speed",                      "Ort. hız") \
  X(TRIP_MAX_SPEED,       "Max speed",                      "Maks. hız") \
  X(TRIP_AVG_CONS,        "Avg consumption",                "Ort. tüketim") \
  X(TRIP_FUEL_USED,       "Fuel used",                      "Harcanan yakıt") \
  X(TRIP_ACCEL,           "Acceleration",                   "Hızlanma") \
  X(TRIP_LAST,            "Last",                           "Son") \
  X(TRIP_BEST,            "Best",                           "En iyi") \
  X(TRIP_ACCEL_IDLE,      "Stop to arm",                    "Durunca hazır") \
  X(TRIP_ACCEL_ARMED,     "Armed, go!",                     "Hazır, gaz!") \
  X(TRIP_ACCEL_RUNNING,   "Measuring…",                     "Ölçülüyor…") \
  X(TRIP_RESET_TITLE,     "Reset trip?",                    "Yolculuk sıfırlansın mı?") \
  X(TRIP_RESET_TEXT,      "Distance, time, consumption and speed statistics will be reset.", "Mesafe, süre, tüketim ve hız istatistikleri sıfırlanacak.") \
  X(TRIP_RESET_DONE,      "Trip reset",                     "Yolculuk sıfırlandı") \
  /* ---- Settings: categories -------------------------------------------- */ \
  X(SET_DISPLAY,          "Display",                        "Ekran") \
  X(SET_UNITS,            "Units",                          "Birimler") \
  X(SET_GAUGES,           "Gauges",                         "Göstergeler") \
  X(SET_WARNINGS,         "Warnings",                       "Uyarılar") \
  X(SET_SOUND,            "Sound & light",                  "Ses ve ışık") \
  X(SET_CONNECTION,       "Connection",                     "Bağlantı") \
  X(SET_SYSTEM,           "System",                         "Sistem") \
  X(SET_ABOUT,            "About",                          "Hakkında") \
  /* ---- Settings: display ----------------------------------------------- */ \
  X(SET_BRIGHTNESS,       "Brightness",                     "Parlaklık") \
  X(SET_AUTO_BRIGHTNESS,  "Auto brightness",                "Otomatik parlaklık") \
  X(SET_AUTO_BRIGHTNESS_SUB, "Uses the ambient light sensor", "Ortam ışığı sensörünü kullanır") \
  X(SET_DAY_BRIGHTNESS,   "Day brightness",                 "Gündüz parlaklığı") \
  X(SET_NIGHT_BRIGHTNESS, "Night brightness",               "Gece parlaklığı") \
  X(SET_AMBIENT,          "Ambient light",                  "Ortam ışığı") \
  X(SET_FLIP,             "Rotate screen 180°",             "Ekranı 180° döndür") \
  X(SET_ACCENT,           "Accent color",                   "Vurgu rengi") \
  /* ---- Settings: units ------------------------------------------------- */ \
  X(SET_UNIT_SPEED,       "Speed & distance",               "Hız ve mesafe") \
  X(SET_UNIT_TEMP,        "Temperature",                    "Sıcaklık") \
  X(SET_UNIT_PRESSURE,    "Pressure",                       "Basınç") \
  /* ---- Settings: gauges ------------------------------------------------ */ \
  X(SET_RPM_MAX,          "Tachometer range",               "Devir saati aralığı") \
  X(SET_REDLINE,          "Redline",                        "Kırmızı bölge") \
  X(SET_SHIFT_LIGHT,      "Shift light",                    "Vites ışığı") \
  X(SET_SHIFT_RPM,        "Shift point",                    "Vites değiştirme devri") \
  X(SET_LAYOUT_RESET,     "Reset gauge layout",             "Gösterge düzenini sıfırla") \
  X(SET_LAYOUT_HINT,      "Long-press any dashboard value to change it.", "Değiştirmek için gösterge değerine basılı tutun.") \
  X(SET_LAYOUT_DONE,      "Gauge layout restored",          "Gösterge düzeni sıfırlandı") \
  /* ---- Settings: warnings ---------------------------------------------- */ \
  X(SET_COOLANT_WARN,     "Coolant temperature",            "Motor suyu sıcaklığı") \
  X(SET_VOLTAGE_WARN,     "Low battery voltage",            "Düşük akü voltajı") \
  X(SET_FUEL_WARN,        "Low fuel level",                 "Düşük yakıt seviyesi") \
  X(SET_OVERSPEED,        "Speed limit",                    "Hız sınırı") \
  /* ---- Settings: sound & light ----------------------------------------- */ \
  X(SET_TOUCH_SOUND,      "Touch sounds",                   "Dokunma sesleri") \
  X(SET_ALERT_SOUND,      "Warning sounds",                 "Uyarı sesleri") \
  X(SET_VOLUME,           "Volume",                         "Ses seviyesi") \
  X(SET_STATUS_LED,       "Status LED",                     "Durum LED'i") \
  X(SET_STATUS_LED_SUB,   "Shift light and warnings",       "Vites ışığı ve uyarılar") \
  /* ---- Settings: connection -------------------------------------------- */ \
  X(SET_DATA_SOURCE,      "Data source",                    "Veri kaynağı") \
  X(SET_SRC_DEMO,         "Demo",                           "Demo") \
  X(SET_SRC_LINK,         "ESP32 link",                     "ESP32 bağlantısı") \
  X(SET_LINK_STATUS,      "Status",                         "Durum") \
  X(SET_LINK_INTERFACE,   "Interface",                      "Arayüz") \
  X(SET_DEMO_HINT,        "Demo mode simulates a driving car. Select ESP32 link to use real vehicle data.", "Demo modu sürüşü simüle eder. Gerçek araç verisi için ESP32 bağlantısını seçin.") \
  X(LINK_CONNECTED,       "Connected",                      "Bağlı") \
  X(LINK_CONNECTING,      "Connecting…",                    "Bağlanıyor…") \
  X(LINK_DISCONNECTED,    "Not connected",                  "Bağlı değil") \
  X(LINK_ERROR,           "Error",                          "Hata") \
  /* ---- Settings: system ------------------------------------------------ */ \
  X(SET_LANGUAGE,         "Language",                       "Dil") \
  X(SET_CALIBRATE,        "Touch calibration",              "Dokunmatik kalibrasyonu") \
  X(SET_RESTART,          "Restart device",                 "Cihazı yeniden başlat") \
  X(SET_RESTART_TEXT,     "The device will restart now.",   "Cihaz şimdi yeniden başlatılacak.") \
  X(SET_FACTORY_RESET,    "Factory reset",                  "Fabrika ayarları") \
  X(SET_FACTORY_TEXT,     "All settings, trip data and the touch calibration will be erased. The device will restart.", "Tüm ayarlar, yolculuk verileri ve dokunmatik kalibrasyonu silinecek. Cihaz yeniden başlayacak.") \
  /* ---- Settings: about ------------------------------------------------- */ \
  X(ABOUT_FIRMWARE,       "Firmware",                       "Yazılım") \
  X(ABOUT_BUILD,          "Build",                          "Derleme") \
  X(ABOUT_BOARD,          "Board",                          "Kart") \
  X(ABOUT_DISPLAY,        "Display",                        "Ekran") \
  X(ABOUT_CHIP,           "Processor",                      "İşlemci") \
  X(ABOUT_FLASH,          "Flash",                          "Flash") \
  X(ABOUT_MEMORY,         "Free memory",                    "Boş bellek") \
  X(ABOUT_UPTIME,         "Uptime",                         "Çalışma süresi") \
  X(ABOUT_GRAPHICS,       "Graphics",                       "Grafik") \
  /* ---- Touch calibration ----------------------------------------------- */ \
  X(CAL_TITLE,            "Touch calibration",              "Dokunmatik kalibrasyonu") \
  X(CAL_TAP,              "Tap the center of the target",   "Hedefin tam ortasına dokunun") \
  X(CAL_VERIFY,           "Tap the target once more to verify", "Doğrulamak için hedefe tekrar dokunun") \
  X(CAL_FMT_STEP,         "Point %u of %u",                 "Nokta %u / %u") \
  X(CAL_HINT,             "A stylus or fingernail gives the best accuracy", "En iyi sonuç için kalem ucu veya tırnak kullanın") \
  X(CAL_DONE,             "Calibration saved",              "Kalibrasyon kaydedildi") \
  X(CAL_FAILED,           "Not accurate enough, please try again", "Yeterince hassas değil, lütfen tekrar deneyin") \
  /* ---- Alerts ---------------------------------------------------------- */ \
  X(ALERT_COOLANT,        "Engine temperature high",        "Motor sıcaklığı yüksek") \
  X(ALERT_OIL,            "Oil temperature high",           "Yağ sıcaklığı yüksek") \
  X(ALERT_BATTERY_LOW,    "Battery voltage low",            "Akü voltajı düşük") \
  X(ALERT_BATTERY_HIGH,   "Charging voltage high",          "Şarj voltajı yüksek") \
  X(ALERT_FUEL,           "Fuel level low",                 "Yakıt seviyesi düşük") \
  X(ALERT_OVERSPEED,      "Speed limit exceeded",           "Hız sınırı aşıldı") \
  X(ALERT_MIL,            "Check engine light on",          "Motor arıza lambası yandı") \
  X(ALERT_MIL_TEXT,       "Open Diagnostics to read the fault codes", "Kodlar için Arıza Teşhis sayfasını açın") \
  X(ALERT_LINK,           "Vehicle connection lost",        "Araç bağlantısı kesildi") \
  X(ALERT_LINK_TEXT,      "Check the vehicle interface",    "Araç arayüzünü kontrol edin") \
  X(ALERT_TAP_HINT,       "Tap to dismiss",                 "Kapatmak için dokunun") \
  X(SETTINGS_RESTORED,    "Settings restored to defaults",  "Ayarlar varsayılana döndü")
// clang-format on

namespace core {

enum class Str : uint16_t {
#define I18N_ENUM(id, en, tr) id,
  I18N_STRINGS(I18N_ENUM)
#undef I18N_ENUM
  Count
};

namespace i18n {

void set_language(Language lang);
Language language();

/** Returns the UTF-8 text of `id` in the active language. Never returns null. */
const char* get(Str id);

/** Native name of a language (e.g. "Türkçe"), independent of the active language. */
const char* language_name(Language lang);

}  // namespace i18n

/** Shorthand used throughout the UI. */
inline const char* tr(Str id) { return i18n::get(id); }

}  // namespace core
