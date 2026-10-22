# PRO visual language

Instrument identity uses one shared RGB palette in `Source/VisualPalette.h`.

| Instrument | RGB | Hex |
| --- | --- | --- |
| Bass | 255, 173, 112 | #FFAD70 |
| Ambient / Pad | 180, 161, 255 | #B4A1FF |
| Arpeggio | 99, 219, 208 | #63DBD0 |
| Kick | 255, 146, 107 | #FF926B |
| Snare | 255, 112, 155 | #FF709B |
| Hi-hat | 114, 207, 255 | #72CFFF |
| Aux drum | 218, 178, 108 | #DAB26C |
| Krell | 170, 235, 118 | #AAEB76 |

Page tabs, parameter labels, Composer track buttons, notes and scene cells share these identities. Scene cells include instrument names; colour is never the only identification. Inactive cells retain coloured text on a dark background. Selection uses a stronger outline, independently of whether the clip is enabled.

Drums as a group use RGB 255, 210, 100. FX use 130, 185, 255; Generate uses 161, 222, 128; Composer uses 230, 210, 170. These utility colours do not replace instrument colours.

Mixer strips use the same track palette, rather than introduce new identities. Signal/activity/clock status lights and clipping warnings are separate semantic indicators, not instrument identity colours.

## PRO 0.7 layout

One global source selector (MIDI / JAM / Arranger), transport, harmony and export entry. Instruments / Arranger / Mixer are the main views. Instruments expose Sound / Generator / FX and an advanced disclosure; power and compact IN/OUT indicators stay in the sidebar. The mixer owns primary levels/pans, returns, routing and master FX. Scene/song view uses a left library, center blocks/matrix and right inspector; clip editing has its own view.

Automatic scaling fits both width and height; minimum window 900×540, default up to 1200×720 subject to available display space. User scale is saved in host state. Controls retain stable host parameter IDs. Expanded panels scroll instead of clipping controls.
