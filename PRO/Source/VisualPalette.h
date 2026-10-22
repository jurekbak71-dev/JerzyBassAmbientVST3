#pragma once
#include <JuceHeader.h>

// Shared RGB identity for instrument pages, Composer clips and drum lanes.
namespace ProVisual {
inline const std::array<juce::Colour,8> tracks{
    juce::Colour::fromRGB(255,173,112), // Bass: orange
    juce::Colour::fromRGB(180,161,255), // Ambient: violet
    juce::Colour::fromRGB(99,219,208),  // Arp: turquoise
    juce::Colour::fromRGB(255,146,107), // Kick: coral
    juce::Colour::fromRGB(255,112,155), // Snare: pink
    juce::Colour::fromRGB(114,207,255), // Hi-hat: blue
    juce::Colour::fromRGB(218,178,108), // Aux: gold
    juce::Colour::fromRGB(170,235,118)  // Krell: lime
};
inline const std::array<juce::Colour,8> pages{
    tracks[0],tracks[1],tracks[2],juce::Colour::fromRGB(255,210,100),
    juce::Colour::fromRGB(130,185,255),juce::Colour::fromRGB(161,222,128),
    juce::Colour::fromRGB(230,210,170),tracks[7]
};
inline void tint(juce::Component& c,juce::Colour colour) {
    if(auto* label=dynamic_cast<juce::Label*>(&c))label->setColour(juce::Label::textColourId,colour);
    if(auto* button=dynamic_cast<juce::Button*>(&c)) {
        button->setColour(juce::TextButton::textColourOnId,colour);
        button->setColour(juce::TextButton::textColourOffId,colour);
        button->setColour(juce::ToggleButton::textColourId,colour);
        button->setColour(juce::ToggleButton::tickColourId,colour);
    }
    for(auto* child:c.getChildren())tint(*child,colour);
}
}

// JUCE char* strings are ASCII; UI literals explicitly decode UTF-8 on every platform.
inline juce::String operator""_ui(const char* value,size_t size){return juce::String::fromUTF8(value,int(size));}
