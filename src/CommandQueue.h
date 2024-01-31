#pragma once


#include <Arduino.h>

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

    virtual size_t peek(char*& msg) = 0;

    virtual void pop() = 0;
};


template <uint16_t LEN_LINES = 16, uint16_t LEN_BYTES = 128, uint8_t SUFFIX_LEN = 1>
class SimpleCounter : public Counter {
public:
    SimpleCounter() {
        freeBytes = LEN_BYTES;
    }

    void clear() override {
        queue.clear();
        freeBytes = LEN_BYTES;
    }

    bool canPush(size_t len) const override {
        return queue.size() < LEN_LINES && freeBytes >= len + SUFFIX_LEN;
    }

    bool push(char* msg, size_t len) override {
        if (!canPush(len)) return false;
        queue.push(len);
        freeBytes -= len + SUFFIX_LEN;
        return true;
    }

    inline size_t size() const override {
        return queue.size();
    }

    inline size_t getFreeLines() const override {
        return LEN_LINES - queue.size();
    }

    inline size_t bytes() const override {
        return LEN_BYTES - freeBytes;
    }

    inline size_t getFreeBytes() const override {
        return freeBytes;
    }

    size_t peek(char*& msg) override {
        if (queue.size() == 0) return 0;
        return queue.front();
    }

    void pop() override {
        if (queue.size() == 0) return;
        size_t v = queue.front();
        queue.pop();
        freeBytes += v + SUFFIX_LEN;
    }


private:
    etl::queue<size_t, LEN_LINES> queue;
    size_t freeBytes;
};
