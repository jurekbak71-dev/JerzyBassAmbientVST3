#include "VisualPalette.h"
#include "PluginEditor.h"

namespace {
JerzyAudioUI::Theme proTheme() {return {0xff202630,0xff11141a,0xff252c37,0xff151a22,0xff61e0c1,0xffedf1f7,0xffa3afbf,0xff465263,0xff61e0c1,0xff61e0c1};}
const juce::Colour background(0xff14171d),panel(0xff20252e),accent(0xff61e0c1),text(0xffedf1f7),muted(0xffa3afbf);
}
class BassAmbientEditor::Theme final : public JerzyAudioUI::HardwareLookAndFeel {
public:
    Theme() : JerzyAudioUI::HardwareLookAndFeel(proTheme()) {
        setColour(juce::Label::textColourId,text);
        setColour(juce::PopupMenu::textColourId,text);
    }
    void drawToggleButton(juce::Graphics& g,juce::ToggleButton& b,bool,bool) override {
        auto r=b.getLocalBounds().toFloat().reduced(1);const bool on=b.getToggleState();
        const auto accent=b.findColour(juce::ToggleButton::tickColourId);
        g.setColour(juce::Colour(0xff151a22));g.fillRoundedRectangle(r,4);
        g.setColour(on?accent:muted.withAlpha(.45f));g.drawRoundedRectangle(r,4,on?1.8f:1.0f);
        if(on){g.setColour(accent);g.fillEllipse(8,r.getCentreY()-3,6,6);}
        auto label=b.getButtonText();label=label.startsWith("MODE:")?label:label=="ON"?(on?"ON":"OFF"):label+(on?" ON":" OFF");
        g.setColour(b.findColour(juce::ToggleButton::textColourId).withMultipliedAlpha(on?1.0f:.65f));g.setFont(11);g.drawText(label,18,0,b.getWidth()-20,b.getHeight(),juce::Justification::centredLeft);
    }
    void drawButtonBackground(juce::Graphics& g,juce::Button& b,const juce::Colour& colour,bool over,bool down) override {
        if(b.getComponentID().startsWith("grid-")) {
            auto r=b.getLocalBounds().toFloat().reduced(2);auto c=b.isEnabled()?colour:colour.withMultipliedBrightness(.35f);
            g.setColour(down?c.darker(.25f):over?c.brighter(.2f):c);g.fillRoundedRectangle(r,5);
            g.setColour(c.brighter(.4f));g.drawRoundedRectangle(r,5,1);return;
        }
        JerzyAudioUI::HardwareLookAndFeel::drawButtonBackground(g,b,colour,over,down);
    }
};

class BassAmbientEditor::Control final : public juce::Component {
    BassAmbientProcessor& processor;
    ParameterSpec spec;
    juce::Label label;
    juce::Slider slider;
    juce::ComboBox choice;
    juce::ToggleButton toggle;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sliderAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> choiceAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> toggleAttachment;
public:
    Control(BassAmbientProcessor& p,const ParameterSpec& s):processor(p),spec(s) {
        addMouseListener(this,true);
        label.setText(s.label,juce::dontSendNotification);label.setJustificationType(juce::Justification::centred);
        label.setFont(juce::Font(juce::FontOptions(13)));addAndMakeVisible(label);
        if(s.kind==1) {
            toggle.setButtonText("ON");addAndMakeVisible(toggle);
            toggleAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.state,s.id,toggle);
        } else if(s.kind==3) {
            choice.addItemList(s.choices,1);addAndMakeVisible(choice);
            choiceAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.state,s.id,choice);
        } else {
            slider.setSliderStyle(s.kind==2?juce::Slider::LinearHorizontal:juce::Slider::RotaryHorizontalVerticalDrag);
            slider.setTextBoxStyle(juce::Slider::TextBoxBelow,false,90,22);
            slider.setNumDecimalPlacesToDisplay(s.kind==2?0:3);
            slider.setTooltip(s.label+" | Right-click: default | Ctrl+right-click: MIDI learn | FL Studio: Browse parameters");
            slider.setPopupMenuEnabled(false);
            addAndMakeVisible(slider);
            sliderAttachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.state,s.id,slider);
            // APVTS installs its own formatter: override it after attachment creation.
            slider.textFromValueFunction=[s](double value) {
                return juce::String(value,s.kind==2?0:(s.maximum>=100?1:3));
            };
            slider.updateText();
            slider.setDoubleClickReturnValue(true,s.initial);
        }
    }
    ~Control() override {removeMouseListener(this);}
    void mouseDown(const juce::MouseEvent& event) override {
        if(!event.mods.isRightButtonDown())return;
        if(!event.mods.isCtrlDown()) {processor.setParameterValue(spec.id,float(spec.initial));return;}
        juce::PopupMenu menu;menu.addItem(1,"MIDI Learn ("+processor.midiAssignment(spec.id)+")");menu.addItem(2,"Remove MIDI assignment");
        const juce::Component::SafePointer<Control> safe(this);
        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this),[safe](int result){if(safe){if(result==1)safe->processor.learnMidi(safe->spec.id);if(result==2)safe->processor.clearMidiMapping(safe->spec.id);}});
    }
    void paint(juce::Graphics& g) override {
        g.setColour(panel);g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(3),8);
        g.setColour(juce::Colour(0xff313947));g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(3),8,1);
    }
    void resized() override {
        auto bounds=getLocalBounds().reduced(10);label.setBounds(bounds.removeFromTop(30));
        if(spec.kind==1) toggle.setBounds(bounds.withSizeKeepingCentre(80,32));
        else if(spec.kind==3) choice.setBounds(bounds.withSizeKeepingCentre(bounds.getWidth(),32));
        else slider.setBounds(bounds);
    }
};

class BassAmbientEditor::Page final : public juce::Component {
    BassAmbientProcessor& processor;
    int index, selectedModule=0;
    juce::String prefix;
    struct Item { std::unique_ptr<Control> control; int module=-1; };
    std::vector<Item> controls;
    std::array<juce::TextButton,6> modules, left, right;
    juce::Label description, generatorHeading;
    int generatorStart=-1;
    bool isFX() const { return (index>=2&&index<=4)||index==8||index==9||index==11; }
    std::array<juce::TextButton,64> steps;
    std::array<juce::TextButton,4> banks;
    std::array<juce::TextButton,4> laneLabels;
    juce::TextButton clearGrid{"CLEAR"},fourFloor{"4 ON FLOOR"},breakbeat{"BREAKBEAT"};
    juce::Label gridHelp;
    int bank=0,selectedDrum=0;
    void editStep(int lane,int step,bool context) {
        const auto id="drum"+juce::String(lane)+"_step"+juce::String(step);
        auto* parameter=processor.state.getRawParameterValue(id);
        if(!context) {processor.setParameterValue(id,parameter->load()>0?0:100);refreshGrid();return;}
        juce::PopupMenu menu;menu.addItem(1,"Soft (48)");menu.addItem(2,"Normal (90)");menu.addItem(3,"Accent (127)");menu.addItem(4,"MIDI Learn velocity");
        juce::Component::SafePointer<Page> safe(this);
        menu.showMenuAsync(juce::PopupMenu::Options(),[safe,id](int result){if(safe){if(result==4)safe->processor.learnMidi(id);else if(result>0){const float velocities[]{48,90,127};safe->processor.setParameterValue(id,velocities[result-1]);safe->refreshGrid();}}});
    }
    void drumPreset(int type) {
        for(int lane=0;lane<4;++lane)for(int k=0;k<64;++k){int value=0;
            if(type==1)value=lane==0?(k%4==0?110:0):lane==1?(k%8==4?105:0):lane==2?(k%2==0?75:0):(k%16==12?65:0);
            if(type==2)value=lane==0?((k%16==0||k%16==6||k%16==10)?115:0):lane==1?(k%8==4?110:0):lane==2?(k%2==0?85:45):(k%16==15?55:0);
            processor.setParameterValue("drum"+juce::String(lane)+"_step"+juce::String(k),float(value));
        }refreshGrid();
    }
    void swapSlots(int a,int b) {
        if(b<0||b>=6) return;
        auto* pa=processor.state.getParameter(prefix+"order"+juce::String(a));
        auto* pb=processor.state.getParameter(prefix+"order"+juce::String(b));
        float va=pa->getValue(),vb=pb->getValue();
        pa->beginChangeGesture();pb->beginChangeGesture();
        pa->setValueNotifyingHost(vb);pb->setValueNotifyingHost(va);
        pa->endChangeGesture();pb->endChangeGesture();updateOrder();
    }
    int slotModule(int slot) const {
        return juce::jlimit(0,5,int(processor.state.getRawParameterValue(prefix+"order"+juce::String(slot))->load()));
    }
public:
    std::function<void()> layoutChanged;
    Page(BassAmbientProcessor& p,int page):processor(p),index(page),prefix(page==3?"pad_":page==4?"master_":page==8?"arp_":page==9?"drums_":page==11?"krell_":"") {
        const char* descriptions[]{
            "BASS: synth controls, followed by the MIDI phrase generator. One Piano Roll note supplies the root.",
            "AMBIENT: two engines per voice, overlapping envelopes and HOLD / FLOW / KRELL. Shared scale and seed below.",
            "BASS FX: independent processing before the layer sum. Select a module to edit; arrows change its position.",
            "AMBIENT FX: independent processing before the layer sum. Select a module to edit; arrows change its position.",
            "MASTER FX: the sum of BASS, AMBIENT, ARPEGGIO and DRUMS. Set amount above zero to hear processing.",
            "ARPEGGIO: hold a MIDI chord or use GENERATE. Rhythm, controlled randomness, AMP, filter and stereo notes.",
            "DRUMS: four synthesized voices / 64 steps. Click pads to toggle; right-click for velocity or MIDI Learn.",
            "GENERATE: play without MIDI. Choose root, scale and tempo. In FL Studio the host supplies tempo and position."};
        description.setText(page==10?"KRELL: independent eight-voice evolving FM / noise / harmonic synthesis, separate envelope, mixer and FX.":page==11?"KRELL FX: dedicated processing before the mixer and send effects.":page==8?"ARPEGGIO FX: independent rack before the mixer and sends. Raise Arpeggio FX amount to hear it.":page==9?"DRUMS FX: common controls, independent processing per voice to preserve separate mixer outputs. Raise Drums FX amount.":descriptions[page],juce::dontSendNotification);
        description.setColour(juce::Label::textColourId,muted);addAndMakeVisible(description);
        generatorHeading.setText("PHRASE GENERATOR / MIDI",juce::dontSendNotification);
        generatorHeading.setColour(juce::Label::textColourId,accent);
        if(index==0) addAndMakeVisible(generatorHeading);
        int module=-1;
        for(auto spec:parameterSpecs()) {
            bool include=false;int itemModule=-1;
            if(index==0) include=spec.group==0||spec.group==1;
            else if(index==1) include=spec.group==2||spec.id=="scale"||spec.id=="seed";
            else if(index==5) include=spec.group==6||spec.id=="chord"||spec.id=="scale"||spec.id=="seed"||spec.id=="evolve";
            else if(index==6) {include=spec.group==7||(spec.group>=8&&spec.group<=11);if(spec.group>=8&&spec.group<=11)itemModule=spec.group-8;}
            else if(index==10)include=spec.group==23||spec.id=="scale"||spec.id=="seed";
            else if(index==7) include=spec.group==5||spec.group==25||spec.id=="scale"||spec.id=="seed"||spec.id=="evolve"||spec.id=="master"||spec.id=="limiter";
            else {
                const bool own=prefix.isEmpty()?(!spec.id.startsWith("pad_")&&!spec.id.startsWith("master_")&&!spec.id.startsWith("arp_")&&!spec.id.startsWith("drums_")&&!spec.id.startsWith("krell_")):spec.id.startsWith(prefix);
                if(spec.group==3&&own) {
                    auto id=spec.id.substring(prefix.length());
                    if(id.startsWith("order")) continue;
                    if(id=="fxDrive") module=0;
                    if(id=="fxChorus") module=1;
                    if(id=="fxDelay") module=2;
                    if(id=="fxGrain") module=3;
                    if(id=="fxReverb") module=4;
                    if(id=="fxWidth") module=5;
                    itemModule=module;include=true;
                    if(index==3) spec.label=spec.label.fromFirstOccurrenceOf("Ambient ",false,false);
                    if(index==11) spec.label=spec.label.fromFirstOccurrenceOf("Krell ",false,false);
                    if(index==8) spec.label=spec.label.fromFirstOccurrenceOf("Arpeggio ",false,false);
                    if(index==9) spec.label=spec.label.fromFirstOccurrenceOf("Drums ",false,false);
                    if(index==4) spec.label=spec.label.fromFirstOccurrenceOf("Master ",false,false);
                }
                include|=spec.id=="fxEnabled";
                include|=spec.id==(index==2?"bassFX":index==3?"padFX":index==8?"arpFX":index==9?"drumsFX":index==11?"krellFX":"masterFX");
                if(index==4) include|=spec.id=="master"||spec.id=="limiter";
            }
            if(!include) continue;
            if(index==0&&spec.group==1&&generatorStart<0) generatorStart=int(controls.size());
            auto control=std::make_unique<Control>(processor,spec);
            control->setComponentID(spec.id);addAndMakeVisible(*control);
            controls.push_back({std::move(control),itemModule});
        }
        if(index==0){auto guitar=std::find_if(controls.begin(),controls.end(),[](const auto& item){return item.control->getComponentID().startsWith("guitar");});if(guitar!=controls.end()){const int count=int(controls.end()-guitar);std::rotate(controls.begin()+2,guitar,controls.end());generatorStart+=count;}}
        if(isFX()) for(int k=0;k<6;++k) {
            addAndMakeVisible(modules[size_t(k)]);
            modules[size_t(k)].onClick=[this,k] {
                selectedModule=slotModule(k);updateOrder();
                if(layoutChanged) layoutChanged();
            };
            left[size_t(k)].setButtonText("<");right[size_t(k)].setButtonText(">");
            addAndMakeVisible(left[size_t(k)]);addAndMakeVisible(right[size_t(k)]);
            left[size_t(k)].onClick=[this,k]{swapSlots(k,k-1);};
            right[size_t(k)].onClick=[this,k]{swapSlots(k,k+1);};
            left[size_t(k)].setEnabled(k>0);right[size_t(k)].setEnabled(k<5);
        }
        if(index==6) {
            const char* names[]{"KICK","SNARE","HI-HAT","AUX"};
            for(int lane=0;lane<4;++lane) {
                auto& label=laneLabels[size_t(lane)];ProVisual::tint(label,ProVisual::tracks[size_t(lane+3)]);label.setButtonText(names[lane]);label.onClick=[this,lane]{selectedDrum=lane;for(int i=0;i<4;++i)laneLabels[size_t(i)].setToggleState(i==lane,juce::dontSendNotification);if(layoutChanged)layoutChanged();};addAndMakeVisible(label);
                for(int step=0;step<16;++step) {auto& button=steps[size_t(lane*16+step)];
                    button.setComponentID("grid-"+juce::String(lane)+"-"+juce::String(step));
                    button.setTooltip("Left: toggle note | Right: velocity / MIDI Learn");
                    button.onClick=[this,lane,step]{editStep(lane,bank*16+step,false);};
                    button.setTriggeredOnMouseDown(false);button.addMouseListener(this,false);addAndMakeVisible(button);
                }
            }
            for(int k=0;k<4;++k){banks[size_t(k)].setButtonText(juce::String(k*16+1)+" - "+juce::String(k*16+16));banks[size_t(k)].onClick=[this,k]{bank=k;refreshGrid();};addAndMakeVisible(banks[size_t(k)]);}
            clearGrid.onClick=[this]{drumPreset(0);};fourFloor.onClick=[this]{drumPreset(1);};breakbeat.onClick=[this]{drumPreset(2);};
            addAndMakeVisible(clearGrid);addAndMakeVisible(fourFloor);addAndMakeVisible(breakbeat);
            gridHelp.setText("PATTERN / 4 x 16 STEPS     |     Bright = accent     |     White = playhead",juce::dontSendNotification);addAndMakeVisible(gridHelp);
            refreshGrid();
        }
        updateOrder();
    }
    void mouseDown(const juce::MouseEvent& event) override {
        if(index!=6||!event.mods.isRightButtonDown())return;
        for(int i=0;i<64;++i)if(event.eventComponent==&steps[size_t(i)]){editStep(i/16,bank*16+i%16,true);return;}
    }
    void refreshGrid() {
        if(index!=6)return;
        const int length=int(processor.state.getRawParameterValue("drumLength")->load());
        const int playhead=processor.drumStep.load();
        const auto* colours=ProVisual::tracks.data()+3;
        for(int lane=0;lane<4;++lane)ProVisual::tint(laneLabels[size_t(lane)],colours[lane]);
        for(int k=0;k<4;++k)banks[size_t(k)].setToggleState(k==bank,juce::dontSendNotification);
        for(int lane=0;lane<4;++lane)for(int k=0;k<16;++k){auto& button=steps[size_t(lane*16+k)];const int step=bank*16+k;
            const float velocity=processor.state.getRawParameterValue("drum"+juce::String(lane)+"_step"+juce::String(step))->load();
            button.setButtonText(juce::String(step+1));button.setEnabled(step<length);
            const auto colour=velocity>0?colours[lane].withMultipliedBrightness(.45f+velocity/200):juce::Colour(0xff303643);
            button.setColour(juce::TextButton::buttonColourId,playhead==step?juce::Colours::white:colour);
            button.setColour(juce::TextButton::textColourOffId,velocity>0||playhead==step?juce::Colours::black:text);
        }
    }
    void updateOrder() {
        if(!isFX()) return;
        const char* names[]{"DRIVE / COMP","CHORUS","DELAY","GRANULAR","REVERB","WIDTH"};
        for(int k=0;k<6;++k) {
            const int id=slotModule(k);
            modules[size_t(k)].setButtonText(juce::String(k+1)+"  "+names[id]);
            modules[size_t(k)].setToggleState(id==selectedModule,juce::dontSendNotification);
        }
    }
    int arrange(int width,bool apply) {
        const int columns=std::max(3,width/155),cell=width/columns;
        int top=isFX()?130:index==6?332:50,count=0;
        for(size_t k=0;k<controls.size();++k) {
            if(int(k)==generatorStart) {
                top+=((count+columns-1)/columns)*148;
                if(apply) generatorHeading.setBounds(12,top,width-24,36);
                top+=40;count=0;
            }
            auto& item=controls[k];bool visible=index==6?(item.module<0||item.module==selectedDrum):(!isFX()||item.module<0||item.module==selectedModule);
            if(item.control->getComponentID().startsWith("guitar"))visible&=int(processor.state.getRawParameterValue("model")->load())==3;
            if(apply) item.control->setVisible(visible);
            if(!visible) continue;
            if(apply) item.control->setBounds((count%columns)*cell,top+(count/columns)*148,cell,142);
            ++count;
        }
        return top+((count+columns-1)/columns)*148;
    }
    int requiredHeight(int width) { return arrange(width,false); }
    void resized() override {
        description.setBounds(10,0,getWidth()-20,44);
        if(isFX()) {
            const int w=getWidth()/6;
            for(int k=0;k<6;++k) {
                const int x=k*w;
                modules[size_t(k)].setBounds(x+2,50,w-4,34);
                left[size_t(k)].setBounds(x+10,88,(w-24)/2,25);
                right[size_t(k)].setBounds(x+14+(w-24)/2,88,(w-24)/2,25);
            }
        }
        if(index==6) {
            gridHelp.setBounds(12,48,getWidth()-24,28);
            for(int k=0;k<4;++k)banks[size_t(k)].setBounds(92+k*92,82,86,28);
            clearGrid.setBounds(getWidth()-310,82,78,28);fourFloor.setBounds(getWidth()-226,82,108,28);breakbeat.setBounds(getWidth()-112,82,104,28);
            const int cell=(getWidth()-100)/16;
            for(int lane=0;lane<4;++lane){laneLabels[size_t(lane)].setBounds(4,122+lane*48,88,42);
                for(int k=0;k<16;++k)steps[size_t(lane*16+k)].setBounds(94+k*cell,122+lane*48,cell-4,40);}
        }
        arrange(getWidth(),true);
    }
};

BassAmbientEditor::BassAmbientEditor(BassAmbientProcessor& p):AudioProcessorEditor(p),processor(p),theme(std::make_unique<Theme>()) {
    setLookAndFeel(theme.get());
    const char* names[]{"BASS","AMBIENT","ARPEGGIO","DRUMS","FX","GENERATE","COMPOSER","MIXER","KRELL"};
    for(int k=0;k<9;++k) {
        ProVisual::tint(tabs[size_t(k)],ProVisual::pages[size_t(k==8?7:std::min(k,6))]);tabs[size_t(k)].setButtonText(names[k]);tabs[size_t(k)].onClick=[this,k]{selectPage(k);};addAndMakeVisible(tabs[size_t(k)]);
    }
    for(int k=0;k<12;++k) {
        pages[size_t(k)]=std::make_unique<Page>(processor,k);
        const int identity[]{0,1,0,1,4,2,3,5,2,3,7,7};ProVisual::tint(*pages[size_t(k)],ProVisual::pages[size_t(identity[k])]);
        pages[size_t(k)]->layoutChanged=[this]{resized();};
    }
    composerView=std::make_unique<ComposerView>(processor);
    mixerView=std::make_unique<MixerView>(processor);transportView=std::make_unique<TransportView>(processor);addAndMakeVisible(*transportView);harmonyView=std::make_unique<HarmonyView>(processor);addAndMakeVisible(*harmonyView);
    for(int k=0;k<5;++k){moduleLeds[size_t(k)]=std::make_unique<SignalLeds>(processor,k==4?7:k,k==4?7:k==3?6:k);addAndMakeVisible(*moduleLeds[size_t(k)]);}

    rackSelector.addItemList({"BASS rack","AMBIENT rack","MASTER rack","ARPEGGIO rack","DRUMS rack","KRELL rack"},1);
    rackSelector.setComponentID("rackSelector");rackSelector.setSelectedId(1,juce::dontSendNotification);
    rackSelector.onChange=[this] { selectedRack=rackSelector.getSelectedId()-1;selectPage(4); };
    addAndMakeVisible(rackSelector);
    addAndMakeVisible(viewport);viewport.setScrollBarsShown(true,false);viewport.setScrollOnDragMode(juce::Viewport::ScrollOnDragMode::never);
    presets.addItemList({"Italo Analog","Acid Machine","808 Foundation","Lush Flow","Krell Dust","Bass + Atmosphere","Deep House PRO","Stereo Glass Arp","Krell Organism PRO","Bass Guitar Groove","Rock Bass Guitar","Metal Bass Guitar","Punk Bass Guitar","Blues Bass Guitar","Independent Krell"},1);
    presets.setText("Factory scenes",juce::dontSendNotification);presets.onChange=[this]{processor.applyPreset(presets.getSelectedId()-1);};addAndMakeVisible(presets);
    mutate.onClick=[this] {
        if(auto* seed=processor.state.getParameter("seed")) { auto next=juce::Random::getSystemRandom().nextInt(65535)+1;seed->beginChangeGesture();seed->setValueNotifyingHost(seed->convertTo0to1(float(next)));seed->endChangeGesture(); }
    };addAndMakeVisible(mutate);
    fxAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.state,"fxEnabled",fxPower);addAndMakeVisible(fxPower);
    panic.onClick=[this] {processor.setParameterValue("autoPlay",0);processor.setParameterValue("composerPlaying",0);processor.requestPanic();};addAndMakeVisible(panic);addAndMakeVisible(status);
    status.setColour(juce::Label::textColourId,muted);
    // setResizeLimits may invoke resized(): pages must exist before applying it.
    setResizable(true,true);setResizeLimits(1000,680,1920,1400);
    setSize(1200,820);selectPage(0);timerCallback();startTimerHz(20);
}
BassAmbientEditor::~BassAmbientEditor() { stopTimer();viewport.setViewedComponent(nullptr,false);setLookAndFeel(nullptr); }
juce::Component& BassAmbientEditor::currentPage() { if(selected==8)return *pages[10];if(selected==7)return *mixerView;if(selected==6)return *composerView;return *pages[size_t(selected==4?(selectedRack<3?2+selectedRack:selectedRack==5?11:5+selectedRack):selected==2?5:selected==3?6:selected==5?7:selected)]; }
void BassAmbientEditor::selectPage(int index) {
    selected=index;for(int k=0;k<9;++k) tabs[size_t(k)].setToggleState(k==index,juce::dontSendNotification);
    rackSelector.setVisible(index==4);
    viewport.setViewedComponent(&currentPage(),false);resized();viewport.setViewPosition(0,0);
}
void BassAmbientEditor::paint(juce::Graphics& g) {
    JerzyAudioUI::paintChassis(g,getLocalBounds().toFloat(),proTheme());
    g.setColour(text);g.setFont(juce::Font(juce::FontOptions(22).withStyle("Bold")));
    g.drawText("JERZY  /  BASS AMBIENT PRO",20,12,400,35,juce::Justification::centredLeft);
    g.setColour(muted);g.setFont(juce::Font(juce::FontOptions(12)));
    g.drawText("0.6.0 DEVELOPMENT  |  MODULAR GENERATIVE GROOVEBOX",22,47,460,20,juce::Justification::centredLeft);
}
void BassAmbientEditor::resized() {
    presets.setBounds(getWidth()-460,22,220,34);mutate.setBounds(getWidth()-228,22,100,34);panic.setBounds(getWidth()-116,22,96,34);
    int tabWidth=(getWidth()-40)/9;for(int k=0;k<9;++k) tabs[size_t(k)].setBounds(20+k*tabWidth,80,tabWidth-6,38);
    for(int k=0;k<5;++k)moduleLeds[size_t(k)]->setBounds(20+(k==4?8:k)*tabWidth,120,tabWidth-6,18);
    transportView->setBounds(20,142,getWidth()-40,34);
    harmonyView->setBounds(20,178,getWidth()-40,28);
    rackSelector.setBounds(getWidth()-232,212,212,32);
    fxPower.setBounds(getWidth()-590,16,124,28);
    viewport.setBounds(16,selected==4?250:214,getWidth()-32,getHeight()-(selected==4?294:258));
    auto& page=currentPage();int w=viewport.getWidth()-18;page.setSize(w,selected==7?mixerView->requiredHeight():selected==6?composerView->requiredHeight():static_cast<Page&>(page).requiredHeight(w));page.resized();
    status.setBounds(20,getHeight()-36,getWidth()-40,26);
}
void BassAmbientEditor::timerCallback() {
    const int model=int(processor.state.getRawParameterValue("model")->load());if(model!=lastBassModel){lastBassModel=model;if(selected==0)resized();}
    for(auto& led:moduleLeds)led->repaint();
    int root=processor.rootDisplay.load();float level=processor.peak.load();
    juce::String note=root<0?"--":juce::MidiMessage::getMidiNoteName(root,true,true,3);
    juce::String db=level<.00001f?"-inf":juce::String(juce::Decibels::gainToDecibels(level),1);
    const juce::String learn=processor.isLearningMidi()?"    |    MIDI LEARN: move a CC controller":"";
    status.setText("MIDI root: "+note+"    |    Peak: "+db+" dBFS    |    FX buffers: "+juce::String(double(processor.fxBytes.load())/1048576,1)+" MB    |    Right-click: default / Ctrl+right: MIDI Learn"+learn,juce::dontSendNotification);
    for(int k=2;k<5;++k) pages[size_t(k)]->updateOrder();
    pages[8]->updateOrder();pages[9]->updateOrder();pages[11]->updateOrder();
    pages[6]->refreshGrid();
}
