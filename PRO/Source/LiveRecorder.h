#pragma once
#include <JuceHeader.h>

// Audio callback only enqueues samples; file creation/finalisation is off-thread.
class LiveRecorder final {
public:
    ~LiveRecorder() { stop(); thread.stopThread(5000); }
    juce::Result start(const juce::File& file, double rate) {
        const juce::ScopedLock control(controlLock);
        if (active.load()) return juce::Result::fail("Recording already running");
        if (file.exists()) return juce::Result::fail("Choose a new WAV filename");
        if (!std::isfinite(rate) || rate < 8000) return juce::Result::fail("Audio device is not ready");
        std::unique_ptr<juce::OutputStream> stream = file.createOutputStream();
        if (!stream) return juce::Result::fail("Cannot create WAV file");
        juce::WavAudioFormat format;
        auto writer = format.createWriterFor(stream, juce::AudioFormatWriterOptions{}
            .withSampleRate(rate).withNumChannels(2).withBitsPerSample(24));
        if (!writer) return juce::Result::fail("Cannot create WAV writer");
        thread.startThread();
        auto ready = std::make_unique<juce::AudioFormatWriter::ThreadedWriter>(writer.release(), thread, 262144);
        { const juce::SpinLock::ScopedLockType lock(writerLock); sink = std::move(ready); frames=0; dropped=0; limit=false; sampleRate=rate; pendingClose=false; }
        active=true;
        return juce::Result::ok();
    }
    void stop() {
        const juce::ScopedLock control(controlLock);
        std::unique_ptr<juce::AudioFormatWriter::ThreadedWriter> finishing;
        active=false;
        { const juce::SpinLock::ScopedLockType lock(writerLock); active=false; pendingClose=false; finishing=std::move(sink); }
        finishing.reset(); // flush and update WAV header outside audio callback/lock
    }
    void push(const juce::AudioBuffer<float>& audio) noexcept {
        if (!active.load()) return;
        const juce::SpinLock::ScopedTryLockType lock(writerLock);
        if (!lock.isLocked()) { if(active.load()){dropped+=audio.getNumSamples();active=false;pendingClose=true;} return; }
        if (!active.load() || !sink || audio.getNumChannels()<2) return;
        constexpr int64_t maximumFrames=0xff000000LL/6; // classic stereo 24-bit WAV <4 GB
        const int count=int(std::min<int64_t>(audio.getNumSamples(),maximumFrames-frames.load()));
        const float* channels[]{audio.getReadPointer(0),audio.getReadPointer(1)};
        if (count>0 && sink->write(channels,count)) frames+=count;
        else { dropped+=std::max(1,count);active=false;pendingClose=true; }
        if (frames>=maximumFrames) {limit=true;active=false;pendingClose=true;}
    }
    bool needsFinish() const {return pendingClose.load();}
    std::atomic<bool> active{false},limit{false};
    std::atomic<int64_t> frames{0},dropped{0};
    std::atomic<double> sampleRate{48000};
private:
    std::atomic<bool> pendingClose{false};
    juce::CriticalSection controlLock;
    juce::SpinLock writerLock;
    juce::TimeSliceThread thread{"PRO live WAV disk writer"};
    std::unique_ptr<juce::AudioFormatWriter::ThreadedWriter> sink;
};
