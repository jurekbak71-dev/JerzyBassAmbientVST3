#include "Composer.h"
#include "MidiClock.h"
#include <iostream>
#include <stdexcept>
using namespace jerzy;using namespace jerzy::composer;
int checks=0;void expect(bool ok,const char* name){++checks;if(!ok)throw std::runtime_error(name);}
int main(){try{
    auto document=factoryDocument();validate(document);auto song=compile(document);
    expect(song.scenes.size()==8&&song.song.beats>100,"factory scenes and song compiled");
    double previous=-1;for(const auto& e:song.song.events){expect(e.beat>=previous,"events sorted");previous=e.beat;expect(e.pitch>=0&&e.pitch<128,"valid MIDI pitch");}
    expect(song.song.events.size()==song.song.notes.size()*2,"matched on/off for every note");
    for(const auto& n:song.scenes[0].notes)expect(n.track==1||n.track==2,"intro only pad and arp");
    for(const auto& n:song.scenes[0].notes)if(n.track==2)expect(n.note.beat>=8,"instrument enters on bar three");
    auto scene=document.scenes[2];auto before=generateClip(scene,2);freeze(scene,2);expect(!scene.clips[2].generated&&scene.clips[2].notes.size()==before.size(),"freeze creates manual notes");
    auto frozen=generateClip(scene,2);for(size_t i=0;i<before.size();++i)expect(before[i].pitch==frozen[i].pitch&&before[i].beat==frozen[i].beat&&before[i].length==frozen[i].length,"freeze preserves realized notes");
    document.scenes[0].clips[1].generated=false;document.scenes[0].clips[1].notes={{0,1,60,100,0}};document.scenes[0].chords={{48,0}};
    auto manual=compile(document);expect(manual.scenes[0].notes.front().note.pitch==60,"manual notes remain exact despite progression");
    document.song={{document.scenes[1].id,2,false}};song=compile(document);const auto length=song.scenes[1].beats;
    expect(song.song.beats==length*2,"song repeat lengths");
    auto first=generateClip(document.scenes[1],0,0),second=generateClip(document.scenes[1],0,0);expect(first.size()==second.size(),"repeatable generation");
    auto changed=generateClip(document.scenes[1],0,13);bool different=first.size()!=changed.size();for(size_t i=0;i<std::min(first.size(),changed.size());++i)different|=first[i].pitch!=changed[i].pitch||first[i].length!=changed[i].length||first[i].velocity!=changed[i].velocity;expect(different,"evolving variants change realized notes");
    document.scenes[1].numerator=7;document.scenes[1].denominator=8;song=compile(document);expect(song.scenes[1].beats==14,"7/8 scene length");
    auto thresholdDocument=factoryDocument();thresholdDocument.scenes[2].intensity=.3;thresholdDocument.scenes[2].clips[6].threshold=.7;auto quiet=compile(thresholdDocument);
    for(const auto& n:quiet.scenes[2].notes)expect(n.track!=6,"intensity removes instruments below their threshold");
    Scene sustained=thresholdDocument.scenes[2];sustained.clips[1].generated=false;sustained.clips[1].notes={{0,12,60,100,0}};thresholdDocument.scenes[2]=sustained;auto checkpoints=compile(thresholdDocument);
    expect(!checkpoints.scenes[2].sections[1].chase.empty(),"bar checkpoints retain held manual notes for seek");
    bool invalid=false;try{auto bad=document;bad.song[0].sceneId=999;validate(bad);}catch(...){invalid=true;}expect(invalid,"missing scene reference rejected");
    for(int track=0;track<7;++track){Instrument engine;engine.prepare(48000);Parameters p;p.bass=track==0;p.pad=track==1;p.arp=track==2;p.drums=track>=3;p.fxEnabled=false;p.padAttack=.01;p.padRelease=.01;p.arpAttack=.001;
        engine.externalNote(track,track>=3?drumNotes[track-3]:60,1,0,100,true,false,false,&p);double stemEnergy=0,otherEnergy=0,mainEnergy=0;
        for(int i=0;i<12000;++i){auto out=engine.next(p,double(i)/24000,4,4,true,1U<<track);mainEnergy+=std::abs(out.l)+std::abs(out.r);const auto& stems=engine.stemOutputs();for(int t=0;t<7;++t){const auto x=stems[size_t(t)];expect(std::isfinite(x.l)&&std::isfinite(x.r),"finite separate output");if(t==track)stemEnergy+=std::abs(x.l)+std::abs(x.r);else otherEnergy+=std::abs(x.l)+std::abs(x.r);}}
        expect(stemEnergy>.01,"every separate output audible");expect(otherEnergy==0,"no bleed into other instruments");expect(mainEnergy==0,"split output excluded from MAIN");
    }
    for(int track=0;track<3;++track){Scene rootScene;rootScene.bars=2;auto& clip=rootScene.clips[size_t(track)];clip.generated=true;clip.explicitRoots=true;clip.notes={{1,2,43,100,0}};
        auto generated=generateClip(rootScene,track);expect(!generated.empty(),"root gate drives every melodic module");
        for(const auto& n:generated)expect(n.beat>=1&&n.beat+n.length<=3.00001,"module phrase remains within root gate");
        if(track==2)expect(generated.size()>1,"single long root expands into arpeggio");
        auto shifted=rootScene;shifted.clips[size_t(track)].notes[0].pitch+=2;auto transposed=generateClip(shifted,track);expect(transposed.size()==generated.size(),"root transposition keeps rhythm");
        for(size_t i=0;i<generated.size();++i)expect(transposed[i].pitch==generated[i].pitch+2,"module follows Composer root");
        clip.generated=false;auto manual=generateClip(rootScene,track);expect(manual.size()==1&&manual[0].pitch==43&&manual[0].length==2,"MANUAL keeps exact root note");
        clip.generated=true;clip.notes.clear();expect(generateClip(rootScene,track).empty(),"deleted explicit roots stay silent");
    }
    ProMidiClock clock;clock.reset(48000);expect(!clock.valid(0),"clock starts waiting");clock.start();
    for(int t=0;t<25;++t)clock.tick(t*1000);expect(clock.valid(24000)&&std::abs(clock.bpm-120)<.001,"24 PPQN clock locks to 120 BPM");
    expect(std::abs(clock.beat-1)<1e-9,"MIDI clock quarter-note position");expect(!clock.valid(72000),"missing MIDI clock is lost");clock.songPosition(16);expect(clock.beat==4,"MIDI song-position sixteenths");clock.stop();expect(!clock.running,"MIDI stop halts clock");
    {Instrument engine;engine.prepare(48000);Parameters p;p.bass=true;p.pad=p.arp=p.drums=false;p.fxEnabled=false;p.mix0Mute=true;
        engine.externalNote(0,48,1,0,1,true,false,false,&p);double energy=0;for(int i=0;i<2000;++i){auto out=engine.next(p,double(i)/24000,4,4,true);energy+=std::abs(out.l)+std::abs(out.r);}expect(energy==0,"mixer mute silences stem and MAIN");
        p.mix0Mute=false;p.mix1Solo=true;for(int i=0;i<2000;++i){auto out=engine.next(p,double(i)/24000,4,4,true);energy+=std::abs(out.l)+std::abs(out.r);}expect(energy==0,"solo excludes other instruments");
        p.mix1Solo=false;p.mix0Pan=-1;for(int i=0;i<2000;++i){engine.next(p,double(i)/24000,4,4,true);expect(engine.stemOutputs()[0].r==0,"mixer hard-left balance");}
    }
    {Instrument engine;engine.prepare(48000);Parameters p;p.bass=true;p.pad=p.arp=p.drums=false;p.bassFX=p.padFX=p.masterFX=0;p.mix0Delay=p.mix0Reverb=.5;
        engine.configureResources(p);engine.serviceResources();engine.configureResources(p);expect(engine.fxMemoryBytes()>0,"send racks allocate outside render loop");
        engine.externalNote(0,48,1,0,1,true,false,false,&p);double delayEnergy=0,reverbEnergy=0,mainEnergy=0;
        for(int i=0;i<72000;++i){if(i==6000)engine.externalNote(0,48,0,0,1,false);auto out=engine.next(p,double(i)/24000,4,4,true,(1U<<0)|(1U<<7)|(1U<<8));mainEnergy+=std::abs(out.l)+std::abs(out.r);delayEnergy+=std::abs(engine.returnOutputs()[0].l)+std::abs(engine.returnOutputs()[0].r);reverbEnergy+=std::abs(engine.returnOutputs()[1].l)+std::abs(engine.returnOutputs()[1].r);}
        expect(delayEnergy>.01&&reverbEnergy>.01,"both send effects produce separate wet returns");expect(mainEnergy==0,"connected sends and stems excluded from MAIN");p.fxEnabled=false;engine.configureResources(p);engine.serviceResources();expect(engine.fxMemoryBytes()==0,"global FX power frees send buffers");
    }
    std::cout<<"PASS "<<checks<<" Composer checks: scenes, progressions, freeze, song repeats, variants, meters, seven isolated outputs\n";
}catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<"\n";return 1;}}
