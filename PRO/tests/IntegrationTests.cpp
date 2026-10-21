#include "PluginProcessor.h"
#include <iostream>
#include <stdexcept>

void require(bool ok,const char* message) { if(!ok) throw std::runtime_error(message); }
#include "AudioRenderChecks.h"
class Host final : public juce::AudioPlayHead {
public:
    double ppq=0,bpm=120;bool playing=true;
    juce::Optional<PositionInfo> getPosition() const override {
        PositionInfo p;p.setPpqPosition(ppq);p.setBpm(bpm);p.setIsPlaying(playing);p.setTimeSignature(TimeSignature{7,8});return p;
    }
};
int main() {
    try {
        juce::ScopedJuceInitialiser_GUI init;
        BassAmbientProcessor processor;Host host;processor.setPlayHead(&host);processor.prepareToPlay(48000,128);
        auto set=[&](const char* id,float v) { auto* p=processor.state.getParameter(id);require(p!=nullptr,"parameter missing");p->setValueNotifyingHost(p->convertTo0to1(v)); };
        auto specs=parameterSpecs();require(processor.getParameters().size()==int(specs.size()),"host parameter count");
        for(const auto& s:specs) require(processor.state.getRawParameterValue(s.id)!=nullptr,"unbound parameter");
        juce::AudioBuffer<float> audio(2,128);juce::MidiBuffer midi;
        midi.addEvent(juce::MidiMessage::noteOn(1,36,1.0f),32);processor.processBlock(audio,midi);
        float early=0,late=0;for(int i=0;i<128;++i) { if(i<32)early+=std::abs(audio.getSample(0,i));else late+=std::abs(audio.getSample(0,i)); }
        require(early==0&&late>0,"sample-offset MIDI note-on");require(processor.rootDisplay.load()==36,"host MIDI root");
        host.ppq+=128.0/24000;midi.addEvent(juce::MidiMessage::noteOff(1,36),64);processor.processBlock(audio,midi);
        require(processor.rootDisplay.load()==-1,"sample-offset MIDI note-off");
        set("cutoff",731);set("seed",4321);set("pad",1);set("order0",4);set("order4",0);
        juce::MemoryBlock state;processor.getStateInformation(state);set("cutoff",12000);set("seed",1);
        processor.setStateInformation(state.getData(),int(state.getSize()));
        require(std::abs(processor.state.getRawParameterValue("cutoff")->load()-731)<.01f,"cutoff state restore");
        require(processor.state.getRawParameterValue("seed")->load()==4321,"seed state restore");
        require(processor.readParameters().order[0]==4,"FX order state restore");
        for(int k=0;k<6;++k) { processor.applyPreset(k);auto p=processor.readParameters();require(p.seed==71,"factory preset complete reset"); }
        set("delayMix",.11f);set("pad_delayMix",.37f);set("master_delayMix",.61f);
        set("pad_order0",4);set("pad_order4",0);set("masterFX",.7f);
        processor.getStateInformation(state);processor.applyPreset(0);
        processor.setStateInformation(state.getData(),int(state.getSize()));
        auto racks=processor.readParameters();
        require(std::abs(racks.delayMix-.11)<1e-5&&std::abs(racks.padRack.delayMix-.37)<1e-5&&std::abs(racks.masterRack.delayMix-.61)<1e-5,"three independent FX states");
        require(racks.padRack.order[0]==4&&racks.order[0]==0&&racks.masterRack.order[0]==0,"three independent FX orders");
        // Rebuild an authentic schema-1 tree: only legacy IDs, shared settings.
        auto old=processor.state.copyState();old.setProperty("schemaVersion",1,nullptr);
        for(int i=old.getNumChildren()-1;i>=0;--i) {
            auto id=old.getChild(i).getProperty("id").toString();
            if(id.startsWith("pad_")||id.startsWith("master_")||id=="masterFX") old.removeChild(i,nullptr);
        }
        juce::MemoryBlock legacy;auto xml=old.createXml();juce::AudioProcessor::copyXmlToBinary(*xml,legacy);
        processor.setStateInformation(legacy.getData(),int(legacy.getSize()));racks=processor.readParameters();
        require(std::abs(racks.padRack.delayMix-racks.delayMix)<1e-6&&racks.masterFX==0,"legacy shared FX migration with dry master");
        processor.applyPreset(0);
        std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
        auto tab=[&](const juce::String& name) {
            for(int i=0;i<editor->getNumChildComponents();++i)
                if(auto* button=dynamic_cast<juce::TextButton*>(editor->getChildComponent(i)))
                    if(button->getButtonText()==name&&button->onClick) {button->onClick();return;}
            throw std::runtime_error("main page missing");
        };
        juce::ComboBox* selector=nullptr;
        for(int i=0;i<editor->getNumChildComponents();++i)
            if(editor->getChildComponent(i)->getComponentID()=="rackSelector") selector=dynamic_cast<juce::ComboBox*>(editor->getChildComponent(i));
        require(selector!=nullptr,"FX rack selector missing");require(selector->getNumItems()==6,"six instrument/master FX racks");
        auto snapshot=[&](const juce::String& suffix) {
            auto image=editor->createComponentSnapshot(editor->getLocalBounds(),true,1);
            require(image.isValid()&&image.getWidth()==editor->getWidth()&&image.getHeight()==editor->getHeight(),"resizable GUI snapshot");
            auto stream=juce::File::getCurrentWorkingDirectory().getChildFile("gui-"+suffix+".png").createOutputStream();
            require(stream!=nullptr,"GUI snapshot output");require(stream->setPosition(0)&&stream->truncate().wasOk(),"fresh GUI snapshot output");juce::PNGImageFormat png;
            require(png.writeImageToStream(image,*stream),"GUI PNG encoding");
        };
        const std::array<std::pair<int,int>,3> sizes{{{1000,680},{1200,820},{1600,1100}}};
        for(const auto& size:sizes) {
            editor->setSize(size.first,size.second);
            for(const auto& name:juce::StringArray{"BASS","AMBIENT","ARPEGGIO","DRUMS","FX","GENERATE","COMPOSER","KRELL"}) {
                tab(name);require(selector->isVisible()==(name=="FX"),"rack selector visibility");
                if(name!="FX") snapshot(name+"-"+juce::String(size.first));
                else for(int rack=1;rack<=6;++rack) {
                    selector->setSelectedId(rack,juce::sendNotificationSync);
                    snapshot("FX-"+juce::String(rack)+"-"+juce::String(size.first));
                }
            }
        }
        // Reordering one rack through its actual GUI leaves the others untouched.
        tab("FX");selector->setSelectedId(2,juce::sendNotificationSync);
        juce::Viewport* viewport=nullptr;
        for(int i=0;i<editor->getNumChildComponents();++i)
            if(auto* v=dynamic_cast<juce::Viewport*>(editor->getChildComponent(i))) viewport=v;
        require(viewport!=nullptr,"page viewport");auto* page=viewport->getViewedComponent();
        bool moved=false;
        for(int i=0;i<page->getNumChildComponents();++i)
            if(auto* button=dynamic_cast<juce::TextButton*>(page->getChildComponent(i)))
                if(button->getButtonText()==">"&&button->isEnabled()) {button->onClick();moved=true;break;}
        racks=processor.readParameters();require(moved&&racks.padRack.order[0]==1&&racks.padRack.order[1]==0&&racks.order[0]==0&&racks.masterRack.order[0]==0,"GUI independent rack reorder");
        bool delaySelected=false;
        for(int i=0;i<page->getNumChildComponents();++i)
            if(auto* button=dynamic_cast<juce::TextButton*>(page->getChildComponent(i)))
                if(button->getButtonText().contains("DELAY")) {button->onClick();delaySelected=true;break;}
        require(delaySelected,"FX module navigation");
        bool attached=false;
        for(int i=0;i<page->getNumChildComponents();++i) {
            auto* control=page->getChildComponent(i);
            if(control->getComponentID()=="pad_delayMix") {
                require(control->isVisible(),"selected module control visible");
                for(int k=0;k<control->getNumChildComponents();++k)
                    if(auto* slider=dynamic_cast<juce::Slider*>(control->getChildComponent(k))) {slider->setValue(.63,juce::sendNotificationSync);attached=true;}
            }
        }
        racks=processor.readParameters();require(attached&&std::abs(racks.padRack.delayMix-.63)<1e-5&&std::abs(racks.delayMix-.2)<1e-5,"rack GUI connected to correct DSP parameters");
        snapshot("AMBIENT-DELAY");
        // All added parameters and all 256 velocities persist through host state.
        set("arp",1);set("drums",1);set("arpGateRandom",.73f);set("drumLength",64);set("drum3_step63",127);
        processor.getStateInformation(state);set("drum3_step63",0);set("arp",0);processor.setStateInformation(state.getData(),int(state.getSize()));
        require(processor.readParameters().arp&&processor.readParameters().drumGrid[3][63]==127,"PRO state roundtrip");
        processor.applyPreset(0);set("pad",0);set("arp",0);set("drums",0);set("autoPlay",0);
        midi.clear();midi.addEvent(juce::MidiMessage::noteOn(1,36,1.0f),0);
        midi.addEvent(juce::MidiMessage::controllerEvent(1,20,0),64);processor.processBlock(audio,midi);
        for(int k=64;k<128;++k)require(audio.getSample(0,k)==0&&audio.getSample(1,k)==0,"MIDI CC20 sample-accurate module off");
        processor.learnMidi("arp");midi.addEvent(juce::MidiMessage::controllerEvent(1,42,127),0);processor.processBlock(audio,midi);
        require(processor.readParameters().arp&&processor.midiAssignment("arp")=="CC 42","MIDI learn toggle");
        processor.getStateInformation(state);processor.clearMidiMapping("arp");processor.setStateInformation(state.getData(),int(state.getSize()));
        require(processor.midiAssignment("arp")=="CC 42","MIDI mapping persists");
        processor.learnMidi("arpCutoff");midi.addEvent(juce::MidiMessage::controllerEvent(1,43,64),0);processor.processBlock(audio,midi);
        require(processor.readParameters().arpCutoff>40&&processor.readParameters().arpCutoff<18000,"MIDI continuous control");
        tab("DRUMS");page=viewport->getViewedComponent();juce::TextButton* grid=nullptr;
        for(int i=0;i<page->getNumChildComponents();++i)if(page->getChildComponent(i)->getComponentID()=="grid-0-0")grid=dynamic_cast<juce::TextButton*>(page->getChildComponent(i));
        require(grid!=nullptr,"drum grid exists");const auto velocity=processor.readParameters().drumGrid[0][0];grid->onClick();
        require((processor.readParameters().drumGrid[0][0]==0)==(velocity>0),"grid connected to sequencer");
        // Right-click on an actual knob resets the underlying host parameter.
        tab("ARPEGGIO");page=viewport->getViewedComponent();juce::Component* cutoff=nullptr;
        for(int i=0;i<page->getNumChildComponents();++i)if(page->getChildComponent(i)->getComponentID()=="arpCutoff")cutoff=page->getChildComponent(i);
        require(cutoff!=nullptr,"arp cutoff GUI");set("arpCutoff",10000);
        auto source=juce::Desktop::getInstance().getMainMouseSource();juce::MouseEvent click(source,{},juce::ModifierKeys(juce::ModifierKeys::rightButtonModifier),0,0,0,0,0,cutoff,cutoff,juce::Time::getCurrentTime(),{},juce::Time::getCurrentTime(),1,false);
        cutoff->mouseDown(click);const double resetError=std::abs(processor.readParameters().arpCutoff-4200);std::cout<<"Right-click cutoff error: "<<resetError<<" Hz\n";require(resetError<.01,"right-click default");
        set("autoPlay",1);midi.addEvent(juce::MidiMessage::controllerEvent(1,120,0),0);processor.processBlock(audio,midi);
        require(!processor.readParameters().autoPlay,"MIDI hard panic disables generator");
        processor.releaseResources();
        auto project=jerzy::composer::factoryDocument();project.scenes.resize(1);auto& scene=project.scenes[0];scene.bars=1;scene.cutTails=true;
        scene.patch.fxEnabled=false;scene.patch.bass=scene.patch.pad=scene.patch.arp=scene.patch.drums=true;scene.patch.padAttack=.01;
        for(int t=0;t<8;++t){auto& c=scene.clips[size_t(t)];c.enabled=true;c.generated=false;c.enterBar=c.leaveBar=0;c.notes={{0,.5,t<3?36+t*12:t==7?60:jerzy::composer::drumNotes[t-3],100,0}};}
        project.song={{scene.id,2,false}};processor.editComposer([&](auto& d){d=project;});
        const auto file=juce::File::getCurrentWorkingDirectory().getChildFile("composer-test.mid");
        auto result=processor.exportComposer(file,true,0);require(result.wasOk(),"song MIDI export");juce::MidiFile exported;auto input=file.createInputStream();require(input&&exported.readFrom(*input),"MIDI readback");
        require(exported.getNumTracks()==9&&exported.getTimeFormat()==960,"type-1 eight-instrument export");int exportedNotes=0;double lastTick=0;
        for(int t=0;t<exported.getNumTracks();++t)for(int i=0;i<exported.getTrack(t)->getNumEvents();++i){const auto& m=exported.getTrack(t)->getEventPointer(i)->message;if(m.isNoteOn())++exportedNotes;lastTick=std::max(lastTick,m.getTimeStamp());}
        require(exportedNotes==16&&lastTick>=8*960,"export matches repeated song notes and duration");
        const auto projectFile=file.withFileExtension("jpro");require(processor.saveComposer(projectFile).wasOk(),"Composer project save");
        processor.editComposer([](auto& d){d.scenes[0].name="CHANGED";});require(processor.loadComposer(projectFile).wasOk(),"Composer project load");require(processor.composerDocument().scenes[0].name==project.scenes[0].name,"project names restored");
        processor.getStateInformation(state);processor.editComposer([](auto& d){d.song.clear();});processor.setStateInformation(state.getData(),int(state.getSize()));require(processor.composerDocument().song.size()==1,"Composer embedded in host state");
        auto buses=processor.getBusesLayout();require(buses.outputBuses.size()==11,"MAIN, eight instruments and two FX return buses");for(auto& layout:buses.outputBuses)layout=juce::AudioChannelSet::stereo();require(processor.setBusesLayout(buses),"enable all mixer outputs");
        auto mono=buses;mono.outputBuses.set(1,juce::AudioChannelSet::mono());std::cout<<"Mono stem: "<<mono.outputBuses[1].getDescription()<<" direct="<<processor.isBusesLayoutSupported(mono)<<" checked="<<processor.checkBusesLayoutSupported(mono)<<"\n";require(!processor.checkBusesLayoutSupported(mono),"reject unsupported stem layout");
        processor.prepareToPlay(48000,128);set("separateOutputs",1);set("fxEnabled",0);processor.launchComposer(0,true);host.ppq=0;host.playing=true;
        juce::AudioBuffer<float> multi(22,128);double mainSum=0;std::array<double,8> energies{};
        for(int block=0;block<100;++block){midi.clear();processor.processBlock(multi,midi);for(int k=0;k<128;++k){mainSum+=std::abs(multi.getSample(0,k))+std::abs(multi.getSample(1,k));for(int t=0;t<8;++t)energies[size_t(t)]+=std::abs(multi.getSample(t==7?20:2+t*2,k))+std::abs(multi.getSample(t==7?21:3+t*2,k));}host.ppq+=128.0/24000;}
        require(mainSum==0,"MAIN does not double routed stems");for(double energy:energies)require(energy>.001,"every FL mixer output carries its instrument");
        for(int block=100;block<1650;++block){processor.processBlock(multi,midi);host.ppq+=128.0/24000;}
        require(!processor.readParameters().composerPlaying,"SONG stops automatically after the final repeat");
        processor.releaseResources();processor.editComposer([](auto& d){d.scenes[0].clips[6].threshold=.7;});processor.prepareToPlay(48000,128);set("composerIntensity",.2f);processor.launchComposer(0,true);host.ppq=0;double mutedAux=0;
        for(int block=0;block<10;++block){processor.processBlock(multi,midi);for(int k=0;k<128;++k)mutedAux+=std::abs(multi.getSample(14,k))+std::abs(multi.getSample(15,k));host.ppq+=128.0/24000;}
        require(mutedAux==0,"global intensity removes instruments below threshold");set("composerIntensity",1);
        set("pad",0);processor.processBlock(multi,midi);host.ppq+=128.0/24000;
        set("composerEnabled",0);set("midiPlayback",1);set("bass",0);set("pad",1);set("arp",0);set("drums",0);midi.addEvent(juce::MidiMessage::noteOn(2,60,1.0f),0);double directPad=0;
        for(int block=0;block<20;++block){processor.processBlock(multi,midi);for(int k=0;k<128;++k)directPad+=std::abs(multi.getSample(4,k))+std::abs(multi.getSample(5,k));host.ppq+=128.0/24000;}
        require(directPad>.001,"first imported MIDI note survives switching from Composer");set("midiPlayback",0);

        for(int t=1;t<11;++t)buses.outputBuses.set(t,juce::AudioChannelSet::disabled());require(processor.setBusesLayout(buses),"disable optional buses");
        processor.releaseResources();processor.prepareToPlay(48000,128);processor.launchComposer(0,true);host.ppq=0;double fallback=0;for(int block=0;block<10;++block){processor.processBlock(audio,midi);for(int k=0;k<128;++k)fallback+=std::abs(audio.getSample(0,k));host.ppq+=128.0/24000;}require(fallback>.01,"unconnected stems fall back to MAIN");
        processor.launchComposer(0,true);
        editor->setSize(1440,1280);tab("COMPOSER");snapshot("COMPOSER-final");tab("MIXER");snapshot("MIXER-final");for(int i=0;i<editor->getNumChildComponents();++i)if(auto* b=dynamic_cast<juce::TextButton*>(editor->getChildComponent(i)))if(b->getButtonText()=="MIXER")require(b->getRight()<=editor->getWidth(),"MIXER tab remains within editor bounds");tab("COMPOSER");
        processor.editComposer([](auto& d){d=jerzy::composer::factoryDocument();});
        const auto demo=juce::File::getCurrentWorkingDirectory();require(processor.exportComposer(demo.getChildFile("PRO-composer-song.mid"),true,0).wasOk(),"factory song export");require(processor.saveComposer(demo.getChildFile("Composer-Demo.jpro")).wasOk(),"factory Composer project");
        const auto factory=processor.composerDocument();const auto compiled=jerzy::composer::compile(factory);const auto pack=demo.getChildFile("MIDI-Pattern-Pack");require(pack.createDirectory().wasOk(),"pattern pack folder");
        for(size_t i=0;i<factory.scenes.size();++i)for(int t=0;t<8;++t)require(jerzy::composer::writeMidi(compiled.scenes[i],factory.tempo,pack.getChildFile(juce::String(int(i)+1)+"-"+juce::String(factory.scenes[i].name)+"-"+jerzy::composer::trackNames[t]+".mid"),t).wasOk(),"per-scene per-instrument MIDI export");
        processor.releaseResources();processor.setPlayHead(nullptr);
        // Clock source selection must not silently use the other source.
        processor.setPlayHead(&host);processor.prepareToPlay(48000,128);
        processor.transport(false);set("composerEnabled",0);set("clockSource",1);host.bpm=97;host.playing=true;processor.processBlock(audio,midi);require(std::abs(processor.tempoDisplay.load()-97)<.01,"HOST tempo follows FL Studio");
        set("clockSource",0);set("internalTempo",113);processor.processBlock(audio,midi);require(std::abs(processor.tempoDisplay.load()-113)<.01,"INTERNAL ignores host BPM");
        set("clockSource",2);processor.processBlock(audio,midi);require(processor.clockStatus.load()==2,"MIDI clock waits for external input");
        int64_t absolute=0;for(int block=0;block<250;++block){midi.clear();if(block==0)midi.addEvent(juce::MidiMessage::midiStart(),0);for(int i=0;i<128;++i)if((absolute+i)%1000==0)midi.addEvent(juce::MidiMessage::midiClock(),i);processor.processBlock(audio,midi);absolute+=128;}
        require(processor.clockStatus.load()==1&&std::abs(processor.tempoDisplay.load()-120)<.1,"external MIDI clock locks to 120 BPM at audio timestamps");
        midi.addEvent(juce::MidiMessage::midiStop(),0);processor.processBlock(audio,midi);require(!processor.readParameters().autoPlay,"MIDI stop stops generation");
        for(int i=0;i<400;++i)processor.processBlock(audio,midi);require(processor.clockStatus.load()==3,"missing external MIDI clock reports LOST");
        set("clockSource",0);set("mix2Gain",.73f);set("mix2Pan",-.4f);set("mix2Delay",.3f);set("mix1Solo",1);processor.getStateInformation(state);set("mix2Gain",1);set("mix1Solo",0);processor.setStateInformation(state.getData(),int(state.getSize()));require(std::abs(processor.readParameters().mix2Gain-.73)<.001&&processor.readParameters().mix1Solo,"mixer fader, pan, sends and solo state round trip");
        processor.launchComposer(0,false);for(int i=0;i<40;++i)processor.processBlock(audio,midi);const double oldBeat=processor.composerBeatDisplay.load();
        set("arpGate",.91f);juce::Thread::sleep(30);processor.processBlock(audio,midi);juce::Thread::sleep(20);processor.processBlock(audio,midi);
        require(std::abs(processor.composerDocument().scenes[0].patch.arpGate-.91)<.001,"live module control updates Composer scene");require(processor.composerBeatDisplay.load()>=oldBeat,"patch recompile preserves arrangement position");
        processor.transport(false);processor.releaseResources();processor.setPlayHead(nullptr);
        // Loading a scene captured with FX off must not defeat the global FX switch.
        {BassAmbientProcessor fxProcessor;fxProcessor.editComposer([](auto& d){d.scenes.resize(1);d.song.clear();auto& scene=d.scenes[0];scene.patch.fxEnabled=false;for(auto& clip:scene.clips)clip.enabled=false;scene.clips[3].enabled=true;});
            fxProcessor.setParameterValue("fxEnabled",1);fxProcessor.setParameterValue("composerLoop",1);fxProcessor.setParameterValue("separateOutputs",1);
            fxProcessor.setParameterValue("mix3Delay",1);fxProcessor.setParameterValue("mix3Reverb",1);
            auto outputs=fxProcessor.getBusesLayout();for(auto& bus:outputs.outputBuses)bus=juce::AudioChannelSet::stereo();require(fxProcessor.setBusesLayout(outputs),"FX return test buses");
            fxProcessor.prepareToPlay(48000,128);fxProcessor.launchComposer(0,false);juce::AudioBuffer<float> wet(22,128);juce::MidiBuffer events;double delay=0,reverb=0;
            for(int block=0;block<1100;++block){fxProcessor.processBlock(wet,events);for(int n=0;n<128;++n){delay+=std::abs(wet.getSample(16,n))+std::abs(wet.getSample(17,n));reverb+=std::abs(wet.getSample(18,n))+std::abs(wet.getSample(19,n));}if(block%16==0)juce::Thread::sleep(1);}
            require(delay>.01&&reverb>.01,"global FX ON overrides saved scene FX OFF for drum sends");
            fxProcessor.setParameterValue("fxEnabled",0);fxProcessor.processBlock(wet,events);for(int n=0;n<128;++n)require(wet.getSample(16,n)==0&&wet.getSample(17,n)==0&&wet.getSample(18,n)==0&&wet.getSample(19,n)==0,"global FX OFF immediately silences both send returns");fxProcessor.releaseResources();
        }
        set("krellFX",.21f);set("krell_delayMix",.49f);set("arpFX",.42f);set("drumsFX",.63f);set("arp_delayMix",.27f);set("drums_delayMix",.81f);set("drums_order0",4);processor.getStateInformation(state);set("arp_delayMix",0);set("drums_order0",0);processor.setStateInformation(state.getData(),int(state.getSize()));const auto newRacks=processor.readParameters();require(std::abs(newRacks.krellRack.delayMix-.49)<.001&&std::abs(newRacks.arpRack.delayMix-.27)<.001&&std::abs(newRacks.drumsRack.delayMix-.81)<.001&&newRacks.drumsRack.order[0]==4&&std::abs(newRacks.drumsFX-.63)<.001,"Arp/Drums FX independent host-state round trip");
        auto stored=jerzy::composer::fromTree(jerzy::composer::toTree([&]{auto d=processor.composerDocument();d.scenes[0].patch=newRacks;return d;}()));require(std::abs(stored.scenes[0].patch.krellRack.delayMix-.49)<.001&&std::abs(stored.scenes[0].patch.arpRack.delayMix-.27)<.001&&stored.scenes[0].patch.drumsRack.order[0]==4,"new FX racks survive Composer project round trip");
        set("model",3);tab("BASS");editor->resized();snapshot("BASS-GUITAR");tab("KRELL");snapshot("KRELL-final");require(processor.readParameters().model==3,"Bass Guitar model selected");
        processor.applyPreset(9);require(processor.readParameters().model==3,"Bass Guitar factory preset");for(int kind=10;kind<=13;++kind){processor.applyPreset(kind);require(processor.readParameters().model==3&&processor.readParameters().style==kind,"Rock Metal Punk Blues presets select guitar and style");}processor.applyPreset(14);require(processor.readParameters().krell&&!processor.readParameters().bass&&!processor.readParameters().pad,"independent Krell audition preset");
        tab("COMPOSER");viewport->setViewPosition(0,1200);snapshot("AUDIO-EXPORT");
        // One held root must drive repeated bass phrases, with shared harmony enabled.
        {BassAmbientProcessor rootProcessor;Host rootHost;rootHost.bpm=240;rootProcessor.setPlayHead(&rootHost);
            for(int style=10;style<=13;++style){
                for(const auto& item:std::initializer_list<std::pair<const char*,float>>{{"composerEnabled",0},{"midiPlayback",0},{"direct",0},{"autoPlay",0},{"harmonyLink",1},{"harmonyRoot",36},{"clockSource",1},{"bass",1},{"pad",0},{"arp",0},{"drums",0},{"krell",0},{"fxEnabled",0},{"model",3},{"style",float(style)},{"density",1},{"division",2},{"bars",1},{"meter",1},{"numerator",4},{"denominator",4}})rootProcessor.setParameterValue(item.first,item.second);
                rootProcessor.prepareToPlay(48000,128);rootHost.ppq=0;rootHost.playing=true;juce::AudioBuffer<float> rootAudio(2,128);juce::MidiBuffer rootMidi;
                rootMidi.addEvent(juce::MidiMessage::noteOn(1,48,1.0f),0);std::array<double,3> barEnergy{};
                for(int block=0;block<1125;++block){rootProcessor.processBlock(rootAudio,rootMidi);const int barIndex=std::min(2,int(rootHost.ppq/4));for(int n=0;n<128;++n)barEnergy[size_t(barIndex)]+=std::abs(rootAudio.getSample(0,n));rootHost.ppq+=128.0/12000;if(block%16==0)juce::Thread::sleep(1);}
                for(auto value:barEnergy)require(value>.01,"LINK ROOT held MIDI note repeats Rock/Metal/Punk/Blues beyond 16 steps");
                require(rootProcessor.rootDisplay.load()==48,"linked root stays held through background Composer rebuilds");
                rootMidi.addEvent(juce::MidiMessage::noteOff(1,48),0);rootProcessor.processBlock(rootAudio,rootMidi);
                require(rootProcessor.rootDisplay.load()==-1,"linked MIDI root releases on real note-off");
                rootProcessor.releaseResources();
            }
        }
        checkAudioRendering();
        std::cout<<"PASS MIDI, three-rack routing state, legacy migration, GUI attachments, reorder PRO MIDI mappings, 64-step state, right-click and 24 page/rack snapshots\n";
        return 0;
    } catch(const std::exception& e) { std::cerr<<"FAIL: "<<e.what()<<"\n";return 1; }
}
