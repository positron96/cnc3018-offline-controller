#pragma once

#include <Arduino.h>
#include <HardwareSerial.h>

class WatchedSerial : public Stream {
private:
    HardwareSerial &upstream;
    int pin;
    uint32_t lastUpdate;
    constexpr static uint32_t CACHE_DURATION = 50;
    bool cachedLocked;
public:
    WatchedSerial(HardwareSerial &s, int pin) : upstream(s), pin(pin) {}

    void begin(size_t baud) {
        upstream.begin(baud);
        pinMode(pin, INPUT);
    }

    void end() {
        upstream.end();
    }

    size_t write(uint8_t v) override {
        if (isLocked()) return 0;
        return upstream.write(v);
    };

    size_t write(const uint8_t *buffer, size_t size) override {
        if (isLocked()) return 0;
        return upstream.write(buffer, size);
    }

    int availableForWrite(void) override {
        if (isLocked()) { return 0; }
        return upstream.availableForWrite();
    };

    int available() override { return upstream.available(); };

    int read() override { return upstream.read(); };

    int peek() override { return upstream.peek(); };

    void flush() override { upstream.flush(); };

    bool isLocked(bool forceRead = false) {
        if (forceRead || int32_t(millis() - lastUpdate) > (int32_t) CACHE_DURATION) {
            // todo read Detector pin
            cachedLocked = digitalRead(pin) == HIGH;
            lastUpdate = millis();
        }
        return cachedLocked;
    }
};
