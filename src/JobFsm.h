#ifndef CNC_3018_JOBFSM_H
#define CNC_3018_JOBFSM_H

#include <SD.h>

#include "etl/fsm.h"
#include "etl/message.h"
#include "etl/array.h"

#include <string>

const etl::message_router_id_t JOB_BUS_NUMBER = 1;

struct StateId {
    enum {
        INIT,
        FINISH,
        READY,
        WAIT_RESP,
        PAUSED,
        NUMBER_OF_STATES
    };
};

struct EventId {
    enum {
        FILE,
        START,
        PAUSE,
        RESUME,
        COMPLETE,
        SEND,
        ACK,
        RESEND
    };
};

struct SetFileMessage : public etl::message<EventId::FILE> {
    const char* fileName;
};
struct CompleteMessage : public etl::message<EventId::COMPLETE> {
};
struct StartMessage : public etl::message<EventId::START> {
};
struct PauseMessage : public etl::message<EventId::PAUSE> {
};
struct ResumeMessage : public etl::message<EventId::RESUME> {
};
struct SendMessage : public etl::message<EventId::SEND> {
    std::string& cmd;
    SendMessage(std::string& s): cmd(s) {};
};
struct AckMessage : public etl::message<EventId::ACK> {
};
struct ResendMessage : public etl::message<EventId::RESEND> {
    uint32_t numer;
};

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

class JobFsm : public etl::fsm {

public:
    JobFsm() : etl::fsm(JOB_BUS_NUMBER) {}

    ~JobFsm() {
        closeFile();
    }

    static constexpr size_t MAX_LINE_LEN = 100;
    static constexpr size_t MAX_BUF = 15;

    File gcodeFile;
    GCodeDevice* dev;
    //file size + file pos used as % of done
    uint32_t fileSize;
    uint32_t filePos;
    uint32_t startTime;
    uint32_t endTime;
    // add LineNumber and CRC to command
    bool addLineN = false;

    char curLine[MAX_LINE_LEN + 1];
    size_t curLinePos;
    size_t curLineNum;

    etl::array<std::string, MAX_BUF> buffer;

    bool readCommandsToBuffer();

    void setFile(const char* file);

    void closeFile();

    uint32_t getPrintDuration();

};

#endif //CNC_3018_JOBFSM_H
