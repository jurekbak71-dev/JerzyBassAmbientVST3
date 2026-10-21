#pragma once
#include "PluginProcessor.h"
#include "JerzyVSTGuiKit.h"
#include "ComposerView.h"
#include "PerformanceView.h"

class BassAmbientEditor final : public juce::AudioProcessorEditor,private juce::Timer {
public:
    explicit BassAmbientEditor(BassAmbientProcessor&);
    ~BassAmbientEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    class Theme;
    class Control;
    class Page;
    BassAmbientProcessor& processor;
    std::unique_ptr<Theme> theme;
    std::array<std::unique_ptr<Page>,12> pages;
    std::array<juce::TextButton,9> tabs;
    std::unique_ptr<MixerView> mixerView;
    std::unique_ptr<TransportView> transportView;
    std::unique_ptr<HarmonyView> harmonyView;
    std::array<std::unique_ptr<SignalLeds>,5> moduleLeds;
    std::unique_ptr<ComposerView> composerView;
    juce::ComboBox rackSelector;
    juce::Viewport viewport;
    juce::ComboBox presets;
    juce::TextButton mutate{"MUTATE"},panic{"PANIC"};
    juce::Label status;
    juce::ToggleButton fxPower{"FX POWER"};
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> fxAttachment;
    juce::TooltipWindow tooltips{this,450};
    int selected=0, selectedRack=0,lastBassModel=-1;
    juce::Component& currentPage();
    void selectPage(int);
    void timerCallback() override;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BassAmbientEditor)
};
