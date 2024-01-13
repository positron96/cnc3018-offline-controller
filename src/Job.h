#pragma once

#include <Arduino.h>
#include <SD.h>
#include <etl/observer.h>
#include <math.h>

#include "debug.h"
class Job;
#include "devices/GCodeDevice.h"


typedef int JobStatusEvent;

enum JobStatus {
    REFRESH_SIG
};

typedef etl::observer<JobStatusEvent> JobObserver;


/**
 * State diagram:
 * ```
 * non valid   <-------------------+
 *    |                            |
 *    | (.setFile)                 |
 *    v                            |
 *   valid ------------------------+
 *    |                            | 
 *    | (.start)                   | (.cancel)
 *    v                            | or EOF
 *   [ valid&running        ]----->^<----+
 *    |                     ^            |
 *    | (.pause)            | (.resume)  |
 *    v                     |            |
 *   [valid&running&paused]-+------------+
 *    
 * ```
 */

// TODO it is FSM. refactor it to FSM !!!
class Job : public DeviceObserver, public etl::observable<JobObserver, 3> {
public:


    Job() {}

    ~Job() {
        if (gcodeFile) {
            gcodeFile.close();
        }
        clear_observers();
    }

    void loop();

    void setDevice(GCodeDevice* de) {
        dev = de;
    }

    void setFile(const char* file);

    void notification(const DeviceStatusEvent& e) override;

    void start();

    void cancel();

    bool isRunning() { return running; }

    bool isCancelled() { return cancelled; }

    ///
    ///  set from UI
    ///
    void setPaused(bool v) { paused = v; }

    bool isPaused() { return paused; }

    uint8_t getCompletion();

    size_t getFilePos() { if (isValid()) return filePos; else return 0; }

    size_t getFileSize() { if (isValid()) return fileSize; else return 0; }

    bool isValid() {
        return (bool) gcodeFile;
    }


    String getFilename();

    uint32_t getPrintDuration();

    void tryResendLine(size_t index);

private:
    static constexpr size_t MAX_LINE_LEN = 100;
    struct Line {
        size_t num;
        char cmd[MAX_LINE_LEN + 1];
    };
    static const size_t MAX_BUF = 30;
    File gcodeFile;
    GCodeDevice* dev;
    uint32_t fileSize;
    uint32_t filePos;
    uint32_t startTime;
    uint32_t endTime;

    char curLine[MAX_LINE_LEN + 1];
    size_t curLinePos;
    size_t curLineNum;

    bool running;
    bool cancelled;
    bool paused;

    int needResend = -1;
    uint8_t resendTried = 0;
    Line lineBuffer[30];
    int currentBufferPosition = -1;

    void stop();

    void readNextLine();

    bool scheduleNextCommand();

    void resendLine();

    bool queueLine(size_t curLinePos, const char* cmd);
};