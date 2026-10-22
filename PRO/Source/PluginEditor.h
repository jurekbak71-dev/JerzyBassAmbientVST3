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
    class Theme;class Control;class Page;class Shell;class SettingsPanel;
    BassAmbientProcessor& processor;std::unique_ptr<Theme> theme;std::unique_ptr<Shell> shell;
    std::array<std::unique_ptr<Page>,12> pages;
    std::array<juce::TextButton,5> tabs;
    std::array<juce::ToggleButton,5> modulePower;
    std::array<juce::TextButton,3> mainTabs,sectionTabs;
    std::array<std::unique_ptr<SignalLeds>,5> moduleLeds;
    std::unique_ptr<MixerView> mixerView;std::unique_ptr<ComposerView> composerView;
    std::unique_ptr<TransportView> transportView;std::unique_ptr<HarmonyView> harmonyView;
    juce::Viewport viewport;juce::ComboBox presets,source,performance,localPreset;
    juce::TextButton variant{"NOWY WARIANT"},undo{"COFNIJ"},advanced{"ZAAWANSOWANE"},panic{"PANIC"},exportButton{"EKSPORT"},settings{"USTAWIENIA"},diagnostics{"DIAGNOSTYKA"},masterFX{"MASTER FX"},returns{"POWROTY FX"},routing{"ROUTING FL"};
    juce::TextButton jamCapture{juce::String::fromUTF8("ZAPISZ JAM JAKO SCENĘ")};
    juce::Label status,title,modeLabel;
    juce::ToggleButton fxGlobal{"FX"};
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> fxGlobalAttachment;
    std::array<int,5> localPresetIds{};
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> powerAttachments;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> performanceAttachment;
    std::unique_ptr<juce::DialogWindow> dialog;std::unique_ptr<SettingsPanel> dialogContent;
    std::vector<std::pair<juce::String,float>> variantUndo;
    juce::TooltipWindow tooltips{this,450};
    int selected=0,selectedModule=0,section=0,selectedRack=0,lastBassModel=-1;bool expanded=false,updating=false;float uiScale=0;
    juce::Component& currentPage();void selectPage(int);void chooseModule(int);void chooseSection(int);void setSource(int);void showSettings(int);void makeVariant();void timerCallback() override;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BassAmbientEditor)
};
