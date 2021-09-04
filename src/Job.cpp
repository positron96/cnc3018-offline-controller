#include "Job.h"

Job Job::job;

//void Job::setJob(Job* _job) { job = *_job; }

Job * Job::getJob() { return &job; }

#define J_DEBUGF(...) // { Serial.printf(__VA_ARGS__); }
#define J_DEBUGS(s)   // { Serial.println(s); }

void Job::readNextLine() {
    if(gcodeFile.available()==0) { 
        stop();
        return; 
    }
    while( gcodeFile.available()>0 ) {
        int rd = gcodeFile.read();
        filePos++;  if(filePos%200==0) notify_observers(JobStatusEvent{0}); // every Nth byte
        if(rd=='\n' || rd=='\r') {
            if(curLinePos!=0) break; // if it's an empty string or LF after last CR, just continue reading
        } else {
            if(curLinePos<MAX_LINE) curLine[curLinePos++] = rd;
            else { 
                stop(); 
                J_DEBUGF("Line length exceeded\n");
                break; 
            }
        }
    }
    curLine[curLinePos]=0;
}

bool Job::scheduleNextCommand(GCodeDevice *dev) {
    if(dev->isInPanic() ) {
        cancel();
        return false;
    }

    if(paused) return false;
    
    if(curLinePos==0) {
        readNextLine();
        if(!running) return false;    // don't run next time

        char* pos = strchr(curLine, ';');
        if(pos!=NULL) {*pos = 0; curLinePos = pos-curLine; }

        bool empty=false;//true;
        //for(int i=0; i<curLinePos; i++) if(!isspace(curLine[i])) empty=false;

        if(curLinePos==0 || empty) { return true; } // can seek next

        #ifdef ADD_LINENUMBERS
            char out[MAX_LINE+1];
            snprintf(out, MAX_LINE, "N%d %s", ++curLineNum, curLine);
            uint8_t checksum = 0, count = strlen(out);
            while (count) checksum ^= out[--count];
            snprintf(curLine, MAX_LINE, "%s*%d", out, checksum);
            curLinePos = strlen(curLine);
        #endif
    }

    if(dev->canSchedule(curLinePos)) {        
        
        J_DEBUGF("  J queueing line '%s', len %d\n", curLine, curLinePos );

        bool queued = dev->scheduleCommand(curLine, curLinePos);
        assert(queued);

        curLinePos = 0;
        return true; //can try next command

    } else return false; // stop trying for now
}

void Job::loop() {
    if(!running || paused) return;

    GCodeDevice * dev = GCodeDevice::getDevice();
    if(dev==nullptr) return;

    while( scheduleNextCommand(dev) ) {}

}