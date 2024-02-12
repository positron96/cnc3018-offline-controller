//
// Created by lima on 2/1/24.
//

#include "JobFsm.h"

#include <string>


bool JobFsm::scheduleNextCommand() {
    if (buffer.full()){
        return false;
    }
    //usually gcode has no long empty regions
    for (size_t gard = 10; gard > 0; --gard) { // loop gard
        /// TODO Concern is it possible this == 0 at the middle of file.
        bool got_comment = false;
        while (gcodeFile.available() > 0) {
            int rd = gcodeFile.read();
            filePos++;

            if(';' == rd ){
                got_comment = true;
                continue;
            }

            if ('\n' == rd || '\r' == rd) {
                got_comment = false;
                if (curLinePos != 0)
                    break; // if it's an empty string or LF after last CR, just continue reading
            } else {
                if (curLinePos < MAX_LINE_LEN && !got_comment ){
                    curLine[curLinePos++] = rd;
                }

            }
        }
        curLine[curLinePos] = 0;
        LOGLN("Got new line");

        bool empty = true;
        for (size_t i = 0; i < curLinePos; i++)
            if (!isspace(curLine[i]))
                empty = false;

        if (curLinePos == 0 || empty) {
            continue; // continue reading
        }
    }
    LOGLN("put line to buf");
    std::string line(curLinePos + 10, '\0');

    // line number and checksum
    line = "N";
    line += std::to_string(++curLineNum);
    line += " ";
    line += curLine;
    uint8_t checksum = 0, count = line.length();
    const char* out = line.c_str();
    while (count)
        checksum ^= out[--count];
    line += '*';
    line += std::to_string(checksum);
    //END line number and checksum

    buffer.insert(etl::pair<size_t, std::string>(curLineNum, line));
    curLinePos = 0;
    return true;
}

void JobFsm::resendLine(size_t lineNumber) {
    if (curLineNum - lineNumber < MAX_BUF ){
        LOGLN("Fail to find in buffer");
    } else {

    }

}

void JobFsm::stop() {
    endTime = millis();
    closeFile();
    // TODO notify_observers(JobStatusEvent{JobStatus::REFRESH_SIG});
}

void JobFsm::setFile(const char *file) {
    gcodeFile = SD.open(file);
    if (gcodeFile) {
        fileSize = gcodeFile.size();
    }
    filePos = 0;
     // todo chekc here to put it
//    notify_observers(JobStatusEvent{JobStatus::REFRESH_SIG});
    curLineNum = 0;
    startTime = 0;
    endTime = 0;
}

uint32_t JobFsm::getPrintDuration() {
    return (endTime != 0 ? endTime : millis()) - startTime;
}

void JobFsm::closeFile() {
    if (gcodeFile) {
        gcodeFile.close();
    }
}
