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
        g.fillAll(selected?juce::Colour(0xff2c3b47):dark);g.setColour(ink);g.setFont(12);g.drawText(juce::String(s.name),8,3,w-16,20,juce::Justification::centredLeft);g.setColour(dim);g.setFont(10);g.drawText(juce::String(s.bars)+" taktów"_ui,8,23,80,18,juce::Justification::centredLeft);
        const int cw=(w-16)/8;for(int t=0;t<8;++t){g.setColour(s.clips[size_t(t)].enabled?colours[t]:juce::Colour(0xff344453));g.fillRoundedRectangle(float(8+t*cw),44,float(cw-3),5.f,2);}
    }
    void listBoxItemClicked(int row,const juce::MouseEvent& e) override {view.selectedSlot=-1;view.selectScene(row);if(e.y>=38){const int t=juce::jlimit(0,7,(e.x-8)/std::max(1,(view.scenes.getWidth()-16)/8));view.selectTrack(t);if(e.mods.isRightButtonDown())view.edit([&](auto& d){auto& c=d.scenes[size_t(row)].clips[size_t(t)];c.enabled=!c.enabled;});}}

    void listBoxItemDoubleClicked(int row,const juce::MouseEvent&) override {view.processor.launchComposer(row,false);}
    juce::var getDragSourceDescription(const juce::SparseSet<int>& rows) override {return rows.size()?juce::var("scene:"+juce::String(view.document.scenes[size_t(rows[0])].id)):juce::var();}
};
class ComposerView::SongModel final : public juce::ListBoxModel {public:explicit SongModel(ComposerView&){}int getNumRows() override{return 0;}void paintListBoxItem(int,juce::Graphics&,int,int,bool) override{}};
class ComposerView::SongList final : public juce::Component,public juce::DragAndDropTarget {
    ComposerView& view;int scrollRow=0,pressed=-1;
    int columns() const{return std::max(1,getWidth()/135);}int cellWidth() const{return std::max(1,getWidth()/columns());}
    int indexAt(juce::Point<int> p) const{return (p.y/88+scrollRow)*columns()+p.x/cellWidth();}
public:
    explicit SongList(ComposerView& v):view(v){setComponentID("composer-song-blocks");}
    void setModel(SongModel*){}void setRowHeight(int){}void updateContent(){repaint();}void selectRow(int,bool,bool){repaint();}
    void paint(juce::Graphics& g) override {g.fillAll(dark);if(view.document.song.empty()){g.setColour(dim);g.setFont(13);g.drawText("Przeciągnij sceny tutaj lub kliknij + SONG"_ui,getLocalBounds().reduced(12),juce::Justification::centred);return;}
        for(size_t n=0;n<view.document.song.size();++n){int row=int(n)/columns()-scrollRow;if(row<0||row*88>=getHeight())continue;int x=(int(n)%columns())*cellWidth(),y=row*88;auto r=juce::Rectangle<int>(x+3,y+3,cellWidth()-6,80);auto& slot=view.document.song[n];auto it=std::find_if(view.document.scenes.begin(),view.document.scenes.end(),[&](const Scene& s){return s.id==slot.sceneId;});if(it==view.document.scenes.end())continue;
            g.setColour(int(n)==view.selectedSlot?juce::Colour(0xff3c3a30):juce::Colour(0xff25313e));g.fillRoundedRectangle(r.toFloat(),5);g.setColour(int(n)==view.processor.composerSlotDisplay?mint:juce::Colour(0xffd9bd86));g.drawRoundedRectangle(r.toFloat(),5,1.4f);g.setFont(12);g.drawText(juce::String(it->name),r.reduced(8).removeFromTop(20),juce::Justification::centredLeft);g.setColour(dim);g.setFont(10);g.drawText(juce::String(it->bars)+" taktów  ×"_ui+juce::String(slot.repeats)+(slot.evolve?"  VAR":""),x+10,y+29,cellWidth()-20,19,juce::Justification::centredLeft);
            for(int t=0;t<8;++t){g.setColour(it->clips[size_t(t)].enabled?colours[t]:juce::Colour(0xff3c4a58));g.fillRoundedRectangle(float(x+10+t*(cellWidth()-20)/8),float(y+62),float((cellWidth()-20)/8-3),5.f,2);}
        }}
    void mouseDown(const juce::MouseEvent& e) override {pressed=indexAt(e.getPosition());if(pressed>=0&&pressed<int(view.document.song.size())){auto id=view.document.song[size_t(pressed)].sceneId;auto it=std::find_if(view.document.scenes.begin(),view.document.scenes.end(),[&](const Scene& s){return s.id==id;});if(it!=view.document.scenes.end())view.selectScene(int(it-view.document.scenes.begin()));view.selectedSlot=pressed;view.refresh(true);}}
    void mouseDrag(const juce::MouseEvent& e) override {if(e.getDistanceFromDragStart()>7&&pressed>=0&&pressed<int(view.document.song.size()))if(auto* c=juce::DragAndDropContainer::findParentDragContainerFor(this))c->startDragging("song:"+juce::String(pressed),this);}
    void mouseWheelMove(const juce::MouseEvent&,const juce::MouseWheelDetails& d) override {scrollRow=juce::jlimit(0,std::max(0,(int(view.document.song.size())+columns()-1)/columns()-std::max(1,getHeight()/88)),scrollRow+(d.deltaY<0?1:-1));repaint();}
    bool isInterestedInDragSource(const SourceDetails& d) override{return d.description.toString().startsWith("scene:")||d.description.toString().startsWith("song:");}
    void itemDropped(const SourceDetails& details) override {auto text=details.description.toString();int target=juce::jlimit(0,int(view.document.song.size()),indexAt(details.localPosition));view.edit([&](auto& d){const int index=text.fromFirstOccurrenceOf(":",false,false).getIntValue();if(text.startsWith("scene:")){if(d.song.size()<maxSlots)d.song.insert(d.song.begin()+target,{index,1,false});}else if(index>=0&&index<int(d.song.size())){auto slot=d.song[size_t(index)];d.song.erase(d.song.begin()+index);if(index<target)--target;d.song.insert(d.song.begin()+target,slot);}});}
};
class ComposerView::Matrix final : public juce::Component {
    ComposerView& view;
    int first() const{return view.selectedSlot<0?0:(view.selectedSlot/6)*6;}
    int count() const{return std::max(0,std::min(6,int(view.document.song.size())-first()));}
public:
    explicit Matrix(ComposerView& v):view(v){setComponentID("composer-instrument-matrix");}
    void paint(juce::Graphics& g) override {g.fillAll(dark);const int n=count(),cw=std::max(1,(getWidth()-85)/std::max(1,n)),rh=std::max(16,(getHeight()-24)/8);g.setFont(10);
        for(int col=0;col<n;++col){const auto id=view.document.song[size_t(first()+col)].sceneId;for(const auto& scene:view.document.scenes)if(scene.id==id){g.setColour(dim);g.drawText(juce::String(first()+col+1)+" "+juce::String(scene.name),85+col*cw,0,cw-5,22,juce::Justification::centred);}}
        for(int t=0;t<8;++t){g.setColour(colours[t]);g.drawText(trackNames[t],4,24+t*rh,78,rh,juce::Justification::centredLeft);for(int col=0;col<n;++col){const auto id=view.document.song[size_t(first()+col)].sceneId;bool on=false;for(const auto& scene:view.document.scenes)if(scene.id==id)on=scene.clips[size_t(t)].enabled;g.setColour(on?colours[t].withAlpha(.65f):juce::Colour(0xff253442));g.fillRoundedRectangle(float(85+col*cw),float(24+t*rh+3),float(cw-5),float(rh-6),3);}}}
    void mouseDown(const juce::MouseEvent& e) override {const int n=count();if(e.x<85||e.y<24||n==0)return;const int col=juce::jlimit(0,n-1,(e.x-85)/std::max(1,(getWidth()-85)/n)),t=juce::jlimit(0,7,(e.y-24)/std::max(16,(getHeight()-24)/8));const int id=view.document.song[size_t(first()+col)].sceneId;view.edit([&](auto& d){for(auto& scene:d.scenes)if(scene.id==id){auto& c=scene.clips[size_t(t)];c.enabled=!c.enabled;}});}
};
class ComposerView::NoteGrid final : public juce::Component,public juce::SettableTooltipClient {
    ComposerView& view;bool dragging=false;double length=.25;int edited=-1;std::vector<Note> preview;
public:
    explicit NoteGrid(ComposerView& v):view(v){setComponentID("composer-note-grid");setTooltip("Click: root/note | Drag: length | Right-click: delete | MODULE: root gates drive the instrument; MANUAL: exact notes.");}
    void paint(juce::Graphics& g) override {
        g.fillAll(dark);if(view.document.scenes.empty())return;const auto& scene=view.document.scenes[size_t(view.selectedScene)];const auto& clip=scene.clips[size_t(view.selectedTrack)];
        const int rows=(view.selectedTrack>=3&&view.selectedTrack<=6)?1:24,base=(int(view.octave.getValue())+1)*12,columns=std::max(1,int(std::lround(scene.barBeats()*4)));const float cw=float(getWidth()-52)/columns,ch=float(getHeight())/rows;const double start=(view.bar.getValue()-1)*scene.barBeats();
        for(int r=0;r<rows;++r){g.setColour(r%2?juce::Colour(0xff222a35):dark);g.fillRect(52.0f,r*ch,float(getWidth()-52),ch);g.setColour(dim);g.setFont(10);g.drawText(rows==1?"HIT":juce::MidiMessage::getMidiNoteName(base+rows-1-r,true,true,4),0,int(r*ch),48,int(ch),juce::Justification::centredRight);}
        g.setColour(juce::Colour(0xff394452));for(int c=0;c<=columns;++c)g.drawVerticalLine(int(52+c*cw),0,float(getHeight()));
        auto notes=dragging?preview:(clip.generated&&(view.selectedTrack<3||view.selectedTrack==7)?rootClip(scene,view.selectedTrack):generateClip(scene,view.selectedTrack));
        for(size_t i=0;i<notes.size();++i){const auto& n=notes[i];if(n.beat+n.length<=start||n.beat>=start+scene.barBeats())continue;const int row=rows==1?0:base+rows-1-n.pitch;if(row<0||row>=rows)continue;
            const float x=52+float(std::max(0.0,n.beat-start)/scene.barBeats())*(getWidth()-52);const float end=52+float(std::min(scene.barBeats(),n.beat+n.length-start)/scene.barBeats())*(getWidth()-52);
            g.setColour(colours[view.selectedTrack].withAlpha(float(.35+n.velocity/200.0)));g.fillRoundedRectangle(x+1,row*ch+1,std::max(2.0f,end-x-2),ch-2,2);
            if(int(i)==view.selectedNote){g.setColour(juce::Colours::white);g.drawRoundedRectangle(x+1,row*ch+1,std::max(2.0f,end-x-2),ch-2,2,1.5f);}}
        if(clip.generated){g.setColour(ink.withAlpha(.7f));g.drawText((view.selectedTrack<3||view.selectedTrack==7)?"MODULE: edit root gates below; the instrument builds the phrase":"MODULE: drum grid pattern",56,4,getWidth()-64,18,juce::Justification::centredLeft);}
    }
    void mouseDown(const juce::MouseEvent& e) override {
        if(e.x<52)return;const auto& scene=view.document.scenes[size_t(view.selectedScene)];const auto& clip=scene.clips[size_t(view.selectedTrack)];if(clip.generated&&(view.selectedTrack>=3&&view.selectedTrack<=6)){view.setMessage("Freeze drum grid to edit individual hits.");return;}
        if(scene.patch.harmonyLink&&view.selectedTrack!=0&&clip.generated){view.setMessage("LINK is ON: edit BASS root gates, or disable LINK for independent module roots.");return;}
        preview=clip.generated?rootClip(scene,view.selectedTrack):clip.notes;const int rows=(view.selectedTrack>=3&&view.selectedTrack<=6)?1:24;const double time=(view.bar.getValue()-1)*scene.barBeats()+std::floor((e.x-52.0)/(getWidth()-52)*scene.barBeats()*4)/4;
        const int pitch=rows==1?(view.selectedTrack==6?(scene.patch.drumAux==1?46:scene.patch.drumAux==2?45:39):drumNotes[view.selectedTrack-3]):(int(view.octave.getValue())+1)*12+rows-1-juce::jlimit(0,rows-1,int(double(e.y)/getHeight()*rows));
        edited=-1;for(size_t i=0;i<preview.size();++i)if(preview[i].pitch==pitch&&time>=preview[i].beat-.001&&time<preview[i].beat+preview[i].length){edited=int(i);break;}
        if(e.mods.isRightButtonDown()){if(edited>=0)view.edit([&](auto& d){auto& c=d.scenes[size_t(view.selectedScene)].clips[size_t(view.selectedTrack)];c.notes=preview;c.explicitRoots=true;c.notes.erase(c.notes.begin()+edited);});view.selectedNote=-1;return;}
        if(edited<0){if(preview.size()>=maxNotes){view.setMessage("512 notes maximum per clip");return;}preview.push_back({time,std::min(view.noteLength.getValue(),scene.beats()-time),pitch,int(view.velocity.getValue()),view.notePan.getValue()});edited=int(preview.size()-1);}
        view.selectedNote=edited;view.noteLength.setValue(preview[size_t(edited)].length,juce::dontSendNotification);length=preview[size_t(edited)].length;dragging=true;repaint();
    }
    void mouseDrag(const juce::MouseEvent& e) override {if(!dragging)return;auto& n=preview[size_t(edited)];const auto& scene=view.document.scenes[size_t(view.selectedScene)];n.length=std::max(.25,length+std::round(e.getDistanceFromDragStartX()/double(getWidth()-52)*scene.barBeats()*4)/4);n.length=std::min(n.length,scene.beats()-n.beat);repaint();}
    void mouseUp(const juce::MouseEvent&) override {if(!dragging)return;dragging=false;view.edit([&](auto& d){auto& c=d.scenes[size_t(view.selectedScene)].clips[size_t(view.selectedTrack)];c.notes=preview;c.explicitRoots=true;});if(view.selectedTrack==0&&view.processor.readParameters().harmonyLink&&!preview.empty())view.processor.setParameterValue("harmonyRoot",float(preview.front().pitch));view.refresh(true);}
    void mouseWheelMove(const juce::MouseEvent&,const juce::MouseWheelDetails& wheel) override {view.octave.setValue(view.octave.getValue()+(wheel.deltaY>0?1:-1));}
};
class ComposerView::MidiDrag final : public juce::TextButton {
    ComposerView& view;bool started=false;
public:
    explicit MidiDrag(ComposerView& v):juce::TextButton("DRAG MIDI TO FL"),view(v){setTooltip("Choose SCENE or WHOLE SONG and SELECTED TRACK or ALL TRACKS, then drag this button into FL Studio. Use selected track for one Piano Roll; import all tracks as a multitrack MIDI file.");}
    void mouseDown(const juce::MouseEvent& e) override {started=false;juce::TextButton::mouseDown(e);}
    void mouseDrag(const juce::MouseEvent& e) override {if(started||e.getDistanceFromDragStart()<8)return;started=true;auto dir=juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("JerzyPRO-midi");dir.createDirectory();const bool wholeSong=view.midiRange.getSelectedId()==2;const int track=view.midiTracks.getSelectedId()==2?-1:view.selectedTrack;auto file=dir.getNonexistentChildFile(wholeSong?"PRO-song":"PRO-scene",".mid");auto result=view.processor.exportComposer(file,wholeSong,view.selectedScene,track);
        if(result.failed()){view.setMessage(result.getErrorMessage());return;}if(!juce::DragAndDropContainer::performExternalDragDropOfFiles({file.getFullPathName()},false,this))view.setMessage("Cannot start MIDI drag. Use EXPORT SCENE / EXPORT SONG and import the .mid file in FL Studio.");}
};

class ComposerView::AudioDrag final : public juce::TextButton {
    ComposerView& view;bool started=false;
public:
    explicit AudioDrag(ComposerView& v):juce::TextButton("DRAG WAV TO FL"),view(v){setEnabled(false);setTooltip("After rendering a stereo WAV, drag here onto the FL Studio Playlist to create an Audio Clip. For stems, import the numbered WAV files from the exported folder.");}
    void mouseDown(const juce::MouseEvent& event) override {started=false;juce::TextButton::mouseDown(event);}
    void mouseDrag(const juce::MouseEvent& event) override {
        if(started||event.getDistanceFromDragStart()<8)return;started=true;
        if(!view.renderedAudio.existsAsFile()){view.setMessage("Render a stereo WAV first. The last WAV file must still exist.");return;}
        if(!juce::DragAndDropContainer::performExternalDragDropOfFiles({view.renderedAudio.getFullPathName()},false,this))
            view.setMessage("Cannot start WAV drag. Drag the saved WAV from File Explorer onto the FL Studio Playlist.");
    }
};

class ComposerView::AudioRenderJob final : public juce::ThreadWithProgressWindow {
    juce::Component::SafePointer<ComposerView> owner;
    Document snapshot;jerzy::Parameters controls;AudioRenderOptions options;juce::File destination;
    juce::Result result=juce::Result::ok();
public:
    AudioRenderJob(ComposerView& view,Document data,jerzy::Parameters patch,AudioRenderOptions settings,juce::File file)
      :juce::ThreadWithProgressWindow("Rendering WAV",true,true,10000,"Cancel",&view),owner(&view),snapshot(std::move(data)),
       controls(std::move(patch)),options(settings),destination(std::move(file)) {}
    ~AudioRenderJob() override {signalThreadShouldExit();stopThread(-1);}
    void run() override {
        setStatusMessage("Rendering an independent snapshot. Cancel keeps existing files intact.");
        result=BassAmbientProcessor::renderAudio(std::move(snapshot),controls,options,destination,[this](double value){setProgress(value);return !threadShouldExit();});
    }
    void threadComplete(bool cancelled) override {
        if(owner){owner->renderAudioButton.setEnabled(true);if(!cancelled&&result.wasOk()&&!options.stems){owner->renderedAudio=destination;owner->dragAudio->setEnabled(true);}owner->setMessage(cancelled?"Audio render cancelled":result.wasOk()?"Audio saved: "+destination.getFullPathName():result.getErrorMessage());}
    }
};

class ComposerView::ExportPanel final : public juce::Component {
    ComposerView& view;
public:explicit ExportPanel(ComposerView& v):view(v){}void resized() override {view.layoutExport();}
    void paint(juce::Graphics& g) override {g.fillAll(juce::Colour(0xff19232f));g.setColour(ink);g.setFont(20);g.drawText("EKSPORT — MIDI / AUDIO"_ui,22,12,getWidth()-44,30,juce::Justification::centredLeft);const juce::String labels[]{"FORMAT","ZAKRES","JAKOŚĆ WAV"_ui,"CZĘSTOTLIWOŚĆ"_ui,"OGON WYBRZMIENIA","EFEKTY"};g.setFont(11);g.setColour(dim);for(int i=0;i<(view.exportFormat.getSelectedId()==2?2:6);++i)g.drawText(labels[i],22+(i%2)*310,65+(i/2)*90,290,20,juce::Justification::centredLeft);if(view.exportFormat.getSelectedId()==2){g.setColour(ink);g.setFont(13);g.drawText("Instrument wybierz w edytorze klipu. Nuty generatorów zostaną utrwalone."_ui,22,155,getWidth()-44,56,juce::Justification::centredLeft);}}
};
namespace {class ComposerExportWindow final : public juce::DialogWindow {public:ComposerExportWindow(juce::Component& c):DialogWindow("Eksport PRO",juce::Colour(0xff19232f),true){setUsingNativeTitleBar(true);setContentNonOwned(&c,true);centreWithSize(c.getWidth(),c.getHeight());}void closeButtonPressed() override{setVisible(false);}};}
ComposerView::ComposerView(BassAmbientProcessor& p):processor(p),sceneModel(std::make_unique<SceneModel>(*this)),songModel(std::make_unique<SongModel>(*this)),song(std::make_unique<SongList>(*this)),grid(std::make_unique<NoteGrid>(*this)),dragMidi(std::make_unique<MidiDrag>(*this)){
    setComponentID("composer-view");matrix=std::make_unique<Matrix>(*this);addAndMakeVisible(*matrix);
    exportPanel=std::make_unique<ExportPanel>(*this);
    overview.setComponentID("composer-overview");clipEditor.setComponentID("composer-edit-clip");exportButton.setComponentID("composer-export");
    for(auto* c:{&overview,&clipEditor,&exportButton})addAndMakeVisible(*c);overview.onClick=[this]{showClip(false);};clipEditor.onClick=[this]{showClip(true);};exportButton.onClick=[this]{showExport();};
    undoEdit.setComponentID("composer-undo");addAndMakeVisible(undoEdit);undoEdit.setEnabled(false);undoEdit.onClick=[this]{if(undoDocuments.empty())return;auto previous=std::move(undoDocuments.back());undoDocuments.pop_back();processor.editComposer([&](auto& d){d=std::move(previous);});selectedNote=-1;refresh(true);undoEdit.setEnabled(!undoDocuments.empty());};
    songMode.setButtonText("SONG");songMode.setTooltip("ON: odtwarzaj cały song. OFF: tylko wybraną scenę."_ui);loop.setButtonText("LOOP");launch.setButtonText("START SCENY");
    add.setButtonText("NOWA");duplicate.setButtonText("WARIANT");remove.setButtonText("USUŃ"_ui);save.setButtonText("ZAPISZ");load.setButtonText("WCZYTAJ");slotRemove.setButtonText("USUŃ"_ui);clear.setButtonText("WYCZYŚĆ"_ui);
    sceneRecall.addItemList({"Mikser bieżący"_ui,"Mikser zapisany w scenach"},1);addAndMakeVisible(sceneRecall);sceneRecall.onChange=[this]{processor.setParameterValue("sceneMixer",sceneRecall.getSelectedId()==2?1:0);};
    exportFormat.setComponentID("export-format");audioScope.setComponentID("export-scope");
    exportFormat.addItemList({"AUDIO WAV","MIDI"},1);exportFormat.setSelectedId(1);exportFormat.onChange=[this]{layoutExport();};exportPanel->addAndMakeVisible(exportFormat);
    addAndMakeVisible(clipEnabled);clipEnabled.onClick=[this]{edit([&](auto& d){d.scenes[size_t(selectedScene)].clips[size_t(selectedTrack)].enabled=clipEnabled.getToggleState();});};
    for(auto& l:inspectorLabels){addAndMakeVisible(l);l.setFont(11);l.setColour(juce::Label::textColourId,dim);}

    heading.setText("COMPOSER / ARRANGER  |  SCENES > SONG > MIDI",juce::dontSendNotification);sceneTitle.setText("SCENES   |   BASS / PAD / ARP / KICK / SNARE / HAT / AUX / KRELL",juce::dontSendNotification);songTitle.setText("SONG — drag scenes here; drag slots to reorder"_ui,juce::dontSendNotification);
    for(auto* label:{&heading,&sceneTitle,&songTitle,&clipTitle,&message}){label->setColour(juce::Label::textColourId,ink);addAndMakeVisible(label);}
    scenes.setModel(sceneModel.get());song->setModel(songModel.get());scenes.setRowHeight(54);song->setRowHeight(28);addAndMakeVisible(scenes);addAndMakeVisible(*song);addAndMakeVisible(*grid);addAndMakeVisible(*dragMidi);
    dragAudio=std::make_unique<AudioDrag>(*this);addAndMakeVisible(*dragAudio);
    audioScope.addItemList({"Scena","Instrument ze sceny","Cały song"_ui,"Osobne ścieżki — scena"_ui,"Osobne ścieżki — song"_ui},1);audioScope.setSelectedId(1);
    audioRate.addItemList({"44.1 kHz","48 kHz","96 kHz"},1);audioRate.setSelectedId(2);
    audioBits.addItemList({"24-bit PCM","32-bit float"},1);audioBits.setSelectedId(1);
    audioTail.addItemList({"TAIL: 0 s","TAIL: 5 s","TAIL: 10 s","TAIL: 20 s","TAIL: 45 s"},1);audioTail.setSelectedId(3);
    audioFX.addItemList({"WITH FX","DRY / NO FX"},1);audioFX.setSelectedId(1);
    for(auto* choice:{&audioScope,&audioRate,&audioBits,&audioTail,&audioFX})addAndMakeVisible(*choice);
    audioScope.setTooltip("Scene, selected scene instrument, full song or time-aligned stems. Stems include separate delay/reverb returns, before MAIN master FX.");
    audioTail.setTooltip("Additional time after the arrangement for release envelopes and effect tails. Fixed duration; 0 cuts at scene/song end.");
    audioFX.setTooltip("WITH FX enables racks at their stored amounts and active sends for this export. DRY disables racks and sends; synthesis drive remains part of the sound.");
    addAndMakeVisible(renderAudioButton);renderAudioButton.onClick=[this]{chooseAudioRender();};
    midiRange.addItemList({"SCENE","WHOLE SONG"},1);midiRange.setSelectedId(1);midiRange.setComponentID("composer-midi-range");midiRange.setTooltip("MIDI drag: selected scene or complete arranged song, including repeats.");addAndMakeVisible(midiRange);
    midiTracks.addItemList({"SELECTED TRACK","ALL TRACKS"},1);midiTracks.setSelectedId(1);midiTracks.setComponentID("composer-midi-tracks");midiTracks.setTooltip("Selected instrument for one Piano Roll, or all instruments for multitrack MIDI import.");addAndMakeVisible(midiTracks);
    juce::ToggleButton* toggles[]{&enabled,&play,&songMode,&loop,&split,&midiPlay};const char* ids[]{"composerEnabled","composerPlaying","composerSongMode","composerLoop","separateOutputs","midiPlayback"};
    for(int i=0;i<6;++i){addAndMakeVisible(*toggles[i]);attachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.state,ids[i],*toggles[i]));}
    for(auto* button:{&add,&duplicate,&remove,&launch,&capture,&append,&slotRemove,&up,&down,&freeze,&clear,&exportScene,&exportSong,&exportPack,&save,&load})addAndMakeVisible(*button);
    for(auto* button:{&generated,&cut,&evolve})addAndMakeVisible(*button);
    for(auto* editor:{&name,&progression}){addAndMakeVisible(*editor);editor->setColour(juce::TextEditor::backgroundColourId,dark);editor->setColour(juce::TextEditor::textColourId,ink);}
    name.setTooltip("Scene name; Return commits");progression.setTooltip("One chord per bar, repeating: Cm Ab Fm G. Supports major, m, 5, sus4, m7, maj7, m9, maj9. Return commits.");
    slider(bars,1,32,1,"Scene bars");slider(tempo,30,240,1,"BPM (set host tempo in FL Studio)");slider(intensity,0,1,.01,"Scene intensity");slider(repeat,1,16,1,"Song slot repeats");slider(bar,1,32,1,"Edit bar");slider(octave,0,7,1,"Lowest MIDI octave");slider(velocity,1,127,1,"Note velocity");slider(notePan,-1,1,.01,"Note pan");slider(gain,0,1,.01,"Track gain");slider(enter,1,32,1,"Track enters at bar");slider(leave,0,32,1,"Last playing bar (0 = end)");slider(threshold,0,1,.01,"Minimum scene intensity for this instrument");
    for(auto* s:{&bars,&tempo,&intensity,&repeat,&bar,&octave,&velocity,&notePan,&gain,&enter,&leave,&threshold})addAndMakeVisible(*s);octave.setValue(2);velocity.setValue(100);
    for(int n=1;n<=12;++n)numerator.addItem(juce::String(n),n);denominator.addItemList({"2","4","8","16"},1);addAndMakeVisible(numerator);addAndMakeVisible(denominator);
    name.onReturnKey=[this]{edit([&](auto& d){d.scenes[size_t(selectedScene)].name=name.getText().toStdString();});};progression.onReturnKey=[this]{updateProgression();};
    bars.onValueChange=[this]{if(!updating)edit([&](auto& d){d.scenes[size_t(selectedScene)].bars=int(bars.getValue());});};
    tempo.onValueChange=[this]{if(!updating){edit([&](auto& d){d.tempo=tempo.getValue();for(auto& s:d.scenes)s.patch.internalTempo=d.tempo;});processor.setParameterValue("internalTempo",float(tempo.getValue()));}};
    intensity.onValueChange=[this]{if(!updating)edit([&](auto& d){d.scenes[size_t(selectedScene)].intensity=intensity.getValue();});};
    auto meter=[this]{if(!updating)edit([&](auto& d){auto& s=d.scenes[size_t(selectedScene)];s.numerator=numerator.getSelectedId();s.denominator=denominator.getText().getIntValue();});};numerator.onChange=meter;denominator.onChange=meter;
    generated.onClick=[this]{edit([&](auto& d){d.scenes[size_t(selectedScene)].clips[size_t(selectedTrack)].generated=generated.getToggleState();});};
    cut.onClick=[this]{edit([&](auto& d){d.scenes[size_t(selectedScene)].cutTails=cut.getToggleState();});};
    for(int t=0;t<8;++t){auto& b=tracks[size_t(t)];b.setButtonText(trackNames[t]);b.onClick=[this,t]{if(juce::ModifierKeys::getCurrentModifiers().isShiftDown())edit([&](auto& d){auto& c=d.scenes[size_t(selectedScene)].clips[size_t(t)];c.enabled=!c.enabled;});selectTrack(t);};addAndMakeVisible(b);}
    gain.onValueChange=[this]{if(!updating)edit([&](auto& d){d.scenes[size_t(selectedScene)].clips[size_t(selectedTrack)].gain=gain.getValue();});};
    enter.onValueChange=[this]{if(!updating)edit([&](auto& d){d.scenes[size_t(selectedScene)].clips[size_t(selectedTrack)].enterBar=int(enter.getValue())-1;});};
    leave.onValueChange=[this]{if(!updating)edit([&](auto& d){d.scenes[size_t(selectedScene)].clips[size_t(selectedTrack)].leaveBar=int(leave.getValue());});};
    threshold.onValueChange=[this]{if(!updating)edit([&](auto& d){d.scenes[size_t(selectedScene)].clips[size_t(selectedTrack)].threshold=threshold.getValue();});};
    bar.onValueChange=[this]{grid->repaint();};octave.onValueChange=[this]{grid->repaint();};
    auto noteEdit=[this]{if(!updating&&selectedNote>=0)edit([&](auto& d){auto& c=d.scenes[size_t(selectedScene)].clips[size_t(selectedTrack)];if(selectedNote<int(c.notes.size())){auto& n=c.notes[size_t(selectedNote)];n.velocity=int(velocity.getValue());n.pan=notePan.getValue();n.length=std::min(noteLength.getValue(),d.scenes[size_t(selectedScene)].beats()-n.beat);}});};velocity.onValueChange=noteEdit;notePan.onValueChange=noteEdit;
    add.onClick=[this]{edit([&](auto& d){if(d.scenes.size()>=maxScenes)return;Scene s;s.id=d.nextId++;s.name="Custom "+std::to_string(s.id);s.patch=processor.readParameters();for(auto& c:s.clips)c.generated=false;d.scenes.push_back(s);selectedScene=int(d.scenes.size()-1);});};
    duplicate.onClick=[this]{edit([&](auto& d){if(d.scenes.size()>=maxScenes)return;auto s=d.scenes[size_t(selectedScene)];s.id=d.nextId++;s.name+=" variation";d.scenes.push_back(s);selectedScene=int(d.scenes.size()-1);});};
    remove.onClick=[this]{edit([&](auto& d){if(d.scenes.size()<2)return;const int id=d.scenes[size_t(selectedScene)].id;d.scenes.erase(d.scenes.begin()+selectedScene);d.song.erase(std::remove_if(d.song.begin(),d.song.end(),[&](const Slot& s){return s.sceneId==id;}),d.song.end());selectedScene=std::min(selectedScene,int(d.scenes.size())-1);});};
    launch.onClick=[this]{processor.launchComposer(selectedScene,false);};capture.onClick=[this]{edit([&](auto& d){d.scenes[size_t(selectedScene)].patch=processor.readParameters();});setMessage("Sound captured. Scene masks control arrangement. Module tab edits now update the selected playing scene automatically.");};
    append.onClick=[this]{edit([&](auto& d){if(d.song.size()<maxSlots)d.song.push_back({d.scenes[size_t(selectedScene)].id,1,false});});};
    slotRemove.onClick=[this]{edit([&](auto& d){if(selectedSlot>=0&&selectedSlot<int(d.song.size()))d.song.erase(d.song.begin()+selectedSlot);});};
    up.onClick=[this]{edit([&](auto& d){if(selectedSlot>0&&selectedSlot<int(d.song.size())){std::swap(d.song[size_t(selectedSlot)],d.song[size_t(selectedSlot-1)]);--selectedSlot;}});};
    down.onClick=[this]{edit([&](auto& d){if(selectedSlot>=0&&selectedSlot+1<int(d.song.size())){std::swap(d.song[size_t(selectedSlot)],d.song[size_t(selectedSlot+1)]);++selectedSlot;}});};
    repeat.onValueChange=[this]{if(!updating)edit([&](auto& d){if(selectedSlot<int(d.song.size())&&selectedSlot>=0)d.song[size_t(selectedSlot)].repeats=int(repeat.getValue());});};
    evolve.onClick=[this]{edit([&](auto& d){if(selectedSlot<int(d.song.size())&&selectedSlot>=0)d.song[size_t(selectedSlot)].evolve=evolve.getToggleState();});};
    freeze.onClick=[this]{edit([&](auto& d){jerzy::composer::freeze(d.scenes[size_t(selectedScene)],selectedTrack);});};clear.onClick=[this]{edit([&](auto& d){auto& c=d.scenes[size_t(selectedScene)].clips[size_t(selectedTrack)];c.notes.clear();c.generated=false;});selectedNote=-1;};
    exportScene.onClick=[this]{chooseExport(0);};exportSong.onClick=[this]{chooseExport(1);};exportPack.onClick=[this]{chooseExport(2);};save.onClick=[this]{chooseProject(true);};load.onClick=[this]{chooseProject(false);};
    play.onClick=[this]{if(play.getToggleState())processor.launchComposer(selectedScene,songMode.getToggleState());};
    juce::Slider* knobs[]{&bars,&tempo,&intensity,&repeat,&bar,&octave,&velocity,&notePan,&gain,&enter,&leave,&threshold};const double defaults[]{4,120,1,1,1,2,100,0,1,1,0,0};
    for(int i=0;i<12;++i){knobs[i]->setPopupMenuEnabled(false);knobs[i]->setDoubleClickReturnValue(true,defaults[i]);knobs[i]->getProperties().set("proDefault",defaults[i]);knobs[i]->addMouseListener(this,false);}
    slider(noteLength,.25,128,.25,"Długość nuty (ćwierćnuty)"_ui);noteLength.setValue(1);addAndMakeVisible(noteLength);noteLength.onValueChange=noteEdit;
    for(auto* c:std::initializer_list<juce::Component*>{&audioScope,&audioRate,&audioBits,&audioTail,&audioFX,&renderAudioButton,dragAudio.get(),&exportScene,&exportSong,&exportPack,dragMidi.get(),&midiRange,&midiTracks,&message})exportPanel->addAndMakeVisible(*c);
    exportScene.setButtonText("ZAPISZ MIDI");exportScene.onClick=[this]{chooseExport(audioScope.getSelectedId()==3?1:0,audioScope.getSelectedId()==2?selectedTrack:-1);};
    audioScope.onChange=[this]{layoutExport();};capture.setButtonText("ZAPISZ BRZMIENIE DO SCENY");freeze.setButtonText("ZAMIEŃ GENERATOR NA NUTY"_ui);
    generated.setButtonText("GENERATOR");cut.setButtonText("UCINAJ OGONY");showClip(false);
    refresh(true);startTimerHz(10);
}
ComposerView::~ComposerView(){exportWindow.reset();audioJob.reset();stopTimer();scenes.setModel(nullptr);song->setModel(nullptr);}
void ComposerView::mouseDown(const juce::MouseEvent& e){if(e.mods.isRightButtonDown())if(auto* s=dynamic_cast<juce::Slider*>(e.eventComponent))s->setValue(double(s->getProperties()["proDefault"]));}
void ComposerView::setMessage(const juce::String& text){message.setText(text,juce::dontSendNotification);clipTitle.setText(text,juce::dontSendNotification);}
void ComposerView::edit(const std::function<void(Document&)>& fn){try{auto before=processor.composerDocument();processor.editComposer(fn);if(undoDocuments.size()>=40)undoDocuments.erase(undoDocuments.begin());undoDocuments.push_back(std::move(before));undoEdit.setEnabled(true);refresh(true);processor.setParameterValue("composerScene",float(selectedScene+1));}catch(const std::exception& e){setMessage(e.what());}}
void ComposerView::selectScene(int index){selectedSlot=-1;selectedScene=juce::jlimit(0,int(document.scenes.size())-1,index);selectedNote=-1;processor.setParameterValue("composerScene",float(selectedScene+1));bar.setValue(1);refresh(true);}
void ComposerView::selectTrack(int index){selectedTrack=juce::jlimit(0,7,index);selectedNote=-1;refresh(true);}
void ComposerView::prepareManual(Scene& scene){if(scene.clips[size_t(selectedTrack)].generated&&(selectedTrack>=3&&selectedTrack<=6))jerzy::composer::freeze(scene,selectedTrack);}
juce::String ComposerView::progressionText() const {if(document.scenes.empty())return {};juce::StringArray result;const char* suffix[]{"","m","5","sus4","m7","maj7","m9","maj9"};for(const auto& c:document.scenes[size_t(selectedScene)].chords)result.add(juce::MidiMessage::getMidiNoteName(c.root,true,false,3)+suffix[c.quality]);return result.joinIntoString(" ");}
void ComposerView::updateProgression(){auto tokens=juce::StringArray::fromTokens(progression.getText()," ,|","");std::vector<Chord> chords;
    for(auto token:tokens){if(token.isEmpty())continue;const auto root=token.substring(0,1).toUpperCase();const juce::String letters="C D EF G A B";int pitch=letters.indexOf(root);if(pitch<0){setMessage("Chord example: Cm Ab Fm G. Use C D E F G A B with # or b.");return;}int pos=1;if(token.substring(1,2)=="#"){++pitch;++pos;}else if(token.substring(1,2)=="b"){--pitch;++pos;}const auto suffix=token.substring(pos).toLowerCase();int quality=suffix=="m"?1:suffix=="5"?2:suffix=="sus4"?3:suffix=="m7"?4:suffix=="maj7"?5:suffix=="m9"?6:suffix=="maj9"?7:suffix.isEmpty()?0:-1;
        if(quality<0){setMessage("Supported: major, m, 5, sus4, m7, maj7, m9, maj9");return;}chords.push_back({36+(pitch+12)%12,quality});}
    if(chords.empty()){setMessage("Enter at least one chord");return;}edit([&](auto& d){d.scenes[size_t(selectedScene)].chords=chords;});}
void ComposerView::refresh(bool force){const auto next=processor.composerRevision.load();if(!force&&revision==next)return;revision=next;document=processor.composerDocument();updating=true;selectedScene=juce::jlimit(0,int(document.scenes.size())-1,selectedScene);selectedSlot=document.song.empty()||selectedSlot<0?-1:juce::jlimit(0,int(document.song.size())-1,selectedSlot);
    const auto& s=document.scenes[size_t(selectedScene)];const auto& c=s.clips[size_t(selectedTrack)];name.setText(s.name,false);progression.setText(progressionText(),false);bars.setValue(s.bars,juce::dontSendNotification);tempo.setValue(document.tempo,juce::dontSendNotification);intensity.setValue(s.intensity,juce::dontSendNotification);numerator.setSelectedId(s.numerator,juce::dontSendNotification);denominator.setText(juce::String(s.denominator),juce::dontSendNotification);cut.setToggleState(s.cutTails,juce::dontSendNotification);
    bar.setRange(1,s.bars,1);if(bar.getValue()<1)bar.setValue(1,juce::dontSendNotification);gain.setValue(c.gain,juce::dontSendNotification);enter.setValue(c.enterBar+1,juce::dontSendNotification);leave.setValue(c.leaveBar,juce::dontSendNotification);threshold.setValue(c.threshold,juce::dontSendNotification);generated.setToggleState(c.generated,juce::dontSendNotification);generated.setButtonText(c.generated?"GENERATOR: ROOT":"NUTY MANUALNE");clipEnabled.setToggleState(c.enabled,juce::dontSendNotification);freeze.setEnabled(c.generated);sceneRecall.setSelectedId(processor.readParameters().sceneMixer?2:1,juce::dontSendNotification);
    if(selectedSlot>=0){repeat.setValue(document.song[size_t(selectedSlot)].repeats,juce::dontSendNotification);evolve.setToggleState(document.song[size_t(selectedSlot)].evolve,juce::dontSendNotification);}
    if(selectedNote>=0&&selectedNote<int(c.notes.size())){velocity.setValue(c.notes[size_t(selectedNote)].velocity,juce::dontSendNotification);notePan.setValue(c.notes[size_t(selectedNote)].pan,juce::dontSendNotification);noteLength.setValue(c.notes[size_t(selectedNote)].length,juce::dontSendNotification);}
    for(int t=0;t<8;++t){ProVisual::tint(tracks[size_t(t)],colours[t]);tracks[size_t(t)].setToggleState(t==selectedTrack,juce::dontSendNotification);tracks[size_t(t)].setButtonText(juce::String(trackNames[t])+(s.clips[size_t(t)].enabled?" ON":" OFF"));}
    clipTitle.setColour(juce::Label::textColourId,colours[selectedTrack]);
    juce::String warning;
    if(!c.notes.empty()){bool overlap=false;const double from=c.enterBar*s.barBeats(),to=c.leaveBar?c.leaveBar*s.barBeats():s.beats();for(const auto& note:c.notes)overlap|=note.beat+note.length>from&&note.beat<to;if(!overlap)warning="   |   NOTES OUTSIDE ACTIVE BARS";}
    clipTitle.setText(!clipMode?"SCENA: "+juce::String(s.name)+"  |  "+juce::String(s.bars)+" taktów"_ui : "KLIP: "+juce::String(s.name)+" / "+trackNames[selectedTrack]+"   |   ENTER BAR "+juce::String(c.enterBar+1)+"   |   LAST "+(c.leaveBar?juce::String(c.leaveBar):juce::String("END"))+(c.enabled?"":"   |   MUTED")+warning,juce::dontSendNotification);
    scenes.updateContent();song->updateContent();scenes.selectRow(selectedScene, false, false);if(selectedSlot>=0)song->selectRow(selectedSlot,true,false);grid->repaint();matrix->repaint();updating=false;resized();
}
void ComposerView::timerCallback(){tempo.setEnabled(processor.readParameters().clockSource==0);if(!tempo.isEnabled())tempo.setValue(processor.tempoDisplay.load(),juce::dontSendNotification);refresh();song->repaint();const auto error=processor.composerError();if(error.isNotEmpty())setMessage(error);}
void ComposerView::chooseExport(int mode,int track){chooser=std::make_unique<juce::FileChooser>(mode==2?"Export MIDI pattern pack":"Export standard MIDI",juce::File{},mode==2?"":"*.mid");const int flags=mode==2?juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectDirectories:juce::FileBrowserComponent::saveMode|juce::FileBrowserComponent::canSelectFiles|juce::FileBrowserComponent::warnAboutOverwriting;
    juce::Component::SafePointer<ComposerView> safe(this);chooser->launchAsync(flags,[safe,mode,track](const auto& choice){if(!safe)return;const auto file=choice.getResult();if(file==juce::File{})return;
        if(mode!=2){auto result=safe->processor.exportComposer(file.withFileExtension("mid"),mode==1,safe->selectedScene,track);safe->setMessage(result.wasOk()?"MIDI exported: "+file.getFullPathName():result.getErrorMessage());return;}
        try{auto d=safe->processor.composerDocument();d.tempo=safe->processor.readParameters().clockSource==0?safe->processor.readParameters().internalTempo:safe->processor.tempoDisplay.load();const auto compiled=compile(d);auto folder=file.getNonexistentChildFile("JerzyPRO-patterns",{},false);auto result=folder.createDirectory();if(result.failed()){safe->setMessage(result.getErrorMessage());return;}
            result=writeMidi(compiled.song,d.tempo,folder.getChildFile("SONG.mid"));for(size_t i=0;i<d.scenes.size()&&result.wasOk();++i)for(int t=0;t<8&&result.wasOk();++t){auto name=juce::String(int(i)+1)+"-"+juce::File::createLegalFileName(juce::String(d.scenes[i].name))+"-"+trackNames[t]+".mid";result=writeMidi(compiled.scenes[i],d.tempo,folder.getChildFile(name),t);}safe->setMessage(result.wasOk()?"Pattern pack exported: "+folder.getFullPathName():result.getErrorMessage());
        }catch(const std::exception& e){safe->setMessage(e.what());}});
}
void ComposerView::chooseAudioRender() {
    if(audioJob&&audioJob->isThreadRunning())return;
    AudioRenderOptions options;const int scope=audioScope.getSelectedId();
    options.song=scope==3||scope==5;options.stems=scope>=4;options.scene=selectedScene;
    options.track=scope==2?selectedTrack:-1;
    const double rates[]{44100,48000,96000};const double tails[]{0,5,10,20,45};
    options.sampleRate=rates[audioRate.getSelectedId()-1];options.bits=audioBits.getSelectedId()==2?32:24;
    options.tailSeconds=tails[audioTail.getSelectedId()-1];options.effects=audioFX.getSelectedId()==1;
    auto data=processor.composerDocument();auto patch=processor.readParameters();
    data.tempo=patch.clockSource==0?patch.internalTempo:processor.tempoDisplay.load();
    if(options.song&&data.song.empty()){setMessage("SONG is empty. Add scenes first.");return;}
    chooser=std::make_unique<juce::FileChooser>(options.stems?"Choose parent folder for WAV stems":"Render stereo WAV",juce::File{},options.stems?"":"*.wav");
    const int flags=options.stems?juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectDirectories:
        juce::FileBrowserComponent::saveMode|juce::FileBrowserComponent::canSelectFiles|juce::FileBrowserComponent::warnAboutOverwriting;
    juce::Component::SafePointer<ComposerView> safe(this);
    chooser->launchAsync(flags,[safe,options,data=std::move(data),patch](const auto& choice) mutable {
        if(!safe)return;auto file=choice.getResult();if(file==juce::File{})return;
        file=options.stems?file.getNonexistentChildFile(options.song?"PRO-song-stems":"PRO-scene-stems",{},false):file.withFileExtension("wav");
        safe->audioJob=std::make_unique<AudioRenderJob>(*safe,std::move(data),patch,options,file);
        safe->renderAudioButton.setEnabled(false);safe->setMessage("Rendering WAV...");safe->audioJob->launchThread();
    });
}
void ComposerView::chooseProject(bool saving){chooser=std::make_unique<juce::FileChooser>(saving?"Save Composer project":"Load Composer project",juce::File{},"*.jpro");const int flags=(saving?juce::FileBrowserComponent::saveMode|juce::FileBrowserComponent::warnAboutOverwriting:juce::FileBrowserComponent::openMode)|juce::FileBrowserComponent::canSelectFiles;
    juce::Component::SafePointer<ComposerView> safe(this);chooser->launchAsync(flags,[safe,saving](const auto& choice){if(!safe)return;const auto file=choice.getResult();if(file==juce::File{})return;const auto result=saving?safe->processor.saveComposer(file.withFileExtension("jpro")):safe->processor.loadComposer(file);safe->setMessage(result.wasOk()?"Composer project saved / loaded":result.getErrorMessage());safe->refresh(true);});
}
void ComposerView::showClip(bool show){clipMode=show;overview.setToggleState(!show,juce::dontSendNotification);clipEditor.setToggleState(show,juce::dontSendNotification);if(!document.scenes.empty())refresh(true);resized();}
void ComposerView::showExport(){exportPanel->setLookAndFeel(&getLookAndFeel());exportPanel->setSize(650,450);layoutExport();if(!exportWindow)exportWindow=std::make_unique<ComposerExportWindow>(*exportPanel);exportWindow->setLookAndFeel(&getLookAndFeel());exportWindow->setVisible(true);exportWindow->toFront(true);}
void ComposerView::layoutExport(){bool midi=exportFormat.getSelectedId()==2;if(midi&&audioScope.getSelectedId()>3)audioScope.setSelectedId(1,juce::dontSendNotification);audioScope.setItemEnabled(4,!midi);audioScope.setItemEnabled(5,!midi);
    exportFormat.setBounds(22,88,290,34);audioScope.setBounds(332,88,290,34);audioBits.setBounds(22,178,290,34);audioRate.setBounds(332,178,290,34);audioTail.setBounds(22,268,290,34);audioFX.setBounds(332,268,290,34);
    for(auto* c:{&audioBits,&audioRate,&audioTail,&audioFX})c->setVisible(!midi);renderAudioButton.setVisible(!midi);dragAudio->setVisible(!midi);exportScene.setVisible(midi);exportPack.setVisible(midi);dragMidi->setVisible(midi);exportSong.setVisible(false);midiRange.setVisible(false);midiTracks.setVisible(false);
    midiRange.setSelectedId(audioScope.getSelectedId()==3?2:1,juce::dontSendNotification);midiTracks.setSelectedId(audioScope.getSelectedId()==2?1:2,juce::dontSendNotification);
    renderAudioButton.setBounds(22,370,175,34);exportScene.setBounds(22,370,175,34);exportPack.setBounds(207,370,160,34);dragAudio->setBounds(422,370,200,34);dragMidi->setBounds(422,370,200,34);message.setBounds(22,320,600,42);exportPanel->repaint();}
void ComposerView::paint(juce::Graphics& g){g.fillAll(juce::Colour(0xff111923));const int right=getWidth()-248;g.setColour(juce::Colour(0xff192430));g.fillRoundedRectangle(float(right),52,240,float(getHeight()-60),6);g.setColour(juce::Colour(0xff354657));g.drawRoundedRectangle(float(right),52,240,float(getHeight()-60),6,1);}
void ComposerView::resized(){if(!matrix)return;const int w=getWidth(),h=getHeight(),left=176,right=w-248,centerWidth=std::max(200,right-left-12);
    for(auto* c:getChildren())if(c!=exportPanel.get())c->setVisible(false);
    auto place=[](juce::Component& c,int x,int y,int width,int height){c.setVisible(true);c.setBounds(x,y,width,height);};
    place(overview,8,4,140,30);place(clipEditor,156,4,140,30);place(songMode,306,4,112,30);place(loop,426,4,96,30);place(launch,530,4,130,30);place(undoEdit,668,4,88,30);
    place(scenes,8,52,156,h-145);place(add,8,h-84,72,28);place(duplicate,84,h-84,80,28);place(remove,8,h-50,72,28);place(save,84,h-50,80,28);
    const int x=right+12,iw=216;auto field=[&](int i,const juce::String& label,juce::Component& c,int y,int height=30){inspectorLabels[size_t(i)].setText(label,juce::dontSendNotification);place(inspectorLabels[size_t(i)],x,y,iw,19);place(c,x,y+21,iw,height);};
    if(!clipMode){field(0,"NAZWA SCENY",name,62);field(1,"DŁUGOŚĆ SCENY — TAKTY"_ui,bars,124);field(2,"PROGRESJA AKORDÓW"_ui,progression,186);}else{field(0,"UDZIAŁ INSTRUMENTU"_ui,clipEnabled,62);field(1,"WEJŚCIE OD TAKTU"_ui,enter,124);field(2,"OSTATNI TAKT (0 = KONIEC)",leave,186);}
    place(clipTitle,left,34,centerWidth,16);if(!clipMode){place(*song,left,52,centerWidth,178);place(*matrix,left,248,centerWidth,std::max(170,h-300));place(append,left,h-42,84,28);place(slotRemove,left+92,h-42,84,28);place(load,left+184,h-42,80,28);
        if(selectedSlot>=0){field(3,"POWTÓRZENIA KLOCKA"_ui,repeat,248);place(evolve,x,310,iw,28);}else field(3,"INTENSYWNOŚĆ SCENY"_ui,intensity,248);
        place(sceneRecall,x,350,iw,30);place(capture,x,388,iw,34);place(cut,x,432,iw,28);place(numerator,x,468,100,28);place(denominator,x+112,468,100,28);numerator.setTooltip("Metrum sceny — licznik"_ui);denominator.setTooltip("Metrum sceny — mianownik"_ui);
    }else{
        const int tw=centerWidth/8;for(int t=0;t<8;++t)place(tracks[size_t(t)],left+t*tw,52,tw-3,30);place(generated,left,91,196,30);place(freeze,left+204,91,centerWidth-204,30);
        place(bar,left,136,135,28);place(octave,left+145,136,140,28);place(clear,left+295,136,centerWidth-295,28);place(*grid,left,176,centerWidth,std::max(150,h-230));
        field(3,"DŁUGOŚĆ NUTY — ĆWIERĆNUTY"_ui,noteLength,248);field(4,"VELOCITY",velocity,310);field(5,"PANORAMA NUTY",notePan,372);place(gain,left,h-42,150,28);place(threshold,left+170,h-42,180,28);
        noteLength.setTooltip("1 = ćwierćnuta, 4 = cała nuta. Przeciągnij prawą krawędź nuty, aby zmienić długość."_ui);bar.setTooltip("Edytowany takt sceny");octave.setTooltip("Zakres widoku klawiatury — nie transponuje nut"_ui);
    }
}
