#pragma once
#include "PluginProcessor.h"
class ComposerView final : public juce::Component,private juce::Timer,public juce::DragAndDropContainer {
public:
    explicit ComposerView(BassAmbientProcessor&);
    ~ComposerView() override;
    void resized() override;
    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;
    int requiredHeight() const {return 1080;}
private:
    class SceneModel;class SongModel;class SongList;class NoteGrid;class MidiDrag;
    BassAmbientProcessor& processor;
    jerzy::composer::Document document;
    std::unique_ptr<SceneModel> sceneModel;std::unique_ptr<SongModel> songModel;
    juce::ListBox scenes;
    std::unique_ptr<SongList> song;
    std::unique_ptr<NoteGrid> grid;
    std::unique_ptr<MidiDrag> dragMidi;
    int selectedScene=0,selectedTrack=0,selectedSlot=0,selectedNote=-1;
    uint64_t revision=0;bool updating=false;
    juce::Label heading,sceneTitle,songTitle,clipTitle,message;
    juce::TextEditor name,progression;
    juce::Slider bars,tempo,intensity,repeat,bar,octave,velocity,notePan,gain,enter,leave,threshold;
    juce::ComboBox numerator,denominator;
    juce::ToggleButton enabled{"COMPOSER"},play{"PLAY"},songMode{"SONG"},loop{"LOOP"},split{"SPLIT OUT"},midiPlay{"MIDI PARTS"},generated{"GENERATED"},cut{"CUT TAILS"},evolve{"EVOLVE"};
    std::array<juce::TextButton,8> tracks;
    juce::TextButton add{"NEW"},duplicate{"VARIANT"},remove{"DELETE"},launch{"LAUNCH SCENE"},capture{"CAPTURE SOUND"},append{"+ SONG"},slotRemove{"REMOVE"},up{"<"},down{">"},freeze{"FREEZE / EDIT"},clear{"CLEAR CLIP"},exportScene{"EXPORT SCENE"},exportSong{"EXPORT SONG"},exportPack{"PATTERN PACK"},save{"SAVE"},load{"LOAD"};
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> attachments;
    std::unique_ptr<juce::FileChooser> chooser;
    void refresh(bool force=false);
    void timerCallback() override;
    void edit(const std::function<void(jerzy::composer::Document&)>&);
    void selectScene(int);
    void selectTrack(int);
    void chooseExport(int);
    void chooseProject(bool);
    void setMessage(const juce::String&);
    void updateProgression();
    juce::String progressionText() const;
    void prepareManual(jerzy::composer::Scene&);
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ComposerView)
};
