/**
 * @file log.h
 * Minimal, platform independent logger.
 *
 * The platform layer installs a sink (e.g. Serial) and a millisecond clock at
 * start-up. Messages below the configured level are discarded cheaply.
 */
#pragma once

#include <stdint.h>

namespace core::log {

enum class Level : uint8_t { Error = 0, Warn, Info, Debug };

using Sink = void (*)(const char* line);
using Clock = uint32_t (*)();

void init(Sink sink, Clock clock, Level min_level);
void write(Level level, const char* tag, const char* fmt, ...) __attribute__((format(printf, 3, 4)));

}  // namespace core::log

#define LOG_E(tag, ...) ::core::log::write(::core::log::Level::Error, tag, __VA_ARGS__)
#define LOG_W(tag, ...) ::core::log::write(::core::log::Level::Warn, tag, __VA_ARGS__)
#define LOG_I(tag, ...) ::core::log::write(::core::log::Level::Info, tag, __VA_ARGS__)
#define LOG_D(tag, ...) ::core::log::write(::core::log::Level::Debug, tag, __VA_ARGS__)
