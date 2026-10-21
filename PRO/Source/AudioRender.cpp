#include "PluginProcessor.h"
using namespace jerzy;
using namespace jerzy::composer;

juce::Result BassAmbientProcessor::renderAudio(Document document,Parameters controls,
        const AudioRenderOptions& options,const juce::File& destination,
        const std::function<bool(double)>& progress) {
    try {
        if((options.bits!=24&&options.bits!=32)||
           (options.sampleRate!=44100&&options.sampleRate!=48000&&options.sampleRate!=96000)||
           options.tailSeconds<0||options.tailSeconds>45||options.track < -1||options.track>=tracks)
            return juce::Result::fail("Invalid audio render settings");
        if(!options.song&&(options.scene<0||options.scene>=int(document.scenes.size())))
            return juce::Result::fail("Select a scene");
        if(options.song&&document.song.empty())return juce::Result::fail("SONG is empty. Add scenes first.");
        if(options.track>=0) {
            for(auto& scene:document.scenes)for(int t=0;t<tracks;++t)
                if(t!=options.track)scene.clips[size_t(t)].enabled=false;
            controls.mix0Solo=controls.mix1Solo=controls.mix2Solo=controls.mix3Solo=false;
            controls.mix4Solo=controls.mix5Solo=controls.mix6Solo=controls.mix7Solo=false;
            for(auto& scene:document.scenes){auto& p=scene.patch;
                p.mix0Solo=p.mix1Solo=p.mix2Solo=p.mix3Solo=false;
                p.mix4Solo=p.mix5Solo=p.mix6Solo=p.mix7Solo=false;}
        }
        auto engine=std::make_unique<BassAmbientProcessor>();
        engine->activeComposer=new Compiled(compile(document));
        const auto& timeline=options.song?engine->activeComposer->song:engine->activeComposer->scenes[size_t(options.scene)];
        if(timeline.beats<=0)return juce::Result::fail("Nothing to render");
        const double tempo=clamp(document.tempo,20.0,400.0);
        const auto musicalFrames=int64_t(std::ceil(timeline.beats*60/tempo*options.sampleRate));
        const auto totalFrames=musicalFrames+int64_t(std::round(options.tailSeconds*options.sampleRate));
        if(totalFrames>int64_t(0xffff0000ULL/8))return juce::Result::fail("Render exceeds the WAV 4 GB limit. Export shorter sections.");
        // A private engine and immutable snapshot: no host transport, live voices or audio device is touched.
        engine->sr=options.sampleRate;engine->instrument.prepare(options.sampleRate);
        controls.composerEnabled=true;controls.composerPlaying=true;controls.composerLoop=false;
        controls.composerSongMode=options.song;controls.composerScene=options.scene+1;
        controls.autoPlay=false;controls.midiPlayback=false;controls.clockSource=0;
        controls.separateOutputs=options.stems;
        controls.fxEnabled=options.effects;
        controls.bass=controls.pad=controls.arp=controls.drums=controls.krell=true;
        engine->current=controls;
        const auto stage=destination.getParentDirectory().getNonexistentChildFile(".JerzyPRO-render",{},false);
        if(!stage.createDirectory())return juce::Result::fail("Cannot create render directory");
        struct Cleanup {juce::File folder;~Cleanup(){folder.deleteRecursively();}} cleanup{stage};
        juce::WavAudioFormat format;
        const auto writerOptions=juce::AudioFormatWriterOptions{}.withSampleRate(options.sampleRate)
            .withNumChannels(2).withBitsPerSample(options.bits)
            .withSampleFormat(options.bits==32?juce::AudioFormatWriterOptions::SampleFormat::floatingPoint:
                                               juce::AudioFormatWriterOptions::SampleFormat::integral);
        const int outputs=options.stems?10:1;
        std::vector<std::unique_ptr<juce::AudioFormatWriter>> writers;
        std::vector<juce::File> files;
        for(int t=0;t<outputs;++t) {
            auto file=stage.getChildFile(options.stems?juce::String(t+1).paddedLeft('0',2)+"-"+juce::String(t<8?trackNames[t]:t==8?"DELAY-RETURN":"REVERB-RETURN")+".wav":"render.wav");
            std::unique_ptr<juce::OutputStream> stream=file.createOutputStream();
            if(!stream)return juce::Result::fail("Cannot open WAV file");
            auto writer=format.createWriterFor(stream,writerOptions);
            if(!writer)return juce::Result::fail("Cannot create WAV writer");
            files.push_back(file);writers.push_back(std::move(writer));
        }
        std::vector<juce::AudioBuffer<float>> buffers;
        for(int t=0;t<outputs;++t)buffers.emplace_back(2,512);
        for(int64_t frame=0;frame<totalFrames;) {
            if(progress&&!progress(double(frame)/double(totalFrames)))return juce::Result::fail("Render cancelled");
            const int count=int(std::min<int64_t>(512,totalFrames-frame));
            engine->instrument.beginBlock(double(frame)/options.sampleRate*tempo/60,tempo,true);
            for(int sample=0;sample<count;++sample) {
                const auto absolute=frame+sample;
                controls.composerPlaying=absolute<musicalFrames;
                const double beat=double(absolute)/options.sampleRate*tempo/60;
                const int oldSection=engine->sectionIndex;
                auto patch=engine->composerSample(controls,beat,tempo,true,4,4);
                if(!options.effects)patch.fxEnabled=false;
                // Rack allocation is synchronous in this offline-only engine.
                if(sample==0||oldSection!=engine->sectionIndex){engine->instrument.configureResources(patch);engine->instrument.serviceResources();engine->instrument.configureResources(patch);}
                const auto out=engine->instrument.next(patch,beat,4,4,true,options.stems?1023U:0U);
                const auto& stems=engine->instrument.stemOutputs();const auto& returns=engine->instrument.returnOutputs();
                for(int t=0;t<outputs;++t) {
                    const auto value=options.stems?(t<8?stems[size_t(t)]:returns[size_t(t-8)]):out;
                    if(!std::isfinite(value.l)||!std::isfinite(value.r))return juce::Result::fail("Non-finite audio sample");
                    buffers[size_t(t)].setSample(0,sample,float(value.l));buffers[size_t(t)].setSample(1,sample,float(value.r));
                }
            }
            for(int t=0;t<outputs;++t)if(!writers[size_t(t)]->writeFromAudioSampleBuffer(buffers[size_t(t)],0,count))
                return juce::Result::fail("Cannot write WAV (check free disk space)");
            frame+=count;
        }
        for(auto& writer:writers)if(!writer->flush())return juce::Result::fail("Cannot flush WAV file");
        writers.clear();
        if(options.stems) {
            stage.getChildFile("README.txt").replaceWithText("Time-aligned stereo stems. Instrument files include instrument FX and mixer gain/pan; shared sends are separate return files. MAIN master FX/limiter are excluded from stems. Sample rate: "+juce::String(options.sampleRate)+" Hz. Tail: "+juce::String(options.tailSeconds)+" seconds.\n");
            if(destination.exists())return juce::Result::fail("Stem destination already exists");
            if(!stage.moveFileTo(destination))return juce::Result::fail("Cannot save stem folder");
        } else {
            // Do not replace a previous export until the entire new file has been written.
            juce::TemporaryFile temporary(destination);
            if(!files.front().moveFileTo(temporary.getFile())||!temporary.overwriteTargetFileWithTemporary())
                return juce::Result::fail("Cannot save rendered WAV");
        }
        if(progress)progress(1);
        return juce::Result::ok();
    } catch(const std::exception& error) {return juce::Result::fail(error.what());}
}
