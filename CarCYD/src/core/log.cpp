#include "log.h"

#include <stdarg.h>
#include <stdio.h>

namespace core::log {
namespace {

Sink g_sink = nullptr;
Clock g_clock = nullptr;
Level g_min_level = Level::Info;

constexpr char kLevelChar[] = {'E', 'W', 'I', 'D'};

}  // namespace

void init(Sink sink, Clock clock, Level min_level) {
  g_sink = sink;
  g_clock = clock;
  g_min_level = min_level;
}

void write(Level level, const char* tag, const char* fmt, ...) {
  if (g_sink == nullptr || level > g_min_level) return;

  char line[192];
  const uint32_t ms = g_clock ? g_clock() : 0;
  int n = snprintf(line, sizeof(line), "[%6lu.%03lu] %c %-5s ", static_cast<unsigned long>(ms / 1000),
                   static_cast<unsigned long>(ms % 1000), kLevelChar[static_cast<uint8_t>(level)], tag);
  if (n < 0) return;
  if (n < static_cast<int>(sizeof(line))) {
    va_list args;
    va_start(args, fmt);
    vsnprintf(line + n, sizeof(line) - n, fmt, args);
    va_end(args);
  }
  g_sink(line);
}

}  // namespace core::log
