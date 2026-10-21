# Composer / Arranger — PRO 0.4.0

## Pierwsze uruchomienie

Standalone Windows: rozpakuj paczkę i uruchom `Jerzy Bass Ambient PRO.exe`. W ustawieniach Audio/MIDI wybierz urządzenie audio, sample rate i wejście kontrolera MIDI. Standalone nie wymaga FL Studio. Do pracy we FL Studio użyj osobnego bundle VST3, nie pliku EXE.

Otwórz zakładkę **COMPOSER**. Włącz COMPOSER, SONG oraz PLAY, aby odtworzyć przykładowy utwór. PLAY w interfejsie uzbraja także cztery moduły instrumentów. Przycisk PANIC zatrzymuje generowanie i Composer. W DAW Song korzysta z pozycji i BPM hosta, gdy transport działa; poza hostem lub przy zatrzymanym transporcie działa własny zegar. BPM w Composer ustawia zegar lokalny i tempo plików MIDI — nie zmienia BPM projektu FL Studio.

## SCENES i SONG

- Wiersz SCENES to część utworu: Intro, Verse, Chorus, Break, Fill, Outro, End albo własna nazwa. Kolumny oznaczają BASS, PAD, ARP, KICK, SNARE, HI-HAT, AUX.
- Kliknij nazwę sceny, aby ją edytować. Kliknij komórkę instrumentu, aby wybrać jego partię. Prawoklik komórki lub Shift+klik przycisku instrumentu przełącza jego udział w scenie.
- Dwuklik sceny lub LAUNCH SCENE odsłuchuje wybraną część. Zmiana uruchamianej sceny podczas grania jest kwantyzowana do granicy taktu.
- Przeciągnij scenę do listy SONG lub użyj + SONG. Przeciągaj pozycje SONG, aby zmieniać kolejność; działają też przyciski < i >. REPEATS określa liczbę powtórzeń pozycji, EVOLVE tworzy jej deterministyczne warianty.
- Wystąpienia sceny w SONG są powiązane. Zmiana sceny zmienia wszystkie jej wystąpienia. VARIANT tworzy niezależną kopię sceny; dodaj ją do SONG, gdy chcesz zmienić tylko jedną część.
- NEW tworzy pustą scenę do ręcznego pisania. DELETE usuwa scenę i jej wystąpienia w SONG; ostatnia scena nie może zostać usunięta.
- Maksymalnie 32 sceny, po 1–32 takty, metrum 1–12 nad 2/4/8/16; 128 pozycji SONG, po 1–16 powtórzeń. Kompilator ogranicza song do 500000 nut. To limity bezpieczeństwa, nie obietnica wydajności przy maksymalnej gęstości.

## Harmonia i dźwięk

Pole CHORDS: wpisz np. `Cm Ab Fm G` i naciśnij Enter. Każdy wpis przypada na jeden takt, progresja powtarza się do końca sceny. Obsługiwane: major, m, 5, sus4, m7, maj7, m9, maj9; znaki # oraz b. Bazowa oktawa progresji jest ustalona, a ręczne nuty można pisać w dowolnej wysokości dostępnej w edytorze. Nie ma jeszcze edytora zmian akordów wewnątrz taktu ani slash chords.

Scena przechowuje osobny snapshot brzmienia i ustawień generatorów. Ustaw brzmienie na zakładkach instrumentów, następnie naciśnij CAPTURE SOUND w wybranej scenie. Zapis obejmuje ADSR, filtry, generatory, FX i grid perkusji. Podczas odtwarzania Composer korzysta z zapisanego brzmienia sceny; globalne wyłączniki modułów/FX oraz poziom Master pozostają nadrzędne. Samo zmienienie gałki instrumentu nie nadpisuje zapisanych scen.

INTENSITY wpływa na dynamikę i udział instrumentów. Każda partia ma próg minimalnej intensywności. Ustal np. Pad 0, Bass 0.2, Kick 0.3, Hat 0.5, Aux 0.8, aby rozbudowywać aranżację jednym suwakiem. ENTER BAR i LAST BAR dodają/odejmują poszczególne instrumenty w przebiegu sceny. LAST BAR = 0 oznacza granie do końca. GAIN reguluje udział partii bez zmiany zapisanych nut.

CUT TAILS czyści głosy i bufory FX na wejściu do części. Przy wyłączonym CUT przejścia SONG pozwalają wybrzmieć zwalnianym warstwom przez ograniczony czas zależny od release i zapasu dla FX (maks. 45 s). Nie utrzymujemy dowolnie długich zamrożonych efektów w nieskończoność. Wygasłe warstwy przestają być liczone, a ich duże bufory są zwalniane poza wątkiem audio.

## Pisanie partii

GENERATED używa odpowiedniego generatora z brzmienia sceny. FREEZE / EDIT zamienia wybraną realizację na nuty ręczne. Po zamrożeniu zmiana progresji nie transponuje automatycznie ręcznie zapisanych nut.

- Wybierz instrument i takt w edytorze. Octave ustala dolną oktawę widoku; kółko myszy przesuwa widok wysokości.
- Kliknij, aby dodać/wybrać nutę. Przeciągnij poziomo, aby zmienić długość. Prawoklik nuty usuwa ją. Velocity i Pan edytują zaznaczoną nutę.
- CLEAR CLIP czyści tylko wybraną partię i przełącza ją na ręczną.
- Prawoklik suwaka przywraca wartość domyślną; działa też dwuklik.
- Maksymalnie 512 ręcznych nut na partię. Dla gęstej długiej części podziel ją na krótsze klocki przed zamrażaniem.

Zmiana projektu podczas odtwarzania przelicza niezmienny plan zdarzeń w wątku pomocniczym. Przy jego przyjęciu odtwarzanie się synchronizuje; edycja nie jest obietnicą bezszwowego nagrywania live ani kompletnego DAW. Cofnięcie historii edycji nie jest jeszcze dostępne — zapisuj warianty i projekty przed dużymi zmianami.

## Eksport i FL Studio

EXPORT SCENE zapisuje wybraną scenę; EXPORT SONG zapisuje cały song. Standard MIDI format 1, 960 PPQ, nazwy ścieżek, markery części, tempo i metrum. PATTERN PACK zapisuje SONG.mid i osobne pliki scen/instrumentów. DRAG MIDI TO FL przeciąga wybraną partię sceny jako plik MIDI do Piano Roll hosta (wymaga hosta/systemu obsługującego zewnętrzny file drag; funkcji tej nie potwierdzono w rzeczywistym FL Studio).

Odsłuch Composer i eksport korzystają z tych samych wygenerowanych nut. Losowość zostaje utrwalona przy tworzeniu planu, a nie ponownie losowana podczas zapisu MIDI. Plik nie zapisuje syntezy, pełnej ewolucji timbru Krella, per-note stereo, efektów, obwiedni filtra ani całej automatyki. Zapis projektu `.jpro` i stan instancji w DAW zachowują ustawienia brzmienia.

Kanały eksportu: Bass 1, Pad 2, Arp 3, perkusja 10 z nutami GM 36/38/42 i Aux 39/46/45. Do odtwarzania gotowych partii w PRO włącz MIDI PARTS, wyłącz COMPOSER i GENERATE oraz włącz potrzebne moduły. MIDI PARTS gra nuty bez ponownego arpeggiowania/generowania. Nuty na kanale 10 nie zmieniają podstawy basu.

W FL Studio można importować MIDI do Piano Roll / projektu, zachowując rozdzielenie ścieżek według opcji importera. Pattern pack ułatwia tworzenie oddzielnych patternów. To eksport MIDI, nie natywny zapis `.flp`/`.fsc` ani automatyczne układanie całej Playlisty przez prywatne API FL Studio.

## Osobne inserty miksera

PRO udostępnia osiem wyjść stereo: MAIN, BASS, PAD, ARP, KICK, SNARE, HI-HAT, AUX. W wrapperze FL Studio skonfiguruj połączenia wyjściowe w Processing / Connections i przypisz je do kolejnych insertów. Następnie w PRO włącz SPLIT OUT.

Podłączona partia jest wtedy wyłączona z MAIN, aby nie podwajać sygnału. Niepodłączone wyjścia wracają do MAIN. Gdy SPLIT OUT jest wyłączone, MAIN zawiera sumę, a dodatkowe aktywne wyjścia mogą służyć do nagrywania stemów — nie odsłuchuj jednocześnie obu dróg bez świadomego routingu.

Wyjścia instrumentów są po ich własnym przetwarzaniu i poziomie Master, przed wspólnym Master FX/limiterem. Master FX przetwarza tylko sygnały pozostające w MAIN. W miksie rozdzielonym używaj wspólnej szyny FX/mastera w FL Studio. Nie da się wiernie rozdzielić nieliniowo przetworzonej sumy z powrotem na instrumenty.

## MIDI sterujące Composer

Domyślnie CC26 = Composer power, 27 = play, 28 = SONG/SCENE, 29 = loop, 30 = wybór sceny (skalowany na 1–32), 31 = globalna intensywność velocity, 32 = split outputs, 33 = MIDI PARTS. CC20–25 zachowują poprzednie znaczenie. CC120 zatrzymuje Composer/GENERATE i czyści głosy. Parametry hosta pozwalają również użyć Link to controller/automatyki FL Studio.

## Windows build na GitHub

Gałąź źródłowa: `pro/composer-arranger`. Push tej gałęzi uruchamia workflow Windows/Visual Studio 2022. Po pomyślnym buildzie i wszystkich testach paczka trafia do Actions artifacts oraz osobnej gałęzi `windows/pro-0.4`, folder `delivery`. BUILD.txt wskazuje dokładny commit źródeł, SHA256SUMS.txt zawiera sumy plików EXE i ZIP VST3. Workflow nie modyfikuje main i nie publikuje publicznego Release.

JUCE 9.0.3 nie obsługuje MinGW, dlatego lokalny cross-build MinGW nie jest drogą dostarczania tego EXE. Wersja Windows powstaje natywnie w MSVC. Test hosta na runnerze nie zastępuje odsłuchu i testów urządzenia audio/FL Studio na komputerze użytkownika.
