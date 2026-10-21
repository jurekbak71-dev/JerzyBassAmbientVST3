#pragma once
#include <JuceHeader.h>
#include "Composer.h"
namespace jerzy::composer {
inline double boundedPatchValue(const juce::var& value,double initial,double lo,double hi){const auto v=double(value);return std::isfinite(v)?clamp(v,lo,hi):initial;}

inline juce::ValueTree patchTree(const Parameters& p){juce::ValueTree tree("Patch");
#define BOOL(f,l,g) tree.setProperty(#f,p.f,nullptr);
#define FLOAT(f,a,b,s,l,g) tree.setProperty(#f,p.f,nullptr);
#define INT(f,a,b,l,g) tree.setProperty(#f,p.f,nullptr);
#define CHOICE(f,l,g,c) tree.setProperty(#f,p.f,nullptr);
#define CHOICE_DEN(f,l,g) tree.setProperty(#f,p.f,nullptr);
#include "Parameters.def"
#undef BOOL
#undef FLOAT
#undef INT
#undef CHOICE
#undef CHOICE_DEN
    tree.setProperty("masterFX",p.masterFX,nullptr);
    for(int rack=0;rack<3;++rack){const auto& fx=rack==0?static_cast<const FXParameters&>(p):rack==1?p.padRack:p.masterRack;juce::ValueTree child("FX");
#define BOOL(f,l,g) child.setProperty(#f,fx.f,nullptr);
#define FLOAT(f,a,b,s,l,g) child.setProperty(#f,fx.f,nullptr);
#define CHOICE(f,l,g,c) child.setProperty(#f,fx.f,nullptr);
#include "FXParameters.def"
#undef BOOL
#undef FLOAT
#undef CHOICE
        for(int k=0;k<6;++k)child.setProperty("order"+juce::String(k),fx.order[size_t(k)],nullptr);tree.appendChild(child,nullptr);
    }
    for(int lane=0;lane<4;++lane){const auto& s=p.drumSounds[size_t(lane)];juce::ValueTree child("Drum");
        child.setProperty("enabled",s.enabled,nullptr);child.setProperty("level",s.level,nullptr);child.setProperty("drive",s.drive,nullptr);child.setProperty("tone",s.tone,nullptr);child.setProperty("cutoff",s.cutoff,nullptr);child.setProperty("decay",s.decay,nullptr);child.setProperty("pan",s.pan,nullptr);
        juce::StringArray steps;for(int v:p.drumGrid[size_t(lane)])steps.add(juce::String(v));child.setProperty("grid",steps.joinIntoString(" "),nullptr);tree.appendChild(child,nullptr);
    }return tree;
}
inline Parameters readPatch(const juce::ValueTree& tree){Parameters p;if(!tree.isValid())return p;
#define BOOL(f,l,g) p.f=bool(tree.getProperty(#f,p.f));
#define FLOAT(f,a,b,s,l,g) p.f=boundedPatchValue(tree.getProperty(#f,p.f),p.f,a,b);
#define INT(f,a,b,l,g) p.f=juce::jlimit(a,b,int(tree.getProperty(#f,p.f)));
#define CHOICE(f,l,g,c) p.f=int(tree.getProperty(#f,p.f));
#define CHOICE_DEN(f,l,g) p.f=int(tree.getProperty(#f,p.f));
#include "Parameters.def"
#undef BOOL
#undef FLOAT
#undef INT
#undef CHOICE
#undef CHOICE_DEN
    p.masterFX=boundedPatchValue(tree.getProperty("masterFX",p.masterFX),p.masterFX,0,1);int rack=0,lane=0;
    for(const auto& child:tree){if(child.hasType("FX")&&rack<3){auto& fx=rack==0?static_cast<FXParameters&>(p):rack==1?p.padRack:p.masterRack;++rack;
#define BOOL(f,l,g) fx.f=bool(child.getProperty(#f,fx.f));
#define FLOAT(f,a,b,s,l,g) fx.f=boundedPatchValue(child.getProperty(#f,fx.f),fx.f,a,b);
#define CHOICE(f,l,g,c) fx.f=int(child.getProperty(#f,fx.f));
#include "FXParameters.def"
#undef BOOL
#undef FLOAT
#undef CHOICE
        for(int k=0;k<6;++k)fx.order[size_t(k)]=int(child.getProperty("order"+juce::String(k),k));
    }else if(child.hasType("Drum")&&lane<4){auto& s=p.drumSounds[size_t(lane)];s.enabled=bool(child.getProperty("enabled",true));s.level=double(child.getProperty("level",.7));s.drive=double(child.getProperty("drive",.1));s.tone=double(child.getProperty("tone",.5));s.cutoff=double(child.getProperty("cutoff",12000));s.decay=double(child.getProperty("decay",.2));s.pan=double(child.getProperty("pan",0));
        auto steps=juce::StringArray::fromTokens(child.getProperty("grid").toString()," ","");for(int k=0;k<std::min(64,steps.size());++k)p.drumGrid[size_t(lane)][size_t(k)]=juce::jlimit(0,127,steps[k].getIntValue());++lane;}}
    return p;
}
inline juce::ValueTree toTree(const Document& d){juce::ValueTree root("Composer");root.setProperty("version",1,nullptr);root.setProperty("tempo",d.tempo,nullptr);root.setProperty("nextId",d.nextId,nullptr);
    for(const auto& s:d.scenes){juce::ValueTree scene("Scene");scene.setProperty("id",s.id,nullptr);scene.setProperty("name",juce::String(s.name),nullptr);scene.setProperty("bars",s.bars,nullptr);scene.setProperty("num",s.numerator,nullptr);scene.setProperty("den",s.denominator,nullptr);scene.setProperty("intensity",s.intensity,nullptr);scene.setProperty("cut",s.cutTails,nullptr);scene.appendChild(patchTree(s.patch),nullptr);
        for(const auto& c:s.chords){juce::ValueTree chord("Chord");chord.setProperty("root",c.root,nullptr);chord.setProperty("quality",c.quality,nullptr);scene.appendChild(chord,nullptr);}
        for(const auto& c:s.clips){juce::ValueTree clip("Clip");clip.setProperty("enabled",c.enabled,nullptr);clip.setProperty("generated",c.generated,nullptr);clip.setProperty("rootGates",c.explicitRoots,nullptr);clip.setProperty("gain",c.gain,nullptr);clip.setProperty("threshold",c.threshold,nullptr);clip.setProperty("enter",c.enterBar,nullptr);clip.setProperty("leave",c.leaveBar,nullptr);
            for(const auto& n:c.notes){juce::ValueTree note("Note");note.setProperty("beat",n.beat,nullptr);note.setProperty("length",n.length,nullptr);note.setProperty("pitch",n.pitch,nullptr);note.setProperty("velocity",n.velocity,nullptr);note.setProperty("pan",n.pan,nullptr);note.setProperty("accent",n.accent,nullptr);note.setProperty("slide",n.slide,nullptr);clip.appendChild(note,nullptr);}scene.appendChild(clip,nullptr);}
        root.appendChild(scene,nullptr);}
    for(const auto& slot:d.song){juce::ValueTree item("Slot");item.setProperty("scene",slot.sceneId,nullptr);item.setProperty("repeats",slot.repeats,nullptr);item.setProperty("evolve",slot.evolve,nullptr);root.appendChild(item,nullptr);}return root;
}
inline Document fromTree(const juce::ValueTree& root){if(!root.hasType("Composer")||int(root.getProperty("version",0))!=1)throw std::runtime_error("Unsupported Composer project");Document d;d.nextId=int(root.getProperty("nextId",1));d.tempo=double(root.getProperty("tempo",120));
    for(const auto& item:root){if(item.hasType("Scene")){if(d.scenes.size()>=maxScenes)throw std::runtime_error("Too many scenes");Scene s;s.chords.clear();s.id=int(item.getProperty("id",0));s.name=item.getProperty("name","Scene").toString().toStdString();s.bars=int(item.getProperty("bars",4));s.numerator=int(item.getProperty("num",4));s.denominator=int(item.getProperty("den",4));s.intensity=double(item.getProperty("intensity",1));s.cutTails=bool(item.getProperty("cut",false));s.patch=readPatch(item.getChildWithName("Patch"));int track=0;
        for(const auto& child:item){if(child.hasType("Chord"))s.chords.push_back({int(child.getProperty("root",36)),int(child.getProperty("quality",1))});else if(child.hasType("Clip")&&track<tracks){auto& c=s.clips[size_t(track++)];c.enabled=bool(child.getProperty("enabled",true));c.generated=bool(child.getProperty("generated",true));c.explicitRoots=bool(child.getProperty("rootGates",false));c.gain=double(child.getProperty("gain",1));c.threshold=double(child.getProperty("threshold",0));c.enterBar=int(child.getProperty("enter",0));c.leaveBar=int(child.getProperty("leave",0));
            for(const auto& x:child)if(x.hasType("Note")){if(c.notes.size()>=maxNotes)throw std::runtime_error("Too many clip notes");c.notes.push_back({double(x.getProperty("beat",0)),double(x.getProperty("length",.25)),int(x.getProperty("pitch",60)),int(x.getProperty("velocity",100)),double(x.getProperty("pan",0)),bool(x.getProperty("accent",false)),bool(x.getProperty("slide",false))});}}}
        d.scenes.push_back(std::move(s));
    }else if(item.hasType("Slot")){if(d.song.size()>=maxSlots)throw std::runtime_error("Too many song slots");d.song.push_back({int(item.getProperty("scene",1)),int(item.getProperty("repeats",1)),bool(item.getProperty("evolve",false))});}}
    validate(d);return d;
}
inline juce::Result writeMidi(const Timeline& timeline,double bpm,const juce::File& file,int onlyTrack=-1){
    juce::MidiFile midi;midi.setTicksPerQuarterNote(960);juce::MidiMessageSequence metadata;
    auto tempo=juce::MidiMessage::tempoMetaEvent(int(60000000/clamp(bpm,20.0,400.0)));metadata.addEvent(tempo,0);
    int previousScene=-1,previousSlot=-2;for(const auto& s:timeline.sections){if(s.sceneId!=previousScene||s.slot!=previousSlot){auto marker=juce::MidiMessage::textMetaEvent(6,juce::String(s.name));metadata.addEvent(marker,s.start*960);auto meter=juce::MidiMessage::timeSignatureMetaEvent(s.numerator,s.denominator);metadata.addEvent(meter,s.start*960);previousScene=s.sceneId;previousSlot=s.slot;}}
    midi.addTrack(metadata);
    for(int track=0;track<tracks;++track){if(onlyTrack>=0&&track!=onlyTrack)continue;juce::MidiMessageSequence sequence;auto name=juce::MidiMessage::textMetaEvent(3,trackNames[track]);sequence.addEvent(name,0);
        for(const auto& event:timeline.events)if(event.track==track){const auto message=event.on?juce::MidiMessage::noteOn(midiChannels[track],event.pitch,juce::uint8(event.velocity)):juce::MidiMessage::noteOff(midiChannels[track],event.pitch);sequence.addEvent(message,event.beat*960);}
        auto end=juce::MidiMessage::endOfTrack();sequence.addEvent(end,timeline.beats*960);sequence.updateMatchedPairs();midi.addTrack(sequence);}
    juce::FileOutputStream out(file);if(out.failedToOpen())return out.getStatus();if(!out.setPosition(0))return juce::Result::fail("Cannot rewind MIDI file");auto result=out.truncate();if(result.failed())return result;
    if(!midi.writeTo(out,1))return juce::Result::fail("MIDI write failed");out.flush();return out.getStatus();
}
} // namespace jerzy::composer
