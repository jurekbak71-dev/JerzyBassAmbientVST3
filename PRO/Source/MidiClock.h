#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>

// MIDI realtime clock uses audio sample timestamps, independent of GUI timing.
struct ProMidiClock {
    double sampleRate=48000,bpm=120,beat=0;
    int64_t lastTick=-1;bool running=false,locked=false;int ticks=0;
    void reset(double rate){sampleRate=rate;bpm=120;beat=0;lastTick=-1;running=locked=false;ticks=0;}
    void start(){beat=0;running=true;ticks=0;lastTick=-1;locked=false;}
    void resume(){running=true;}
    void stop(){running=false;}
    void songPosition(int sixteenths){beat=std::max(0,sixteenths)*.25;ticks=int(std::lround(beat*24));}
    void tick(int64_t sample){
        if(lastTick>=0&&sample>lastTick){const double measured=60.0*sampleRate/(24.0*double(sample-lastTick));
            if(measured>=20&&measured<=400){bpm=locked?bpm*.75+measured*.25:measured;locked=true;}}
        lastTick=sample;if(running)beat=double(ticks++)/24;
    }
    bool valid(int64_t sample) const {return locked&&lastTick>=0&&sample-lastTick<int64_t(sampleRate*std::max(.75,5.0/bpm));}
};
