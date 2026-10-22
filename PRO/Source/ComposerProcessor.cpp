#include "PluginProcessor.h"
using namespace jerzy;
using namespace jerzy::composer;

juce::AudioProcessor::BusesProperties BassAmbientProcessor::buses(){
    BusesProperties result;result.addBus(false,"MAIN",juce::AudioChannelSet::stereo(),true);
    for(int t=0;t<7;++t)result.addBus(false,trackNames[t],juce::AudioChannelSet::stereo(),false);
    result.addBus(false,"DELAY RETURN",juce::AudioChannelSet::stereo(),false);result.addBus(false,"REVERB RETURN",juce::AudioChannelSet::stereo(),false);
    result.addBus(false,"KRELL",juce::AudioChannelSet::stereo(),false);
    return result;
}
Document BassAmbientProcessor::composerDocument() const {const juce::ScopedLock lock(composerLock);auto result=composerData;const auto controls=readParameters();for(auto& scene:result.scenes){scene.patch.harmonyLink=controls.harmonyLink;scene.patch.harmonyRoot=controls.harmonyRoot;if(controls.harmonyLink)scene.patch.scale=controls.scale;}return result;}
void BassAmbientProcessor::editComposer(const std::function<void(Document&)>& edit){
    const juce::ScopedLock lock(composerLock);auto next=composerData;const auto controls=readParameters();for(auto& scene:next.scenes){scene.patch.harmonyLink=controls.harmonyLink;scene.patch.harmonyRoot=controls.harmonyRoot;if(controls.harmonyLink)scene.patch.scale=controls.scale;}edit(next);validate(next);composerData=std::move(next);composerFailure.clear();++composerRevision;
}
juce::String BassAmbientProcessor::composerError() const {const juce::ScopedLock lock(composerLock);return composerFailure;}
void BassAmbientProcessor::serviceComposer(){
    // Control edits are copied to the selected scene off the audio thread.
    // Only changed fields are copied: selecting a scene does not overwrite its patch.
    const auto controls=readParameters();if(controlSnapshotReset.exchange(false))lastControlSnapshot=controls;
    {const juce::ScopedLock lock(composerLock);bool changed=false;
        const int selected=juce::jlimit(0,int(composerData.scenes.size())-1,controls.composerScene-1);auto& patch=composerData.scenes[size_t(selected)].patch;
#define BOOL(f,l,g) if(controls.composerEnabled&&((g)<=4||(g)==6||(g)==7||(g)==23||(controls.sceneMixer&&(g)>=14&&((g)<=21||(g)==24)))&&juce::String(#f)!="bass"&&juce::String(#f)!="pad"&&juce::String(#f)!="arp"&&juce::String(#f)!="drums"&&juce::String(#f)!="fxEnabled"&&juce::String(#f)!="krell"&&controls.f!=lastControlSnapshot.f){patch.f=controls.f;changed=true;}
#define FLOAT(f,a,b,s,l,g) if(controls.composerEnabled&&((g)<=4||(g)==6||(g)==7||(g)==23||(controls.sceneMixer&&(g)>=14&&((g)<=21||(g)==24)))&&controls.f!=lastControlSnapshot.f){patch.f=controls.f;changed=true;}
#define INT(f,a,b,l,g) if(controls.composerEnabled&&((g)<=4||(g)==6||(g)==7||(g)==23||(controls.sceneMixer&&(g)>=14&&((g)<=21||(g)==24)))&&controls.f!=lastControlSnapshot.f){patch.f=controls.f;changed=true;}
#define CHOICE(f,l,g,c) if(controls.composerEnabled&&((g)<=4||(g)==6||(g)==7||(g)==23||(controls.sceneMixer&&(g)>=14&&((g)<=21||(g)==24)))&&controls.f!=lastControlSnapshot.f){patch.f=controls.f;changed=true;}
#define CHOICE_DEN(f,l,g) if(controls.composerEnabled&&controls.f!=lastControlSnapshot.f){patch.f=controls.f;changed=true;}
#include "Parameters.def"
#undef BOOL
#undef FLOAT
#undef INT
#undef CHOICE
#undef CHOICE_DEN
        if(controls.composerEnabled){
            for(int rack=0;rack<5;++rack){const auto& source=rack==0?controls.padRack:rack==1?controls.masterRack:rack==2?controls.arpRack:rack==3?controls.drumsRack:controls.krellRack;const auto& previous=rack==0?lastControlSnapshot.padRack:rack==1?lastControlSnapshot.masterRack:rack==2?lastControlSnapshot.arpRack:rack==3?lastControlSnapshot.drumsRack:lastControlSnapshot.krellRack;auto& target=rack==0?patch.padRack:rack==1?patch.masterRack:rack==2?patch.arpRack:rack==3?patch.drumsRack:patch.krellRack;
#define BOOL(f,l,g) if(source.f!=previous.f){target.f=source.f;changed=true;}
#define FLOAT(f,a,b,s,l,g) if(source.f!=previous.f){target.f=source.f;changed=true;}
#define CHOICE(f,l,g,c) if(source.f!=previous.f){target.f=source.f;changed=true;}
#include "FXParameters.def"
#undef BOOL
#undef FLOAT
#undef CHOICE
                if(source.order!=previous.order){target.order=source.order;changed=true;}
            }
            if(controls.order!=lastControlSnapshot.order){patch.order=controls.order;changed=true;}
            if(controls.masterFX!=lastControlSnapshot.masterFX){patch.masterFX=controls.masterFX;changed=true;}
            for(int lane=0;lane<4;++lane){const auto& source=controls.drumSounds[size_t(lane)];const auto& previous=lastControlSnapshot.drumSounds[size_t(lane)];auto& target=patch.drumSounds[size_t(lane)];
                if(source.level!=previous.level||source.drive!=previous.drive||source.tone!=previous.tone||source.cutoff!=previous.cutoff||source.decay!=previous.decay||source.pan!=previous.pan){const bool enabled=target.enabled;target=source;target.enabled=enabled;changed=true;}
                if(controls.drumGrid[size_t(lane)]!=lastControlSnapshot.drumGrid[size_t(lane)]){patch.drumGrid[size_t(lane)]=controls.drumGrid[size_t(lane)];changed=true;}
            }
        }
        if(controls.internalTempo!=composerData.tempo){composerData.tempo=controls.internalTempo;for(auto& scene:composerData.scenes)scene.patch.internalTempo=composerData.tempo;changed=true;}
        if(controls.harmonyLink!=lastControlSnapshot.harmonyLink||controls.harmonyRoot!=lastControlSnapshot.harmonyRoot||(controls.harmonyLink&&controls.scale!=lastControlSnapshot.scale))changed=true;
        lastControlSnapshot=controls;if(changed)++composerRevision;
    }
    delete retiredComposer.exchange(nullptr,std::memory_order_acquire);
    const auto revision=composerRevision.load(std::memory_order_acquire);
    if(revision==compiledRevision||pendingComposer.load(std::memory_order_acquire))return;
    try {auto data=composerDocument();auto result=std::make_unique<Compiled>(compile(std::move(data)));pendingComposer.store(result.release(),std::memory_order_release);compiledRevision=revision;}
    catch(const std::exception& e){const juce::ScopedLock lock(composerLock);composerFailure=e.what();compiledRevision=revision;}
}
void BassAmbientProcessor::adoptComposer(bool resetVoices){
    if(retiredComposer.load(std::memory_order_acquire))return;
    if(auto* ready=pendingComposer.exchange(nullptr,std::memory_order_acquire)){
        if(activeComposer)retiredComposer.store(activeComposer,std::memory_order_release);
        activeComposer=ready;playback=wasSongMode?&ready->song:&ready->scenes[size_t(juce::jlimit(0,int(ready->scenes.size())-1,playingScene))];sectionIndex=-1;previousComposerBeat=-1;eventCursor=0;
        // A background harmony/scene edit must not release roots held by live MIDI.
        if(resetVoices)instrument.allOff();
    }
}
void BassAmbientProcessor::launchComposer(int scene,bool song){
    setParameterValue("performanceSource",3);setParameterValue("modularMidi",0);
    setParameterValue("composerScene",float(scene+1));setParameterValue("composerSongMode",song?1:0);
    for(const char* name:{"bass","pad","arp","drums","krell"})setParameterValue(name,1);
    setParameterValue("autoPlay",0);setParameterValue("midiPlayback",0);setParameterValue("composerEnabled",1);setParameterValue("composerPlaying",1);
}
Parameters BassAmbientProcessor::composerSample(const Parameters& controls,double ppq,double tempo,bool hostPlaying,int num,int den){
    if(!activeComposer){auto silent=controls;silent.bass=silent.pad=silent.arp=silent.drums=silent.krell=false;return silent;}
    const bool running=controls.composerPlaying;
    int selected=juce::jlimit(0,std::max(0,int(activeComposer->scenes.size())-1),controls.composerScene-1);
    bool change=!playback||controls.composerSongMode!=wasSongMode;
    if(!controls.composerSongMode&&selected!=playingScene){
        // Quantize a live scene launch to the next bar; initial launch is immediate.
        const double bar=double(num)*4/std::max(1,den);
        change|=!wasComposerPlaying||std::floor(ppq/bar)!=std::floor(lastComposerPPQ/bar);
    }
    if(change){playback=controls.composerSongMode?&activeComposer->song:&activeComposer->scenes[size_t(selected)];playingScene=selected;wasSongMode=controls.composerSongMode;wasComposerPlaying=false;sectionIndex=-1;previousComposerBeat=-1;eventCursor=0;}
    if(running&&!wasComposerPlaying){composerOrigin=hostPlaying&&controls.composerSongMode?0:ppq;previousComposerBeat=-1;sectionIndex=-1;eventCursor=0;instrument.allOff();tailUntil.fill(0);previousTracks.fill(false);}
    if(!running&&wasComposerPlaying){instrument.allOff();for(int t=0;t<tracks;++t)tailUntil[size_t(t)]=ppq+tempo/60*std::min(45.0,std::max({sectionPatch.padRelease,sectionPatch.arpRelease,sectionPatch.bassRelease,sectionPatch.krellRelease})+3);}
    wasComposerPlaying=running;lastComposerPPQ=ppq;
    const auto& timeline=*playback;double beat=ppq-composerOrigin;
    if(controls.composerLoop&&timeline.beats>0)beat=std::fmod(std::max(0.0,beat),timeline.beats);
    const bool within=running&&beat>=0&&beat<timeline.beats;
    const bool seek=within&&(previousComposerBeat<0||beat<previousComposerBeat-.0001||beat-previousComposerBeat>.25);
    if(seek){instrument.allOff();sectionIndex=-1;eventCursor=size_t(std::lower_bound(timeline.events.begin(),timeline.events.end(),beat,[](const Event& e,double b){return e.beat<b;})-timeline.events.begin());}
    int section=-1;
    if(within){auto it=std::upper_bound(timeline.sections.begin(),timeline.sections.end(),beat,[](double b,const Section& s){return b<s.start;});if(it!=timeline.sections.begin())section=int(it-timeline.sections.begin()-1);}
    auto powerForTrack=[&](int track){const bool power=track==0?controls.bass:track==1?controls.pad:track==2?controls.arp:track==7?controls.krell:controls.drums&&controls.drumSounds[size_t(track-3)].enabled;
        const double threshold=section>=0?timeline.sections[size_t(section)].clips[size_t(track)].threshold:0;
        return power&&controls.composerIntensity>0&&controls.composerIntensity+1e-6>=threshold;};
    if(section!=sectionIndex||(sectionIndex<0&&within)){
        const auto previous=sectionPatch;
        std::array<bool,tracks> enabled{};
        if(section>=0){const auto& part=timeline.sections[size_t(section)];sectionPatch=part.patch;
            if(part.cutTails) {instrument.reset();tailUntil.fill(0);lastResourceSection=-2;}
            for(int t=0;t<tracks;++t){enabled[size_t(t)]=part.clips[size_t(t)].enabled;
                if(previousTracks[size_t(t)]&&!enabled[size_t(t)]&&!part.cutTails){const double release=t==0?previous.bassRelease:t==1?previous.padRelease:t==2?previous.arpRelease:t==7?previous.krellRelease:2;tailUntil[size_t(t)]=ppq+tempo/60*std::min(45.0,release+(previous.fxEnabled?8:0));}}
            clockBarBeats=double(part.numerator)*4/std::max(1,part.denominator);composerSceneDisplay=part.sceneId;composerSlotDisplay=part.slot;
        }else for(int t=0;t<tracks;++t)if(previousTracks[size_t(t)])tailUntil[size_t(t)]=ppq+tempo/60*std::min(45.0,std::max({previous.padRelease,previous.arpRelease,previous.bassRelease,previous.krellRelease})+8);
        previousTracks=enabled;sectionIndex=section;
    }
    if(seek&&section>=0){
        // Offline bar checkpoints bound seeking work to at most one bar of events.
        const auto& checkpoint=timeline.sections[size_t(section)];
        for(const auto& n:checkpoint.chase)if(n.note.beat+n.note.length>beat&&powerForTrack(n.track))instrument.externalNote(n.track,n.note.pitch,n.note.velocity/127.0*controls.composerIntensity,n.note.pan,n.id,true,n.note.accent,n.note.slide,&sectionPatch);
        eventCursor=checkpoint.firstEvent;
        while(eventCursor<timeline.events.size()&&timeline.events[eventCursor].beat<beat){const auto& e=timeline.events[eventCursor++];if(e.on&&e.endBeat>beat&&powerForTrack(e.track))instrument.externalNote(e.track,e.pitch,e.velocity/127.0*controls.composerIntensity,e.pan,e.id,true,e.accent,e.slide,&sectionPatch);}
    }
    if(running)while(eventCursor<timeline.events.size()&&timeline.events[eventCursor].beat<=beat+1e-9){const auto& e=timeline.events[eventCursor++];const bool power=powerForTrack(e.track);if(e.on&&(!within||!power))continue;if(e.on)inputPulse[size_t(e.track)]=int(sr*.15);instrument.externalNote(e.track,e.pitch,e.velocity/127.0*controls.composerIntensity,e.pan,e.id,e.on,e.accent,e.slide,&sectionPatch);}
    if(running&&!controls.composerLoop&&timeline.beats>0&&beat>=timeline.beats){if(auto* p=state.getParameter("composerPlaying"))p->setValueNotifyingHost(0);}
    previousComposerBeat=within?beat:-1;composerBeatDisplay=std::max(0.0,beat);
    auto patch=sectionPatch;patch.autoPlay=false;patch.direct=true;patch.composerEnabled=true;patch.composerPlaying=running;
    std::array<bool,tracks> enabled{};for(int t=0;t<tracks;++t)enabled[size_t(t)]=(within&&previousTracks[size_t(t)]&&powerForTrack(t))||ppq<tailUntil[size_t(t)];
    patch.bass=enabled[0]&&controls.bass;patch.pad=enabled[1]&&controls.pad;patch.arp=enabled[2]&&controls.arp;patch.krell=enabled[7]&&controls.krell;patch.drums=controls.drums&&(enabled[3]||enabled[4]||enabled[5]||enabled[6]);
    if(section>=0){const auto& c=timeline.sections[size_t(section)].clips;patch.bassLevel*=c[0].gain;patch.padLevel*=c[1].gain;patch.arpLevel*=c[2].gain;for(int lane=0;lane<4;++lane)patch.drumSounds[size_t(lane)].level*=c[size_t(lane+3)].gain;}
    for(int lane=0;lane<4;++lane)patch.drumSounds[size_t(lane)].enabled=enabled[size_t(lane+3)]&&controls.drumSounds[size_t(lane)].enabled;
    if(!controls.sceneMixer){
        patch.masterFX=controls.masterFX;patch.masterRack=controls.masterRack;
        patch.arpFX=controls.arpFX;patch.drumsFX=controls.drumsFX;patch.arpRack=controls.arpRack;patch.drumsRack=controls.drumsRack;patch.krellRack=controls.krellRack;patch.krellFX=controls.krellFX;
#define BOOL(f,l,g) patch.f=controls.f;
#define FLOAT(f,a,b,s,l,g) patch.f=controls.f;
#define CHOICE(f,l,g,c) patch.f=controls.f;
#include "MixerParameters.def"
#undef BOOL
#undef FLOAT
#undef CHOICE
    }
    patch.fxEnabled=controls.fxEnabled;patch.master=controls.master;patch.limiter=controls.limiter;patch.sendDelayPower=patch.sendDelayPower&&controls.sendDelayPower;patch.sendReverbPower=patch.sendReverbPower&&controls.sendReverbPower;patch.separateOutputs=controls.separateOutputs;
    if(section!=lastResourceSection){instrument.configureResources(patch);lastResourceSection=section;}
    return patch;
}
void BassAmbientProcessor::routePartMidi(const juce::MidiMessage& m,const Parameters& patch){
    int track=m.getChannel()==1?0:m.getChannel()==2?1:m.getChannel()==3?2:m.getChannel()==4?7:-1;
    if(m.getChannel()==10){const int n=m.getNoteNumber();track=n==36?3:(n==38||n==40)?4:(n==42||n==44)?5:(n==39||n==46||n==45||n==47)?6:-1;}
    if(track==0&&m.isNoteOn()&&patch.harmonyLink)setParameterValue("harmonyRoot",float(m.getNoteNumber()));
    if(track<0||(!m.isNoteOn()&&!m.isNoteOff()))return;const bool power=track==0?patch.bass:track==1?patch.pad:track==2?patch.arp:track==7?patch.krell:patch.drums&&patch.drumSounds[size_t(track-3)].enabled;if(m.isNoteOn()&&!power)return;const auto id=0x80000000U|uint32_t(track*128+m.getNoteNumber());
    instrument.externalNote(track,m.getNoteNumber(),m.getFloatVelocity(),0,id,m.isNoteOn(),false,track==0&&patch.legato,&patch);
}
juce::Result BassAmbientProcessor::exportComposer(const juce::File& file,bool song,int scene,int track) const {
    try{auto data=composerDocument();data.tempo=readParameters().clockSource==0?readParameters().internalTempo:tempoDisplay.load();const auto result=compile(data);if(!song&&(scene<0||scene>=int(result.scenes.size())))return juce::Result::fail("Select a scene");return writeMidi(song?result.song:result.scenes[size_t(scene)],data.tempo,file,track);}
    catch(const std::exception& e){return juce::Result::fail(e.what());}
}
juce::Result BassAmbientProcessor::saveComposer(const juce::File& file) const {const auto xml=toTree(composerDocument()).createXml();return xml&&file.replaceWithText(xml->toString())?juce::Result::ok():juce::Result::fail("Cannot save Composer project");}
juce::Result BassAmbientProcessor::loadComposer(const juce::File& file){
    if(file.getSize()>16*1024*1024)return juce::Result::fail("Composer project exceeds 16 MB");
    try{const auto xml=juce::XmlDocument::parse(file);if(!xml)return juce::Result::fail("Invalid Composer file");auto document=fromTree(juce::ValueTree::fromXml(*xml));setParameterValue("internalTempo",float(document.tempo));setParameterValue("harmonyLink",document.scenes[0].patch.harmonyLink?1:0);setParameterValue("harmonyRoot",float(document.scenes[0].patch.harmonyRoot));if(document.scenes[0].patch.harmonyLink)setParameterValue("scale",float(document.scenes[0].patch.scale));editComposer([&](auto& d){d=std::move(document);});return juce::Result::ok();}catch(const std::exception& e){return juce::Result::fail(e.what());}
}
