#pragma once
inline void checkLivePerformance() {
    BassAmbientProcessor p;
    auto set=[&](const char* id,float value){p.setParameterValue(id,value);};
    auto folder=juce::File::getSpecialLocation(juce::File::tempDirectory).getNonexistentChildFile("PRO-Live-tests",{},false);
    require(folder.createDirectory(),"live test folder");
    struct Cleanup {juce::File path;~Cleanup(){path.deleteRecursively();}} cleanup{folder};
    const auto file=folder.getChildFile("concert.wav");p.armLiveRecording(file);require(p.startLiveRecording().failed()&&!file.exists(),"REC requires prepared audio and keeps destination intact");
    for(const auto& value:std::initializer_list<std::pair<const char*,float>>{{"performanceSource",1},{"modularMidi",1},{"bassMidiMode",0},{"clockSource",0},{"bass",1},{"pad",0},{"arp",0},{"drums",0},{"krell",0},{"fxEnabled",0},{"cutoff",900}})set(value.first,value.second);
    p.prepareToPlay(48000,128);juce::AudioBuffer<float> audio(2,128);juce::MidiBuffer midi;
    midi.addEvent(juce::MidiMessage::noteOn(1,48,1.f),0);p.processBlock(audio,midi);require(p.rootDisplay==48,"live snapshot setup held root");
    p.captureLiveSnapshot();set("cutoff",5000);set("mix0Gain",.3f);p.processBlock(audio,midi);
    const auto before=p.readParameters();require(p.returnLiveSnapshot(),"RETURN has captured state");p.processBlock(audio,midi);
    require(std::abs(p.readParameters().cutoff-900)<.01&&std::abs(p.readParameters().mix0Gain-1)<1e-5,"RETURN restores timbre and mixer");require(p.rootDisplay==48&&p.readParameters().performanceSource==before.performanceSource,"RETURN preserves held MIDI root and source");
    require(p.startLiveRecording().wasOk(),"start real-time stereo WAV");
    std::vector<float> reference;
    {std::unique_ptr<juce::AudioProcessorEditor> gui(p.createEditor());for(int block=0;block<400;++block){p.processBlock(audio,midi);for(int n=0;n<128;++n)reference.push_back(audio.getSample(0,n));}}
    require(p.liveRecorder.active,"closing editor does not stop concert recording");
    for(int block=0;block<400;++block){p.processBlock(audio,midi);for(int n=0;n<128;++n)reference.push_back(audio.getSample(0,n));}
    p.stopLiveRecording();require(!p.liveRecorder.active&&p.liveRecorder.dropped==0,"REC flushes without queue loss");
    juce::AudioFormatManager formats;formats.registerBasicFormats();std::unique_ptr<juce::AudioFormatReader> reader(formats.createReaderFor(file));require(reader&&reader->sampleRate==48000&&reader->numChannels==2&&reader->bitsPerSample==24,"live WAV format");require(reader->lengthInSamples==int64_t(reference.size()),"live WAV exact frame count includes closed-GUI playback");
    juce::AudioBuffer<float> recorded(2,int(reference.size()));reader->read(&recorded,0,recorded.getNumSamples(),0,true,true);double energy=0;for(int i=0;i<int(reference.size());++i){energy+=std::abs(reference[size_t(i)]);require(std::abs(reference[size_t(i)]-recorded.getSample(0,i))<.000002f,"WAV contains actual MAIN output, not an offline regeneration");}require(energy>.01,"live WAV contains audible music");reader.reset();
    juce::MemoryBlock firstBytes;require(file.loadFileAsData(firstBytes),"load first take");
    require(p.startLiveRecording().wasOk(),"second take gets unique filename");p.processBlock(audio,midi);p.stopLiveRecording();require(p.liveRecordingPath()!=file.getFullPathName(),"new take never overwrites first concert");juce::MemoryBlock retained;file.loadFileAsData(retained);require(retained==firstBytes,"first WAV preserved byte for byte");
    auto send=[&](int cc,int value){midi.addEvent(juce::MidiMessage::controllerEvent(1,cc,value),0);p.processBlock(audio,midi);};
    auto wait=[&](const std::function<bool()>& ready){for(int n=0;n<200&&!ready();++n)juce::Thread::sleep(5);require(ready(),"live MIDI action completes off audio thread");};
    send(43,127);wait([&]{return p.readParameters().composerScene==2;});send(43,127);juce::Thread::sleep(25);require(p.readParameters().composerScene==2,"held MIDI button does not repeat scene launch");send(43,0);send(43,127);wait([&]{return p.readParameters().composerScene==3;});
    p.liveSnapshotReady=false;send(45,127);wait([&]{return p.liveSnapshotReady.load();});set("cutoff",6000);send(46,127);wait([&]{return std::abs(p.readParameters().cutoff-900)<.01;});
    p.liveSnapshotReady=false;p.learnMidi("cutoff");send(45,0);send(45,127);juce::Thread::sleep(25);require(!p.liveSnapshotReady&&p.midiAssignment("cutoff")=="CC 45","MIDI Learn overrides live defaults");
    send(47,127);wait([&]{return !p.readParameters().composerPlaying;});
    // Run a repeated scene performance and ensure finite, audible audio throughout.
    set("clockSource",0);p.launchLiveScene(0);set("composerLoop",1);double liveEnergy=0;for(int block=0;block<120000;++block){if(block%375==0)p.launchLiveScene((block/375)%7);p.processBlock(audio,midi);for(int n=0;n<128;++n){const float sample=audio.getSample(0,n);require(std::isfinite(sample),"live scene stress finite output");liveEnergy+=std::abs(sample);}if(block%32==0)juce::Thread::sleep(1);}require(liveEnergy>.1,"repeated live scenes produce audio");
    p.transport(false);p.editComposer([](auto& d){d.scenes[0].bars=1;});juce::Thread::sleep(40);set("internalTempo",240);set("composerLoop",0);p.launchLiveScene(0);for(int i=0;i<400;++i)p.processBlock(audio,midi);require(p.composerBeatDisplay>4,"non-looping scene reaches end");p.launchLiveScene(0);p.processBlock(audio,midi);require(p.composerBeatDisplay<.1,"same pad retriggers a completed scene");
    require(p.startLiveRecording().wasOk(),"record device-release take");p.processBlock(audio,midi);const auto lastFile=juce::File(p.liveRecordingPath());p.releaseResources();require(!p.liveRecorder.active,"audio device release finalises recording");std::unique_ptr<juce::AudioFormatReader> finalReader(formats.createReaderFor(lastFile));require(finalReader&&finalReader->lengthInSamples==128,"device release leaves readable WAV");finalReader.reset();
    p.prepareToPlay(96000,128);const auto highRate=folder.getChildFile("96k.wav");p.armLiveRecording(highRate);require(p.startLiveRecording().wasOk(),"new sample-rate take");p.processBlock(audio,midi);p.stopLiveRecording();std::unique_ptr<juce::AudioFormatReader> highReader(formats.createReaderFor(highRate));require(highReader&&highReader->sampleRate==96000&&highReader->lengthInSamples==128,"REC follows changed host sample rate");p.releaseResources();
}
