#include "i18n.h"

#include <stddef.h>

namespace core::i18n {
namespace {

// clang-format off
const char* const kEnglish[] = {
#define I18N_EN(id, en, tr) en,
  I18N_STRINGS(I18N_EN)
#undef I18N_EN
};

const char* const kTurkish[] = {
#define I18N_TR(id, en, tr) tr,
  I18N_STRINGS(I18N_TR)
#undef I18N_TR
};
// clang-format on

static_assert(sizeof(kEnglish) / sizeof(kEnglish[0]) == static_cast<size_t>(Str::Count), "string table size");
static_assert(sizeof(kTurkish) / sizeof(kTurkish[0]) == static_cast<size_t>(Str::Count), "string table size");

Language g_language = Language::English;

}  // namespace

void set_language(Language lang) {
  g_language = (lang < Language::Count) ? lang : Language::English;
}

Language language() { return g_language; }

const char* get(Str id) {
  const auto index = static_cast<uint16_t>(id);
  if (index >= static_cast<uint16_t>(Str::Count)) return "";
  return (g_language == Language::Turkish) ? kTurkish[index] : kEnglish[index];
}

const char* language_name(Language lang) {
  switch (lang) {
    case Language::Turkish: return "Türkçe";
    case Language::English:
    default: return "English";
  }
}

}  // namespace core::i18n
