#pragma once
#include "PluginProcessor.h"
#include "VisualPalette.h"

class SignalLeds final : public juce::Component {
    BassAmbientProcessor& processor;int first,last;
public:
    SignalLeds(BassAmbientProcessor& p,int a,int b):processor(p),first(a),last(b){setInterceptsMouseClicks(false,false);}
    void paint(juce::Graphics& g) override {
        bool in=false,active=false;float out=0;
        for(int t=first;t<=last;++t){in|=processor.inputPulse[size_t(t)].load()>0;active|=processor.activePulse[size_t(t)].load()>0;out=std::max(out,processor.channelPeak[size_t(t)].load());}
        if(getWidth()<110){const char* labels[]{"IN","OUT"};for(int i=0;i<2;++i){float x=float(i)*getWidth()/2+3;g.setColour((i==0?juce::Colour(0xffffd064):out>=1?juce::Colours::red:juce::Colour(0xff63dbd0)).withAlpha((i==0?in:out>.00003f)?1.f:.18f));g.fillEllipse(x,5,5,5);g.setColour(juce::Colour(0xff9aabba));g.setFont(8);g.drawText(labels[i],int(x+7),0,getWidth()/2-9,16,juce::Justification::centredLeft);}return;}
        const bool states[]{in,active,out>.00003f,out>=1};const juce::Colour colours[]{juce::Colour(0xffffd064),juce::Colour(0xff72ee84),juce::Colour(0xff63dbd0),juce::Colour(0xffff534f)};const juce::String names[]{"IN","ACT","OUT","CLP"};
        const float width=float(getWidth())/4;
        for(int i=0;i<4;++i){const float x=i*width+6;g.setColour(colours[i].withAlpha(states[i]?1.0f:.15f));g.fillEllipse(x,5,6,6);g.setColour(juce::Colour(0xffb9c5d2));g.setFont(9);g.drawText(names[i],int(x+9),0,int(width-13),16,juce::Justification::centredLeft);}
    }
};

class TransportView final : public juce::Component,private juce::Timer {
    BassAmbientProcessor& processor;juce::ComboBox clock;juce::Slider tempo;juce::TextButton play{"PLAY"},stop{"STOP"},tap{"TAP"};juce::Label status;
    juce::AudioProcessorValueTreeState::ComboBoxAttachment clockAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment tempoAttachment;
    double lastTap=0;std::vector<double> intervals;
public:
    explicit TransportView(BassAmbientProcessor& p):processor(p),clockAttachment(p.state,"clockSource",clock),tempoAttachment(p.state,"internalTempo",tempo){
        clock.addItemList({"INTERNAL","HOST / FL STUDIO","MIDI CLOCK"},1);clock.setSelectedId(int(p.readParameters().clockSource)+1,juce::dontSendNotification);
        tempo.setRange(30,240,1);tempo.setSliderStyle(juce::Slider::IncDecButtons);tempo.setTextBoxStyle(juce::Slider::TextBoxLeft,false,62,28);tempo.setTooltip("Internal tempo BPM. HOST / MIDI show the received BPM beside this field.");tempo.setDoubleClickReturnValue(true,120);tempo.textFromValueFunction=[](double value){return juce::String(value,1);};tempo.valueFromTextFunction=[](const juce::String& text){return text.getDoubleValue();};tempo.updateText();
        for(auto* c:std::initializer_list<juce::Component*>{&clock,&tempo,&play,&stop,&tap,&status})addAndMakeVisible(*c);
        play.onClick=[this]{processor.transport(true);};stop.onClick=[this]{processor.transport(false);};
        tap.onClick=[this]{const double now=juce::Time::getMillisecondCounterHiRes();if(lastTap>0&&now-lastTap<2000&&now-lastTap>150){intervals.push_back(now-lastTap);if(intervals.size()>4)intervals.erase(intervals.begin());double sum=0;for(auto value:intervals)sum+=value;processor.setParameterValue("internalTempo",float(juce::jlimit(30.0,240.0,60000*intervals.size()/sum)));}else intervals.clear();lastTap=now;};timerCallback();startTimerHz(15);
    }
    void timerCallback() override {const auto p=processor.readParameters();tempo.setEnabled(p.clockSource==0);if(p.clockSource!=0)tempo.setValue(processor.tempoDisplay.load(),juce::dontSendNotification);tap.setEnabled(p.clockSource==0);const int sync=processor.clockStatus.load();const juce::String names[]{"INTERNAL","SYNC","WAIT","LOST"};const double beat=processor.beatDisplay.load();status.setText(juce::String(p.clockSource==0?"BPM":sync>=2?"FL: STOP / CLOCK?":"SYNC"),juce::dontSendNotification);status.setColour(juce::Label::textColourId,sync<2?juce::Colour(0xff61e0c1):juce::Colour(0xffffd064));play.setToggleState(p.composerPlaying||p.autoPlay,juce::dontSendNotification);repaint();}
    void paint(juce::Graphics& g) override {const bool pulse=processor.clockStatus.load()<2&&std::fmod(processor.beatDisplay.load(),1.0)<.16;g.setColour(juce::Colour(0xff61e0c1).withAlpha(pulse?1.0f:.15f));g.fillEllipse(float(getWidth()-12),12,8,8);}
    void resized() override {clock.setVisible(false);tap.setVisible(false);tempo.setBounds(0,2,86,30);play.setBounds(92,2,62,30);stop.setBounds(160,2,50,30);status.setBounds(216,2,getWidth()-228,30);}
};

class HarmonyView final : public juce::Component {
    BassAmbientProcessor& processor;
    juce::Slider root;juce::ComboBox scale;juce::ToggleButton link{"WSPÓLNY ROOT"_ui};juce::Label label,info;
    juce::AudioProcessorValueTreeState::SliderAttachment rootAttachment;
    juce::AudioProcessorValueTreeState::ComboBoxAttachment scaleAttachment;
    juce::AudioProcessorValueTreeState::ButtonAttachment linkAttachment;
public:
    explicit HarmonyView(BassAmbientProcessor& p):processor(p),rootAttachment(p.state,"harmonyRoot",root),scaleAttachment(p.state,"scale",scale),linkAttachment(p.state,"harmonyLink",link){
        label.setText("ROOT / SKALA",juce::dontSendNotification);info.setText("LINK: generated parts follow Bass roots; MANUAL notes stay exact",juce::dontSendNotification);
        root.setSliderStyle(juce::Slider::IncDecButtons);root.setTextBoxStyle(juce::Slider::TextBoxLeft,false,66,26);root.textFromValueFunction=[](double v){return juce::MidiMessage::getMidiNoteName(int(v),true,true,4);};root.valueFromTextFunction=[](const juce::String& text){for(int n=12;n<=96;++n)if(juce::MidiMessage::getMidiNoteName(n,true,true,4).equalsIgnoreCase(text.trim()))return double(n);return 36.0;};root.updateText();root.setTooltip("Shared root MIDI note. C2 = MIDI 36. LINK applies it to generators, not MANUAL notes.");
        scale.addItemList({"Major","Minor","Dorian","Harmonic minor","Minor pentatonic"},1);scale.setSelectedId(p.readParameters().scale+1,juce::dontSendNotification);
        for(auto* c:std::initializer_list<juce::Component*>{&root,&scale,&link,&label,&info})addAndMakeVisible(*c);
    }
    void refresh(){auto p=processor.readParameters();bool arrange=p.composerEnabled;bool jam=p.performanceSource==2||p.autoPlay;root.setEnabled(jam);root.setVisible(!arrange);label.setText(arrange?"AKORDY / SKALA":"ROOT / SKALA",juce::dontSendNotification);info.setVisible(arrange);info.setText("Ze sceny",juce::dontSendNotification);info.setBounds(94,0,90,28);if(!jam&&processor.rootDisplay>=0)root.setValue(processor.rootDisplay.load(),juce::dontSendNotification);root.setTooltip(jam?"Wspólny root dla JAM. Rejestr pozostaje w ustawieniach instrumentów."_ui:"Root sterowany nutami MIDI 1; w aranżerze — progresją sceny."_ui);}
    void resized() override {label.setBounds(0,0,90,28);root.setBounds(94,0,90,28);scale.setBounds(192,0,130,28);link.setBounds(330,0,135,28);info.setVisible(false);}
};

class MixerView final : public juce::Component,private juce::Timer {
    using SA=juce::AudioProcessorValueTreeState::SliderAttachment;
    using BA=juce::AudioProcessorValueTreeState::ButtonAttachment;
    BassAmbientProcessor& processor;
    struct Strip {
        juce::Label title;juce::Slider fader,pan,delay,reverb;juce::ToggleButton mute{"M"},solo{"S"},power{"POWER"};
        std::unique_ptr<SignalLeds> leds;
    };
    std::array<Strip,8> strips;juce::Slider master,delayReturn,reverbReturn,feedback,room;
    juce::ToggleButton drumsPower{"DRUMS"},limiter{"LIMITER"},split{"SPLIT OUT"},fx{"FX POWER"},delayPower{"DELAY"},reverbPower{"REVERB"},sceneRecall{"SCENE MIX"};
    juce::ComboBox division;
    std::vector<std::unique_ptr<SA>> sliders;std::vector<std::unique_ptr<BA>> buttons;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> choice;
    void bind(juce::Slider& slider,const juce::String& id,bool vertical=false){addAndMakeVisible(slider);slider.setComponentID(id);slider.setSliderStyle(vertical?juce::Slider::LinearVertical:juce::Slider::LinearHorizontal);slider.setTextBoxStyle(juce::Slider::TextBoxBelow,false,64,22);auto* p=processor.state.getParameter(id);slider.setDoubleClickReturnValue(true,p->convertFrom0to1(p->getDefaultValue()));slider.setTooltip(p->getName(80)+" | Right-click: default / Ctrl+right: MIDI Learn");slider.addMouseListener(this,false);sliders.push_back(std::make_unique<SA>(processor.state,id,slider));if(vertical){slider.textFromValueFunction=[](double value){return value<.00001?juce::String("-inf dB"):juce::String(juce::Decibels::gainToDecibels(value),1)+" dB";};slider.valueFromTextFunction=[](const juce::String& value){return value.containsIgnoreCase("inf")?0.0:juce::Decibels::decibelsToGain(value.getDoubleValue());};slider.updateText();}else{slider.textFromValueFunction=[](double value){return juce::String(value,2);};slider.valueFromTextFunction=[](const juce::String& value){return value.getDoubleValue();};slider.updateText();}}
    void bind(juce::ToggleButton& button,const juce::String& id){addAndMakeVisible(button);button.setComponentID(id);button.addMouseListener(this,false);buttons.push_back(std::make_unique<BA>(processor.state,id,button));}
public:
    explicit MixerView(BassAmbientProcessor& p):processor(p){
        for(int t=0;t<8;++t){auto& s=strips[size_t(t)];const auto prefix="mix"+juce::String(t);s.title.setText(jerzy::composer::trackNames[t],juce::dontSendNotification);s.title.setJustificationType(juce::Justification::centred);addAndMakeVisible(s.title);ProVisual::tint(s.title,ProVisual::tracks[size_t(t)]);
            bind(s.fader,prefix+"Gain",true);bind(s.pan,prefix+"Pan");bind(s.delay,prefix+"Delay");bind(s.reverb,prefix+"Reverb");bind(s.mute,prefix+"Mute");bind(s.solo,prefix+"Solo");bind(s.power,t==0?"bass":t==1?"pad":t==2?"arp":t==7?"krell":"drum"+juce::String(t-3)+"_enabled");s.leds=std::make_unique<SignalLeds>(p,t,t);addAndMakeVisible(*s.leds);}
        bind(master,"master",true);bind(delayReturn,"sendDelayReturn");bind(reverbReturn,"sendReverbReturn");bind(feedback,"sendFeedback");bind(room,"sendRoom");bind(drumsPower,"drums");bind(limiter,"limiter");bind(split,"separateOutputs");bind(fx,"fxEnabled");bind(delayPower,"sendDelayPower");bind(reverbPower,"sendReverbPower");bind(sceneRecall,"sceneMixer");division.addItemList({"1/4","1/8","1/16","1/32","1/8T","1/16T","1/16D"},1);addAndMakeVisible(division);choice=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.state,"sendDivision",division);startTimerHz(25);
    }
    void mouseDown(const juce::MouseEvent& e) override {if(!e.mods.isRightButtonDown())return;const auto id=e.eventComponent->getComponentID();if(id.isEmpty())return;if(e.mods.isCtrlDown())processor.learnMidi(id);else if(auto* p=processor.state.getParameter(id))p->setValueNotifyingHost(p->getDefaultValue());}
    int requiredHeight() const {return 520;}
    void timerCallback() override {for(auto& s:strips)s.leds->repaint();repaint();}
    void resized() override {const int cw=(getWidth()-24)/9;
        for(int t=0;t<8;++t){auto& s=strips[size_t(t)];int x=12+t*cw;s.title.setBounds(x,5,cw-6,24);s.leds->setBounds(x,30,cw-6,18);s.power.setVisible(false);s.mute.setBounds(x,52,(cw-8)/2,25);s.solo.setBounds(x+cw/2,52,(cw-8)/2,25);s.pan.setBounds(x+5,96,cw-16,42);s.fader.setBounds(x+8,145,cw-35,185);s.delay.setBounds(x+5,350,cw-16,42);s.reverb.setBounds(x+5,410,cw-16,42);}
        const int x=12+8*cw;master.setBounds(x+3,52,cw-24,280);limiter.setBounds(x,350,cw-6,28);split.setVisible(false);fx.setVisible(false);drumsPower.setVisible(false);sceneRecall.setVisible(false);
        for(auto* c:std::initializer_list<juce::Component*>{&feedback,&division,&room})c->setVisible(false);delayPower.setBounds(16,470,140,28);delayReturn.setBounds(165,463,170,48);reverbPower.setBounds(410,470,140,28);reverbReturn.setBounds(560,463,170,48);
    }
    void paint(juce::Graphics& g) override {g.fillAll(juce::Colour(0xff111720));const int cw=(getWidth()-24)/9;g.setFont(11);
        auto meter=[&](int channel,int x,int y,int h){const float value=channel<10?processor.channelPeak[size_t(channel)].load():processor.peak.load();const float db=juce::jlimit(0.0f,1.0f,(juce::Decibels::gainToDecibels(std::max(value,.000001f))+60)/60);g.setColour(juce::Colour(0xff26303c));g.fillRect(x,y,10,h);g.setColour(value>=1?juce::Colour(0xffff534f):juce::Colour(0xff61e0c1));g.fillRect(x,y+h-int(db*h),10,int(db*h));};
        for(int t=0;t<8;++t){int x=12+t*cw;g.setColour(ProVisual::tracks[size_t(t)].withAlpha(.6f));g.drawRoundedRectangle(float(x),4,float(cw-6),452,6,1);g.setColour(ProVisual::tracks[size_t(t)]);g.drawText("PAN",x,78,cw-6,18,juce::Justification::centred);g.drawText("DELAY SEND",x,330,cw-6,18,juce::Justification::centred);g.drawText("REVERB SEND",x,390,cw-6,18,juce::Justification::centred);meter(t,x+cw-22,151,145);}
        const int x=12+8*cw;g.setColour(juce::Colours::white);g.drawText("MASTER",x,10,cw-6,26,juce::Justification::centred);meter(10,x+cw-20,58,240);g.setColour(juce::Colour(0xffa3afbf));g.drawText("Sendy: po faderze / Mute. Powroty Delay i Reverb: przycisk POWROTY FX. Routing: USTAWIENIA.",760,474,getWidth()-775,32,juce::Justification::centredLeft);
    }
};
