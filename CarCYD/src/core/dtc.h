/**
 * @file dtc.h
 * Diagnostic trouble code helpers (SAE J2012 / ISO 15031-6).
 *
 * Codes use the two-byte OBD-II encoding:
 *   bits 15..14  system  (00 P, 01 C, 10 B, 11 U)
 *   bits 13..12  first digit (0..3)
 *   bits 11..0   remaining three hex digits
 * e.g. 0x0301 = P0301, 0xC100 = U0100.
 */
#pragma once

#include <stddef.h>
#include <stdint.h>

#include "i18n.h"
#include "types.h"

namespace core::dtc {

enum class System : uint8_t { Powertrain = 0, Chassis, Body, Network };

/** Formats a code as "P0301". `out` must hold at least 6 bytes. */
void format(uint16_t code, char* out, size_t len);

/** Parses "P0301" style strings. Returns false on malformed input. */
bool parse(const char* text, uint16_t& code);

System system_of(uint16_t code);
Str system_name(uint16_t code);

/** True for SAE generic codes, false for manufacturer specific ones. */
bool is_generic(uint16_t code);

/** Severity used for colouring and advice. */
Severity severity_of(uint16_t code);

/** Human readable description in the active language (specific or by code range). */
const char* describe(uint16_t code);

}  // namespace core::dtc
