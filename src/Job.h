#pragma once

#include <Arduino.h>
#include <SD.h>
#include <etl/observer.h>

#include "devices/GCodeDevice.h"


//#define ADD_LINENUMBERS 

//struct JobStatusEvent{  int status;  };
typedef int JobStatusEvent;

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
class Job : public DeviceObserver, public etl::observable<JobObserver, 3> {

public:

    static Job* getJob();
    //static void setJob(Job* job);

    ~Job() { if(gcodeFile) gcodeFile.close(); clear_observers(); }

    void loop();

    void setFile(String file) { 
        if(gcodeFile) gcodeFile.close();

        gcodeFile = SD.open(file);
        if(gcodeFile) fileSize = gcodeFile.size();
        filePos = 0;
        running = false; 
        paused = false;
        cancelled = false;
        notify_observers(JobStatusEvent{0}); 
        curLineNum = 0;
        startTime=0;
        endTime=0;
    }

    void notification(const DeviceStatusEvent& e) override {
        if(e.statusField==1 && isValid() ) {
            Serial.println("Device error, canceling job");
            cancel();
        }
    }

    void start() { startTime = millis(); paused=false; running=true;  notify_observers(JobStatusEvent{0}); }
    void cancel() { cancelled=true; stop(); notify_observers(JobStatusEvent{0});  }
    bool isRunning() {  return running; }
    bool isCancelled() { return cancelled; }

    void pause() { setPaused(true);  }
    void resume() { setPaused(false); }
    void setPaused(bool v) { paused = v; notify_observers(JobStatusEvent{0}); }
    bool isPaused() { return paused; }

    float getCompletion() { if(isValid()) return 1.0 * filePos/fileSize; else return 0; }
    size_t getFilePos() { if(isValid()) return filePos; else return 0;}
    size_t getFileSize() { if(isValid()) return fileSize; else return 0;}
    bool isValid() { return (bool)gcodeFile; }
    String getFilename() { if(isValid()) return gcodeFile.name(); else return ""; }
    uint32_t getPrintDuration() { return (endTime!=0 ? endTime : millis())-startTime; }

private:

    File gcodeFile;
    uint32_t fileSize;
    uint32_t filePos;
    uint32_t startTime;
    uint32_t endTime;
    static const int MAX_LINE = 100;
    char curLine[MAX_LINE+1];
    size_t curLinePos;

    size_t curLineNum;

    //float percentage = 0;
    bool running;
    bool cancelled;
    bool paused;

    void stop() {   
        paused = false;
        running = false; 
        endTime=millis();
        if(gcodeFile) gcodeFile.close();
        notify_observers(JobStatusEvent{0}); 
    }
    void readNextLine();
    bool scheduleNextCommand(GCodeDevice *dev);


    static Job job;

};