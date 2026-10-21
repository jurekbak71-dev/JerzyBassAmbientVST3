#pragma once
#include <JuceHeader.h>
#include "Core.h"
#include "ComposerIO.h"
#include <thread>

struct ParameterSpec {
    juce::String id,label; int group=0; int kind=0;
    double minimum=0,maximum=1,initial=0,skew=1;
    juce::StringArray choices;
};
std::vector<ParameterSpec> parameterSpecs();

class BassAmbientProcessor final : public juce::AudioProcessor {
public:
    BassAmbientProcessor();
    ~BassAmbientProcessor() override;
    void prepareToPlay(double,int) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&,juce::MidiBuffer&) override;
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 45; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int,const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*,int) override;
    jerzy::Parameters readParameters() const;
    juce::AudioProcessorValueTreeState state;
    std::atomic<int> rootDisplay{-1};
    std::atomic<float> peak{0};
    std::atomic<int> drumStep{-1},arpNote{-1};
    std::atomic<size_t> fxBytes{0};
    std::atomic<int> learnCC{-1};
    void learnMidi(const juce::String& id);
    bool isLearningMidi() const {return learnParameter.load()>=0;}
    void clearMidiMapping(const juce::String& id);
    juce::String midiAssignment(const juce::String& id) const;
    void requestPanic() {panicRequested=true;}
    void setParameterValue(const juce::String& id,float value);

    void applyPreset(int preset);
    jerzy::composer::Document composerDocument() const;
    void editComposer(const std::function<void(jerzy::composer::Document&)>& edit);
    juce::String composerError() const;
    juce::Result exportComposer(const juce::File&,bool song,int scene,int track=-1) const;
    juce::Result saveComposer(const juce::File&) const;
    juce::Result loadComposer(const juce::File&);
    void launchComposer(int scene,bool song);
    std::atomic<int> composerSceneDisplay{-1},composerSlotDisplay{-1};
    std::atomic<double> composerBeatDisplay{0};
    std::atomic<uint64_t> composerRevision{1};

private:
    mutable juce::CriticalSection composerLock;
    jerzy::composer::Document composerData=jerzy::composer::factoryDocument();
    juce::String composerFailure;
    std::atomic<jerzy::composer::Compiled*> pendingComposer{nullptr},retiredComposer{nullptr};
    jerzy::composer::Compiled* activeComposer=nullptr;
    uint64_t compiledRevision=0;
    const jerzy::composer::Timeline* playback=nullptr;
    size_t eventCursor=0;int sectionIndex=-1,playingScene=-1,lastResourceSection=-2;
    double composerOrigin=0,previousComposerBeat=-1,lastComposerPPQ=-1;
    bool wasComposerPlaying=false,wasSongMode=false;
    jerzy::Parameters sectionPatch;
    std::array<double,7> tailUntil{};
    std::array<bool,7> previousTracks{};
    void serviceComposer();
    void adoptComposer();
    jerzy::Parameters composerSample(const jerzy::Parameters&,double,double,bool,int,int);
    void routePartMidi(const juce::MidiMessage&,const jerzy::Parameters&);
    static BusesProperties buses();
    static juce::AudioProcessorValueTreeState::ParameterLayout layout();
    std::vector<std::atomic<float>*> values;
    jerzy::Instrument instrument;
    jerzy::Parameters current;
    double sr=48000, localPPQ=0;
    std::atomic<bool> resetRequested{false},panicRequested{false},stopWorker{true};
    std::thread resourceWorker;
    std::vector<juce::RangedAudioParameter*> hostParameters;
    juce::RangedAudioParameter* generateParameter=nullptr;
    std::array<std::atomic<int>,128> ccMap{};
    std::atomic<int> learnParameter{-1};
    void stopResourceWorker();
    void handleMappedController(int,int,jerzy::Parameters&);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BassAmbientProcessor)
};
