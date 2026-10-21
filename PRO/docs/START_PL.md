# Jerzy Bass Ambient PRO 0.6 — szybki start

W EXE wybierz urządzenie wyjściowe w ustawieniach Audio/MIDI. Ustaw CLOCK na INTERNAL, wybierz BPM i naciśnij PLAY. W FL Studio wybierz HOST / FL STUDIO i uruchom transport DAW.

## Bass Guitar i style

W BASS wybierz model Bass Guitar. Finger/Pick/Slap określa sposób pobudzenia struny; Tone, Palm Mute, String Decay i Pluck Position zmieniają charakter dźwięku. To synteza modelująca strunę, a nie biblioteka sampli.

Generator ma style Rock, Metal, Punk i Blues. Użyj bazowego podziału 1/16; do bluesowego shuffle ustaw Swing około 0.6. W menu Factory scenes są gotowe presety Bass Guitar Groove oraz Rock/Metal/Punk/Blues Bass Guitar. PLAY pozwala ich posłuchać bez klawiatury MIDI.

## Wspólny root i skala

Panel HARMONY jest stale widoczny. ROOT ustawia nutę bazową, SCALE skalę, a LINK łączy generatory.

Przy LINK ON partie MODULE Ambient, Arp i Krell podążają za rootami basu. Wpisz pierwszą nutę bazową C w partii BASS lub zagraj ją przez MIDI: wspólny ROOT zostanie ustawiony na C. Zmiana ROOT transponuje połączone partie generowane. Partie MANUAL zachowują wpisane nuty. Jawnie wpisana progresja akordów nadal określa jakość akordów sceny.

Przy LINK OFF każdy moduł może mieć własne rooty. To domyślne ustawienie, aby zachować niezależne partie ze starszych projektów. Gdy LINK jest włączony, rooty połączonych generatorów edytuj w BASS; komunikat w Composerze wyjaśni blokadę niezależnej edycji ich rootów.

## Osobny Krell

Zakładka KRELL to oddzielny instrument, działający równolegle z Ambientem. Ma własne silniki Evolving FM, Resonant Noise i Harmonic Cloud oraz obwiednię, czasy cykli, zakres wysokości, filtr, panoramę i poziom.

Do szybkiego odsłuchu wybierz Independent Krell z Factory scenes i PLAY. W Composerze włącz klocek KRELL w wybranej scenie i użyj LAUNCH SCENE. Krell ma własny kanał miksera, rack FX, wyjście audio i nuty MIDI na kanale 4. CC34 przełącza jego zasilanie. Dawny tryb Krell w Ambiencie pozostał dla starszych projektów.

## Composer: długość i sposób grania

Bars pod polem CHORDS określa długość sceny (1–32 takty). Bar nad siatką wybiera takt do edycji. Kliknięcie dodaje nutę; przeciągnięcie poziomo zmienia jej długość; prawoklik ją usuwa.

MODE: MANUAL odtwarza wpisane nuty. MODE: MODULE rozwija rooty w frazę basową, arpeggio, akordy lub cykle Krella. FREEZE / EDIT zamienia wynik generatora na nuty MANUAL. Eksport MIDI zapisuje wynikowe nuty.

Enter bar i Last bar pod siatką ograniczają czas aktywności partii. Fabryczne Intro wpuszcza ARP od 3. taktu. Aby grał od początku, ustaw Enter bar na 1. Nuty poza aktywnym zakresem nie zagrają.

## FX i mikser

W FX są racki BASS, AMBIENT, MASTER, ARPEGGIO, DRUMS i KRELL. Własny regulator FX amount musi być większy od zera. Wspólne ustawienia DRUMS są przetwarzane niezależnie dla czterech głosów, aby zachować osobne wyjścia Kick/Snare/Hi-hat/Aux.

FX POWER jest nadrzędnym włącznikiem. Naprawiono przypadek, w którym scena zapisana z FX OFF blokowała wysyłki mimo widocznego globalnego ON.

W MIXER podnieś DELAY SEND lub REVERB SEND na wybranym kanale. Włącz odpowiedni powrót DELAY/REVERB i ustaw jego poziom. Mute oraz Solo obejmują nowy sygnał kierowany do wysyłek. POWER zatrzymuje instrument, a Mute pozostawia pracujące głosy.

SCENE MIX OFF daje globalne sterowanie miksem oraz rackami MASTER/ARPEGGIO/DRUMS/KRELL. ON przywołuje ustawienia sceny; CAPTURE SOUND zapisuje bieżący zestaw.

## Wyjścia w FL Studio

Włącz dodatkowe wyjścia w wrapperze wtyczki i przypisz je do insertów miksera. SPLIT OUT usuwa podłączone stem-y i powroty z MAIN; niepodłączone pozostają w MAIN.

Kolejność 11 wyjść stereo: MAIN, BASS, PAD, ARP, KICK, SNARE, HI-HAT, AUX, DELAY RETURN, REVERB RETURN, KRELL. Nowy Krell jest na końcu, więc numery wcześniejszych wyjść nie zmieniły się. Osobne wyjścia omijają końcowy limiter MAIN.

Diody IN, ACT, OUT i CLP oznaczają odpowiednio sterowanie wejściowe, aktywne głosy, audio na wyjściu i poziom co najmniej 0 dBFS. Prawoklik resetuje regulator; Ctrl+prawoklik pozwala przypisać MIDI.

### Przeciąganie MIDI do FL Studio

Na dole Composera wybierz **SCENE** (wybrana scena) lub **WHOLE SONG** (cały song z powtórzeniami). Drugi wybór: **SELECTED TRACK** eksportuje zaznaczony instrument do jednego Piano Roll; **ALL TRACKS** tworzy wielościeżkowy plik MIDI do importu w FL Studio. Następnie przeciągnij **DRAG MIDI TO FL**. Eksport songu korzysta z listy SONG, więc najpierw dodaj do niej sceny. Osobne przyciski EXPORT SCENE / EXPORT SONG zapisują wszystkie ścieżki do pliku MIDI.

### Układ na mniejszym ekranie

Okno startuje w rozmiarze dopasowanym do obszaru roboczego ekranu, z rezerwą na ramkę FL Studio. W DRUMS kliknij KICK / SNARE / HI-HAT / AUX po lewej stronie siatki: parametry wybranego instrumentu są po prawej. Ustawienia wspólne są pod siatką. Panele regulatorów mają mniejszą wysokość; dłuższe strony nadal można przewijać.

### Rendering audio WAV (EXE i VST3)

Na dole strony COMPOSER wybierz zakres i kliknij **RENDER WAV**:

- **WAV: SCENE** — wybrana scena, wszystkie aktywne klipy.
- **WAV: SCENE INSTRUMENT** — zaznaczony instrument z wybranej sceny.
- **WAV: WHOLE SONG** — cała lista SONG z powtórzeniami i wariantami EVOLVE.
- **STEMS: SCENE / WHOLE SONG** — folder z ośmioma ścieżkami instrumentów oraz osobnymi DELAY-RETURN i REVERB-RETURN. Wszystkie pliki zaczynają się w tym samym miejscu i mają taką samą długość. Nazwy są numerowane (01-BASS.wav … 07-AUX.wav … 10-REVERB-RETURN.wav), co zapewnia zgodność z nazwami plików Windows.

Wybierz 44,1 / 48 / 96 kHz, WAV 24-bit PCM lub 32-bit float oraz dodatkowy ogon 0 / 5 / 10 / 20 / 45 sekund. TAIL 0 ucina plik na końcu sceny/songu. Dłuższy ogon zachowuje wybrzmiewanie obwiedni i efektów do wybranego limitu; nie jest to automatyczne wykrywanie ciszy.

**WITH FX** włącza efekty na potrzeby renderu z ustawionymi ilościami i wysyłkami; **DRY / NO FX** pomija racki i efekty wysyłkowe, zachowując syntezę, jej drive oraz ustawienia gain/pan miksera. Zwykły stereo WAV zawiera także MASTER FX i limiter. Stems są po efektach instrumentów i mikserze, przed MASTER FX, filtrem DC oraz limiterem sumy. Wspólne wysyłki są w osobnych plikach, więc nie należy dodawać ich ponownie do każdego instrumentu. Suma stems po dodatkowym przetwarzaniu mastera może różnić się od gotowego stereo WAV.

Rendering używa kopii bieżących ustawień i aranżacji. Tempo pochodzi z INTERNAL BPM lub aktualnego tempa hosta/zegara. Nie wymaga uruchomienia transportu ani urządzenia audio. Tak jak LAUNCH SCENE, uruchamia moduły zgodnie z klipami scen; respektuje gain, mute, solo i progi intensywności. Eksport jednego instrumentu pomija SOLO pozostałych instrumentów. SCENE MIX decyduje o odczycie miksera sceny. Zmiany wykonane po otwarciu okna zapisu nie zmieniają już tego renderu.

Postęp widać w osobnym oknie; **Cancel** anuluje operację. Dotychczasowy plik jest zastępowany dopiero po ukończeniu nowego WAV. Stems trafiają do nowego folderu. Przy 24-bit PCM zbyt głośny sygnał może zostać obcięty — do dalszego miksowania wybierz 32-bit float. Maksymalny rozmiar pojedynczego WAV to 4 GB; dłuższe aranżacje eksportuj częściami.

Po ukończeniu stereo WAV aktywuje się **DRAG WAV TO FL**. Przeciągnij ten przycisk na **Playlistę** FL Studio: FL utworzy Audio Clip. Plik pozostaje w wybranej lokalizacji — nie przenoś go ani nie usuwaj, bo projekt FL odwołuje się do niego. Przycisk używa ostatniego udanego renderu stereo w tym oknie; po eksporcie stems przeciągaj numerowane pliki WAV z folderu. Automatyczne wstawianie Audio Clip bez przeciągnięcia nie jest dostępne przez standardowe API VST3.

### LINK ROOT + SCALE i nuty z Piano Roll

Przy COMPOSER OFF zmiana root z MIDI aktualizuje wspólną harmonię, zachowując trzymane nuty. Przebudowa danych Composera w tle nie może ucinać generatorów. Długa nuta root steruje kolejnymi powtórzeniami frazy Rock / Metal / Punk / Blues aż do Note Off (przy MIDI PARTS OFF i DIRECT MIDI OFF).
