#pragma once
inline void checkAudioRendering() {
    using namespace jerzy;using namespace jerzy::composer;
    const auto dir=juce::File::getSpecialLocation(juce::File::tempDirectory).getNonexistentChildFile("PRO-audio-tests",{},false);
    require(dir.createDirectory().wasOk(),"audio render test directory");
    struct Cleanup{juce::File folder;~Cleanup(){folder.deleteRecursively();}} cleanup{dir};
    Parameters patch;patch.bass=patch.pad=patch.arp=patch.drums=patch.krell=true;
    patch.fxEnabled=false;patch.padAttack=.05;patch.padRelease=.1;
    Document data;data.tempo=240;Scene scene;scene.bars=1;scene.numerator=1;scene.patch=patch;
    for(int t=0;t<tracks;++t){auto& clip=scene.clips[size_t(t)];clip.generated=false;clip.notes={{0,.5,t>=3&&t<=6?drumNotes[t-3]:t==0?36:60,100}};}
    data.scenes={scene};data.song={{scene.id,2,false}};
    AudioRenderOptions options;options.sampleRate=48000;options.tailSeconds=0;options.effects=false;
    juce::WavAudioFormat wav;
    auto read=[&](const juce::File& file){auto stream=file.createInputStream();require(stream!=nullptr,"rendered WAV exists");return std::unique_ptr<juce::AudioFormatReader>(wav.createReaderFor(stream.release(),true));};
    auto file=dir.getChildFile("scene.wav");
    require(BassAmbientProcessor::renderAudio(data,patch,options,file).wasOk(),"scene audio render");
    auto reader=read(file);require(reader&&reader->lengthInSamples==12000&&reader->bitsPerSample==24&&reader->numChannels==2,"scene WAV format and exact length");
    juce::AudioBuffer<float> samples(2,12000);require(reader->read(&samples,0,12000,0,true,true),"read rendered audio");require(samples.getRMSLevel(0,0,12000)>.0001,"scene WAV contains sound");reader.reset();
    options.song=true;options.bits=32;options.sampleRate=44100;options.tailSeconds=.1;options.track=1;
    file=dir.getChildFile("song-pad.wav");
    require(BassAmbientProcessor::renderAudio(data,patch,options,file).wasOk(),"whole song instrument render");reader=read(file);
    require(reader&&reader->lengthInSamples==26460&&reader->usesFloatingPointData&&reader->sampleRate==44100,"song repeats, tail and float WAV format");reader.reset();
    options.song=false;options.sampleRate=96000;options.tailSeconds=0;options.track=-1;options.stems=true;
    const auto stems=dir.getChildFile("stems");require(BassAmbientProcessor::renderAudio(data,patch,options,stems).wasOk(),"stem export");
    require(stems.findChildFiles(juce::File::findFiles,false,"*.wav").size()==10,"eight instrument stems and two return stems");
    for(int t=0;t<10;++t){reader=read(stems.getChildFile(juce::String(t<8?trackNames[t]:t==8?"DELAY-RETURN":"REVERB-RETURN")+".wav"));require(reader&&reader->lengthInSamples==24000,"all stems time aligned");}reader.reset();
    // A selected instrument must not contain notes from other parts, even with another part soloed.
    options.stems=false;options.track=1;options.bits=32;options.sampleRate=48000;options.tailSeconds=.1;
    patch.mix0Solo=true;
    auto isolated=dir.getChildFile("pad-only.wav");require(BassAmbientProcessor::renderAudio(data,patch,options,isolated).wasOk(),"isolated PAD render");
    auto onlyPad=data;for(int t=0;t<tracks;++t)if(t!=1)onlyPad.scenes[0].clips[size_t(t)].notes.clear();
    auto reference=dir.getChildFile("pad-reference.wav");require(BassAmbientProcessor::renderAudio(onlyPad,patch,options,reference).wasOk(),"isolated PAD reference");
    require(isolated.hasIdenticalContentTo(reference),"instrument render excludes all other notes and is deterministic");
    options.track=-1;options.stems=true;options.effects=true;options.tailSeconds=.5;patch.mix0Solo=false;patch.mix0Delay=1;patch.mix0Reverb=1;
    const auto wetStems=dir.getChildFile("wet-stems");require(BassAmbientProcessor::renderAudio(data,patch,options,wetStems).wasOk(),"wet stem render");
    for(const auto* name:{"DELAY-RETURN.wav","REVERB-RETURN.wav"}){reader=read(wetStems.getChildFile(name));juce::AudioBuffer<float> wet(2,int(reader->lengthInSamples));reader->read(&wet,0,wet.getNumSamples(),0,true,true);require(wet.getRMSLevel(0,12000,wet.getNumSamples()-12000)>.00001,"render includes audible send return tails");}reader.reset();
    options.stems=false;file=dir.getChildFile("preserve.wav");require(file.replaceWithText("existing export"),"existing render fixture");
    auto cancelled=BassAmbientProcessor::renderAudio(data,patch,options,file,[](double value){return value<.2;});
    require(cancelled.failed()&&file.loadFileAsString()=="existing export","cancel preserves existing export");
    require(dir.findChildFiles(juce::File::findDirectories,false,".JerzyPRO-render*").isEmpty(),"cancel cleans temporary render files");
    data.song.clear();options.song=true;require(BassAmbientProcessor::renderAudio(data,patch,options,dir.getChildFile("empty.wav")).failed(),"empty song rejected");
}
