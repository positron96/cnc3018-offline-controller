#include "Job.h"

extern uint8_t cmdBuf;

/// file can be shorter then Marlin buffer.
// it dump it all and close
void Job::readNextLine() {
    if (gcodeFile.available() == 0) {
        stop();
        return;
    }
    while (gcodeFile.available() > 0) {
        int rd = gcodeFile.read();
        filePos++;
        if (filePos % 200 == 0)
            notify_observers(JobStatusEvent{JobStatus::REFRESH_SIG}); // every Nth byte
        if (rd == '\n' || rd == '\r') {
            if (curLinePos != 0)
                break; // if it's an empty string or LF after last CR, just continue reading
        } else {
            if (curLinePos < MAX_LINE_LEN)
                curLine[curLinePos++] = rd;
            else {
                stop();
                break;
            }
        }
    }
    curLine[curLinePos] = 0;
}

bool Job::scheduleNextCommand() {
    if (dev->isInPanic()) {
        cancel();
        return false;
    }

    if (paused)
        return false;

    if (curLinePos == 0) {
        readNextLine();
        LOGLN("Got new line");
        if (!running)
            return false;    // don't run next time

        char *pos = strchr(curLine, ';'); // strip comments
        if (pos != nullptr) {
            *pos = 0;
            curLinePos = pos - curLine;
        }

        bool empty = true;
        for (size_t i = 0; i < curLinePos; i++)
            if (!isspace(curLine[i]))
                empty = false;

        if (curLinePos == 0 || empty) {
            return true;
        } // can seek next
        // line number and checksum
        char out[MAX_LINE_LEN + 1];
        snprintf(out, MAX_LINE_LEN, "N%d %s", ++curLineNum, curLine);
        uint8_t checksum = 0, count = strlen(out);
        while (count)
            checksum ^= out[--count];

        snprintf(curLine, MAX_LINE_LEN, "%s*%d", out, checksum);
        curLinePos = strlen(curLine);
        // buffer 
        Line line{curLineNum};
        memcpy(line.cmd, curLine, curLinePos);
        size_t idx = ++currentBufferPosition % MAX_BUF;
        lineBuffer[idx] = line;
    }

    if (queueLine(curLinePos, curLine)) {
        curLinePos = 0;
        return true;
    } else {
        return false;
    }
}

bool Job::queueLine(size_t line, const char *cmd) {
    if (dev->canSchedule(line)) {
        LOGF("queueing line '%s', len %d ", cmd, line);
        if (dev->scheduleCommand(cmd, line)) {
            LOGLN("queued");
            return true;
        }
    }
    return false; // stop trying for this cycle
}

void Job::tryResendLine(size_t number) {
    if (number >= 0)
        needResend = number;
}

void Job::resendLine() { // TODO may be done faster with no re search
    for (size_t i = 0; i < MAX_BUF; ++i) {
        size_t i1 = (curLineNum - i) % MAX_BUF;
        if (lineBuffer[i1].num == needResend) {
            LOGF("Found %d line in buffer\n", lineBuffer[i1].num);
            if (queueLine(needResend, lineBuffer[i1].cmd)) {
                needResend = -1;
                resendTried = 0;
                return;
            } else {
                ++resendTried;
                return;
            }
        }
    }
    LOGLN("Fail to find in buffer");
    cancel();
};

void Job::loop() {
    if (!running || paused) return;

    if (dev == nullptr) return;

    if (needResend > 0) {
        LOGLN("resend");
        if (resendTried > 3) {
            needResend = -1;
            resendTried = 0;
            LOGF("Give up trying to resend");
            stop();
            return;
        }
        resendLine();
    }
    for (uint8_t i = 0; i < 0x3
                        && scheduleNextCommand(); ++i) {}
}

void Job::stop() {
    paused = false;
    running = false;
    endTime = millis();
    currentBufferPosition = 0;
    if (gcodeFile) {
        gcodeFile.close();
    }
    notify_observers(JobStatusEvent{JobStatus::REFRESH_SIG});
}

void Job::setFile(const char *file) {
    if (gcodeFile) {
        gcodeFile.close();
    }
    gcodeFile = SD.open(file);
    if (gcodeFile) {
        fileSize = gcodeFile.size();
    }
    filePos = 0;
    LOGF("fPos: %d, fSiz: %d\n", filePos, fileSize);
    running = false;
    paused = false;
    cancelled = false;
    notify_observers(JobStatusEvent{JobStatus::REFRESH_SIG});
    curLineNum = 0;
    startTime = 0;
    endTime = 0;
    needResend = -1;
    resendTried = 0;
}

void Job::notification(const DeviceStatusEvent &e) {
    if (e.statusField == GCodeDevice::DeviceStatus::DEV_ERROR && isValid()) {
        LOGLN("Device error, canceling job");
        cancel();
    }
}

uint8_t Job::getCompletion() {
    if (isValid())
        return round((100.0 * filePos) / (fileSize > 0 ? fileSize : 1));
    else
        return 0;
}

void Job::start() {
    startTime = millis();
    currentBufferPosition = 0;
    needResend = -1;
    resendTried = 0;
    paused = false;
    running = true;
    notify_observers(JobStatusEvent{JobStatus::REFRESH_SIG});
}

void Job::cancel() {
    cancelled = true;
    stop();
    notify_observers(JobStatusEvent{JobStatus::REFRESH_SIG});
}

String Job::getFilename() {
    if (gcodeFile)
        return gcodeFile.name();
    else
        return "";
}

uint32_t Job::getPrintDuration() {
    return (endTime != 0 ? endTime : millis()) - startTime;
}
