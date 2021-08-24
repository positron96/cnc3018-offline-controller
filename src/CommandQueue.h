#pragma once


#include <Arduino.h>
#include <message_buffer.h>
#include <etl/queue.h>

class Counter {
public:
    virtual void clear() = 0;

    virtual bool canPush(size_t len) const = 0;

    virtual bool push(char* msg, size_t len) = 0;

    virtual size_t size() const = 0;

    virtual size_t getFreeLines() const = 0;

    virtual size_t bytes() const = 0;

    virtual size_t getFreeBytes() const = 0;

    virtual size_t peek(char* &msg) = 0;

    virtual void pop() = 0;
};


template< uint16_t LEN_LINES = 16, uint16_t LEN_BYTES = 128, uint8_t MAX_LINE_LEN = 100 >
class SizedQueue: public Counter {
public:
    SizedQueue() {
        freeLines = LEN_LINES;
        freeBytes = LEN_BYTES;
        buf = xMessageBufferCreateStatic(LEN_BYTES, data, &bufStruct);
    }

    void clear() override {
        xMessageBufferReset(buf);
        havePeekedLine = false;
        peekedLineLen = 0;
        freeLines = LEN_LINES;
        freeBytes = LEN_BYTES;
    }

    bool canPush(size_t len) const override {
        if(len>MAX_LINE_LEN) len = MAX_LINE_LEN;
        return freeBytes>len+1 && freeLines>0;
    }

    bool push(char* msg, size_t len)  override  {
        if(!canPush(len)) return false;
        if(len>MAX_LINE_LEN) len = MAX_LINE_LEN;
        assert( xMessageBufferSend(buf, msg, len, 0) );
        freeLines --;
        freeBytes -= len+1;
        return true;
    }

    inline size_t size() const override  {
        return LEN_LINES-freeLines;
    }

    inline size_t getFreeLines() const override  {
        return freeLines;
    }

    inline size_t bytes() const override  {
        return LEN_BYTES-freeBytes;
    }

    inline size_t getFreeBytes() const  override {
        return freeBytes;
    }

    size_t peek(char* &msg) override  {
        if(size() == 0) return 0;
        if(!havePeekedLine) {
            loadLineFromBuf();
        }
        //len = peekedLineLen;
        msg = peekedLine;
        return peekedLineLen;
    }

    void pop()  override {
        if(size() == 0) return;
        freeLines ++;
        if(havePeekedLine) {
            freeBytes += peekedLineLen+1;
            havePeekedLine = false;
            peekedLineLen = 0;
        } else {
            size_t r = xMessageBufferReceive(buf, peekedLine, MAX_LINE_LEN, 0);
            freeBytes += r+1;
        }

    }


private:
    MessageBufferHandle_t buf;
    StaticMessageBuffer_t bufStruct;
    uint8_t data[LEN_BYTES];

    size_t freeLines;
    size_t freeBytes;
    char peekedLine[MAX_LINE_LEN+1];
    size_t peekedLineLen;
    bool havePeekedLine;

    bool loadLineFromBuf() {
        peekedLineLen = xMessageBufferReceive(buf, peekedLine, MAX_LINE_LEN, 0);
        havePeekedLine = peekedLineLen>0;
        peekedLine[peekedLineLen]=0;
        return havePeekedLine;
    }
};


template< uint16_t LEN_LINES = 16, uint16_t LEN_BYTES = 128, uint8_t SUFFIX_LEN=1>
class SimpleCounter : public Counter {
public:
    SimpleCounter() {
        freeBytes = LEN_BYTES;
    }

    void clear()  override {
        queue.clear();
        freeBytes = LEN_BYTES;
    }

    bool canPush(size_t len) const override {
        return queue.size()<LEN_LINES && freeBytes >= len+SUFFIX_LEN;
    }

    bool push(char* msg, size_t len) override {
        if(!canPush(len)) return false;
        queue.push(len);
        freeBytes -= len+SUFFIX_LEN;
        return true;
    }

    inline size_t size() const  override {
        return queue.size();
    }

    inline size_t getFreeLines() const  override {
        return LEN_LINES - queue.size();
    }

    inline size_t bytes() const  override {
        return LEN_BYTES-freeBytes;
    }

    inline size_t getFreeBytes() const  override {
        return freeBytes;
    }

    size_t peek(char* &msg)  override {
        if(queue.size()==0) return 0;
        return queue.front();
    }

    void pop()  override {
        if(queue.size()==0) return ;
        size_t v = queue.front();
        queue.pop();
        freeBytes += v+SUFFIX_LEN;
    }



private:
    etl::queue<size_t, LEN_LINES> queue;
    size_t freeBytes;
};
