#pragma once
#include "Core.h"
#include <string>
#include <stdexcept>

namespace jerzy::composer {
constexpr int tracks=8,maxScenes=32,maxSlots=128,maxNotes=512;
inline constexpr const char* trackNames[]{"BASS","PAD","ARP","KICK","SNARE","HI-HAT","AUX","KRELL"};
inline constexpr int midiChannels[]{1,2,3,10,10,10,10,4};
inline constexpr int drumNotes[]{36,38,42,39};
struct Note {double beat=0,length=.25;int pitch=60,velocity=100;double pan=0;bool accent=false,slide=false;};
struct Clip {
    bool enabled=true,generated=true,explicitRoots=false;double gain=1,threshold=0;int enterBar=0,leaveBar=0;
    std::vector<Note> notes;
};
struct Chord {int root=36,quality=1;};
struct Scene {
    int id=1;std::string name="Verse";int bars=4,numerator=4,denominator=4;
    double intensity=1;bool cutTails=false;
    Parameters patch;
    std::vector<Chord> chords{{36,1}};
    std::array<Clip,tracks> clips{};
    double barBeats() const {return double(numerator)*4/denominator;}
    double beats() const {return bars*barBeats();}
};
struct Slot {int sceneId=1,repeats=1;bool evolve=false;};
struct Document {int nextId=9;double tempo=120;std::vector<Scene> scenes;std::vector<Slot> song;};
struct ScheduledNote {Note note;int track=0;uint32_t id=0;};
struct Event {double beat=0;int track=0,pitch=60,velocity=0;double pan=0;bool on=false,accent=false,slide=false;uint32_t id=0;double endBeat=0;};
struct Section {double start=0,end=0;int sceneId=0,slot=-1;std::string name;Parameters patch;std::array<Clip,tracks> clips;bool cutTails=false;int numerator=4,denominator=4;std::vector<ScheduledNote> chase;size_t firstEvent=0;};
struct Timeline {double beats=0;std::vector<ScheduledNote> notes;std::vector<Event> events;std::vector<Section> sections;};
struct Compiled {std::vector<Timeline> scenes;std::vector<int> sceneIds;Timeline song;};
inline void validate(Document& d) {
    if(d.scenes.empty()||d.scenes.size()>maxScenes||d.song.size()>maxSlots)throw std::runtime_error("Composer: 1-32 scenes / 0-128 song slots required");
    d.tempo=std::isfinite(d.tempo)?clamp(d.tempo,30.0,240.0):120;
    std::vector<int> ids;
    for(auto& s:d.scenes) {
        if(s.id<1||std::find(ids.begin(),ids.end(),s.id)!=ids.end())throw std::runtime_error("Duplicate scene ID");
        ids.push_back(s.id);d.nextId=std::max(d.nextId,s.id+1);
        s.bars=clamp(s.bars,1,32);s.numerator=clamp(s.numerator,1,12);
        if(s.denominator!=2&&s.denominator!=4&&s.denominator!=8&&s.denominator!=16)s.denominator=4;
        s.intensity=clamp(s.intensity,0.0,1.0);
        if(s.chords.empty())s.chords.push_back({36,1});
        if(s.chords.size()>32)throw std::runtime_error("Maximum 32 chords per scene");
        for(auto& c:s.chords){c.root=clamp(c.root,12,96);c.quality=clamp(c.quality,0,7);}
        for(auto& c:s.clips){c.gain=clamp(c.gain,0.0,1.0);c.threshold=clamp(c.threshold,0.0,1.0);c.enterBar=clamp(c.enterBar,0,s.bars-1);c.leaveBar=clamp(c.leaveBar,0,s.bars);
            if(c.notes.size()>maxNotes)throw std::runtime_error("Maximum 512 manual notes per clip");
            for(auto& n:c.notes){if(!std::isfinite(n.beat)||!std::isfinite(n.length)||!std::isfinite(n.pan))throw std::runtime_error("Invalid note time");n.beat=clamp(n.beat,0.0,s.beats()-.001);n.length=clamp(n.length,.01,s.beats()-n.beat);n.pitch=clamp(n.pitch,0,127);n.velocity=clamp(n.velocity,1,127);n.pan=clamp(n.pan,-1.0,1.0);}
        }
    }
    for(auto& slot:d.song){if(std::find(ids.begin(),ids.end(),slot.sceneId)==ids.end())throw std::runtime_error("Song references missing scene");slot.repeats=clamp(slot.repeats,1,16);}
}
inline Document factoryDocument() {
    Document d;const char* names[]{"Intro","Verse","Chorus","Break","Fill","Outro","End","Custom"};
    const unsigned masks[]{6,107,127,6,120,7,2,127};
    for(int k=0;k<8;++k){Scene s;s.id=k+1;s.name=names[k];s.bars=k==4?1:k==6?2:4;
        s.patch.style=k==2?5:0;s.patch.bass=s.patch.pad=s.patch.arp=s.patch.drums=true;s.patch.padLevel=.32;s.patch.arpLevel=.22;
        s.patch.arpPulses=k==2?12:5;s.patch.padAttack=.6;s.patch.padRelease=2;s.patch.fxEnabled=true;
        s.chords={{36,1},{44,0},{41,1},{43,0}};
        for(int t=0;t<tracks;++t)s.clips[size_t(t)].enabled=(masks[k]&(1U<<t))!=0;
        if(k==0){s.clips[2].enterBar=2;}if(k==5){s.clips[0].leaveBar=2;s.clips[2].leaveBar=3;}
        if(k==4) {s.patch.density=1;for(int i=8;i<16;++i)s.patch.drumGrid[1][size_t(i)]=90+(i%4)*10;}
        d.scenes.push_back(s);
    }
    d.song={{1,1,false},{2,2,false},{3,2,false},{4,1,false},{5,1,true},{3,2,true},{6,1,false},{7,1,false}};
    return d;
}
inline bool activeAt(const Scene& s,int track,double beat) {const auto& c=s.clips[size_t(track)];const int bar=int(beat/s.barBeats());return s.intensity>0&&s.intensity>=c.threshold&&c.enabled&&c.gain>0&&bar>=c.enterBar&&(!c.leaveBar||bar<c.leaveBar);}
inline Chord chordAt(const Scene& s,double beat){return s.chords[size_t(int(beat/s.barBeats()))%s.chords.size()];}
inline std::vector<Note> generateLegacyClip(const Scene& s,int track,int variant=0) {
    const auto& clip=s.clips[size_t(track)];if(!clip.generated)return clip.notes;
    auto p=s.patch;p.seed=int((uint32_t(p.seed)^hash(uint32_t(variant)))%65535)+1;
    std::vector<Note> result;
    if(track==7){
        for(int voice=0;voice<4;++voice){double beat=voice*.125;uint32_t cycle=0;while(beat<s.beats()){
            const uint32_t id=uint32_t(voice)*1009+cycle++;const auto c=chordAt(s,beat);const double seconds=std::min(p.krellCycleMin,p.krellCycleMax)+std::abs(p.krellCycleMax-p.krellCycleMin)*unit(uint32_t(p.seed),id*19+4);
            const double length=std::max(.125,seconds*p.internalTempo/60);const int degree=int(unit(uint32_t(p.seed),id*19+1)*(p.krellPitchRange+1));
            result.push_back({beat,std::min(length,s.beats()-beat),clamp(c.root+12+scaleOffset(degree,p.scale),24,100),int(60+45*unit(uint32_t(p.seed),id*19+2)),unit(uint32_t(p.seed),id*19+3)*2-1});beat+=length;}}
    }else if(track==1) {
        static constexpr int chords[8][4]={{0,4,7,12},{0,3,7,12},{0,7,12,19},{0,5,7,12},{0,3,7,10},{0,4,7,11},{0,3,10,14},{0,4,11,14}};
        if(p.padMode==2){
            // Offline Krell decisions become ordinary timed notes, shared by play/export.
            for(int voice=0;voice<4;++voice){double beat=voice*.19;uint32_t cycle=0;
                while(beat<s.beats()){const auto id=uint32_t(voice)*1009+cycle++;const auto c=chordAt(s,beat);const double seconds=std::min(p.krellMin,p.krellMax)+std::abs(p.krellMax-p.krellMin)*unit(uint32_t(p.seed),id);
                    const double duration=std::max(.125,seconds*s.patch.internalTempo/60);const int degree=int(unit(uint32_t(p.seed),id+17)*(p.krellRange+1));
                    result.push_back({beat,std::min(duration,s.beats()-beat),clamp(c.root+12+scaleOffset(degree,p.scale),24,108),int(55+55*unit(uint32_t(p.seed),id+31)),unit(uint32_t(p.seed),id+53)*2-1});beat+=duration*(1+p.krellSpace);}
            }
        }else for(int bar=0;bar<s.bars;++bar){const double beat=bar*s.barBeats();const auto c=chordAt(s,beat);for(int voice=0;voice<4;++voice)result.push_back({beat,s.barBeats()*.96,clamp(c.root+12+chords[c.quality][voice],24,108),90,-.8+voice*.53});}
    }else {
        const int division=track==0?p.division:track==2?p.arpDivision:p.drumDivision;
        const double period=divisionBeats(division),swing=track==0?p.swing:track==2?p.arpSwing:p.drumSwing;
        const int count=int(std::ceil(s.beats()/period));
        for(int k=0;k<count;++k){const double beat=k*period+(k%2?period*swing*.5:0);if(beat>=s.beats())break;const auto c=chordAt(s,beat);Note n;n.beat=beat;
            if(track==0){const auto step=makeStep(p,k,std::max(1,int(std::lround(s.barBeats()*p.bars/period))));if(step.rest)continue;n.pitch=clamp(c.root+scaleOffset(step.degree,p.scale)+step.semitones+12*step.octave,12,108);n.velocity=int(step.velocity*115);n.length=period*step.length;n.slide=step.slide;n.accent=step.accent;}
            else if(track==2){auto arp=p;arp.chord=c.quality;const auto e=makeArpEvent(arp,k,nullptr,0,c.root);if(e.note<0)continue;n.pitch=e.note;n.velocity=int(e.velocity*110);n.length=period*e.gate;n.pan=e.pan;}
            else {const int lane=track-3,index=k%clamp(p.drumLength,1,64);const auto velocity=p.drumGrid[size_t(lane)][size_t(index)];if(velocity<=0)continue;n.pitch=lane==3?(p.drumAux==1?46:p.drumAux==2?45:39):drumNotes[lane];n.velocity=int(velocity*(1-p.drumHumanize*.35*unit(uint32_t(p.seed),uint32_t(k)*7+uint32_t(lane))));n.length=std::min(.12,period*.8);n.pan=p.drumSounds[size_t(lane)].pan;}
            n.length=std::min(n.length,s.beats()-beat);n.velocity=clamp(n.velocity,1,127);result.push_back(n);
        }
    }
    return result;
}
// MODULE clips store root gates; empty root clips follow the scene progression.
inline std::vector<Note> ownRootClip(const Scene& s,int track) {
    const auto& clip=s.clips[size_t(track)];
    if(clip.explicitRoots||!clip.notes.empty()){
        auto roots=clip.notes;if(s.patch.harmonyLink){const int anchor=s.clips[0].notes.empty()?s.chords[0].root:s.clips[0].notes[0].pitch;for(auto& n:roots)n.pitch=clamp(n.pitch+s.patch.harmonyRoot-anchor,0,127);}return roots;
    }
    std::vector<Note> roots;
    for(int bar=0;bar<s.bars;++bar)roots.push_back({bar*s.barBeats(),s.barBeats(),clamp(chordAt(s,bar*s.barBeats()).root+(s.patch.harmonyLink?s.patch.harmonyRoot-s.chords[0].root:0),0,127),100,0});
    return roots;
}
inline std::vector<Note> rootClip(const Scene& s,int track){return ownRootClip(s,s.patch.harmonyLink&&track!=0?0:track);}
inline std::vector<Note> generateClip(const Scene& s,int track,int variant=0) {
    const auto& clip=s.clips[size_t(track)];
    if(!clip.generated||(track>=3&&track<=6))return generateLegacyClip(s,track,variant);
    const auto roots=rootClip(s,track);std::vector<Note> result;
    // A root gate owns its whole phrase. The latest overlapping root wins.
    auto ordered=roots;std::stable_sort(ordered.begin(),ordered.end(),[](const Note& a,const Note& b){return a.beat<b.beat;});
    for(size_t i=0;i<ordered.size();++i){const auto& root=ordered[i];
        const double end=std::min({s.beats(),root.beat+root.length,i+1<ordered.size()?ordered[i+1].beat:s.beats()});
        if(end<=root.beat)continue;
        Scene phrase=s;phrase.clips[size_t(track)].notes.clear();
        const int quality=chordAt(s,root.beat).quality;phrase.chords={{root.pitch,quality}};
        std::vector<Note> notes;
        if(track==1&&phrase.patch.padMode!=2){
            static constexpr int intervals[8][4]={{0,4,7,12},{0,3,7,12},{0,7,12,19},{0,5,7,12},{0,3,7,10},{0,4,7,11},{0,3,10,14},{0,4,11,14}};
            const double duration=end-root.beat;
            const double cycle=phrase.patch.padMode==0?duration:std::max(.125,phrase.patch.evolution*phrase.patch.internalTempo/60);
            for(double at=0;at<duration;at+=cycle)for(int voice=0;voice<4;++voice)notes.push_back({at,std::min(cycle,duration-at),clamp(root.pitch+12+intervals[quality][voice],12,108),90,(-.8+voice*.53)*phrase.patch.spread});
        }else notes=generateLegacyClip(phrase,track,variant);
        std::stable_sort(notes.begin(),notes.end(),[](const Note& a,const Note& b){return a.beat<b.beat;});
        // Generate on a local clock so each root starts its module phrase immediately.
        for(auto n:notes){if(n.beat>=end-root.beat)break;n.beat+=root.beat;n.length=std::min(n.length,end-n.beat);n.velocity=clamp(int(n.velocity*root.velocity/100.0),1,127);n.pan=clamp(n.pan+root.pan,-1.0,1.0);result.push_back(n);}
    }
    return result;
}
inline void finish(Timeline& timeline) {
    std::stable_sort(timeline.notes.begin(),timeline.notes.end(),[](const auto& a,const auto& b){return a.note.beat<b.note.beat;});
    uint32_t id=1;for(auto& n:timeline.notes){n.id=id++;const auto& x=n.note;
        timeline.events.push_back({x.beat,n.track,x.pitch,x.velocity,x.pan,true,x.accent,x.slide,n.id,x.beat+x.length});
        timeline.events.push_back({x.beat+x.length,n.track,x.pitch,0,x.pan,false,false,false,n.id,x.beat+x.length});
    }
    std::sort(timeline.events.begin(),timeline.events.end(),[](const Event& a,const Event& b){if(a.beat!=b.beat)return a.beat<b.beat;if(a.on!=b.on)return !a.on;return a.id<b.id;});
    size_t nextNote=0;std::vector<ScheduledNote> active;
    for(auto& section:timeline.sections){
        active.erase(std::remove_if(active.begin(),active.end(),[&](const auto& n){return n.note.beat+n.note.length<=section.start;}),active.end());
        while(nextNote<timeline.notes.size()&&timeline.notes[nextNote].note.beat<section.start){const auto& n=timeline.notes[nextNote++];if(n.note.beat+n.note.length>section.start){active.push_back(n);if(active.size()>32)active.erase(active.begin());}}
        section.chase=active;section.firstEvent=size_t(std::lower_bound(timeline.events.begin(),timeline.events.end(),section.start,[](const Event& e,double beat){return e.beat<beat;})-timeline.events.begin());
    }
}
inline void appendScene(Timeline& target,const Scene& scene,int variant,int slot) {
    const double start=target.beats;
    // Emit a section at each enter/leave boundary so resources follow arrangement masks.
    for(int bar=0;bar<scene.bars;++bar){Section section;section.start=start+bar*scene.barBeats();section.end=section.start+scene.barBeats();section.sceneId=scene.id;section.slot=slot;section.name=scene.name;section.patch=scene.patch;section.cutTails=scene.cutTails;section.numerator=scene.numerator;section.denominator=scene.denominator;
        for(int t=0;t<tracks;++t){section.clips[size_t(t)]=scene.clips[size_t(t)];section.clips[size_t(t)].notes.clear();section.clips[size_t(t)].enabled=activeAt(scene,t,bar*scene.barBeats());}
        target.sections.push_back(std::move(section));}
    for(int t=0;t<tracks;++t)for(auto n:generateClip(scene,t,variant)){
        const auto& clip=scene.clips[size_t(t)];const double end=clip.leaveBar?std::min(scene.beats(),clip.leaveBar*scene.barBeats()):scene.beats();
        const double begin=std::max(n.beat,clip.enterBar*scene.barBeats());const double noteEnd=std::min(n.beat+n.length,end);
        if(noteEnd<=begin||!activeAt(scene,t,begin))continue;n.beat=begin;n.length=noteEnd-begin;n.velocity=clamp(int(n.velocity*scene.intensity),1,127);
        if(scene.intensity<=0)continue;n.beat+=start;target.notes.push_back({n,t,0});
        if(target.notes.size()>500000)throw std::runtime_error("Song too dense: maximum 500000 notes");
    }
    target.beats+=scene.beats();
}
inline Compiled compile(Document document) {
    validate(document);Compiled result;
    for(const auto& scene:document.scenes){Timeline timeline;appendScene(timeline,scene,0,-1);finish(timeline);result.scenes.push_back(std::move(timeline));result.sceneIds.push_back(scene.id);}
    int occurrence=0;
    for(size_t i=0;i<document.song.size();++i){const auto& slot=document.song[i];const auto it=std::find_if(document.scenes.begin(),document.scenes.end(),[&](const Scene& s){return s.id==slot.sceneId;});
        for(int r=0;r<slot.repeats;++r)appendScene(result.song,*it,slot.evolve?++occurrence:0,int(i));}
    finish(result.song);return result;
}
inline void freeze(Scene& scene,int track){scene.clips[size_t(track)].notes=generateClip(scene,track);scene.clips[size_t(track)].generated=false;}
} // namespace jerzy::composer
