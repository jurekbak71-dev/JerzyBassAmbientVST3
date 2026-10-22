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
        auto label=b.getButtonText();label=(label.startsWith("MODE:")||label.startsWith("GENERATOR:")||label.startsWith("NUTY"))?label:label=="ON"?(on?"ON":"OFF"):label+(on?" ON":" OFF");
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
        auto display=s.label;
        if(s.id=="bars")display="Długość frazy (takty)"_ui;if(s.id=="style")display="Styl gry";if(s.id=="division")display="Podział rytmu"_ui;
        if(s.id=="arpPulses")display="Aktywne kroki";if(s.id=="arpSteps")display="Długość rytmu"_ui;
        label.setText(display,juce::dontSendNotification);label.setJustificationType(juce::Justification::centred);
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
            slider.updateText();slider.onValueChange=[this]{if(auto* parent=getParentComponent())parent->repaint();};
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
        juce::ignoreUnused(g);
    }
    void resized() override {
        auto bounds=getLocalBounds().reduced(getHeight()<120?6:10);label.setBounds(bounds.removeFromTop(getHeight()<120?24:30));
        if(spec.kind==1) toggle.setBounds(bounds.withSizeKeepingCentre(80,32));
        else if(spec.kind==3) choice.setBounds(bounds.withSizeKeepingCentre(bounds.getWidth(),32));
        else slider.setBounds(bounds);
    }
};

class BassAmbientEditor::Page final : public juce::Component,public juce::DragAndDropContainer,public juce::DragAndDropTarget {
    BassAmbientProcessor& processor;
    int index, selectedModule=0, section=0;bool expanded=false;
    struct GroupBox{juce::Rectangle<int> bounds;juce::String name;};std::vector<GroupBox> groupBoxes;
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
    int bank=0,selectedDrum=0,dragSlot=-1;
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
        const juce::String descriptions[]{
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
            control->setComponentID(spec.id);control->getProperties().set("parameterGroup",spec.group);addAndMakeVisible(*control);
            controls.push_back({std::move(control),itemModule});
        }
        if(index==0){auto guitar=std::find_if(controls.begin(),controls.end(),[](const auto& item){return item.control->getComponentID().startsWith("guitar");});if(guitar!=controls.end()){const int count=int(controls.end()-guitar);std::rotate(controls.begin()+2,guitar,controls.end());generatorStart+=count;}}
        if(isFX()) for(int k=0;k<6;++k) {
            addAndMakeVisible(modules[size_t(k)]);modules[size_t(k)].addMouseListener(this,false);modules[size_t(k)].setTooltip("Przeciągnij, aby zmienić kolejność; prawy klik: przesuń moduł"_ui);
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
            const juce::String names[]{"KICK","SNARE","HI-HAT","AUX"};
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
    void mouseDrag(const juce::MouseEvent& e) override {if(!isFX()||e.getDistanceFromDragStart()<8)return;for(int k=0;k<6;++k)if(e.eventComponent==&modules[size_t(k)]){startDragging("fx-slot:"+juce::String(index)+":"+juce::String(k),&modules[size_t(k)]);return;}}
    bool isInterestedInDragSource(const SourceDetails& d) override{return isFX()&&d.description.toString().startsWith("fx-slot:"+juce::String(index)+":");}
    void itemDragMove(const SourceDetails& d) override{dragSlot=juce::jlimit(0,5,d.localPosition.x/std::max(1,getWidth()/6));repaint();}
    void itemDragExit(const SourceDetails&) override{dragSlot=-1;repaint();}
    void itemDropped(const SourceDetails& d) override{if(!isInterestedInDragSource(d)||d.localPosition.y>95)return;int a=d.description.toString().fromLastOccurrenceOf(":",false,false).getIntValue(),b=juce::jlimit(0,5,d.localPosition.x/std::max(1,getWidth()/6));if(a<0||a>5)return;while(a<b){swapSlots(a,a+1);++a;}while(a>b){swapSlots(a,a-1);--a;}dragSlot=-1;repaint();}
    void mouseDown(const juce::MouseEvent& event) override {
        if(isFX()&&event.mods.isRightButtonDown()){for(int k=0;k<6;++k)if(event.eventComponent==&modules[size_t(k)]){juce::PopupMenu menu;menu.addItem(1,"Przesuń w lewo"_ui,k>0);menu.addItem(2,"Przesuń w prawo"_ui,k<5);juce::Component::SafePointer<Page> safe(this);menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&modules[size_t(k)]),[safe,k](int result){if(safe&&result>0)safe->swapSlots(k,k+(result==1?-1:1));});return;}}
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
        const juce::String names[]{"DRIVE / COMP","CHORUS","DELAY","GRANULAR","REVERB","WIDTH"};
        for(int k=0;k<6;++k) {
            const int id=slotModule(k);
            modules[size_t(k)].setButtonText(juce::String(k+1)+"  "+names[id]);
            modules[size_t(k)].setToggleState(id==selectedModule,juce::dontSendNotification);
        }
    }
    void applySoundPreset(int choice){auto set=[&](const char* id,float value){processor.setParameterValue(id,value);};
        if(index==0){set("model",3);set("guitarArticulation",choice==0?0:1);set("guitarTone",choice==0?.55f:choice==1?.78f:.4f);set("guitarDamping",choice==2?.65f:.2f);set("guitarDecay",choice==2?.8f:2.5f);set("drive",choice==1?.28f:.08f);set("cutoff",choice==1?5500:3200);}
        else if(index==1){set("engine1",choice==1?1:0);set("engine2",choice==2?4:2);set("padBlend",choice==2?.7f:.5f);set("padAttack",choice==1?1.f:3.f);set("padRelease",choice==2?9.f:5.f);set("motion",choice==2?.7f:.3f);set("padCutoff",choice==1?1700:3000);}
        else if(index==5){set("arpModel",choice==0?1:choice==1?0:3);set("arpCutoff",choice==1?2800:5500);set("arpDecay",choice==1?.25f:.6f);set("arpAttack",.003f);set("arpSustain",.15f);set("arpRelease",choice==2?.8f:.2f);}
        else if(index==6){const auto prefix="drum"+juce::String(selectedDrum)+"_";processor.setParameterValue(prefix+"drive",choice==1?.4f:.08f);processor.setParameterValue(prefix+"tone",choice==1?.7f:.4f);processor.setParameterValue(prefix+"decay",choice==2?.12f:.3f);processor.setParameterValue(prefix+"cutoff",choice==2?3500:12000);}
        else if(index==10){set("krellEngine",float(choice));set("krellAttack",choice==1?2.f:.7f);set("krellRelease",choice==2?5.f:2.f);set("krellTimbre",choice==1?.8f:.5f);set("krellCutoff",choice==1?2300:4200);}
    }
    void setSection(int value,bool advanced){section=value;expanded=advanced;resized();}
    void paint(juce::Graphics& g) override {
        g.fillAll(juce::Colour(0xff111923));if(dragSlot>=0){g.setColour(accent);g.drawRect(dragSlot*(getWidth()/6),0,getWidth()/6,42,2);}for(const auto& box:groupBoxes){g.setColour(juce::Colour(0xff19232f));g.fillRoundedRectangle(box.bounds.toFloat(),6);g.setColour(juce::Colour(0xff354657));g.drawRoundedRectangle(box.bounds.toFloat(),6,1);g.setColour(muted);g.setFont(11);g.drawText(box.name,box.bounds.reduced(12).removeFromTop(18),juce::Justification::centredLeft);
            if(box.name.startsWith("OBWIEDNIA")){const auto p=processor.readParameters();double a=index==0?p.bassAttack:index==1?p.padAttack:p.arpAttack,d=index==0?p.bassDecay:index==1?p.padDecay:p.arpDecay,s=index==0?p.bassSustain:index==1?p.padSustain:p.arpSustain,r=index==0?p.bassRelease:index==1?p.padRelease:p.arpRelease;const float x=float(box.bounds.getX()+14),y=float(box.bounds.getY()+50),w=float(box.bounds.getWidth()/3-28),h=60;const double sum=a+d+r+1;juce::Path path;path.startNewSubPath(x,y+h);path.lineTo(x+float(a/sum)*w,y);path.lineTo(x+float((a+d)/sum)*w,y+float(1-s)*h);path.lineTo(x+float((a+d+1)/sum)*w,y+float(1-s)*h);path.lineTo(x+w,y+h);g.setColour(ProVisual::pages[size_t(index==0?0:index==1?1:2)]);g.strokePath(path,juce::PathStrokeType(2));}
            if(box.name=="RYTM / PODGLĄD"_ui){auto p=processor.readParameters();int w=(box.bounds.getWidth()-24)/16;for(int k=0;k<16;++k){bool on=index==0?!jerzy::makeStep(p,k,std::max(1,int(p.bars*4/jerzy::divisionBeats(p.division)))).rest:jerzy::makeArpEvent(p,k,nullptr,0,60).note>=0;g.setColour(on?ProVisual::pages[size_t(index==0?0:2)]:juce::Colour(0xff334657));g.fillRoundedRectangle(float(box.bounds.getX()+12+k*w),float(box.bounds.getY()+28),float(w-3),14.f,2);}}
        }
    }
    int arrange(int width,bool apply) {
        // Drums: keep the selected voice beside the sequencer, not below it.
        if(index==6) {
            const int side=std::min(360,width/3), leftWidth=width-side-12;
            const int globalColumns=std::max(3,leftWidth/145), globalCell=leftWidth/globalColumns;
            int globalCount=0,voiceCount=0;
            for(auto& item:controls) {
                const auto id=item.control->getComponentID();bool visible=item.module<0||item.module==selectedDrum;if(!expanded&&(id=="drums"||id=="drumLevel"||id.endsWith("_level")||id.endsWith("_pan")))visible=false;
                if(apply)item.control->setVisible(visible);
                if(!visible)continue;
                if(item.module<0) {
                    if(apply)item.control->setBounds((globalCount%globalColumns)*globalCell,252+(globalCount/globalColumns)*90,globalCell,86);
                    ++globalCount;
                } else {
                    if(apply)item.control->setBounds(leftWidth+12+(voiceCount%2)*(side/2),48+(voiceCount/2)*96,side/2,92);
                    ++voiceCount;
                }
            }
            return std::max(252+((globalCount+globalColumns-1)/globalColumns)*90,48+((voiceCount+1)/2)*96);
        }
        if(apply){groupBoxes.clear();for(auto& c:controls)c.control->setVisible(false);}
        struct Group{juce::String name;juce::StringArray ids;};std::vector<Group> groups;
        auto group=[&](const juce::String& name,const char* ids){groups.push_back({name,juce::StringArray::fromTokens(ids," ","")});};
        if(isFX()) {
            Group g{"USTAWIENIA EFEKTU",{}};for(auto& item:controls)if(item.module==selectedModule||item.module<0){auto id=item.control->getComponentID();if(id=="fxEnabled")continue;auto shortId=id.startsWith(prefix)?id.substring(prefix.length()):id;const auto essentials=juce::StringArray::fromTokens("fxDrive fxSaturation compThreshold compRatio compMakeup fxChorus chorusMix chorusRate chorusDepth chorusMode fxDelay delayMix feedback delayTone delayMode delayDivision delayType fxGrain grainSize grainMix grainPitch grainScatter fxReverb reverbMix reverbSize reverbDamp fxWidth width bassFX padFX masterFX arpFX drumsFX krellFX"," ","");if(expanded||essentials.contains(shortId)||item.module<0)g.ids.add(id);}groups.push_back(g);
        } else if(expanded) {
            Group g{"ZAAWANSOWANE — PARAMETRY SZCZEGÓŁOWE"_ui,{}};
            for(auto& item:controls){auto id=item.control->getComponentID();if(id=="scale"||id=="master"||id=="limiter"||id=="fxEnabled"||id=="internalTempo"||id=="harmonyRoot"||id=="harmonyLink"||id=="autoPlay"||id=="direct")continue;if(id=="bass"||id=="pad"||id=="arp"||id=="drums"||id=="krell")continue;int pg=int(item.control->getProperties()["parameterGroup"]);
                const juce::StringArray basic=juce::StringArray::fromTokens(index==0?(section==1?"style division bars density movement gate swing humanize":"model bassWave guitarArticulation guitarTone guitarDamping guitarDecay cutoff resonance filterEnv drive sub pulse glide bassAttack bassDecay bassSustain bassRelease"):index==1?"padMode engine1 engine2 chord padBlend motion evolution padCutoff padResonance spread padAttack padDecay padSustain padRelease":index==5?"arpModel arpPattern arpDivision arpOctaves arpSteps arpPulses arpGate arpSwing arpVelocity arpPitchRandom arpGateRandom arpRange arpCutoff arpResonance arpFilterEnv arpPanMode arpWidth arpAttack arpDecay arpSustain arpRelease":"krellEngine krellCycleMin krellCycleMax krellTimbre krellPitchRange krellStereo krellCutoff krellAttack krellRelease krellResonance"," ","");
                if(id=="seed"||(basic.contains(id)&&!(index==0&&id=="glide"&&int(processor.state.getRawParameterValue("model")->load())==3))||(index==0&&pg!=(section==1?1:0)))continue;g.ids.add(id);}groups.push_back(g);
        } else if(index==0&&section==1){
            group("STYL I FRAZA","style division bars");group("GRA I EKSPRESJA","density movement gate swing humanize");
        } else if(index==0){
            group("INSTRUMENT",int(processor.state.getRawParameterValue("model")->load())==3?"model guitarArticulation":"model bassWave");
            group("CHARAKTER",int(processor.state.getRawParameterValue("model")->load())==3?"guitarTone guitarDamping guitarDecay":"sub pulse glide");group("FILTR I DRIVE","cutoff resonance filterEnv drive");group("OBWIEDNIA AMP","bassAttack bassDecay bassSustain bassRelease");
        } else if(index==1){group("BRZMIENIE I ZACHOWANIE","padMode engine1 engine2 chord");group("SILNIKI I EWOLUCJA","padBlend motion evolution");group("FILTR I PRZESTRZEŃ"_ui,"padCutoff padResonance spread");group("OBWIEDNIA AMP","padAttack padDecay padSustain padRelease");
        } else if(index==5&&section==1){group("KIERUNEK I RYTM","arpPattern arpDivision arpOctaves arpSteps arpPulses");group("ARTYKULACJA","arpGate arpSwing arpVelocity");group("LOSOWOŚĆ"_ui,"arpPitchRandom arpGateRandom arpRange");
        } else if(index==5){group("SYNTEZATOR","arpModel");group("FILTR","arpCutoff arpResonance arpFilterEnv");group("STEREO","arpPanMode arpWidth");group("OBWIEDNIA AMP","arpAttack arpDecay arpSustain arpRelease");
        } else if(index==10){group("SILNIK SYNTEZY","krellEngine");group("EWOLUCJA","krellCycleMin krellCycleMax krellTimbre");group("WYSOKOŚĆ I PRZESTRZEŃ"_ui,"krellPitchRange krellStereo krellCutoff");group("ODDYCHANIE DŹWIĘKU"_ui,"krellAttack krellRelease krellResonance");
        } else if(index==7){group("HARMONIA — GRA BEZ MIDI"_ui,"autoHarmony harmonyBars chord");group("EWOLUCJA","evolve");}
        int y=isFX()?66:6;const int gap=10;
        for(size_t gi=0;gi<groups.size();++gi){auto& groupData=groups[gi];std::vector<Control*> list;
            for(const auto& id:groupData.ids)for(auto& item:controls)if(item.control->getComponentID()==id){if(id.startsWith("guitar")&&int(processor.state.getRawParameterValue("model")->load())!=3)continue;list.push_back(item.control.get());}
            if(list.empty())continue;
            bool side=!expanded&&!isFX()&&groups.size()>=4&&(gi==1||gi==2);
            int x=8,boxWidth=width-16;if(side){boxWidth=(width-26)/2;x=gi==1?8:18+boxWidth;}
            const bool choiceRow=!expanded&&gi==0&&!isFX();const int cols=expanded||isFX()?std::max(3,boxWidth/148):int(list.size());const int cell=boxWidth/std::max(1,cols);const int rowHeight=choiceRow?72:112;
            const int height=28+((int(list.size())+cols-1)/cols)*rowHeight;
            int graphWidth=groupData.name.startsWith("OBWIEDNIA")?boxWidth/3:0;
            const int controlCell=graphWidth?(boxWidth-graphWidth)/std::max(1,cols):cell;
            if(apply){groupBoxes.push_back({{x,y,boxWidth,height},groupData.name});for(size_t n=0;n<list.size();++n){list[n]->setVisible(true);list[n]->setBounds(x+graphWidth+int(n%size_t(cols))*controlCell+3,y+28+int(n/size_t(cols))*rowHeight,controlCell-6,rowHeight-4);}}
            if(!side||gi==2)y+=height+gap;
            if(!expanded&&section==1&&(index==0||index==5)&&gi==0){if(apply)groupBoxes.push_back({{8,y,width-16,50},"RYTM / PODGLĄD"_ui});y+=60;}

        }
        if(apply)repaint();return y+6;
    }
    int requiredHeight(int width) { return arrange(width,false); }
    void resized() override {
        description.setBounds(10,0,getWidth()-20,44);description.setVisible(index==6);generatorHeading.setVisible(false);
        if(isFX()) {
            const int w=getWidth()/6;
            for(int k=0;k<6;++k) {
                const int x=k*w;
                modules[size_t(k)].setBounds(x+2,4,w-4,34);
                left[size_t(k)].setVisible(false);
                right[size_t(k)].setVisible(false);
            }
        }
        if(index==6) {
            const int gridWidth=getWidth()-std::min(360,getWidth()/3)-12;
            gridHelp.setText("PATTERN 64 STEPS | Click instrument to edit at right",juce::dontSendNotification);
            gridHelp.setBounds(8,44,gridWidth-16,24);
            const int bankWidth=(gridWidth-90)/4;
            for(int k=0;k<4;++k)banks[size_t(k)].setBounds(86+k*bankWidth,72,bankWidth-4,26);
            clearGrid.setBounds(8,218,78,28);fourFloor.setBounds(92,218,108,28);breakbeat.setBounds(206,218,104,28);
            const int cell=(gridWidth-88)/16;
            for(int lane=0;lane<4;++lane){laneLabels[size_t(lane)].setBounds(4,104+lane*28,78,26);
                for(int k=0;k<16;++k)steps[size_t(lane*16+k)].setBounds(86+k*cell,104+lane*28,cell-2,26);}

        }
        arrange(getWidth(),true);
    }
};


class BassAmbientEditor::Shell final : public juce::Component {
public:
    bool showModules=true;
    void paint(juce::Graphics& g) override {
        g.fillAll(juce::Colour(0xff121a24));g.setColour(juce::Colour(0xff344353));g.drawHorizontalLine(57,0,float(getWidth()));g.drawHorizontalLine(104,0,float(getWidth()));g.drawHorizontalLine(getHeight()-38,0,float(getWidth()));
        g.setColour(text);g.setFont(juce::Font(juce::FontOptions(17).withStyle("Bold")));g.drawText("JERZY BASS AMBIENT PRO",18,10,240,24,juce::Justification::centredLeft);
        g.setColour(muted);g.setFont(9);g.drawText(juce::String(JucePlugin_VersionString)+" | MODULAR GENERATIVE GROOVEBOX",18,34,240,16,juce::Justification::centredLeft);
        if(!showModules)return;g.setColour(juce::Colour(0xff18232e));g.fillRoundedRectangle(12,116,150,float(getHeight()-166),6);
        g.setColour(muted);g.setFont(10);g.drawText("MODUŁY / POWER"_ui,24,122,130,22,juce::Justification::centredLeft);
    }
};
class BassAmbientEditor::SettingsPanel final : public juce::Component,private juce::Timer {
    BassAmbientEditor& editor;BassAmbientProcessor& p;int kind;
    std::vector<std::unique_ptr<juce::Component>> fields;std::vector<std::unique_ptr<juce::Label>> labels;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> choices;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> buttons;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sliders;
    juce::Label info;juce::TextButton close{"ZAMKNIJ"};
    void label(const juce::String& name){auto l=std::make_unique<juce::Label>();l->setText(name,juce::dontSendNotification);addAndMakeVisible(*l);labels.push_back(std::move(l));}
    void choice(const juce::String& name,const char* id,const juce::StringArray& values){label(name);auto c=std::make_unique<juce::ComboBox>();c->addItemList(values,1);c->setComponentID(id);addAndMakeVisible(*c);choices.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.state,id,*c));fields.push_back(std::move(c));}
    void value(const juce::String& name,const char* id){label(name);auto c=std::make_unique<juce::Slider>();c->setSliderStyle(juce::Slider::LinearHorizontal);c->setTextBoxStyle(juce::Slider::TextBoxRight,false,70,24);c->setComponentID(id);addAndMakeVisible(*c);sliders.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.state,id,*c));fields.push_back(std::move(c));}
    void toggle(const juce::String& name,const char* id){label(name);auto c=std::make_unique<juce::ToggleButton>(name);c->setComponentID(id);addAndMakeVisible(*c);buttons.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.state,id,*c));fields.push_back(std::move(c));}
public:
    SettingsPanel(BassAmbientEditor& e,int type):editor(e),p(e.processor),kind(type){
        if(kind==0){choice("Źródło zegara"_ui,"clockSource",{"INTERNAL","HOST / FL STUDIO","MIDI CLOCK"});choice("Metrum frazy basu","meter",{"Z hosta","Własne"_ui});value("Licznik metrum","numerator");choice("Mianownik metrum","denominator",{"2","4","8","16"});value("Wspólny seed (zgodność)"_ui,"seed");
            label("Skala interfejsu");auto c=std::make_unique<juce::ComboBox>();c->addItemList({"Dopasuj do ekranu","100%","85%","75%"},1);c->setSelectedId(editor.uiScale==0?1:editor.uiScale==1?2:editor.uiScale==.85f?3:4);c->onChange=[this,raw=c.get()]{const float scales[]{0,1,.85f,.75f};editor.uiScale=scales[raw->getSelectedId()-1];p.editorScale=editor.uiScale;editor.resized();};addAndMakeVisible(*c);fields.push_back(std::move(c));
        }else if(kind==1){toggle("Osobne wyjścia audio"_ui,"separateOutputs");info.setText("FL Studio: wrapper → Processing → Connections.\nBASS / PAD / ARP / KICK / SNARE / HI-HAT / AUX / KRELL\nPowroty: DELAY RETURN oraz REVERB RETURN.\nNiepodłączone wyjścia pozostają na MAIN."_ui,juce::dontSendNotification);
        }else if(kind==2){value("Feedback","sendFeedback");choice("Podział delaya"_ui,"sendDivision",{"1/4","1/8","1/16","1/32","1/8T","1/16T","1/16D"});value("Wielkość pomieszczenia"_ui,"sendRoom");
        }else{startTimerHz(10);timerCallback();}
        addAndMakeVisible(info);addAndMakeVisible(close);close.onClick=[this]{if(editor.dialog){editor.dialog->exitModalState(0);editor.dialog->setVisible(false);}};setSize(650,kind==0?400:kind==2?280:310);
    }
    void timerCallback() override {const auto q=p.readParameters();juce::String t="Źródło: "_ui+juce::String(q.composerEnabled?"ARANŻER"_ui:q.autoPlay?"JAM":q.modularMidi?"MIDI — kanały instrumentów"_ui:"MIDI — zgodność"_ui)+"\nZegar: "+juce::String(p.clockStatus.load()>=2?"BRAK SYGNAŁU / HOST ZATRZYMANY"_ui:"OK")+"  "+juce::String(p.tempoDisplay.load(),1)+" BPM\n";for(int i=0;i<8;++i)t+=juce::String(jerzy::composer::trackNames[i])+": IN "+(p.inputPulse[size_t(i)]>0?"●"_ui:"—"_ui)+"   ACT "+(p.activePulse[size_t(i)]>0?"●"_ui:"—"_ui)+"   OUT "+juce::String(juce::Decibels::gainToDecibels(std::max(.000001f,p.channelPeak[size_t(i)].load())),1)+" dBFS\n";t+="FX buffers: "+juce::String(double(p.fxBytes.load())/1048576,1)+" MB";info.setText(t,juce::dontSendNotification);}
    void resized() override {for(size_t i=0;i<fields.size();++i){int col=int(i%2),row=int(i/2);labels[i]->setBounds(18+col*310,12+row*82,295,24);fields[i]->setBounds(18+col*310,38+row*82,295,32);}info.setBounds(18,kind==1?65:kind==3?8:350,614,kind==3?250:140);info.setVisible(kind==1||kind==3);close.setBounds(getWidth()-138,getHeight()-44,120,30);}
    void paint(juce::Graphics& g) override {g.fillAll(juce::Colour(0xff19232f));}
};
namespace {class ProDialog final : public juce::DialogWindow{public:ProDialog(const juce::String& name,juce::Component& content):DialogWindow(name,juce::Colour(0xff19232f),true){setUsingNativeTitleBar(true);setContentNonOwned(&content,true);centreWithSize(content.getWidth(),content.getHeight());}void closeButtonPressed() override {exitModalState(0);setVisible(false);}};}
BassAmbientEditor::BassAmbientEditor(BassAmbientProcessor& p):AudioProcessorEditor(p),processor(p),theme(std::make_unique<Theme>()),shell(std::make_unique<Shell>()) {
    uiScale=processor.editorScale.load();setLookAndFeel(theme.get());addAndMakeVisible(*shell);
    const juce::String names[]{"BASS","AMBIENT","ARP","DRUMS","KRELL"};const char* ids[]{"bass","pad","arp","drums","krell"};
    for(int k=0;k<5;++k){tabs[size_t(k)].setButtonText(names[k]);tabs[size_t(k)].setComponentID("module-"+juce::String(k));tabs[size_t(k)].onClick=[this,k]{chooseModule(k);};shell->addAndMakeVisible(tabs[size_t(k)]);ProVisual::tint(tabs[size_t(k)],ProVisual::pages[size_t(k==4?7:k)]);shell->addAndMakeVisible(modulePower[size_t(k)]);modulePower[size_t(k)].setButtonText("ON");ProVisual::tint(modulePower[size_t(k)],ProVisual::pages[size_t(k==4?7:k)]);powerAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.state,ids[k],modulePower[size_t(k)]));moduleLeds[size_t(k)]=std::make_unique<SignalLeds>(p,k==4?7:k==3?3:k,k==4?7:k==3?6:k);shell->addAndMakeVisible(*moduleLeds[size_t(k)]);}
    const juce::String nav[]{"INSTRUMENTY","ARANŻER"_ui,"MIKSER"};for(int k=0;k<3;++k){mainTabs[size_t(k)].setButtonText(nav[k]);mainTabs[size_t(k)].setComponentID("nav-"+juce::String(k));mainTabs[size_t(k)].onClick=[this,k]{if(k==0)chooseModule(selectedModule);else selectPage(k==1?6:7);};shell->addAndMakeVisible(mainTabs[size_t(k)]);}
    const juce::String sections[]{"BRZMIENIE","GENERATOR","EFEKTY"};for(int k=0;k<3;++k){sectionTabs[size_t(k)].setButtonText(sections[k]);sectionTabs[size_t(k)].setComponentID("section-"+juce::String(k));sectionTabs[size_t(k)].onClick=[this,k]{chooseSection(k);};shell->addAndMakeVisible(sectionTabs[size_t(k)]);}
    for(int k=0;k<12;++k){pages[size_t(k)]=std::make_unique<Page>(processor,k);const int identity[]{0,1,0,1,4,2,3,5,2,3,7,7};ProVisual::tint(*pages[size_t(k)],ProVisual::pages[size_t(identity[k])]);pages[size_t(k)]->layoutChanged=[this]{resized();};}
    composerView=std::make_unique<ComposerView>(p);composerView->setLookAndFeel(theme.get());mixerView=std::make_unique<MixerView>(p);transportView=std::make_unique<TransportView>(p);harmonyView=std::make_unique<HarmonyView>(p);
    shell->addAndMakeVisible(*transportView);shell->addAndMakeVisible(*harmonyView);shell->addAndMakeVisible(viewport);viewport.setScrollBarsShown(true,false);viewport.setScrollOnDragMode(juce::Viewport::ScrollOnDragMode::never);
    presets.addItemList({"Italo Analog","Acid Machine","808 Foundation","Lush Flow","Krell Dust","Bass + Atmosphere","Deep House PRO","Stereo Glass Arp","Krell Organism PRO","Bass Guitar Groove","Rock Bass Guitar","Metal Bass Guitar","Punk Bass Guitar","Blues Bass Guitar","Independent Krell"},1);presets.setText("Preset całości"_ui,juce::dontSendNotification);presets.onChange=[this]{processor.applyPreset(presets.getSelectedId()-1);};shell->addAndMakeVisible(presets);
    source.addItemList({"MIDI","JAM","ARANŻER"_ui,"MIDI — zgodność"_ui},1);source.setComponentID("performance-source");source.onChange=[this]{if(!updating)setSource(source.getSelectedId());};shell->addAndMakeVisible(source);shell->addAndMakeVisible(localPreset);localPreset.setComponentID("instrument-preset");localPreset.onChange=[this]{if(localPreset.getSelectedId()>0){localPresetIds[size_t(selectedModule)]=localPreset.getSelectedId();const int map[]{0,1,5,6,10};pages[size_t(map[selectedModule])]->applySoundPreset(localPreset.getSelectedId()-1);}};fxGlobalAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.state,"fxEnabled",fxGlobal);shell->addAndMakeVisible(fxGlobal);
    performance.setComponentID("instrument-performance");shell->addAndMakeVisible(performance);modeLabel.setText("ODTWARZANIE",juce::dontSendNotification);shell->addAndMakeVisible(modeLabel);
    variant.onClick=[this]{makeVariant();};undo.onClick=[this]{for(const auto& item:variantUndo)processor.setParameterValue(item.first,item.second);variantUndo.clear();undo.setEnabled(false);};undo.setEnabled(false);
    shell->addAndMakeVisible(jamCapture);jamCapture.setComponentID("jam-capture");jamCapture.onClick=[this]{auto patch=processor.readParameters();int scene=-1;processor.editComposer([&](auto& d){if(d.scenes.size()>=jerzy::composer::maxScenes)return;jerzy::composer::Scene s;s.id=d.nextId++;s.name="JAM "+std::to_string(s.id);s.patch=patch;s.bars=std::max(1,patch.bars);s.chords={{patch.harmonyRoot,patch.chord}};const bool enabled[]{patch.bass,patch.pad,patch.arp,patch.drums,patch.drums,patch.drums,patch.drums,patch.krell};for(int t=0;t<8;++t){s.clips[size_t(t)].generated=true;s.clips[size_t(t)].enabled=enabled[t];}d.scenes.push_back(s);scene=int(d.scenes.size()-1);});if(scene>=0){processor.setParameterValue("composerScene",float(scene+1));composerView->focusScene(scene);selectPage(6);}};
    advanced.onClick=[this]{expanded=!expanded;resized();};panic.onClick=[this]{processor.transport(false);};exportButton.onClick=[this]{composerView->showExport();};settings.onClick=[this]{showSettings(0);};diagnostics.onClick=[this]{showSettings(3);};masterFX.onClick=[this]{selectedRack=2;selectPage(4);};returns.onClick=[this]{showSettings(2);};routing.onClick=[this]{showSettings(1);};
    for(auto* c:std::initializer_list<juce::Component*>{&variant,&undo,&advanced,&panic,&exportButton,&settings,&diagnostics,&masterFX,&returns,&routing,&status,&title})shell->addAndMakeVisible(*c);
    advanced.setComponentID("instrument-advanced");variant.setComponentID("instrument-variant");undo.setComponentID("instrument-undo");exportButton.setComponentID("global-export");settings.setComponentID("global-settings");diagnostics.setComponentID("global-diagnostics");routing.setComponentID("global-routing");masterFX.setComponentID("global-master-fx");returns.setComponentID("global-returns");
    status.setColour(juce::Label::textColourId,muted);title.setFont(juce::Font(juce::FontOptions(18).withStyle("Bold")));
    setResizable(true,true);setResizeLimits(900,540,1920,1200);if(auto* display=juce::Desktop::getInstance().getDisplays().getPrimaryDisplay()){const auto area=display->userArea;setSize(juce::jlimit(900,1200,area.getWidth()-80),juce::jlimit(540,720,area.getHeight()-150));}else setSize(1200,720);chooseModule(0);if(!processor.restoredSession&&processor.readParameters().performanceSource==0&&!processor.readParameters().autoPlay&&!processor.readParameters().composerEnabled){setSource(1);if(processor.wrapperType==juce::AudioProcessor::wrapperType_VST3)processor.setParameterValue("clockSource",1);}timerCallback();startTimerHz(20);
}
BassAmbientEditor::~BassAmbientEditor(){stopTimer();dialog.reset();dialogContent.reset();viewport.setViewedComponent(nullptr,false);setLookAndFeel(nullptr);}
juce::Component& BassAmbientEditor::currentPage(){if(selected==8)return *pages[10];if(selected==7)return *mixerView;if(selected==6)return *composerView;return *pages[size_t(selected==4?(selectedRack<3?2+selectedRack:selectedRack==5?11:5+selectedRack):selected==2?5:selected==3?6:selected==5?7:selected)];}
void BassAmbientEditor::chooseModule(int module){selectedModule=module;localPreset.onChange=nullptr;localPreset.clear(juce::dontSendNotification);localPreset.addItemList(module==0?juce::StringArray{"Warm Finger","Rock Pick","Muted Punk"}:module==1?juce::StringArray{"Lush Motion","Soft Horizon","Deep Space"}:module==2?juce::StringArray{"Glass FM","Gentle Pluck","PWM Keys"}:module==3?juce::StringArray{"Round","Punch","Short"}:juce::StringArray{"Breathing FM","Resonant Dust","Harmonic Cloud"},1);localPreset.setSelectedId(localPresetIds[size_t(module)],juce::dontSendNotification);if(localPreset.getSelectedId()==0)localPreset.setText("Preset modułu"_ui,juce::dontSendNotification);localPreset.onChange=[this]{if(localPreset.getSelectedId()>0){localPresetIds[size_t(selectedModule)]=localPreset.getSelectedId();const int map[]{0,1,5,6,10};pages[size_t(map[selectedModule])]->applySoundPreset(localPreset.getSelectedId()-1);}};selectedRack=module==0?0:module==1?1:module==2?3:module==3?4:5;const int map[]{0,1,2,3,8};selectPage(section==2?4:map[module]);performance.onChange=nullptr;performanceAttachment.reset();performance.clear(juce::dontSendNotification);performance.addItemList(module==3?juce::StringArray{"NUTY MIDI / GM","SEKWENCER"}:juce::StringArray{"NUTY","GENERATOR"},1);const char* ids[]{"bassMidiMode","padMidiMode","arpMidiMode","drumsMidiMode","krellMidiMode"};performanceAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.state,ids[module],performance);performance.onChange=[this]{if(!updating&&!processor.readParameters().composerEnabled&&!processor.readParameters().autoPlay){bool legacy=!processor.readParameters().modularMidi;processor.setParameterValue("performanceSource",1);processor.setParameterValue("modularMidi",1);processor.setParameterValue("midiPlayback",0);const int tracks[]{0,1,2,3,7};if(legacy)processor.requestPanic();else processor.resetMidiPart(tracks[selectedModule]);}};}
void BassAmbientEditor::chooseSection(int value){section=value;expanded=false;chooseModule(selectedModule);}
void BassAmbientEditor::selectPage(int value){selected=value;viewport.setViewedComponent(&currentPage(),false);resized();viewport.setViewPosition(0,0);}
void BassAmbientEditor::setSource(int value){processor.setParameterValue("performanceSource",value==4?0:float(value));processor.requestPanic();processor.setParameterValue("composerPlaying",0);processor.setParameterValue("autoPlay",0);processor.setParameterValue("composerEnabled",value==3?1:0);processor.setParameterValue("modularMidi",value==1?1:0);processor.setParameterValue("midiPlayback",0);if(value==2){processor.setParameterValue("direct",0);processor.setParameterValue("harmonyLink",1);selectPage(5);}else if(value==3)selectPage(6);else chooseModule(selectedModule);harmonyView->refresh();}
void BassAmbientEditor::showSettings(int type){dialog.reset();dialogContent=std::make_unique<SettingsPanel>(*this,type);const juce::String names[]{"USTAWIENIA","ROUTING FL STUDIO","POWROTY EFEKTÓW"_ui,"DIAGNOSTYKA"};dialog=std::make_unique<ProDialog>(names[type],*dialogContent);dialog->setLookAndFeel(theme.get());dialog->setVisible(true);dialog->toFront(true);}
void BassAmbientEditor::makeVariant(){variantUndo.clear();juce::StringArray ids=selectedModule==0?juce::StringArray{"density","movement","gate","swing","humanize"}:selectedModule==1?juce::StringArray{"motion","padDrift","spread"}:selectedModule==2?juce::StringArray{"arpGateRandom","arpPitchRandom","arpVelocity","arpSwing"}:selectedModule==4?juce::StringArray{"krellTimbre","krellStereo"}:juce::StringArray{"drumSwing","drumHumanize"};for(const auto& id:ids)if(auto* v=processor.state.getRawParameterValue(id)){variantUndo.emplace_back(id,v->load());auto* param=processor.state.getParameter(id);processor.setParameterValue(id,param->convertFrom0to1(.15f+juce::Random::getSystemRandom().nextFloat()*.6f));}undo.setEnabled(true);}
void BassAmbientEditor::paint(juce::Graphics& g){g.fillAll(background);}
void BassAmbientEditor::resized(){if(!shell||!composerView)return;float scale=uiScale>0?uiScale:std::min({1.f,float(getWidth())/1200,float(getHeight())/720});const int w=int(getWidth()/scale),h=int(getHeight()/scale);shell->setTransform(juce::AffineTransform::scale(scale));shell->setBounds(0,0,w,h);
    presets.setBounds(270,14,205,30);source.setBounds(490,14,155,30);transportView->setBounds(658,10,w-885,38);fxGlobal.setBounds(w-216,14,98,30);panic.setBounds(w-110,14,92,30);
    for(int k=0;k<3;++k)mainTabs[size_t(k)].setBounds(18+k*125,65,118,30);harmonyView->setBounds(408,65,w-675,30);exportButton.setBounds(w-253,65,105,30);settings.setBounds(w-140,65,122,30);
    bool instrument=selected!=6&&selected!=7&&selected!=5&&!(selected==4&&selectedRack==2);shell->showModules=instrument;shell->repaint();int left=instrument?176:16;for(int k=0;k<5;++k){tabs[size_t(k)].setVisible(instrument);modulePower[size_t(k)].setVisible(instrument);moduleLeds[size_t(k)]->setVisible(instrument);tabs[size_t(k)].setBounds(24,154+k*78,126,36);modulePower[size_t(k)].setBounds(24,192+k*78,64,23);moduleLeds[size_t(k)]->setBounds(90,190+k*78,60,24);tabs[size_t(k)].setToggleState(instrument&&selectedModule==k,juce::dontSendNotification);}
    const juce::String names[]{"BASS","AMBIENT","ARP","DRUMS","KRELL"};title.setText(selected==6?"ARANŻER"_ui:selected==7?"MIKSER":selected==5?"JAM — GRA BEZ MIDI"_ui:selected==4&&selectedRack==2?"MASTER FX":names[selectedModule],juce::dontSendNotification);title.setBounds(left+6,114,300,30);title.setColour(juce::Label::textColourId,instrument?ProVisual::pages[size_t(selectedModule==4?7:selectedModule)]:accent);
    mainTabs[0].setToggleState(instrument||selected==5,juce::dontSendNotification);mainTabs[1].setToggleState(selected==6,juce::dontSendNotification);mainTabs[2].setToggleState(selected==7,juce::dontSendNotification);
    for(int k=0;k<3;++k){sectionTabs[size_t(k)].setVisible(instrument);sectionTabs[size_t(k)].setBounds(w-355+k*112,116,106,28);sectionTabs[size_t(k)].setToggleState(section==k,juce::dontSendNotification);}
    const auto controls=processor.readParameters();bool sourceOwns=controls.performanceSource==2||controls.composerEnabled||controls.autoPlay;modeLabel.setText(sourceOwns?(controls.composerEnabled?"STEROWANIE: KLIP ARANŻERA"_ui:"STEROWANIE: JAM"_ui):"ODTWARZANIE",juce::dontSendNotification);modeLabel.setVisible(instrument&&selected!=4);performance.setVisible(instrument&&selected!=4&&!sourceOwns);modeLabel.setBounds(left+12,153,sourceOwns?310:112,28);localPreset.setVisible(instrument&&section==0);localPreset.setBounds(left+340,153,250,28);performance.setBounds(left+130,153,190,28);
    variant.setVisible(instrument&&section==1);undo.setVisible(variant.isVisible());variant.setBounds(w-255,153,150,28);undo.setBounds(w-99,153,80,28);
    const bool hasAdvanced=instrument||(selected==4&&selectedRack==2);advanced.setVisible(hasAdvanced);advanced.setButtonText(expanded?"ZWIŃ ZAAWANSOWANE"_ui:"ZAAWANSOWANE");advanced.setBounds(left+8,h-78,w-left-28,28);
    jamCapture.setVisible(selected==5);jamCapture.setBounds(w-275,116,255,28);
    masterFX.setVisible(selected==7);returns.setVisible(selected==7);routing.setVisible(selected==7);routing.setBounds(w-425,116,132,28);masterFX.setBounds(w-285,116,128,28);returns.setBounds(w-150,116,130,28);
    int top=instrument?(selected==4?151:190):151;viewport.setBounds(left,top,w-left-14,h-top-(hasAdvanced?86:46));auto& page=currentPage();const int pw=viewport.getWidth()-16;
    int height;if(selected==6)height=viewport.getHeight();else if(selected==7)height=mixerView->requiredHeight();else{auto& p=static_cast<Page&>(page);p.setSection(section,expanded);height=p.requiredHeight(pw);}page.setSize(pw,std::max(height,viewport.getHeight()));page.resized();
    status.setBounds(18,h-34,w-174,28);diagnostics.setBounds(w-151,h-33,133,27);
}
void BassAmbientEditor::timerCallback(){auto p=processor.readParameters();harmonyView->refresh();updating=true;source.setSelectedId(p.performanceSource>0?p.performanceSource:p.composerEnabled?3:p.autoPlay?2:p.modularMidi?1:4,juce::dontSendNotification);updating=false;const int model=p.model;if(model!=lastBassModel){lastBassModel=model;if(selectedModule==0&&selected!=6&&selected!=7)resized();}
    for(auto& led:moduleLeds)led->repaint();for(auto& page:pages)page->updateOrder();pages[6]->refreshGrid();auto root=processor.rootDisplay.load();juce::String input=p.composerEnabled?"ARANŻER"_ui:p.performanceSource==2?(p.autoPlay?"JAM — gra"_ui:"JAM — naciśnij PLAY"_ui):processor.lastMidiNote>=0?"MIDI "+juce::String(processor.lastMidiChannel.load())+" · "_ui+juce::MidiMessage::getMidiNoteName(processor.lastMidiNote.load(),true,true,4):"Oczekiwanie na MIDI"_ui;
    if(root>=0)input+="  ROOT "+juce::MidiMessage::getMidiNoteName(root,true,true,4);
    status.setText(input+"    |    OUT "+juce::String(juce::Decibels::gainToDecibels(std::max(.000001f,processor.peak.load())),1)+" dBFS"+(processor.clockStatus>=2?"    |    ZEGAR: FL ZATRZYMANE / BRAK CLOCK":""),juce::dontSendNotification);
}
