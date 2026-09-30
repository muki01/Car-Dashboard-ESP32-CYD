/**
 * @file app.h
 * Application entry points (called from CarCYD.ino).
 *
 * The application is the composition root: it owns the hardware drivers, the
 * data link and the persistent storage, and wires them to the core services
 * and the UI. Everything runs cooperatively in the Arduino loop task.
 */
#pragma once

namespace app {

void setup();
void loop();

}  // namespace app
