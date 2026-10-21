#include "Core.h"
#include <iostream>
#include <stdexcept>
#include <thread>
#include <chrono>
using namespace jerzy;
static int checks=0;
void require(bool condition,const char* message){++checks;if(!condition)throw std::runtime_error(message);}
void warm(Instrument& engine,const Parameters& p){engine.configureResources(p);engine.serviceResources();engine.configureResources(p);}
double render(Instrument& engine,const Parameters& p,int samples,double start=0,double sr=48000){double total=0;for(int i=0;i<samples;++i){auto x=engine.next(p,start+double(i)*2/sr);require(std::isfinite(x.l)&&std::isfinite(x.r),"finite PRO render");require(std::abs(x.l)<=1&&std::abs(x.r)<=1,"limited PRO render");total+=x.l*x.l+x.r*x.r;}return total;}
int main(){try{
    Parameters p;p.bass=false;p.arp=true;p.fxEnabled=false;p.autoPlay=true;
    const int chord[]{60,64,67};
    p.arpPulses=16;p.arpGateRandom=.9;p.arpPitchRandom=1;p.arpRange=7;
    for(int pattern=0;pattern<6;++pattern){p.arpPattern=pattern;
        for(int step=0;step<64;++step){const auto a=makeArpEvent(p,step,chord,3,48),b=makeArpEvent(p,step,chord,3,48);
            require(a.note==b.note&&a.gate==b.gate,"deterministic random arp");require(a.note>=36&&a.note<=84,"bounded scale range");require(a.gate>=.02&&a.gate<=1.8,"bounded random length");}
    }
    p.arpPitchRandom=0;p.arpPattern=0;p.arpPanMode=1;p.arpWidth=1;
    require(makeArpEvent(p,0,chord,3,48).pan==-1&&makeArpEvent(p,1,chord,3,48).pan==1,"alternating stereo notes");
    p.arpSteps=13;p.arpPulses=5;int pulses=0;for(int k=0;k<13;++k)pulses+=makeArpEvent(p,k,chord,3,48).note>=0;require(pulses==5,"Euclidean pulse count");
    for(int model=0;model<4;++model){p.arpModel=model;Instrument engine;engine.prepare(48000);require(render(engine,p,24000)>.001,"four audible arp synths without MIDI");}
    p=Parameters{};p.bass=false;p.drums=true;p.autoPlay=true;p.fxEnabled=false;
    for(auto& lane:p.drumGrid)lane.fill(0);
    for(int lane=0;lane<4;++lane){for(int aux=0;aux<(lane==3?3:1);++aux){p.drumAux=aux;p.drumGrid[size_t(lane)][0]=127;Instrument engine;engine.prepare(48000);require(render(engine,p,12000)>.001,"each drum voice audible");p.drumGrid[size_t(lane)][0]=0;}}
    p.drumLength=64;p.drumGrid[0][63]=127;Instrument drums;drums.prepare(48000);drums.beginBlock(0,120,true);
    require(render(drums,p,4000,15.75)>.001,"step 64 plays");require(drums.drumPosition()==63,"64-step length");
    p.drumLength=16;drums.reset();require(render(drums,p,4000,15.75)==0,"steps beyond length excluded");
    p=Parameters{};p.fxEnabled=false;p.bass=false;p.drums=true;Instrument midiDrums;midiDrums.prepare(48000);midiDrums.noteOn(36,10,1,p,0);
    require(midiDrums.currentRoot()==-1,"GM percussion does not change root");require(render(midiDrums,p,5000)>.001,"GM channel 10 drum trigger");
    p=Parameters{};p.autoPlay=true;p.pad=true;p.padMode=2;p.krellMin=.1;p.krellMax=.4;p.padAttack=.1;p.padRelease=.2;p.fxEnabled=false;p.bass=false;
    Instrument a,b;a.prepare(48000);b.prepare(48000);double difference=0,stereo=0,total=0;
    for(int i=0;i<96000;++i){auto x=a.next(p,double(i)/24000),y=b.next(p,double(i)/24000);difference+=std::abs(x.l-y.l)+std::abs(x.r-y.r);stereo+=std::abs(x.l-x.r);total+=x.l*x.l+x.r*x.r;}
    require(difference==0&&stereo>.1&&total>.01,"Krell independent cycles: repeatable audible stereo");
    p=Parameters{};p.autoPlay=true;p.pad=true;p.masterFX=.5;p.fxGrain=true;p.padRack.fxGrain=true;Instrument resources;resources.prepare(48000);warm(resources,p);
    const auto before=resources.fxMemoryBytes();require(before>10000000,"active FX buffers allocated");
    p.fxEnabled=false;
    for(int i=0;i<4;++i){resources.configureResources(p);resources.serviceResources();}
    require(resources.fxMemoryBytes()==0,"FX power releases buffers");
    p.fxEnabled=true;warm(resources,p);require(resources.fxMemoryBytes()>0,"FX buffers recover after enable");
    p.bass=p.pad=false;p.arp=p.drums=false;warm(resources,p);
    require(render(resources,p,256)==0,"all modules off is exact silence");
    // Exercise the actual producer/consumer exchange while masks change rapidly.
    ManagedRack rack;rack.prepare(48000);std::atomic<bool> done{false};
    std::thread worker([&]{while(!done.load())rack.maintenance();rack.maintenance();});
    for(int block=0;block<1000;++block){FXParameters fx;fx.fxGrain=block%3==0;rack.configure(fx,block%5!=0);for(int k=0;k<16;++k){auto x=rack.process({.01,.01},fx,120);require(std::isfinite(x.l),"concurrent resource handoff");}}
    done=true;worker.join();rack.configure(p,false);rack.maintenance();rack.configure(p,false);rack.maintenance();
    Parameters harmonic;harmonic.autoPlay=true;harmonic.autoHarmony=1;harmonic.harmonyBars=1;harmonic.fxEnabled=false;
    Instrument progression;progression.prepare(48000);progression.next(harmonic,0);const int firstRoot=progression.currentRoot();progression.next(harmonic,4);
    require(firstRoot!=progression.currentRoot(),"generative harmonic changes follow bars");
    auto benchmark=[](bool enabled){Parameters params;params.autoPlay=true;params.fxEnabled=false;params.bass=params.pad=params.arp=params.drums=enabled;params.padAttack=.1;
        Instrument engine;engine.prepare(48000);volatile double sink=0;const auto begin=std::chrono::steady_clock::now();
        for(int i=0;i<24000;++i){const auto sample=engine.next(params,double(i)/24000);sink+=sample.l+sample.r;}
        (void)sink;return std::chrono::duration<double>(std::chrono::steady_clock::now()-begin).count();};
    const auto idle=benchmark(false),active=benchmark(true);
    std::cout<<"CPU benchmark (0.5 seconds audio, FX off): modules active "<<active<<" s; modules disabled "<<idle<<" s\n";
    std::cout<<"PASS "<<checks<<" PRO checks; active FX buffers "<<before<<" bytes, power-off 0 bytes\n";
}catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<"\n";return 1;}}
