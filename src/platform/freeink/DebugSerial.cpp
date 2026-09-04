#if defined(ARDUINO) && defined(HABITINK_DEBUG_SERIAL)

#include "DebugSerial.h"

#include <Arduino.h>
#include <BoardConfig.h>
#include <HalGPIO.h>
#include <InputManager.h>
#include <esp_system.h>

namespace habitink {
namespace debugserial {

namespace {
const char* const kButtonNames[] = {"BACK", "CONFIRM", "LEFT", "RIGHT", "UP", "DOWN", "POWER"};
uint8_t g_lastMask = 0xFF;
unsigned long g_lastHeartbeat = 0;

uint8_t pressedMask() {
  uint8_t m = 0;
  for (uint8_t i = 0; i <= HalGPIO::BTN_POWER; ++i) {
    if (gpio.isPressed(i)) m |= (1u << i);
  }
  return m;
}
}  // namespace

void begin() {
  Serial.begin(115200);
  // Give the host ~2.5 s to open the CDC port after enumeration so the banner
  // is not lost; HWCDC drops writes when nothing is listening.
  delay(2500);
  Serial.println();
  Serial.println("[DBG] HabitInk debug build " HABITINK_VERSION " starting");
  // Why did we boot? 1=power-on 3=sw 4=panic 5/6/7=watchdogs 8=deep-sleep 9=brownout
  Serial.printf("[DBG] resetReason=%d freeHeap=%u\n", static_cast<int>(esp_reset_reason()),
                static_cast<unsigned>(ESP.getFreeHeap()));
}

void banner() {
  const auto& p = BoardConfig::ACTIVE;
  Serial.printf("[DBG] deviceIsX3=%d board=%d (%s) inputStyle=%d display=%dx%d\n",
                gpio.deviceIsX3() ? 1 : 0, static_cast<int>(p.board), p.name,
                static_cast<int>(p.inputStyle), p.displayWidth, p.displayHeight);
  Serial.printf("[DBG] input pins back=%d confirm=%d left=%d right=%d up=%d down=%d power=%d activeHigh=%d\n",
                p.input.back, p.input.confirm, p.input.left, p.input.right, p.input.up,
                p.input.down, p.input.power, p.input.powerActiveHigh ? 1 : 0);
  Serial.printf("[DBG] adc ladder pins %d / %d\n", InputManager::BUTTON_ADC_PIN_1,
                InputManager::BUTTON_ADC_PIN_2);
  Serial.printf("[DBG] wakeupReason=%d powerHeldMs=%lu\n", static_cast<int>(gpio.getWakeupReason()),
                gpio.getPowerButtonHeldTime());
  Serial.println("[DBG] press each button now; state changes and ADC readings follow");
}

void trace(const char* what, int arg) {
  Serial.printf("[DBG] %s %d heap=%u\n", what, arg, static_cast<unsigned>(ESP.getFreeHeap()));
  Serial.flush();
}

void tick() {
  const unsigned long now = millis();
  const uint8_t mask = pressedMask();
  const bool heartbeat = (now - g_lastHeartbeat) >= 2000;
  if (mask == g_lastMask && !heartbeat) return;
  g_lastMask = mask;
  g_lastHeartbeat = now;
  const int adc1 = analogRead(InputManager::BUTTON_ADC_PIN_1);
  const int adc2 = analogRead(InputManager::BUTTON_ADC_PIN_2);
  Serial.printf("[DBG] t=%lums adc1=%d adc2=%d power=%d pressed=", now, adc1, adc2,
                digitalRead(BoardConfig::ACTIVE.input.power));
  if (mask == 0) Serial.print("-");
  for (uint8_t i = 0; i <= HalGPIO::BTN_POWER; ++i) {
    if (mask & (1u << i)) { Serial.print(kButtonNames[i]); Serial.print(' '); }
  }
  Serial.println();
}

}  // namespace debugserial
}  // namespace habitink

#endif  // ARDUINO && HABITINK_DEBUG_SERIAL
