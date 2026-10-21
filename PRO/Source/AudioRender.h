#pragma once
#include <JuceHeader.h>
struct AudioRenderOptions {
    bool song=false,stems=false,effects=true;
    int scene=0,track=-1,bits=24;
    double sampleRate=48000,tailSeconds=10;
};
