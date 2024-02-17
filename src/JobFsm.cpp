#include "JobFsm.h"
#include <string>

bool JobFsm::readCommandsToBuffer() {
    if (filePos >= fileSize){
        return false;
    }
    //usually gcode has no long empty regions
    for (size_t gard = 10; gard > 0; --gard) { // loop gard
        bool got_comment = false;
        while (gcodeFile.available() > 0) {
            int readChar = gcodeFile.read();
            filePos++;
            if (';' == readChar) {
                got_comment = true;
                continue;
            }
            if ('\n' == readChar || '\r' == readChar) {
                got_comment = false;
                if (curLinePos != 0)
                    break; // if it's an empty string or LF after last CR, just continue reading
            } else {
                if (curLinePos < MAX_LINE_LEN && !got_comment) {
                    curLine[curLinePos++] = readChar;
                }
            }
        }
        curLine[curLinePos] = 0;
        LOGLN("Got new line");

        bool empty = true;
        for (size_t i = 0; i < curLinePos; i++) {
            if (!isspace(curLine[i])) {
                empty = false;
            }
        }

        if (curLinePos == 0 || empty)
           curLinePos = 0;
        else 
            break;
    }
    LOGLN("put line to buf");
    std::string line(curLinePos + (addLineN ? 10 : 0), '\0');
    if (addLineN) {
        // line number and checksum
        line = "N";
        line += std::to_string(++curLineNum);
        line += " ";
        line += curLine;
        // right trim
        line.erase(std::find_if(line.rbegin(), line.rend(),
                                std::not1(std::ptr_fun<int, int>(std::isspace))).base(), line.end());
        uint8_t checksum = 0, count = line.length();
        const char* out = line.c_str();
        while (count)
            checksum ^= out[--count];
        line += '*';
        line += std::to_string(checksum);
    } else {
        line = curLine;
    }
    //END line number and checksum
    buffer.insert_at(curLineNum % MAX_BUF, line);
    curLinePos = 0;
    return true;
}


void JobFsm::setFile(const char* file) {
    gcodeFile = SD.open(file);
    if (gcodeFile) {
        fileSize = gcodeFile.size();
    }
    filePos = 0;
    curLinePos = 0;
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
