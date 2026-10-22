# Jerzy Bass Ambient PRO 0.7 — szybki start

## Gdzie włączyć muzykę

Na górze wybierz źródło sterowania:

- **MIDI**: graj z klawiatury lub Piano Roll. W instrumencie wybierz **NUTY**, aby odtwarzać zapis dokładnie, albo **GENERATOR**, aby przytrzymana nuta uruchamiała styl, arpeggio lub generowane akordy. PLAY nie uruchamia samoczynnej muzyki w tym trybie.
- **JAM**: granie bez klawiatury. Wybierz preset całości, root i skalę, włącz potrzebne instrumenty i naciśnij **PLAY**. Przycisk **ZAPISZ JAM JAKO SCENĘ** zachowuje ustawienia oraz udział instrumentów w nowej scenie.
- **ARANŻER**: przejdź do scen i songu. **SONG ON** odtwarza song, **SONG OFF** wybraną scenę. **START SCENY** uruchamia zaznaczoną scenę. **LOOP** zapętla odtwarzanie. PLAY i STOP są wspólne dla całego instrumentu.
- **MIDI — zgodność**: zachowana obsługa starszych projektów PRO. Stary zapis wybiera ją automatycznie; przejście na nowe MIDI jest świadomą zmianą sposobu sterowania.

W EXE wybierz urządzenie wyjściowe w systemowym panelu Audio/MIDI aplikacji. W **USTAWIENIA** wybierz zegar **INTERNAL** i ustaw BPM na górnym pasku. We FL Studio użyj **HOST / FL STUDIO** oraz transportu FL; pole BPM pokazuje wtedy tempo hosta. **MIDI CLOCK** wymaga przychodzącego zegara. Komunikat o zatrzymanym zegarze pomaga rozpoznać brak transportu.

## Trzy główne widoki

**INSTRUMENTY**: po lewej Bass, Ambient, Arp, Drums i Krell, każdy z własnym POWER i diodami IN/OUT. Po prawej **BRZMIENIE**, **GENERATOR** oraz **EFEKTY**. Parametry rzadziej używane są pod **ZAAWANSOWANE**. Lokalny preset zmienia tylko wybrany instrument. **NOWY WARIANT / COFNIJ** dotyczy ustawień muzycznych wybranego generatora.

**ARANŻER**: biblioteka scen po lewej, klocki songu i macierz instrumentów pośrodku, właściwości po prawej. Edytor klipu jest osobnym widokiem. **COFNIJ** przywraca ostatnie edycje dokumentu z tej sesji edytora (do 40 zmian); nie obejmuje globalnych parametrów syntezy ani wczytywania projektu.

**MIKSER**: osiem ścieżek instrumentów, powroty Delay/Reverb i master. Dostęp do **MASTER FX**, **POWROTY FX** i **ROUTING FL** jest tutaj. Głośność i panorama instrumentu mają główne miejsce w mikserze. Parametry techniczne i rozszerzone pozostają dostępne w ustawieniach zaawansowanych.

## MIDI: kanały i generatory

| Instrument | Kanał MIDI | Działanie |
| --- | --- | --- |
| Bass | 1 | NUTY albo generator stylu |
| Ambient / Pad | 2 | NUTY albo generowane akordy / Flow |
| Arp | 3 | NUTY albo arpeggiowanie trzymanych nut |
| Krell | 4 | NUTY albo własny generator Krell |
| Drums | 10 | NUTY MIDI / GM albo SEKWENCER |

Perkusja: Kick 36, Snare 38/40, Hat 42/44, Aux według wybranego brzmienia. **Aux** to czwarty głos: Clap, Open Hat lub Tom.

Nuty wyeksportowane z generatorów odtwarzaj w trybie **NUTY**, aby nie generować ich ponownie. Dopilnuj kanału MIDI: Pad wymaga kanału 2, Arp 3. Samo rozdzielenie wyjść audio nie kieruje nut MIDI.

**WSPÓLNY ROOT** łączy generatory ze źródłem basu i wspólną skalą. Nie zmienia dosłownych nut w trybie NUTY. W MIDI root pochodzi z granego basu; w JAM ustawiasz go na pasku; w aranżerze pochodzi z progresji / klipu basu. Wyłączenie linku daje generatorom niezależne rooty. Przytrzymana długa nuta basu wyzwala powtarzające się frazy aż do note-off.

## Bass Guitar i style

W **Bass → Brzmienie → Bass model** wybierz **Bass Guitar**. Finger/Pick/Slap, Tone, Palm Mute i String Decay zmieniają modelowaną strunę. To synteza, nie biblioteka sampli. W **Generator** wybierz Rock, Metal, Punk albo Blues oraz długość frazy w taktach. W MIDI ustaw GENERATOR i przytrzymaj nutę; do odsłuchu bez nut użyj JAM. Swing pomaga uzyskać shuffle bluesowy.

## Sceny i długość nut

Długość sceny ustawiasz w prawym inspektorze **DŁUGOŚĆ SCENY — TAKTY**. Metrum sceny jest na dole inspektora. Pole progresji przyjmuje np. `Cm Ab Fm G`; zatwierdź Enterem. Jeden akord przypada na takt, progresja powtarza się.

W **EDYTOR KLIPU** wybierz instrument i **GENERATOR: ROOT** lub **NUTY MANUALNE**. Generator interpretuje nuty jako podstawy i sam buduje frazę. Przycisk **ZAMIEŃ GENERATOR NA NUTY** utrwala jego wynik do edycji. Gdy wspólny root jest włączony, podstawy połączonych generatorów edytuj w basie.

Kliknięcie siatki dodaje nutę, prawoklik usuwa, przeciągnięcie poziomo zmienia długość. **DŁUGOŚĆ NUTY — ĆWIERĆNUTY**: 1 to ćwierćnuta, 4 cała nuta. Wartość dotyczy zaznaczonej lub następnej dodawanej nuty. Wybór taktu i oktawy przy siatce zmienia zakres edycji / widoku. **UDZIAŁ INSTRUMENTU**, wejście od taktu i ostatni takt ograniczają granie partii.

Sceny przeciągaj do songu; klocki przeciągaj, aby zmieniać kolejność. W inspektorze wybieraj powtórzenia i EVOLVE. WARIANT tworzy osobną kopię sceny; kolejne wystąpienia tej samej sceny są ze sobą powiązane. ZAPISZ/WCZYTAJ obsługuje projekt `.jpro`.

## FX i wysyłki

Każdy instrument ma własny rack, a mikser osobny MASTER FX. Przeciągaj bloki efektów, aby zmieniać kolejność; prawoklik bloku udostępnia przesuwanie. Włącz efekt i podnieś jego Mix oraz udział racka. Jeden globalny przycisk **FX** znajduje się na górze.

W mikserze podnieś Delay lub Reverb Send, a w **POWROTY FX** włącz i ustaw odpowiedni powrót. POWER instrumentu zatrzymuje jego obliczenia, Mute wycisza miks. Wyłączenie FX zwalnia duże bufory poza wątkiem audio. Stałe małe struktury głosów pozostają zarezerwowane.

**Mikser bieżący / Mikser zapisany w scenach** określa źródło miksu aranżera. **ZAPISZ BRZMIENIE DO SCENY** zapisuje bieżący zestaw w scenie. Preset całości zmienia zestaw instrumentów; w nowych trybach zachowuje źródło sterowania, zegar i wspólną harmonię.

## Eksport MIDI i audio do FL Studio

Wspólny przycisk **EKSPORT** na górze otwiera wybór **AUDIO WAV / MIDI** oraz zakres **Scena / Instrument ze sceny / Cały song**. Dla WAV dostępne są też osobne ścieżki sceny lub songu, 44,1/48/96 kHz, 24-bit PCM/32-bit float i ogon 0/5/10/20/45 s. Instrument wybierz wcześniej w edytorze klipu. Rendering korzysta z niezależnej kopii aranżacji i ustawień.

Po renderingu przeciągnij **DRAG WAV TO FL** do Playlisty, aby utworzyć Audio Clip. MIDI przeciągaj do Piano Roll, a wielościeżkowe MIDI importuj z zachowaniem kanałów. **PATTERN PACK** zapisuje oddzielne pliki MIDI. To standardowe pliki i przeciąganie systemowe; aplikacja nie zapisuje natywnych patternów FL przez prywatne API. Przeciąganie wymaga obsługi przez host i system.

Osobne WAV-y są wyrównane w czasie i obejmują osiem instrumentów oraz dwa powroty FX. Są przed MASTER FX i limiterem; eksport stereo zawiera końcowy master. DRY pomija racki i wysyłki, zachowując syntezę oraz gain/pan.

## Osobne kanały miksera FL

W wrapperze FL: ustawienia wtyczki → Processing / Connections → przypisz dodatkowe wyjścia do insertów. W **Mikser → ROUTING FL** włącz rozdzielenie. Kolejność stereo: MAIN, BASS, PAD, ARP, KICK, SNARE, HI-HAT, AUX, DELAY RETURN, REVERB RETURN, KRELL. Podłączone i rozdzielone ścieżki są usuwane z MAIN, aby nie podwajać odsłuchu. Niepodłączone pozostają w sumie.

## Ekran, diagnostyka i zgodność

Okno dopasowuje się do dostępnego ekranu i można je zmniejszyć do 900×540. W USTAWIENIA jest skala Auto / 75 / 85 / 100%. Auto dopasowuje cały układ; dłuższe panele zaawansowane przewijasz. Diody IN i OUT pokazują sterowanie i audio, a DIAGNOSTYKA także aktywność głosów, poziomy i zegar.

Prawoklik regulatora przywraca wartość domyślną, Ctrl+prawoklik obsługuje MIDI Learn. PANIC zatrzymuje generowanie i głosy. PRO nadal ma własną nazwę i identyfikator `pl.jerzy.bassambient.pro` / `Jbpr`; może współistnieć z oryginalną wtyczką bez PRO. Aktualizacja 0.7 zachowuje identyfikator PRO, istniejące identyfikatory parametrów i ich kolejność. Starsze zapisy zachowują dawną ścieżkę MIDI.

Paczka Windows powstaje w MSVC na GitHub; BUILD.txt wskazuje commit, SHA256SUMS.txt sumy plików. Test hosta i renderingu nie zastępuje odsłuchu konkretnego projektu i konfiguracji FL Studio.
