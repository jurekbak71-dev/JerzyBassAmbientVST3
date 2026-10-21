# Jerzy Bass Ambient PRO — 0.4.0 development

Osobny instrument VST3 / Standalone. Projekt bazowy 0.2.0 w katalogu głównym pozostaje osobnym produktem. PRO ma nową nazwę, bundle ID `pl.jerzy.bassambient.pro` i kod `Jbpr`; nie podmienia starej wtyczki ani automatycznie jej instancji w sesjach.

## Composer / Arranger

Nowy moduł nadrzędny: sceny, progresje akordów, ręczna edycja nut, generowanie i zamrażanie partii, układanie SONG, eksport MIDI/pattern pack oraz osiem wyjść stereo. [Pełna instrukcja Composer, Windows EXE i routingu FL Studio](docs/COMPOSER.md).

## Moduły i obsługa

- **BASS**: ACID, 808 i Classic Analog; dziesięć stylów (Italo, Disco Polo, ACID, Funk, Techno, Deep House, Dub, Synthwave, Half-time, Generative). Tonacja, gęstość, ruch, swing, slide, akcenty, zakres oktaw, humanizacja velocity i zakończenia fraz. DIRECT gra nuty z Piano Roll.
- **AMBIENT**: dwa wybierane silniki, po dwie rozstrojone składowe każdego, niezależne filtry stereo, shimmer oktawowy, ADSR, drift i modulacja. HOLD / FLOW / KRELL. Krell losuje niezależne czasy cykli, ataku, zanikania, przestrzeni między wejściami, dynamikę, panoramę i wysokości w skali. To proceduralne syntezowanie, bez importowania sampli.
- **ARPEGGIO**: cztery brzmienia syntezatora; Up, Down, Up/Down, As Played, Random i Brownian Walk. Graj akord MIDI albo włącz GENERATE. Do czterech oktaw, rytm euklidesowy (liczba kroków / impulsów), swing, ADSR, filtr z obwiednią, kontrolowane losowanie długości i wysokości w skali, dynamika i stereo: centre, ping-pong, random, orbit. Maksymalnie osiem nakładających się głosów. Ziarno i Evolve kontrolują powtarzalność; losowanie nie jest funkcją eksportu MIDI.
- **DRUMS**: syntezowane Kick / Snare / Hi-hat / Aux (Clap, Open Hat, Tom). Jedna wspólna długość patternu 1–64 kroki, cztery banki po 16 kroków. Kliknięcie aktywuje nutę, prawoklik wybiera velocity lub MIDI Learn. Kliknięcie nazwy instrumentu wybiera jego regulatory: power, level, drive, tone, filtr, decay, panorama. Presety rytmu i czyszczenie. Nie jest to sampler ani pełny piano roll.
- **FX**: osobne racki Bass, Ambient i Master. Master obejmuje także Arpeggio i Drums. Kolejność sześciu efektów, osobne przełączniki, udział każdego racka oraz globalne FX POWER. Arp i Drums korzystają z racka Master; nie mają dodatkowych osobnych racków.
- **GENERATE**: granie bez trzymania nuty MIDI, wybór podstawy, skali, BPM i zmian harmonicznych co zadaną liczbę taktów. Host dostarcza BPM/metrum/pozycję, gdy udostępnia te informacje; poza hostem działa zegar lokalny. GENERATE gra także przy zatrzymanym transporcie jako odsłuch. PANIC wyłącza GENERATE i czyści głosy.

Każdy instrument ma własną zakładkę i przełącznik zasilania. Wyłączone silniki nie liczą próbek; ich małe, stałe struktury głosów są zerowane i pozostają zarezerwowane. Duże bufory FX są alokowane i zwalniane w wątku konserwacji, poza callbackiem audio. Wyłączenie instrumentu usuwa bufory jego racka, wyłączenie FX POWER usuwa wszystkie; wyłączenie pojedynczego efektu przygotowuje rack bez jego buforów. Zmiana zestawu efektów resetuje ogony danego racka. W czasie przygotowania buforów sygnał przechodzi na sucho; nie obiecujemy bezszwowego przełączania długich ogonów. Wskaźnik pamięci pokazuje aktywne bufory racków, nie cały RSS procesu ani chwilowe bufory oczekujące na wymianę.

## MIDI i FL Studio

Wszystkie parametry brzmienia, przełączniki, ustawienia generatorów i 256 velocity kroków są parametrami hosta. W FL Studio: skan VST3 → Channel Rack → Piano Roll; dla automatyki użyj **Browse parameters** oraz **Link to controller** / **Create automation clip**. PRO udostępnia standardowy interfejs VST3; testy integracji w samym FL Studio na Windows są nadal konieczne.

Domyślne CC (dowolny kanał, wartości 0–63 OFF / 64–127 ON):

| CC | Funkcja |
|---|---|
| 20 | BASS |
| 21 | AMBIENT |
| 22 | ARPEGGIO |
| 23 | DRUMS |
| 24 | FX POWER |
| 25 | GENERATE |

**Ctrl + prawoklik regulatora** otwiera MIDI Learn / usunięcie przypisania. Następny CC przypisuje się do wybranego parametru; mapowania zapisują się w stanie wtyczki. Jeden CC steruje jednym parametrem, a parametr ma jedno przypisanie CC, na dowolnym kanale. Nowe MIDI Learn zastępuje wcześniejsze przypisania. Parametry ciągłe przechodzą przez wygładzanie 20 ms, przełączniki modułów działają w próbce komunikatu. CC64 pozostaje sustain, CC120 hard panic (wyłącza GENERATE), CC123 all notes off — nie podlegają MIDI Learn. Pitch bend: ±2 półtony. Kanał 10: GM kick 36, snare 38/40, hat 42/44, Aux 39/46/45/47 (wybierany typ Aux pozostaje zgodny z regulatorem).

**Prawoklik regulatora** przywraca wartość domyślną (nie zawsze liczbę 0: np. cutoff = 4200 Hz). Działa też dwuklik. Ekranowe akcje edycyjne, np. wybór zakładki, nie są parametrami dźwiękowymi MIDI.

## Build i testy

JUCE 9.0.3, CMake >= 3.24, C++17. Z katalogu `PRO`:

```sh
# Windows x64 / Visual Studio 2022
cmake -S . -B build-pro -G "Visual Studio 17 2022" -A x64
cmake --build build-pro --config Release --parallel 2
ctest --test-dir build-pro -C Release --output-on-failure
```

```sh
# Sam silnik, również na Linux/ARM, bez JUCE i bez urządzenia audio
cmake -S . -B build-core -DBASS_AMBIENT_CORE_ONLY=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build-core --parallel 2
ctest --test-dir build-core --output-on-failure
./build-core/BassAmbientRender ./demo
```

Pełny Linux: kompilator C/C++, CMake, Ninja, pkg-config, biblioteki deweloperskie ALSA, X11/Xext/Xi/Xinerama/Xrandr/Xcursor, Freetype, Fontconfig i OpenGL. `cmake -S . -B build-pro -G Ninja -DCMAKE_BUILD_TYPE=Release`; następnie build i `xvfb-run -a ctest --test-dir build-pro --output-on-failure` w środowisku bez ekranu. W chmurze `-DPRO_ENABLE_LTO=OFF` skraca iteracje kompilacji; optymalizacja Release pozostaje aktywna. Dla dystrybucji LTO jest domyślnie włączone. WebKit i curl nie są potrzebne wtyczce (wyłączone).

W przygotowanej chmurze: `source /workspace/cloud-setup/activate.sh`; build `/workspace/cloud-setup/pro`, testy GUI pod Xvfb. Artefakty w `JerzyBassAmbientPRO_artefacts/Release`. Linuksowy `.vst3` nie działa w Windows. Instalacja Windows wymaga zbudowania wersji Windows i skopiowania całego bundle do `C:\Program Files\Common Files\VST3`.

To wersja do testów rozwojowych. Przed dystrybucją: licencja JUCE, odsłuch, automatyka i zapis sesji w FL Studio, DPI/skalowanie, przełączanie modułów, testy sterowników audio i pomiary CPU.
