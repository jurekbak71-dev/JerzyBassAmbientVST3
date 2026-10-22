#include "PluginProcessor.h"

void BassAmbientProcessor::armLiveRecording(const juce::File& file) {
    const juce::ScopedLock lock(liveLock);
    if(!liveRecorder.active) {liveFile=file.withFileExtension("wav");liveStatus="WAV armed: "+liveFile.getFileName();}
}
juce::String BassAmbientProcessor::liveRecordingPath() const {const juce::ScopedLock lock(liveLock);return liveFile.getFullPathName();}
juce::String BassAmbientProcessor::liveMessage() const {const juce::ScopedLock lock(liveLock);return liveStatus;}
juce::Result BassAmbientProcessor::startLiveRecording() {
    const juce::ScopedLock lock(liveLock);
    auto fail=[&](const juce::String& text){liveStatus=text;return juce::Result::fail(text);};
    if(!liveAudioReady)return fail("Audio device is not ready");
    if(liveFile==juce::File{})return fail("Choose WAV file before REC / MIDI CC49");
    if(liveRecorder.active)return fail("Recording already running");
    if(liveFile.exists())liveFile=liveFile.getNonexistentSibling();
    auto result=liveRecorder.start(liveFile,liveAudioRate.load());
    liveStatus=result.wasOk()?"REC: "+liveFile.getFileName():result.getErrorMessage();return result;
}
void BassAmbientProcessor::stopLiveRecording() {
    liveRecorder.stop();
    const juce::ScopedLock lock(liveLock);
    if(liveRecorder.dropped>0)liveStatus="REC ERROR: buffer overflow; recording stopped, WAV kept";
    else if(liveRecorder.limit)liveStatus="WAV 4 GB limit: stopped, WAV kept";
    else if(liveRecorder.frames>0)liveStatus="Saved WAV: "+liveFile.getFileName();
}
void BassAmbientProcessor::captureLiveSnapshot() {
    const juce::ScopedLock lock(liveLock);
    liveSnapshot.clear();
    // Keep the running transport, input routing, clock and selected scene intact.
    const juce::StringArray excluded{"autoPlay","composerEnabled","composerPlaying","composerSongMode","composerLoop","composerScene","clockSource","internalTempo","transportRun","direct","midiPlayback","separateOutputs","meter","numerator","denominator"};
    for(const auto& spec:parameterSpecs())if(spec.group!=26&&!excluded.contains(spec.id))
        liveSnapshot.emplace_back(spec.id,state.getRawParameterValue(spec.id)->load());
    liveSnapshotReady=true;liveStatus="SNAPSHOT: sound, generators and mix saved";
}
bool BassAmbientProcessor::returnLiveSnapshot() {
    const juce::ScopedLock lock(liveLock);
    if(!liveSnapshotReady){liveStatus="RETURN: capture a SNAPSHOT first";return false;}
    for(const auto& value:liveSnapshot)setParameterValue(value.first,value.second);
    liveStatus="RETURN: sound and mix restored; transport continues";return true;
}
void BassAmbientProcessor::launchLiveScene(int scene) {
    const auto data=composerDocument();
    if(scene<0||scene>=int(data.scenes.size()))return;
    const auto controls=readParameters();liveRetrigger=controls.composerEnabled&&controls.composerPlaying&&!controls.composerSongMode&&controls.composerScene==scene+1;
    launchComposer(scene,false);
}
void BassAmbientProcessor::queueLiveController(int cc,int value) {
    if(cc<35||cc>49)return;
    const bool down=value>=64,trigger=down&&!liveCCDown[size_t(cc)];liveCCDown[size_t(cc)]=down;
    if(!trigger)return;
    auto range=liveActionFifo.write(1);
    if(range.blockSize1>0)liveActions[size_t(range.startIndex1)]=cc;
}
void BassAmbientProcessor::serviceLive() {
    if(liveRecorder.needsFinish())stopLiveRecording();
    auto range=liveActionFifo.read(liveActionFifo.getNumReady());
    auto action=[this](int cc){
        if(cc>=35&&cc<=42)launchLiveScene(cc-35);
        else if(cc==43||cc==44){const auto size=int(composerDocument().scenes.size());launchLiveScene((readParameters().composerScene-1+(cc==43?1:size-1))%size);}
        else if(cc==45)captureLiveSnapshot();else if(cc==46)returnLiveSnapshot();
        else if(cc==47)transport(false);else if(cc==48)transport(true);
        else if(cc==49){if(liveRecorder.active)stopLiveRecording();else startLiveRecording();}
    };
    for(int i=0;i<range.blockSize1;++i)action(liveActions[size_t(range.startIndex1+i)]);
    for(int i=0;i<range.blockSize2;++i)action(liveActions[size_t(range.startIndex2+i)]);
}
