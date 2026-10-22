# Jerzy Bass Ambient PRO 0.7

Modularny instrument VST3 i Windows EXE: Bass Guitar / Classic / Acid / 808, Ambient, Arp, czterogłosowe Drums i osobny Krell, aranżer scen/songu, mikser i eksport MIDI/WAV.

**Nowy interfejs:** trzy główne widoki INSTRUMENTY / ARANŻER / MIKSER, źródło MIDI / JAM / ARANŻER, lokalne sekcje Brzmienie / Generator / Efekty, parametry zaawansowane na żądanie i skalowanie ekranu. [Instrukcja uruchomienia, MIDI i FL Studio](docs/START_PL.md).

Nowe MIDI kieruje kanały niezależnie: 1 Bass, 2 Pad, 3 Arp, 4 Krell, 10 Drums. Każdy instrument wybiera nuty lub generator. Starsze stany PRO zachowują wcześniejszy sposób sterowania; identyfikatory i kolejność istniejących parametrów pozostają zgodne.

Każdy instrument ma własny rack FX. Mikser ma osiem ścieżek instrumentów, powroty Delay/Reverb i master; VST3 udostępnia 11 wyjść stereo. Eksport: scena, instrument, song, osobne WAV-y, pliki MIDI i przeciąganie do hosta. [Composer](docs/COMPOSER.md), [wydajność i ograniczenia](docs/PERFORMANCE.md).

PRO zachowuje bundle ID `pl.jerzy.bassambient.pro` i kod `Jbpr`. Oryginalna wtyczka bez PRO może działać równocześnie. Źródła PRO są oddzielone od oryginalnego katalogu Source.

## Budowanie

```sh
cmake -S PRO -B build-pro -DPRO_ENABLE_LTO=OFF
cmake --build build-pro --config Release --parallel 2
ctest --test-dir build-pro -C Release --output-on-failure
```

JUCE 9.0.3 jest pobierane przez CMake. Windows x64 jest budowany natywnie w MSVC przez `.github/workflows/pro-windows.yml`, z czterema zestawami testów. Gotowe pliki, BUILD.txt oraz sumy SHA256 trafiają do `windows/pro-0.7/delivery`.

Testy obejmują DSP, generatory, stan hosta, migrację, MIDI, aranżer, WAV, połączenia parametrów GUI, nawigację, lokalne presety i zrzuty skompilowanego interfejsu. Nie zastępują odsłuchu i testu urządzenia audio w rzeczywistym FL Studio.
