#pragma once
#include <raylib.h>

class Timer {
    float len, tm;
    bool tmout;
    public:
    Timer(float length) : len(length), tm(0.0f), tmout(false) {}
    bool step(float tmDelta){
        tm += tmDelta;
        if(tm >= len){
            tm -= len;
            tmout = true;
            return true;
        }
        return false;
    }
    void prime() { tm = len; tmout = true; }
    bool isTmOut() const { return tmout; }
    float getTime() const { return tm; }
    void reset() { tm = 0.0f; tmout = false; }
    float getLength() const { return len; }

};

class Animation{
    Timer timer;
    int frameCount;
public:
    Animation() : timer(0) , frameCount(0) {}
    Animation(int frameCount, float frameLength) : timer(frameLength), frameCount(frameCount) {}
    int curFrame() const {
        if (frameCount <= 0 || timer.getLength() <= 0.0f) return 0;
        int frame = static_cast<int>(timer.getTime() / timer.getLength() * frameCount);
        return frame >= frameCount ? frameCount - 1 : frame;
    }
    float getLength() const { return timer.getLength(); }
    void step(float tmDelta){
        timer.step(tmDelta);
    }
    bool done() const { return timer.isTmOut(); }
};