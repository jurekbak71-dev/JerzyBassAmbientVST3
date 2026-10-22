#pragma once
#include "PluginProcessor.h"
#include "VisualPalette.h"

class LiveView final : public juce::Component,private juce::Timer {
public:
    explicit LiveView(BassAmbientProcessor& p):processor(p) {
        setComponentID("live-view");
        for(int i=0;i<8;++i){auto& pad=pads[size_t(i)];pad.setComponentID("live-scene-"+juce::String(i));pad.onClick=[this,i]{processor.launchLiveScene(bank.getSelectedId()*8-8+i);};addAndMakeVisible(pad);}
        bank.addItemList({"SCENY 1–8"_ui,"SCENY 9–16"_ui,"SCENY 17–24"_ui,"SCENY 25–32"_ui},1);bank.setSelectedId(1);bank.onChange=[this]{refresh();};addAndMakeVisible(bank);
        snapshot.setComponentID("live-snapshot");snapshot.onClick=[this]{processor.captureLiveSnapshot();refresh();};
        back.setComponentID("live-return");back.onClick=[this]{processor.returnLiveSnapshot();refresh();};
        file.setComponentID("live-file");file.onClick=[this]{chooseFile();};
        record.setComponentID("live-record");record.onClick=[this]{if(processor.liveRecorder.active)processor.stopLiveRecording();else if(processor.liveRecordingPath().isEmpty())chooseFile();else processor.startLiveRecording();refresh();};
        for(auto* button:{&snapshot,&back,&file,&record})addAndMakeVisible(*button);
        for(auto* label:{&now,&next,&recordStatus,&help}){addAndMakeVisible(*label);label->setColour(juce::Label::textColourId,juce::Colour(0xffdce8ef));}
        now.setFont(20);next.setFont(13);recordStatus.setFont(13);
        help.setText("Pady MIDI: CC35–42 · następna/poprzednia: CC43/44 · snapshot/return: CC45/46 · stop/play: CC47/48 · REC: CC49\nWybierz plik przed REC. Nagrywanie trwa po zamknięciu GUI. Zatrzymaj REC, aby sfinalizować WAV."_ui,juce::dontSendNotification);help.setFont(11);
        for(int i=0;i<8;++i){auto& strip=strips[size_t(i)];strip.name.setText(jerzy::composer::trackNames[i],juce::dontSendNotification);strip.name.setJustificationType(juce::Justification::centred);strip.name.setColour(juce::Label::textColourId,ProVisual::tracks[size_t(i)]);addAndMakeVisible(strip.name);
            strip.level.setComponentID("live-gain-"+juce::String(i));strip.level.setSliderStyle(juce::Slider::LinearVertical);strip.level.setTextBoxStyle(juce::Slider::TextBoxBelow,false,74,22);strip.level.setTooltip("Poziom ścieżki — wspólny z mikserem"_ui);addAndMakeVisible(strip.level);strip.mute.setButtonText("MUTE");addAndMakeVisible(strip.mute);ProVisual::tint(strip.mute,ProVisual::tracks[size_t(i)]);
            strip.levelAttachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.state,"mix"+juce::String(i)+"Gain",strip.level);strip.level.textFromValueFunction=[](double gain){return gain<=0?juce::String("-inf dB"):juce::String(juce::Decibels::gainToDecibels(gain),1)+" dB";};strip.level.valueFromTextFunction=[](const juce::String& text){return text.containsIgnoreCase("inf")?0.:juce::Decibels::decibelsToGain(text.getDoubleValue());};strip.level.updateText();strip.muteAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.state,"mix"+juce::String(i)+"Mute",strip.mute);
        }
        startTimerHz(20);refresh();
    }
    int requiredHeight() const {return 510;}
    void resized() override {
        const int w=getWidth(),side=w-310,padWidth=(side-26)/4;
        now.setBounds(12,0,side-20,32);next.setBounds(12,34,side-20,28);
        for(int i=0;i<8;++i)pads[size_t(i)].setBounds(12+(i%4)*padWidth,76+(i/4)*87,padWidth-8,78);
        bank.setBounds(12,258,230,30);help.setBounds(12,294,w-30,42);
        snapshot.setBounds(side+8,8,280,38);back.setBounds(side+8,54,280,38);file.setBounds(side+8,112,280,34);record.setBounds(side+8,154,280,48);recordStatus.setBounds(side+8,210,280,78);
        const int cw=(w-24)/8;for(int i=0;i<8;++i){auto& strip=strips[size_t(i)];int x=12+i*cw;strip.name.setBounds(x,344,cw-8,22);strip.level.setBounds(x+5,370,cw-20,96);strip.mute.setBounds(x+4,472,cw-16,30);}
    }
    void paint(juce::Graphics& g) override {g.fillAll(juce::Colour(0xff111923));g.setColour(juce::Colour(0xff223442));g.fillRoundedRectangle(float(getWidth()-302),0,294,288,7);}
private:
    struct Strip {juce::Label name;juce::Slider level;juce::ToggleButton mute;std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> levelAttachment;std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> muteAttachment;};
    BassAmbientProcessor& processor;
    jerzy::composer::Document cachedDocument;uint64_t cachedRevision=0;
    std::array<juce::TextButton,8> pads;
    std::array<Strip,8> strips;
    juce::ComboBox bank;
    juce::TextButton snapshot{"SNAPSHOT"},back{"RETURN"},file{"WYBIERZ WAV"},record{"REC LIVE"};
    juce::Label now,next,recordStatus,help;
    std::unique_ptr<juce::FileChooser> chooser;
    void chooseFile(){chooser=std::make_unique<juce::FileChooser>("Nagranie koncertu — nowy plik WAV"_ui,juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getNonexistentChildFile("PRO-Live",".wav"),"*.wav");juce::Component::SafePointer<LiveView> safe(this);chooser->launchAsync(juce::FileBrowserComponent::saveMode|juce::FileBrowserComponent::canSelectFiles,[safe](const auto& choice){if(safe&&choice.getResult()!=juce::File{}){safe->processor.armLiveRecording(choice.getResult());safe->refresh();}});}
    void timerCallback() override {refresh();}
    void refresh(){const auto revision=processor.composerRevision.load();if(revision!=cachedRevision){cachedDocument=processor.composerDocument();cachedRevision=revision;}const auto& d=cachedDocument;const auto p=processor.readParameters();juce::String name=p.autoPlay?"JAM":"STOP";for(const auto& scene:d.scenes)if(scene.id==processor.composerSceneDisplay&&p.composerPlaying)name=juce::String(scene.name);
        now.setText(processor.clockStatus>=2?"CZEKAM NA ZEGAR":"TERAZ: "+name,juce::dontSendNotification);int queued=processor.liveQueuedScene.load();const double bar=std::max(1.,processor.clockBarBeats.load());next.setText(queued>=0&&queued<int(d.scenes.size())?"NASTĘPNA: "_ui+juce::String(d.scenes[size_t(queued)].name)+" · granica taktu"_ui:"TAKT: "+juce::String(int(processor.beatDisplay.load()/bar)+1)+"  |  "+juce::String(processor.tempoDisplay.load(),1)+" BPM",juce::dontSendNotification);
        for(int i=0;i<8;++i){const int index=(bank.getSelectedId()-1)*8+i;auto& pad=pads[size_t(i)];pad.setEnabled(index<int(d.scenes.size()));pad.setButtonText(index<int(d.scenes.size())?juce::String(index+1)+"  "+juce::String(d.scenes[size_t(index)].name):"—"_ui);pad.setToggleState(index<int(d.scenes.size())&&d.scenes[size_t(index)].id==processor.composerSceneDisplay&&p.composerPlaying,juce::dontSendNotification);pad.setColour(juce::TextButton::textColourOffId,index==queued?juce::Colour(0xffffd064):juce::Colour(0xffdce8ef));}
        back.setEnabled(processor.liveSnapshotReady);const bool active=processor.liveRecorder.active;record.setButtonText(active?"■ STOP REC"_ui:"● REC LIVE"_ui);record.setColour(juce::TextButton::textColourOffId,active?juce::Colour(0xffff708f):juce::Colour(0xffdce8ef));file.setEnabled(!active);
        const double seconds=processor.liveRecorder.frames.load()/processor.liveRecorder.sampleRate.load();recordStatus.setText((active?"REC ":"CZAS ")+juce::String(int(seconds)/60)+":"+juce::String(int(seconds)%60).paddedLeft('0',2)+"\n"+processor.liveMessage()+(p.separateOutputs?"\nREC: tylko MAIN — SPLIT aktywny"_ui:""),juce::dontSendNotification);
    }
};
