# FL Studio integration — development branch v0.1
## Available bridge workflow
1. `python -m jerzybridge bass bass.jpf.json --chords Am,F,C,G --style rock`.
2. `python -m jerzybridge to-midi bass.jpf.json bass.mid`.
3. Import BASS MIDI track to FL Studio Piano Roll; select DIRECT to play exact note positions and velocities.
4. Import CHORD GUIDE into separate track to guide ambient harmony; this guide is NOT automatic chord following in the VST3.
5. Use ambient pad envelope on separate VST channel if a continuous soundscape is required.
## Styles
rock, dub, acid, funk, ambient (external generator, not added to internal presets).
## Verification required
Check host tempo jumps, start/stop, tail release and memory usage before plugin-level sequencer export. Preserve sound quality and current bank/state IDs.
No DSP/GUI changes were merged by this integration notes commit.