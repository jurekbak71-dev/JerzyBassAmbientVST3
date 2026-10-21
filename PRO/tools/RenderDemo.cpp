#include "Core.h"
#include <fstream>
#include <iostream>
#include <string>
#include <chrono>
void word(std::ostream& out,uint32_t value,int bytes){for(int i=0;i<bytes;++i)out.put(char((value>>(8*i))&255));}
int main(int argc,char** argv){
    const std::string prefix=argc>1?argv[1]:"pro";
    constexpr int sr=48000,seconds=12,count=sr*seconds;
    for(int scene=0;scene<4;++scene){jerzy::Parameters p;p.autoPlay=true;p.autoHarmony=1;p.harmonyBars=1;p.master=.65;
        std::string name;
        if(scene==0){name="deep-house";p.style=5;p.model=2;p.drums=true;p.swing=.2;p.fill=.5;p.arp=true;p.arpPulses=5;p.arpLevel=.2;}
        if(scene==1){name="stereo-arp";p.bass=false;p.arp=true;p.arpModel=1;p.arpPattern=4;p.arpPitchRandom=.5;p.arpGateRandom=.8;p.arpPanMode=1;p.arpWidth=1;}
        if(scene==2){name="lush-pad";p.bass=false;p.pad=true;p.padAttack=1;p.padRelease=4;p.engine2=2;p.padDetune=16;p.padShimmer=.45;}
        if(scene==3){name="krell";p.bass=false;p.pad=true;p.padMode=2;p.padAttack=.8;p.krellMin=.4;p.krellMax=4;p.krellSpace=.5;p.engine1=3;p.engine2=2;p.padDetune=12;}
        jerzy::Instrument engine;engine.prepare(sr);engine.configureResources(p);engine.serviceResources();engine.configureResources(p);
        const auto file=prefix+"-"+name+".wav";std::ofstream out(file,std::ios::binary);if(!out){std::cerr<<"Cannot write "<<file<<"\n";return 1;}
        out.write("RIFF",4);word(out,36+count*4,4);out.write("WAVEfmt ",8);word(out,16,4);word(out,1,2);word(out,2,2);word(out,sr,4);word(out,sr*4,4);word(out,4,2);word(out,16,2);out.write("data",4);word(out,count*4,4);
        double peak=0,energy=0;const auto start=std::chrono::steady_clock::now();
        for(int i=0;i<count;++i){auto x=engine.next(p,double(i)*2/sr);const double fade=std::min(1.0,std::min(double(i)/480,double(count-1-i)/4800));x=x*fade;
            if(!std::isfinite(x.l)||!std::isfinite(x.r)){std::cerr<<"Nonfinite audio\n";return 1;}
            peak=std::max(peak,std::max(std::abs(x.l),std::abs(x.r)));energy+=x.l*x.l+x.r*x.r;
            word(out,uint16_t(int16_t(jerzy::clamp(x.l,-1.0,1.0)*32767)),2);word(out,uint16_t(int16_t(jerzy::clamp(x.r,-1.0,1.0)*32767)),2);
        }
        if(!out||energy<.001){std::cerr<<"Silent or failed render\n";return 1;}
        std::cout<<file<<" peak="<<peak<<" rms="<<std::sqrt(energy/(count*2))<<" render_seconds="<<std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count()<<"\n";
    }
}
