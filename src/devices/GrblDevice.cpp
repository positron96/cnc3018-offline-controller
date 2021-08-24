#include "GCodeDevice.h"


    bool GrblDevice::jog(uint8_t axis, float dist, int feed) {
        constexpr static char AXIS[] = {'X', 'Y', 'Z'};
        char msg[81]; snprintf(msg, 81, "$J=G91 F%d %c%.3f", feed, AXIS[axis], dist);
        return scheduleCommand(msg, strlen(msg) );
    }
        
    bool GrblDevice::canJog() {        
        return status=="Idle" || status=="Jog";
        
    }

    bool GrblDevice::isCmdRealtime(char* data, size_t len) {
        if (len != 1) return false;
        char c = data[0];
        switch(c) {
            case '?': // status
            case '~': // cycle start/stop
            case '!': // feedhold
            case 0x18: // ^x, reset
            case 0x84: // door
            case 0x85: // jog cancel  
            case 0x9E: // toggle spindle
            case 0xA0: // toggle flood coolant
            case 0xA1: // toggle mist coolant      
                return true;
            default:
                // feed override, rapid override, spindle override
                if( c>=0x90 && c<=0x9D) return true;
                return false;
        }
    }

    void GrblDevice::trySendCommand() {

        if(isCmdRealtime(curUnsentPriorityCmd, curUnsentPriorityCmdLen) ) {
            printerSerial->write(curUnsentPriorityCmd, curUnsentPriorityCmdLen);  
            GD_DEBUGF("<  (f%3d,%3d) '%c' RT\n", sentCounter->getFreeLines(), sentCounter->getFreeBytes(), curUnsentPriorityCmd[0] );
            curUnsentPriorityCmdLen = 0;
            return;
        }

        char* cmd  = curUnsentPriorityCmdLen!=0 ? &curUnsentPriorityCmd[0] :  &curUnsentCmd[0]; 
        size_t * len = curUnsentPriorityCmdLen!=0 ? &curUnsentPriorityCmdLen : &curUnsentCmdLen ;

        if( sentCounter->canPush(*len) ) {
            sentCounter->push( cmd, *len );
            printerSerial->write(cmd, *len);  
            printerSerial->print('\n');
            GD_DEBUGF("<  (f%3d,%3d) '%s' (%d)\n", sentCounter->getFreeLines(), sentCounter->getFreeBytes(), cmd, *len );
            *len = 0;
        } else {
            //if(loadedNewCmd) GD_DEBUGF("<  Not sent, free lines: %d, free space: %d\n", sentQueue.getFreeLines() , sentQueue.getFreeBytes()  );
        }

    }

    void GrblDevice::tryParseResponse( char* resp, size_t len ) {
        if (startsWith(resp, "ok")) {
            sentQueue.pop();
            //responseDetail = "ok";
            connected = true;
            panic = false;
        } else 
        if (startsWith(resp, "error") || startsWith(resp, "ALARM:") ) {
            sentQueue.pop();
            panic = true;
            GD_DEBUGF("ERR '%s'\n", resp ); 
            notify_observers(DeviceStatusEvent{1}); 
            lastResponse = resp;
        } else
        if ( startsWith(resp, "<") ) {
            parseGrblStatus(resp+1);
        } else 
        if(startsWith(resp, "[MSG:")) {
            GD_DEBUGF("Msg '%s'\n", resp ); 
            lastResponse = resp;
        }        
        
        GD_DEBUGF(" > (f%3d,%3d) '%s' \n", sentQueue.getFreeLines(), sentQueue.getFreeBytes(),resp );
    }

    void mystrcpy(char* dst, const char* start, const char* end) {
        while(start!=end) {
            *(dst++) = *(start++);
        }
        *dst=0;
    }

    void GrblDevice::parseGrblStatus(char* v) {
        //<Idle|MPos:9.800,0.000,0.000|FS:0,0|WCO:0.000,0.000,0.000>
        //<Idle|MPos:9.800,0.000,0.000|FS:0,0|Ov:100,100,100>
        //GD_DEBUGF("parsing %s\n", v.c_str() );
        
        char buf[10];
        bool mpos;
        char cpy[100];
        strncpy(cpy, v, 100);
        v=cpy;

        // idle/jogging
        char* pch = strtok(v, "|");
        if(pch==nullptr) return;
        status = pch; 
        //GD_DEBUGF("Parsed Status: %s\n", status.c_str() );

        // MPos:0.000,0.000,0.000
        pch = strtok(nullptr, "|"); 
        if(pch==nullptr) return;
        
        char *st, *fi;
        st=pch+5;fi = strchr(st, ',');   mystrcpy(buf, st, fi);  x = atof(buf);
        st=fi+1; fi = strchr(st, ',');   mystrcpy(buf, st, fi);  y = atof(buf);
        st=fi+1;                                                 z = atof(st);
        mpos = startsWith(pch, "MPos");
        //GD_DEBUGF("Parsed Pos: %f %f %f\n", x,y,z);

        // FS:500,8000 or F:500    
        pch = strtok(nullptr, "|"); 
        while(pch!=nullptr) {
        
            if( startsWith(pch, "FS:") || startsWith(pch, "F:")) {
                if(pch[1] == 'S') {
                    st=pch+3; fi = strchr(st, ','); mystrcpy(buf, st, fi);  feed = atoi(buf);
                    st=fi+1;  spindleVal = atoi(st);
                } else {
                    feed = atoi(pch+2);
                }
            } else 
            if(startsWith(pch, "WCO:")) {
                st=pch+4;fi = strchr(st, ',');   mystrcpy(buf, st, fi);  ofsX = atof(buf);
                st=fi+1; fi = strchr(st, ',');   mystrcpy(buf, st, fi);  ofsY = atof(buf);
                st=fi+1;                                                 ofsZ = atof(st);
                //GD_DEBUGF("Parsed WCO: %f %f %f\n", ofsX, ofsY, ofsZ);
            }

            pch = strtok(nullptr, "|"); 

        }
        
        if(!mpos) {
            x -= ofsX; y -= ofsY; z -= ofsZ;
        }
        
        notify_observers(DeviceStatusEvent{0});
    }

