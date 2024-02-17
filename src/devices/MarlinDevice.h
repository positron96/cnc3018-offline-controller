#pragma once

#include "Job.h"
#include "GCodeDevice.h"
#include "gcode/gcode.h"
#include "etl/deque.h"
#include <string>


class MarlinDevice : public GCodeDevice {
public:
    constexpr static uint32_t BUFFER_LEN = 255;
    constexpr static uint32_t SHORT_BUFFER_LEN = 100;
    const etl::vector<u_int16_t, sizeof(u_int16_t) * 5> SPINDLE_VALS{0, 1, 64, 128, 255};

    static void sendProbe(Stream& serial);

    static bool checkProbeResponse(String s);


    //// CONSTRUCTORS
    MarlinDevice(WatchedSerial* s, Job* job);

    MarlinDevice() : GCodeDevice() {
    }

    virtual ~MarlinDevice() {}

    const etl::ivector<u_int16_t>& getSpindleValues() const override;

    bool jog(uint8_t axis, float dist, int feed) override;

    bool canJog() override;

    void begin() override;

    void reset() override;

    void requestStatusUpdate() override;

    bool schedulePriorityCommand(const char* cmd, size_t len) override;

    bool scheduleCommand(const char* cmd, size_t len) override;

    const char* getStatusStr() const override;

    void toggleRelative();

    bool tempChange(uint8_t temp);

    bool isRelative() const { return relative; }

    float getE() const { return e; }

    float getTemp() const { return hotendTemp; }

    float getBedTemp() const { return bedTemp; }

protected:
    void trySendCommand() override;

    void tryParseResponse(char* cmd, size_t len) override;

private:
    etl::deque<String, 10> outQueue;

    float hotendTemp = 0.0, bedTemp = 0.0;
    size_t hotendPower = 0;
    size_t bedPower = 0;
    float hotendRequestedTemp = 0.0,
            bedRequestedTemp = 0.0,
            e = 0.0;

    bool relative = false;

    int resendLine = -1;

    void parseError(const char* input);

    void parseOk(const char* v, size_t len);
};
