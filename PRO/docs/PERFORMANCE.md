# PRO 0.6 — Composer, Mixer and Clock

## Start music in the standalone

1. Select your output device in the standalone Audio/MIDI settings.
2. Keep the top CLOCK selector on INTERNAL and set BPM or use TAP.
3. Open COMPOSER, select a scene and press LAUNCH SCENE. For the full arrangement enable SONG and press PLAY.
4. The top STOP button stops both Composer and Generate. PANIC silences held voices.

HOST / FL STUDIO follows the host tempo and transport. Select this in the VST3 when FL Studio controls playback. MIDI CLOCK follows 24-PPQN realtime clock, Start, Continue, Stop and Song Position Pointer from an enabled MIDI input. SYNC indicates received clock; WAIT or LOST identifies a missing clock. BPM editing and TAP are available only on INTERNAL. MIDI Clock is not an audio connection: still select an audio output device.

## Manual notes or module generation

Each BASS, AMBIENT and ARP clip has a MODE button:

- MANUAL plays the written pitches and durations exactly, bypassing phrase generation.
- MODULE treats the written notes as root gates. One long G root drives the bass phrase, an Ambient chord/Krell texture or an arpeggio for the length of that gate. Harmony comes from the scene chord progression. With no custom root gates, factory clips follow one chord/root per bar. After editing/deleting explicit roots, an empty clip stays silent.

Edit root gates directly on the grid: click to add/select, drag horizontally to change duration, right-click to delete. The latest root wins where roots overlap. FREEZE / EDIT renders the module phrase into ordinary MANUAL notes. Drum MODULE mode uses the 64-step drum grid; freeze it to edit individual hits.

Module parameter edits update the selected Composer scene in the worker, and the compiled playback resynchronises at the current song position. CAPTURE SOUND explicitly saves the complete current sound and mixer snapshot in that scene. For fully independent scene sounds, select/launch the intended scene before editing its module controls. Editing a running phrase may rearticulate held notes; it does not restart the whole arrangement.

MIDI scene/song/pattern exports contain the resulting notes, not only root instructions. The synthesizer/FX sound remains in the plugin or .jpro project. MIDI files do not encode audio effects or per-note stereo synthesis. Set the desired export tempo using INTERNAL; HOST/MIDI song export records the currently received tempo. Import the MIDI file into FL Studio or drag the clip from the plugin; native FL pattern files are not authored.

## Mixer

The MIXER page has eight channel strips: Bass, Ambient, Arp, Kick, Snare, Hi-hat Aux and Krell. Each strip has POWER, Mute, Solo, stereo balance, a vertical fader, meter and post-fader Delay/Reverb sends. Mute and Solo also mute new send input. Existing return tails can continue; use return power or FX POWER to cut/release the associated buffers.

POWER stops that instrument's DSP. Mute keeps its voices running so it can return in time. The global DRUMS power switch still controls all four drum voices. Right-click resets a mixer control; Ctrl+right-click arms MIDI Learn. All mixer parameters are host-automatable and are saved in plugin state.

Delay and Reverb have separate power switches, returns, feedback/time division and room size. FX POWER is the global authority for instrument racks, master rack and both sends; a scene saved with FX off cannot silently override it. With SCENE MIX off, the visible MASTER rack and its amount apply globally. The master fader and soft limiter control MAIN. Stems and returns are post master level, before MAIN's master FX/DC filter/limiter: leave headroom on the host mixer.

SCENE MIX recalls mixer settings from each scene patch (save them with CAPTURE SOUND). When off, the visible mixer is a global performance mix across all scenes. With SCENE MIX enabled the stored scene settings override global strip faders; turn it off to mix manually.

In FL Studio enable the plugin wrapper's extra output buses and map them to mixer inserts. SPLIT OUT removes connected stems/returns from MAIN, avoiding doubled audio; outputs that are not connected continue to MAIN. The eleven stereo buses are MAIN, Bass, Pad, Arp, Kick, Snare, Hi-hat, Aux, Delay Return, Reverb Return, Krell. The standalone normally uses MAIN stereo.

## Activity LEDs

Module tabs and mixer strips show IN (yellow note/control activity), ACT (green voice activity), OUT (turquoise audio above the meter threshold), and CLP (red when that output reaches 0 dBFS). IN pulses persist for 150 ms; meters decay so short sounds remain visible. A muted instrument can have IN and ACT while OUT is dark. OUT can remain lit for a release/FX tail after input stops. Instrument identity colours are separate from these status colours.

## Validation boundaries

Automated DSP/host/GUI tests exercise root gates, transposition, manual bypass, mixer mute/solo/pan, multi-output routing, MIDI clock and state. Actual FL Studio wrapper routing, real audio/MIDI devices and Raspberry Pi 4 hardware require testing on those systems. This release does not add a Raspberry Pi hardware build or realtime MIDI output; use MIDI export.

## Six FX racks (0.6)

The FX rack selector has BASS, AMBIENT, MASTER, ARPEGGIO, DRUMS and KRELL. Each has independent effect settings, order and FX amount. Raise Arpeggio FX amount or Drums FX amount above zero to engage the new racks. They process the instrument before the mixer and send effects. DRUMS shares settings across four independent voice processors, preserving Kick/Snare/Hi-hat/Aux isolation on the separate FL mixer outputs. Disabling FX power, the instrument or its FX amount releases the associated large buffers through the maintenance worker.

With SCENE MIX off, MASTER, ARPEGGIO and DRUMS racks follow the visible global controls. With SCENE MIX on they recall the scene patch. FX POWER always remains a global switch; a saved scene FX OFF no longer blocks live mixer sends when FX POWER is ON.

## Bass Guitar (0.6)

Select Bass Guitar in the BASS model menu. This is a procedural plucked-string waveguide, not a sampled guitar. Finger/Pick/Slap change excitation; string tone, palm mute, decay and pluck position control the string. The existing AMP envelope, filter, drive, MIDI velocity, pitch bend and glide still apply. Both MANUAL notes and MODULE-generated bass lines can play it. String storage is allocated during prepare, with no allocation on note-on.

Bass styles now include Rock (eighth-note drive and fifths), Metal (gallops and short gates), Punk (driving roots/fifths), and Blues (walking major-third/fifth/sixth/flat-seventh vocabulary). Use 1/16 division for the base grid; set Swing near 0.6 for blues shuffle. Density, movement, fills and humanisation remain available. Blues deliberately adds idiomatic chromatic notes outside the selected scale.

## Linked harmony and independent Krell (0.6)

The always-visible HARMONY panel provides ROOT and SCALE. LINK is optional and defaults off for existing independent parts. With LINK on, generated Ambient, Arp and Krell parts follow the Bass root gates and share the global scale. Changing ROOT transposes generated root gates and scene progression relative to the Bass/scene anchor. MANUAL pitches remain unchanged. Writing the first Bass root on the Composer grid, or playing a MIDI bass root, updates the shared ROOT while LINK is on. Chord qualities explicitly written in a scene progression remain that progression's harmonic instructions.

KRELL is an independent instrument and tab, with evolving FM, resonant noise and harmonic-cloud synthesis, separate attack/release, cycle range, pitch range, stereo motion, power, FX rack, Composer clip, MIDI channel 4 and mixer strip. Its output bus is appended after the existing Delay/Reverb buses to preserve previous bus indices. The Ambient legacy Krell mode remains available for compatibility. Select Independent Krell from Factory scenes and press PLAY for a quick standalone audition; inside Composer enable the KRELL clip in the scene. CC34 controls Krell power by default.

Bass Guitar Groove, Rock/Metal/Punk/Blues Bass Guitar presets and Independent Krell are included in the factory menu.
