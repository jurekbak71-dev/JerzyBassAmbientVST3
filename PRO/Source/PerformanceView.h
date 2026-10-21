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
        const bool states[]{in,active,out>.00003f,out>=1};const juce::Colour colours[]{juce::Colour(0xffffd064),juce::Colour(0xff72ee84),juce::Colour(0xff63dbd0),juce::Colour(0xffff534f)};const char* names[]{"IN","ACT","OUT","CLP"};
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
    void timerCallback() override {const auto p=processor.readParameters();tempo.setEnabled(p.clockSource==0);tap.setEnabled(p.clockSource==0);const int sync=processor.clockStatus.load();const char* names[]{"INTERNAL","SYNC","WAIT","LOST"};const double beat=processor.beatDisplay.load();status.setText(juce::String(names[juce::jlimit(0,3,sync)])+"   "+juce::String(processor.tempoDisplay.load(),1)+" BPM   |   "+juce::String(int(beat/std::max(.25,processor.clockBarBeats.load()))+1)+":"+juce::String(1+std::fmod(beat,std::max(.25,processor.clockBarBeats.load())),1),juce::dontSendNotification);status.setColour(juce::Label::textColourId,sync<2?juce::Colour(0xff61e0c1):juce::Colour(0xffffd064));play.setToggleState(p.composerPlaying||p.autoPlay,juce::dontSendNotification);repaint();}
    void paint(juce::Graphics& g) override {const bool pulse=processor.clockStatus.load()<2&&std::fmod(processor.beatDisplay.load(),1.0)<.16;g.setColour(juce::Colour(0xff61e0c1).withAlpha(pulse?1.0f:.15f));g.fillEllipse(float(getWidth()-12),12,8,8);}
    void resized() override {clock.setBounds(0,2,170,30);tempo.setBounds(178,2,110,30);play.setBounds(296,2,66,30);stop.setBounds(368,2,66,30);tap.setBounds(440,2,58,30);status.setBounds(508,2,getWidth()-508,30);}
};

class MixerView final : public juce::Component,private juce::Timer {
    using SA=juce::AudioProcessorValueTreeState::SliderAttachment;
    using BA=juce::AudioProcessorValueTreeState::ButtonAttachment;
    BassAmbientProcessor& processor;
    struct Strip {
        juce::Label title;juce::Slider fader,pan,delay,reverb;juce::ToggleButton mute{"M"},solo{"S"},power{"POWER"};
        std::unique_ptr<SignalLeds> leds;
    };
    std::array<Strip,7> strips;juce::Slider master,delayReturn,reverbReturn,feedback,room;
    juce::ToggleButton drumsPower{"DRUMS"},limiter{"LIMITER"},split{"SPLIT OUT"},fx{"FX POWER"},delayPower{"DELAY"},reverbPower{"REVERB"},sceneRecall{"SCENE MIX"};
    juce::ComboBox division;
    std::vector<std::unique_ptr<SA>> sliders;std::vector<std::unique_ptr<BA>> buttons;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> choice;
    void bind(juce::Slider& slider,const juce::String& id,bool vertical=false){addAndMakeVisible(slider);slider.setComponentID(id);slider.setSliderStyle(vertical?juce::Slider::LinearVertical:juce::Slider::LinearHorizontal);slider.setTextBoxStyle(juce::Slider::TextBoxBelow,false,64,22);auto* p=processor.state.getParameter(id);slider.setDoubleClickReturnValue(true,p->convertFrom0to1(p->getDefaultValue()));slider.setTooltip(p->getName(80)+" | Right-click: default / Ctrl+right: MIDI Learn");slider.addMouseListener(this,false);sliders.push_back(std::make_unique<SA>(processor.state,id,slider));if(vertical){slider.textFromValueFunction=[](double value){return value<.00001?juce::String("-inf dB"):juce::String(juce::Decibels::gainToDecibels(value),1)+" dB";};slider.valueFromTextFunction=[](const juce::String& value){return value.containsIgnoreCase("inf")?0.0:juce::Decibels::decibelsToGain(value.getDoubleValue());};slider.updateText();}else{slider.textFromValueFunction=[](double value){return juce::String(value,2);};slider.valueFromTextFunction=[](const juce::String& value){return value.getDoubleValue();};slider.updateText();}}
    void bind(juce::ToggleButton& button,const juce::String& id){addAndMakeVisible(button);button.setComponentID(id);button.addMouseListener(this,false);buttons.push_back(std::make_unique<BA>(processor.state,id,button));}
public:
    explicit MixerView(BassAmbientProcessor& p):processor(p){
        for(int t=0;t<7;++t){auto& s=strips[size_t(t)];const auto prefix="mix"+juce::String(t);s.title.setText(jerzy::composer::trackNames[t],juce::dontSendNotification);s.title.setJustificationType(juce::Justification::centred);addAndMakeVisible(s.title);ProVisual::tint(s.title,ProVisual::tracks[size_t(t)]);
            bind(s.fader,prefix+"Gain",true);bind(s.pan,prefix+"Pan");bind(s.delay,prefix+"Delay");bind(s.reverb,prefix+"Reverb");bind(s.mute,prefix+"Mute");bind(s.solo,prefix+"Solo");bind(s.power,t==0?"bass":t==1?"pad":t==2?"arp":"drum"+juce::String(t-3)+"_enabled");s.leds=std::make_unique<SignalLeds>(p,t,t);addAndMakeVisible(*s.leds);}
        bind(master,"master",true);bind(delayReturn,"sendDelayReturn");bind(reverbReturn,"sendReverbReturn");bind(feedback,"sendFeedback");bind(room,"sendRoom");bind(drumsPower,"drums");bind(limiter,"limiter");bind(split,"separateOutputs");bind(fx,"fxEnabled");bind(delayPower,"sendDelayPower");bind(reverbPower,"sendReverbPower");bind(sceneRecall,"sceneMixer");division.addItemList({"1/4","1/8","1/16","1/32","1/8T","1/16T","1/16D"},1);addAndMakeVisible(division);choice=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.state,"sendDivision",division);startTimerHz(25);
    }
    void mouseDown(const juce::MouseEvent& e) override {if(!e.mods.isRightButtonDown())return;const auto id=e.eventComponent->getComponentID();if(id.isEmpty())return;if(e.mods.isCtrlDown())processor.learnMidi(id);else if(auto* p=processor.state.getParameter(id))p->setValueNotifyingHost(p->getDefaultValue());}
    int requiredHeight() const {return 700;}
    void timerCallback() override {for(auto& s:strips)s.leds->repaint();repaint();}
    void resized() override {const int cw=(getWidth()-24)/8;
        for(int t=0;t<7;++t){auto& s=strips[size_t(t)];int x=12+t*cw;s.title.setBounds(x,10,cw-6,26);s.leds->setBounds(x,40,cw-6,20);s.power.setBounds(x,66,cw-6,26);s.mute.setBounds(x,98,(cw-8)/2,26);s.solo.setBounds(x+cw/2,98,(cw-8)/2,26);s.pan.setBounds(x+6,148,cw-18,48);s.fader.setBounds(x+12,206,cw-44,230);s.delay.setBounds(x+6,466,cw-18,50);s.reverb.setBounds(x+6,548,cw-18,50);}
        const int x=12+7*cw;master.setBounds(x+4,90,cw-24,340);limiter.setBounds(x,440,cw-6,28);split.setBounds(x,476,cw-6,28);fx.setBounds(x,512,cw-6,28);drumsPower.setBounds(x,548,cw-6,28);
        const int rw=(getWidth()-32)/8;delayPower.setBounds(16,620,rw-6,28);delayReturn.setBounds(16+rw,616,rw-6,48);feedback.setBounds(16+2*rw,616,rw-6,48);division.setBounds(16+3*rw,620,rw-6,28);reverbPower.setBounds(16+4*rw,620,rw-6,28);reverbReturn.setBounds(16+5*rw,616,rw-6,48);room.setBounds(16+6*rw,616,rw-6,48);sceneRecall.setBounds(getWidth()-150,668,138,26);
    }
    void paint(juce::Graphics& g) override {g.fillAll(juce::Colour(0xff111720));const int cw=(getWidth()-24)/8;g.setFont(11);
        auto meter=[&](int channel,int x,int y,int h){const float value=channel<9?processor.channelPeak[size_t(channel)].load():processor.peak.load();const float db=juce::jlimit(0.0f,1.0f,(juce::Decibels::gainToDecibels(std::max(value,.000001f))+60)/60);g.setColour(juce::Colour(0xff26303c));g.fillRect(x,y,10,h);g.setColour(value>=1?juce::Colour(0xffff534f):juce::Colour(0xff61e0c1));g.fillRect(x,y+h-int(db*h),10,int(db*h));};
        for(int t=0;t<7;++t){int x=12+t*cw;g.setColour(ProVisual::tracks[size_t(t)].withAlpha(.6f));g.drawRoundedRectangle(float(x),4,float(cw-6),596,6,1);g.setColour(ProVisual::tracks[size_t(t)]);g.drawText("PAN",x,128,cw-6,18,juce::Justification::centred);g.drawText("DELAY SEND",x,444,cw-6,18,juce::Justification::centred);g.drawText("REVERB SEND",x,526,cw-6,18,juce::Justification::centred);meter(t,x+cw-26,212,194);}
        const int x=12+7*cw;g.setColour(juce::Colours::white);g.drawText("MASTER",x,10,cw-6,26,juce::Justification::centred);meter(9,x+cw-20,96,300);meter(7,132,620,28);meter(8,720,620,28);g.setColour(juce::Colour(0xffa3afbf));g.drawText("Delay return       Feedback                         Reverb return       Size   |   Sends: post fader / post mute",16,684,getWidth()-190,16,juce::Justification::centredLeft);
    }
};
