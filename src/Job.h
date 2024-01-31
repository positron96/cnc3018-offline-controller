#pragma once

#include "JobFsm.h"
#include "debug.h"

typedef etl::observer<JobStatusEvent> JobObserver;

class StopState : public etl::fsm_state<JobFsm, StopState, StateId::INVALID, SetFileMessage, CompleteMessage> {
public:
    etl::fsm_state_id_t on_enter_state() {
        get_fsm_context().closeFile();
        return STATE_ID;
    }

    etl::fsm_state_id_t on_event(const SetFileMessage& event) {
        get_fsm_context().setFile(event.fileName);
        return StateId::READY;
    }

    etl::fsm_state_id_t on_event(const CompleteMessage& event) {
        get_fsm_context().closeFile();
        return StateId::INVALID;
    }

    etl::fsm_state_id_t on_event_unknown(const etl::imessage& msg) {
        LOGF("error state: %d", msg.get_message_id());
        return STATE_ID;
    }
};

class ReadyState
        : public etl::fsm_state<JobFsm, ReadyState, StateId::READY, StartMessage, PauseMessage, CompleteMessage, ResendMessage> {
public:
    etl::fsm_state_id_t on_enter_state() {
        //TODO
        return STATE_ID;
    }

    etl::fsm_state_id_t on_event(const StartMessage& event) {
        get_fsm_context().startTime = millis();
        return StateId::READY;
    }

    etl::fsm_state_id_t on_event(const PauseMessage& event) {
        return StateId::PAUSED;
    }

    etl::fsm_state_id_t on_event(const ResendMessage& event) {
        // todo
        return StateId::READY;
    }

    etl::fsm_state_id_t on_event(const CompleteMessage& event) {
        // todo ?? may be different state??
        return StateId::INVALID;
    }

    etl::fsm_state_id_t on_event_unknown(const etl::imessage& msg) {
        LOGF("error state: %d", msg.get_message_id());
        return STATE_ID;
    }
};

class PauseState : public etl::fsm_state<JobFsm, PauseState, StateId::PAUSED, ResumeMessage, CompleteMessage> {
public:
    etl::fsm_state_id_t on_event(const ResumeMessage& event) {
        return StateId::READY;
    }

    etl::fsm_state_id_t on_event(const CompleteMessage& event) {
        // todo ?? may be different state??
        return StateId::INVALID;
    }

    etl::fsm_state_id_t on_event_unknown(const etl::imessage& msg) {
        LOGF("error state: %d", msg.get_message_id());
        return STATE_ID;
    }
};

class WaitState
        : public etl::fsm_state<JobFsm, WaitState, StateId::WAIT_RESP, SendMessage, AckMessage, PauseMessage, CompleteMessage> {
public:
    etl::fsm_state_id_t on_event(const SendMessage& event) {
        // todo
        return StateId::WAIT_RESP;
    }

    etl::fsm_state_id_t on_event(const AckMessage& event) {
        // todo
        return StateId::READY;
    }

    etl::fsm_state_id_t on_event(const PauseMessage& event) {
        // todo
        return StateId::PAUSED;
    }

    etl::fsm_state_id_t on_event(const CompleteMessage& event) {
        // todo ?? may be different state??
        return StateId::INVALID;
    }

    etl::fsm_state_id_t on_event_unknown(const etl::imessage& msg) {
        LOGF("error state: %d", msg.get_message_id());
        return STATE_ID;
    }
};

///
/// Represents gcode program read from SD card.
/// Shim class abstracting FSM as method calls.
///
class Job :  public etl::observable<JobObserver, 3> {
    JobFsm* fsm;
public:

    Job() {
        fsm = new JobFsm();

        StopState stopState;
        ReadyState readyState;
        PauseState pauseState;
        WaitState waitState;
        etl::ifsm_state* stateList[StateId::NUMBER_OF_STATES] = {&stopState, &readyState, &waitState, &pauseState};

        fsm->set_states(stateList, etl::size(stateList));
    }

    void inline setDevice(GCodeDevice* dev_) {
        fsm->dev = dev_;
    }

    void inline setFile(const char* file) {
        fsm->setFile(file);
    }

    void start() {
        fsm->receive(StartMessage{});
    }

    void inline setPaused(bool v) { fsm->receive(PauseMessage{}); }

    bool inline isRunning() {
        unsigned char i = fsm->get_state_id();
        return i == StateId::READY || i == StateId::WAIT_RESP;
    }

    bool inline isValid() {
        return (bool) fsm->gcodeFile;
    }

    uint8_t getCompletion() {
        if (isValid())
            return round((100.0 * fsm->filePos) / (fsm->fileSize > 0 ? fsm->fileSize : 1));
        else
            return 0;
    }

    void step() {
        switch (fsm->get_state_id()) {
            case StateId::READY:
                fsm->scheduleNextCommand();
                notify_observers(JobStatusEvent{JobStatus::REFRESH_SIG});
                break;
            case StateId::WAIT_RESP:

                break;
            case StateId::PAUSED:
            default:
                return;
        }
    }

    void notification(const DeviceStatusEvent& e) {
        //todo
        if (e.statusField == GCodeDevice::DeviceStatus::DEV_ERROR && isValid()) {
            fsm->receive(CompleteMessage());
        }
    }

};