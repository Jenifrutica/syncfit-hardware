// SyncFit Edge firmware entry point (skeleton).
//
// Captures biomechanical telemetry (PPG, thermal, isometric load) and streams it
// as JSON frames. The sensor drivers and WebSocket client are added in the full
// implementation; this skeleton establishes the structure and the Ring Buffer.

#include <Arduino.h>

#include "ring_buffer.hpp"

namespace {
constexpr std::size_t kWindowSize = 256;
syncfit::RingBuffer<float, kWindowSize> gPpgWindow;
constexpr int kSampleIntervalUs = 1000000 / 100;  // 100 Hz
unsigned long gLastSampleUs = 0;
}  // namespace

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("SyncFit Edge firmware (skeleton)");
}

void loop() {
  const unsigned long now = micros();
  if (now - gLastSampleUs >= static_cast<unsigned long>(kSampleIntervalUs)) {
    gLastSampleUs = now;

    // TODO: replace with MAX30102 reading.
    const float sample = analogRead(34);
    gPpgWindow.push(sample);

    if (gPpgWindow.full()) {
      // TODO: build the telemetry frame and send it over the WebSocket client.
      Serial.printf("window full: latest=%.2f\n", gPpgWindow.latest());
    }
  }
}
