#include "Composer.h"
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
    std::cout<<"PASS "<<checks<<" Composer checks: scenes, progressions, freeze, song repeats, variants, meters, seven isolated outputs\n";
}catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<"\n";return 1;}}
