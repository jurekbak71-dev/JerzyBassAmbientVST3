#include "VisualPalette.h"
#include "ComposerView.h"
using namespace jerzy::composer;
namespace {
const juce::Colour ink(0xffedf1f7),dim(0xff9eafc2),mint(0xff61e0c1),dark(0xff181e27);
const auto& colours=ProVisual::tracks;
void slider(juce::Slider& s,double lo,double hi,double step,const juce::String& label){s.setRange(lo,hi,step);s.setSliderStyle(juce::Slider::LinearHorizontal);s.setTextBoxStyle(juce::Slider::TextBoxRight,false,55,24);s.setName(label);s.setTooltip(label);}
}
class ComposerView::SceneModel final : public juce::ListBoxModel {
    ComposerView& view;
public:
    explicit SceneModel(ComposerView& v):view(v){}
    int getNumRows() override {return int(view.document.scenes.size());}
    void paintListBoxItem(int row,juce::Graphics& g,int w,int h,bool selected) override {
        if(row>=getNumRows())return;const auto& s=view.document.scenes[size_t(row)];
        g.fillAll(selected?juce::Colour(0xff324252):dark);g.setColour(ink);g.setFont(12);g.drawText(juce::String(s.name)+"  "+juce::String(s.bars)+"b",6,0,140,h,juce::Justification::centredLeft);
        const int cell=std::max(1,(w-152)/7);for(int t=0;t<7;++t){g.setColour(s.clips[size_t(t)].enabled?colours[t].withAlpha(.8f):juce::Colour(0xff303743));g.fillRoundedRectangle(float(150+t*cell),5,float(cell-4),float(h-10),3);g.setColour(s.clips[size_t(t)].enabled?dark:colours[t]);g.setFont(9);g.drawText(trackNames[t],150+t*cell,5,cell-4,h-10,juce::Justification::centred);}
    }
    void listBoxItemClicked(int row,const juce::MouseEvent& e) override {view.selectScene(row);if(e.x>=150){const int t=juce::jlimit(0,6,(e.x-150)/std::max(1,(view.scenes.getWidth()-152)/7));view.selectTrack(t);if(e.mods.isRightButtonDown())view.edit([&](auto& d){auto& c=d.scenes[size_t(row)].clips[size_t(t)];c.enabled=!c.enabled;});}}
    void listBoxItemDoubleClicked(int row,const juce::MouseEvent&) override {view.processor.launchComposer(row,false);}
    juce::var getDragSourceDescription(const juce::SparseSet<int>& rows) override {return rows.size()?juce::var("scene:"+juce::String(view.document.scenes[size_t(rows[0])].id)):juce::var();}
};
class ComposerView::SongModel final : public juce::ListBoxModel {
    ComposerView& view;
public:
    explicit SongModel(ComposerView& v):view(v){}
    int getNumRows() override {return int(view.document.song.size());}
    void paintListBoxItem(int row,juce::Graphics& g,int w,int h,bool selected) override {
        if(row>=getNumRows())return;const auto& slot=view.document.song[size_t(row)];const auto s=std::find_if(view.document.scenes.begin(),view.document.scenes.end(),[&](const Scene& x){return x.id==slot.sceneId;});
        g.fillAll(selected?juce::Colour(0xff324252):dark);g.setColour(row==view.processor.composerSlotDisplay.load()?mint:ink);g.setFont(13);
        g.drawText(juce::String(row+1)+"   "+(s!=view.document.scenes.end()?juce::String(s->name):"?")+"    x"+juce::String(slot.repeats)+(slot.evolve?"   EVOLVE":""),8,0,w-16,h,juce::Justification::centredLeft);
    }
    void listBoxItemClicked(int row,const juce::MouseEvent&) override {view.selectedSlot=row;view.refresh(true);}
    juce::var getDragSourceDescription(const juce::SparseSet<int>& rows) override {return rows.size()?juce::var("song:"+juce::String(rows[0])):juce::var();}
};
class ComposerView::SongList final : public juce::ListBox,public juce::DragAndDropTarget {
    ComposerView& view;
public:
    explicit SongList(ComposerView& v):view(v){}
    bool isInterestedInDragSource(const SourceDetails& d) override {return d.description.toString().startsWith("scene:")||d.description.toString().startsWith("song:");}
    void itemDropped(const SourceDetails& details) override {
        const auto text=details.description.toString();int target=getRowContainingPosition(details.localPosition.x,details.localPosition.y);if(target<0)target=int(view.document.song.size());
        view.edit([&](auto& d){target=juce::jlimit(0,int(d.song.size()),target);const int index=text.fromFirstOccurrenceOf(":",false,false).getIntValue();
            if(text.startsWith("scene:")){if(d.song.size()<maxSlots)d.song.insert(d.song.begin()+target,{index,1,false});}
            else if(index>=0&&index<int(d.song.size())){auto slot=d.song[size_t(index)];d.song.erase(d.song.begin()+index);if(index<target)--target;d.song.insert(d.song.begin()+target,slot);}});
    }
};
class ComposerView::NoteGrid final : public juce::Component,public juce::SettableTooltipClient {
    ComposerView& view;bool dragging=false;double length=.25;int edited=-1;std::vector<Note> preview;
public:
    explicit NoteGrid(ComposerView& v):view(v){setComponentID("composer-note-grid");setTooltip("Click: add/select | Drag: note length | Right-click: delete | Mouse wheel: pitch octave. Freeze generated clips to edit.");}
    void paint(juce::Graphics& g) override {
        g.fillAll(dark);if(view.document.scenes.empty())return;const auto& scene=view.document.scenes[size_t(view.selectedScene)];const auto& clip=scene.clips[size_t(view.selectedTrack)];
        const int rows=view.selectedTrack>=3?1:24,base=int(view.octave.getValue())*12,columns=std::max(1,int(std::lround(scene.barBeats()*4)));const float cw=float(getWidth()-52)/columns,ch=float(getHeight())/rows;const double start=(view.bar.getValue()-1)*scene.barBeats();
        for(int r=0;r<rows;++r){g.setColour(r%2?juce::Colour(0xff222a35):dark);g.fillRect(52.0f,r*ch,float(getWidth()-52),ch);g.setColour(dim);g.setFont(10);g.drawText(rows==1?"HIT":juce::MidiMessage::getMidiNoteName(base+rows-1-r,true,true,3),0,int(r*ch),48,int(ch),juce::Justification::centredRight);}
        g.setColour(juce::Colour(0xff394452));for(int c=0;c<=columns;++c)g.drawVerticalLine(int(52+c*cw),0,float(getHeight()));
        auto notes=dragging?preview:generateClip(scene,view.selectedTrack);
        for(size_t i=0;i<notes.size();++i){const auto& n=notes[i];if(n.beat+n.length<=start||n.beat>=start+scene.barBeats())continue;const int row=rows==1?0:base+rows-1-n.pitch;if(row<0||row>=rows)continue;
            const float x=52+float(std::max(0.0,n.beat-start)/scene.barBeats())*(getWidth()-52);const float end=52+float(std::min(scene.barBeats(),n.beat+n.length-start)/scene.barBeats())*(getWidth()-52);
            g.setColour(colours[view.selectedTrack].withAlpha(float(.35+n.velocity/200.0)));g.fillRoundedRectangle(x+1,row*ch+1,std::max(2.0f,end-x-2),ch-2,2);
            if(int(i)==view.selectedNote){g.setColour(juce::Colours::white);g.drawRoundedRectangle(x+1,row*ch+1,std::max(2.0f,end-x-2),ch-2,2,1.5f);}}
        if(clip.generated){g.setColour(ink.withAlpha(.7f));g.drawText("GENERATED — FREEZE / EDIT to write your notes",56,4,getWidth()-64,18,juce::Justification::centredLeft);}
    }
    void mouseDown(const juce::MouseEvent& e) override {
        if(e.x<52)return;const auto& scene=view.document.scenes[size_t(view.selectedScene)];const auto& clip=scene.clips[size_t(view.selectedTrack)];if(clip.generated){view.setMessage("Press FREEZE / EDIT first; the generated notes become editable.");return;}
        preview=clip.notes;const int rows=view.selectedTrack>=3?1:24;const double time=(view.bar.getValue()-1)*scene.barBeats()+std::floor((e.x-52.0)/(getWidth()-52)*scene.barBeats()*4)/4;
        const int pitch=rows==1?(view.selectedTrack==6?(scene.patch.drumAux==1?46:scene.patch.drumAux==2?45:39):drumNotes[view.selectedTrack-3]):int(view.octave.getValue())*12+rows-1-juce::jlimit(0,rows-1,int(double(e.y)/getHeight()*rows));
        edited=-1;for(size_t i=0;i<preview.size();++i)if(preview[i].pitch==pitch&&time>=preview[i].beat-.001&&time<preview[i].beat+preview[i].length){edited=int(i);break;}
        if(e.mods.isRightButtonDown()){if(edited>=0)view.edit([&](auto& d){auto& notes=d.scenes[size_t(view.selectedScene)].clips[size_t(view.selectedTrack)].notes;notes.erase(notes.begin()+edited);});view.selectedNote=-1;return;}
        if(edited<0){if(preview.size()>=maxNotes){view.setMessage("512 notes maximum per clip");return;}preview.push_back({time,.25,pitch,int(view.velocity.getValue()),view.notePan.getValue()});edited=int(preview.size()-1);}
        view.selectedNote=edited;length=preview[size_t(edited)].length;dragging=true;repaint();
    }
    void mouseDrag(const juce::MouseEvent& e) override {if(!dragging)return;auto& n=preview[size_t(edited)];const auto& scene=view.document.scenes[size_t(view.selectedScene)];n.length=std::max(.25,length+std::round(e.getDistanceFromDragStartX()/double(getWidth()-52)*scene.barBeats()*4)/4);n.length=std::min(n.length,scene.beats()-n.beat);repaint();}
    void mouseUp(const juce::MouseEvent&) override {if(!dragging)return;dragging=false;view.edit([&](auto& d){d.scenes[size_t(view.selectedScene)].clips[size_t(view.selectedTrack)].notes=preview;});view.refresh(true);}
    void mouseWheelMove(const juce::MouseEvent&,const juce::MouseWheelDetails& wheel) override {view.octave.setValue(view.octave.getValue()+(wheel.deltaY>0?1:-1));}
};
class ComposerView::MidiDrag final : public juce::TextButton {
    ComposerView& view;bool started=false;
public:
    explicit MidiDrag(ComposerView& v):juce::TextButton("DRAG MIDI TO FL"),view(v){setTooltip("Drag the selected scene/track into FL Studio Piano Roll. Export Song writes all tracks.");}
    void mouseDown(const juce::MouseEvent& e) override {started=false;juce::TextButton::mouseDown(e);}
    void mouseDrag(const juce::MouseEvent& e) override {if(started||e.getDistanceFromDragStart()<8)return;started=true;auto dir=juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("JerzyPRO-midi");dir.createDirectory();auto file=dir.getNonexistentChildFile("PRO-clip",".mid");auto result=view.processor.exportComposer(file,false,view.selectedScene,view.selectedTrack);
        if(result.failed()){view.setMessage(result.getErrorMessage());return;}juce::DragAndDropContainer::performExternalDragDropOfFiles({file.getFullPathName()},false,this);}
};

ComposerView::ComposerView(BassAmbientProcessor& p):processor(p),sceneModel(std::make_unique<SceneModel>(*this)),songModel(std::make_unique<SongModel>(*this)),song(std::make_unique<SongList>(*this)),grid(std::make_unique<NoteGrid>(*this)),dragMidi(std::make_unique<MidiDrag>(*this)){
    setComponentID("composer-view");
    heading.setText("COMPOSER / ARRANGER  |  SCENES > SONG > MIDI",juce::dontSendNotification);sceneTitle.setText("SCENES   |   BASS / PAD / ARP / KICK / SNARE / HAT / AUX",juce::dontSendNotification);songTitle.setText("SONG — drag scenes here; drag slots to reorder",juce::dontSendNotification);
    for(auto* label:{&heading,&sceneTitle,&songTitle,&clipTitle,&message}){label->setColour(juce::Label::textColourId,ink);addAndMakeVisible(label);}
    scenes.setModel(sceneModel.get());song->setModel(songModel.get());scenes.setRowHeight(28);song->setRowHeight(28);addAndMakeVisible(scenes);addAndMakeVisible(*song);addAndMakeVisible(*grid);addAndMakeVisible(*dragMidi);
    juce::ToggleButton* toggles[]{&enabled,&play,&songMode,&loop,&split,&midiPlay};const char* ids[]{"composerEnabled","composerPlaying","composerSongMode","composerLoop","separateOutputs","midiPlayback"};
    for(int i=0;i<6;++i){addAndMakeVisible(*toggles[i]);attachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.state,ids[i],*toggles[i]));}
    for(auto* button:{&add,&duplicate,&remove,&launch,&capture,&append,&slotRemove,&up,&down,&freeze,&clear,&exportScene,&exportSong,&exportPack,&save,&load})addAndMakeVisible(*button);
    for(auto* button:{&generated,&cut,&evolve})addAndMakeVisible(*button);
    for(auto* editor:{&name,&progression}){addAndMakeVisible(*editor);editor->setColour(juce::TextEditor::backgroundColourId,dark);editor->setColour(juce::TextEditor::textColourId,ink);}
    name.setTooltip("Scene name; Return commits");progression.setTooltip("One chord per bar, repeating: Cm Ab Fm G. Supports major, m, 5, sus4, m7, maj7, m9, maj9. Return commits.");
    slider(bars,1,32,1,"Scene bars");slider(tempo,30,240,1,"BPM (set host tempo in FL Studio)");slider(intensity,0,1,.01,"Scene intensity");slider(repeat,1,16,1,"Song slot repeats");slider(bar,1,32,1,"Edit bar");slider(octave,1,7,1,"Lowest MIDI octave");slider(velocity,1,127,1,"Note velocity");slider(notePan,-1,1,.01,"Note pan");slider(gain,0,1,.01,"Track gain");slider(enter,1,32,1,"Track enters at bar");slider(leave,0,32,1,"Last playing bar (0 = end)");slider(threshold,0,1,.01,"Minimum scene intensity for this instrument");
    for(auto* s:{&bars,&tempo,&intensity,&repeat,&bar,&octave,&velocity,&notePan,&gain,&enter,&leave,&threshold})addAndMakeVisible(*s);octave.setValue(3);velocity.setValue(100);
    for(int n=1;n<=12;++n)numerator.addItem(juce::String(n),n);denominator.addItemList({"2","4","8","16"},1);addAndMakeVisible(numerator);addAndMakeVisible(denominator);
    name.onReturnKey=[this]{edit([&](auto& d){d.scenes[size_t(selectedScene)].name=name.getText().toStdString();});};progression.onReturnKey=[this]{updateProgression();};
    bars.onValueChange=[this]{if(!updating)edit([&](auto& d){d.scenes[size_t(selectedScene)].bars=int(bars.getValue());});};
    tempo.onValueChange=[this]{if(!updating){edit([&](auto& d){d.tempo=tempo.getValue();for(auto& s:d.scenes)s.patch.internalTempo=d.tempo;});processor.setParameterValue("internalTempo",float(tempo.getValue()));}};
    intensity.onValueChange=[this]{if(!updating)edit([&](auto& d){d.scenes[size_t(selectedScene)].intensity=intensity.getValue();});};
    auto meter=[this]{if(!updating)edit([&](auto& d){auto& s=d.scenes[size_t(selectedScene)];s.numerator=numerator.getSelectedId();s.denominator=denominator.getText().getIntValue();});};numerator.onChange=meter;denominator.onChange=meter;
    generated.onClick=[this]{edit([&](auto& d){d.scenes[size_t(selectedScene)].clips[size_t(selectedTrack)].generated=generated.getToggleState();});};
    cut.onClick=[this]{edit([&](auto& d){d.scenes[size_t(selectedScene)].cutTails=cut.getToggleState();});};
    for(int t=0;t<7;++t){auto& b=tracks[size_t(t)];b.setButtonText(trackNames[t]);b.onClick=[this,t]{if(juce::ModifierKeys::getCurrentModifiers().isShiftDown())edit([&](auto& d){auto& c=d.scenes[size_t(selectedScene)].clips[size_t(t)];c.enabled=!c.enabled;});selectTrack(t);};addAndMakeVisible(b);}
    gain.onValueChange=[this]{if(!updating)edit([&](auto& d){d.scenes[size_t(selectedScene)].clips[size_t(selectedTrack)].gain=gain.getValue();});};
    enter.onValueChange=[this]{if(!updating)edit([&](auto& d){d.scenes[size_t(selectedScene)].clips[size_t(selectedTrack)].enterBar=int(enter.getValue())-1;});};
    leave.onValueChange=[this]{if(!updating)edit([&](auto& d){d.scenes[size_t(selectedScene)].clips[size_t(selectedTrack)].leaveBar=int(leave.getValue());});};
    threshold.onValueChange=[this]{if(!updating)edit([&](auto& d){d.scenes[size_t(selectedScene)].clips[size_t(selectedTrack)].threshold=threshold.getValue();});};
    bar.onValueChange=[this]{grid->repaint();};octave.onValueChange=[this]{grid->repaint();};
    auto noteEdit=[this]{if(!updating&&selectedNote>=0)edit([&](auto& d){auto& c=d.scenes[size_t(selectedScene)].clips[size_t(selectedTrack)];if(!c.generated&&selectedNote<int(c.notes.size())){auto& n=c.notes[size_t(selectedNote)];n.velocity=int(velocity.getValue());n.pan=notePan.getValue();}});};velocity.onValueChange=noteEdit;notePan.onValueChange=noteEdit;
    add.onClick=[this]{edit([&](auto& d){if(d.scenes.size()>=maxScenes)return;Scene s;s.id=d.nextId++;s.name="Custom "+std::to_string(s.id);s.patch=processor.readParameters();for(auto& c:s.clips)c.generated=false;d.scenes.push_back(s);selectedScene=int(d.scenes.size()-1);});};
    duplicate.onClick=[this]{edit([&](auto& d){if(d.scenes.size()>=maxScenes)return;auto s=d.scenes[size_t(selectedScene)];s.id=d.nextId++;s.name+=" variation";d.scenes.push_back(s);selectedScene=int(d.scenes.size()-1);});};
    remove.onClick=[this]{edit([&](auto& d){if(d.scenes.size()<2)return;const int id=d.scenes[size_t(selectedScene)].id;d.scenes.erase(d.scenes.begin()+selectedScene);d.song.erase(std::remove_if(d.song.begin(),d.song.end(),[&](const Slot& s){return s.sceneId==id;}),d.song.end());selectedScene=std::min(selectedScene,int(d.scenes.size())-1);});};
    launch.onClick=[this]{processor.launchComposer(selectedScene,false);};capture.onClick=[this]{edit([&](auto& d){d.scenes[size_t(selectedScene)].patch=processor.readParameters();});setMessage("Sound captured. Scene masks control the arrangement; module tabs edit the sound for the next capture.");};
    append.onClick=[this]{edit([&](auto& d){if(d.song.size()<maxSlots)d.song.push_back({d.scenes[size_t(selectedScene)].id,1,false});});};
    slotRemove.onClick=[this]{edit([&](auto& d){if(selectedSlot>=0&&selectedSlot<int(d.song.size()))d.song.erase(d.song.begin()+selectedSlot);});};
    up.onClick=[this]{edit([&](auto& d){if(selectedSlot>0&&selectedSlot<int(d.song.size())){std::swap(d.song[size_t(selectedSlot)],d.song[size_t(selectedSlot-1)]);--selectedSlot;}});};
    down.onClick=[this]{edit([&](auto& d){if(selectedSlot>=0&&selectedSlot+1<int(d.song.size())){std::swap(d.song[size_t(selectedSlot)],d.song[size_t(selectedSlot+1)]);++selectedSlot;}});};
    repeat.onValueChange=[this]{if(!updating)edit([&](auto& d){if(selectedSlot<int(d.song.size())&&selectedSlot>=0)d.song[size_t(selectedSlot)].repeats=int(repeat.getValue());});};
    evolve.onClick=[this]{edit([&](auto& d){if(selectedSlot<int(d.song.size())&&selectedSlot>=0)d.song[size_t(selectedSlot)].evolve=evolve.getToggleState();});};
    freeze.onClick=[this]{edit([&](auto& d){jerzy::composer::freeze(d.scenes[size_t(selectedScene)],selectedTrack);});};clear.onClick=[this]{edit([&](auto& d){auto& c=d.scenes[size_t(selectedScene)].clips[size_t(selectedTrack)];c.notes.clear();c.generated=false;});selectedNote=-1;};
    exportScene.onClick=[this]{chooseExport(0);};exportSong.onClick=[this]{chooseExport(1);};exportPack.onClick=[this]{chooseExport(2);};save.onClick=[this]{chooseProject(true);};load.onClick=[this]{chooseProject(false);};
    play.onClick=[this]{if(play.getToggleState())processor.launchComposer(selectedScene,songMode.getToggleState());};
    juce::Slider* knobs[]{&bars,&tempo,&intensity,&repeat,&bar,&octave,&velocity,&notePan,&gain,&enter,&leave,&threshold};const double defaults[]{4,120,1,1,1,3,100,0,1,1,0,0};
    for(int i=0;i<12;++i){knobs[i]->setPopupMenuEnabled(false);knobs[i]->setDoubleClickReturnValue(true,defaults[i]);knobs[i]->getProperties().set("proDefault",defaults[i]);knobs[i]->addMouseListener(this,false);}
    refresh(true);startTimerHz(10);
}
ComposerView::~ComposerView(){stopTimer();scenes.setModel(nullptr);song->setModel(nullptr);}
void ComposerView::mouseDown(const juce::MouseEvent& e){if(e.mods.isRightButtonDown())if(auto* s=dynamic_cast<juce::Slider*>(e.eventComponent))s->setValue(double(s->getProperties()["proDefault"]));}
void ComposerView::setMessage(const juce::String& text){message.setText(text,juce::dontSendNotification);}
void ComposerView::edit(const std::function<void(Document&)>& fn){try{processor.editComposer(fn);refresh(true);}catch(const std::exception& e){setMessage(e.what());}}
void ComposerView::selectScene(int index){selectedScene=juce::jlimit(0,int(document.scenes.size())-1,index);selectedNote=-1;bar.setValue(1);refresh(true);}
void ComposerView::selectTrack(int index){selectedTrack=juce::jlimit(0,6,index);selectedNote=-1;refresh(true);}
void ComposerView::prepareManual(Scene& scene){if(scene.clips[size_t(selectedTrack)].generated)jerzy::composer::freeze(scene,selectedTrack);}
juce::String ComposerView::progressionText() const {if(document.scenes.empty())return {};juce::StringArray result;const char* suffix[]{"","m","5","sus4","m7","maj7","m9","maj9"};for(const auto& c:document.scenes[size_t(selectedScene)].chords)result.add(juce::MidiMessage::getMidiNoteName(c.root,true,false,3)+suffix[c.quality]);return result.joinIntoString(" ");}
void ComposerView::updateProgression(){auto tokens=juce::StringArray::fromTokens(progression.getText()," ,|","");std::vector<Chord> chords;
    for(auto token:tokens){if(token.isEmpty())continue;const auto root=token.substring(0,1).toUpperCase();const juce::String letters="C D EF G A B";int pitch=letters.indexOf(root);if(pitch<0){setMessage("Chord example: Cm Ab Fm G. Use C D E F G A B with # or b.");return;}int pos=1;if(token.substring(1,2)=="#"){++pitch;++pos;}else if(token.substring(1,2)=="b"){--pitch;++pos;}const auto suffix=token.substring(pos).toLowerCase();int quality=suffix=="m"?1:suffix=="5"?2:suffix=="sus4"?3:suffix=="m7"?4:suffix=="maj7"?5:suffix=="m9"?6:suffix=="maj9"?7:suffix.isEmpty()?0:-1;
        if(quality<0){setMessage("Supported: major, m, 5, sus4, m7, maj7, m9, maj9");return;}chords.push_back({36+(pitch+12)%12,quality});}
    if(chords.empty()){setMessage("Enter at least one chord");return;}edit([&](auto& d){d.scenes[size_t(selectedScene)].chords=chords;});}
void ComposerView::refresh(bool force){const auto next=processor.composerRevision.load();if(!force&&revision==next)return;revision=next;document=processor.composerDocument();updating=true;selectedScene=juce::jlimit(0,int(document.scenes.size())-1,selectedScene);selectedSlot=document.song.empty()?-1:juce::jlimit(0,int(document.song.size())-1,selectedSlot);
    const auto& s=document.scenes[size_t(selectedScene)];const auto& c=s.clips[size_t(selectedTrack)];name.setText(s.name,false);progression.setText(progressionText(),false);bars.setValue(s.bars,juce::dontSendNotification);tempo.setValue(document.tempo,juce::dontSendNotification);intensity.setValue(s.intensity,juce::dontSendNotification);numerator.setSelectedId(s.numerator,juce::dontSendNotification);denominator.setText(juce::String(s.denominator),juce::dontSendNotification);cut.setToggleState(s.cutTails,juce::dontSendNotification);
    bar.setRange(1,s.bars,1);if(bar.getValue()<1)bar.setValue(1,juce::dontSendNotification);gain.setValue(c.gain,juce::dontSendNotification);enter.setValue(c.enterBar+1,juce::dontSendNotification);leave.setValue(c.leaveBar,juce::dontSendNotification);threshold.setValue(c.threshold,juce::dontSendNotification);generated.setToggleState(c.generated,juce::dontSendNotification);
    if(selectedSlot>=0){repeat.setValue(document.song[size_t(selectedSlot)].repeats,juce::dontSendNotification);evolve.setToggleState(document.song[size_t(selectedSlot)].evolve,juce::dontSendNotification);}
    if(selectedNote>=0&&selectedNote<int(c.notes.size())){velocity.setValue(c.notes[size_t(selectedNote)].velocity,juce::dontSendNotification);notePan.setValue(c.notes[size_t(selectedNote)].pan,juce::dontSendNotification);}
    for(int t=0;t<7;++t){ProVisual::tint(tracks[size_t(t)],colours[t]);tracks[size_t(t)].setToggleState(t==selectedTrack,juce::dontSendNotification);tracks[size_t(t)].setButtonText(juce::String(trackNames[t])+(s.clips[size_t(t)].enabled?" ON":" OFF"));}
    clipTitle.setColour(juce::Label::textColourId,colours[selectedTrack]);
    clipTitle.setText("CLIP: "+juce::String(s.name)+" / "+trackNames[selectedTrack]+"   |   Shift-click track: mute   |   Bar / octave / velocity / pan",juce::dontSendNotification);
    scenes.updateContent();song->updateContent();scenes.selectRow(selectedScene, true, false);if(selectedSlot>=0)song->selectRow(selectedSlot,true,false);grid->repaint();updating=false;
}
void ComposerView::timerCallback(){refresh();song->repaint();const auto error=processor.composerError();if(error.isNotEmpty())setMessage(error);}
void ComposerView::chooseExport(int mode){chooser=std::make_unique<juce::FileChooser>(mode==2?"Export MIDI pattern pack":"Export standard MIDI",juce::File{},mode==2?"":"*.mid");const int flags=mode==2?juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectDirectories:juce::FileBrowserComponent::saveMode|juce::FileBrowserComponent::canSelectFiles|juce::FileBrowserComponent::warnAboutOverwriting;
    juce::Component::SafePointer<ComposerView> safe(this);chooser->launchAsync(flags,[safe,mode](const auto& choice){if(!safe)return;const auto file=choice.getResult();if(file==juce::File{})return;
        if(mode!=2){auto result=safe->processor.exportComposer(file.withFileExtension("mid"),mode==1,safe->selectedScene);safe->setMessage(result.wasOk()?"MIDI exported: "+file.getFullPathName():result.getErrorMessage());return;}
        try{const auto d=safe->processor.composerDocument();const auto compiled=compile(d);auto folder=file.getNonexistentChildFile("JerzyPRO-patterns",{},false);auto result=folder.createDirectory();if(result.failed()){safe->setMessage(result.getErrorMessage());return;}
            result=writeMidi(compiled.song,d.tempo,folder.getChildFile("SONG.mid"));for(size_t i=0;i<d.scenes.size()&&result.wasOk();++i)for(int t=0;t<7&&result.wasOk();++t){auto name=juce::String(int(i)+1)+"-"+juce::File::createLegalFileName(juce::String(d.scenes[i].name))+"-"+trackNames[t]+".mid";result=writeMidi(compiled.scenes[i],d.tempo,folder.getChildFile(name),t);}safe->setMessage(result.wasOk()?"Pattern pack exported: "+folder.getFullPathName():result.getErrorMessage());
        }catch(const std::exception& e){safe->setMessage(e.what());}});
}
void ComposerView::chooseProject(bool saving){chooser=std::make_unique<juce::FileChooser>(saving?"Save Composer project":"Load Composer project",juce::File{},"*.jpro");const int flags=(saving?juce::FileBrowserComponent::saveMode|juce::FileBrowserComponent::warnAboutOverwriting:juce::FileBrowserComponent::openMode)|juce::FileBrowserComponent::canSelectFiles;
    juce::Component::SafePointer<ComposerView> safe(this);chooser->launchAsync(flags,[safe,saving](const auto& choice){if(!safe)return;const auto file=choice.getResult();if(file==juce::File{})return;const auto result=saving?safe->processor.saveComposer(file.withFileExtension("jpro")):safe->processor.loadComposer(file);safe->setMessage(result.wasOk()?"Composer project saved / loaded":result.getErrorMessage());safe->refresh(true);});
}
void ComposerView::paint(juce::Graphics& g){g.fillAll(juce::Colour(0xff111720));g.setColour(dim);g.setFont(12);g.drawText("NAME",12,304,42,28,juce::Justification::centredLeft);g.drawText("CHORDS",12,342,60,28,juce::Justification::centredLeft);g.drawText("Bars / BPM / Intensity / Meter",12,384,260,22,juce::Justification::centredLeft);g.drawText("Bar / Octave / Velocity / Pan",12,520,260,22,juce::Justification::centredLeft);g.drawText("Gain / Enter bar / Last bar (0 = end) / Intensity threshold",12,934,550,22,juce::Justification::centredLeft);}
void ComposerView::resized(){const int w=getWidth(),half=(w-36)/2;heading.setBounds(12,0,w-24,30);juce::ToggleButton* toggles[]{&enabled,&play,&songMode,&loop,&split,&midiPlay};for(int k=0;k<6;++k)toggles[k]->setBounds(12+k*(w-24)/6,34,(w-30)/6-4,28);
    sceneTitle.setBounds(12,66,half,26);songTitle.setBounds(24+half,66,half,26);scenes.setBounds(12,96,half,164);song->setBounds(24+half,96,half,164);
    const int b=half/5;add.setBounds(12,268,b-4,28);duplicate.setBounds(12+b,268,b-4,28);remove.setBounds(12+2*b,268,b-4,28);launch.setBounds(12+3*b,268,2*b-4,28);
    append.setBounds(24+half,268,72,28);slotRemove.setBounds(102+half,268,70,28);up.setBounds(178+half,268,28,28);down.setBounds(212+half,268,28,28);repeat.setBounds(246+half,268,108,28);evolve.setBounds(360+half,268,std::min(100,half-336),28);
    name.setBounds(72,306,w-370,28);capture.setBounds(w-286,306,144,28);cut.setBounds(w-136,306,124,28);progression.setBounds(72,344,w-84,28);
    bars.setBounds(12,408,150,30);tempo.setBounds(174,408,190,30);intensity.setBounds(376,408,190,30);numerator.setBounds(580,408,58,30);denominator.setBounds(644,408,58,30);save.setBounds(w-192,408,84,30);load.setBounds(w-102,408,90,30);
    clipTitle.setBounds(12,450,w-24,26);for(int t=0;t<7;++t)tracks[size_t(t)].setBounds(12+t*(w-24)/7,482,(w-24)/7-5,30);
    const int cell=(w-24)/7;juce::Component* editors[]{&bar,&octave,&velocity,&notePan,&generated,&freeze,&clear};for(int i=0;i<7;++i)editors[i]->setBounds(12+i*cell,546,cell-6,30);
    grid->setBounds(12,586,w-24,336);gain.setBounds(12,960,200,30);enter.setBounds(224,960,180,30);leave.setBounds(416,960,180,30);threshold.setBounds(608,960,200,30);
    exportScene.setBounds(12,1002,156,30);exportSong.setBounds(178,1002,156,30);exportPack.setBounds(344,1002,156,30);dragMidi->setBounds(510,1002,188,30);message.setBounds(12,1040,w-24,30);
}
