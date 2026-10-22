# PRO 0.8 — LIVE: granie i nagrywanie występu

Pierwsza wersja trybu koncertowego obejmuje pady scen, kolejkę sceny, mikser podręczny, Snapshot/Return, sterowanie MIDI i nagrywanie rzeczywistego audio stereo. Otwórz **LIVE** na górnym pasku. Przejście do tego widoku nie uruchamia ani nie zatrzymuje muzyki.

## Granie scenami

Pady korzystają ze scen przygotowanych w Aranżerze: nazwy, długości, progresje, klipy i udział instrumentów pozostają wspólne. Wybór banku udostępnia do 32 scen, po osiem na ekranie.

Pierwsze naciśnięcie pada uruchamia scenę. Następna scena jest kolejkowana do granicy taktu zegara; na ekranie widać **GRA** oraz **NASTĘPNA**. Kolejne naciśnięcie przed granicą zmienia wybór oczekujący. Krótkie wygaszenie sygnału przy przejściu ogranicza skok poziomu; nie jest to pełny crossfade dwóch równoległych silników. Pady grają sceny, nie listę songu. LOOP w Aranżerze określa, czy scena ma się powtarzać. W FL Studio uruchom zegar hosta; zatrzymany HOST nie będzie odliczał granicy zmiany. PLAY i STOP pozostają na wspólnym pasku transportu.

Osiem faderów i MUTE na dole LIVE steruje tym samym mikserem co widok MIKSER. Jeśli chcesz miksować ręcznie podczas całego setu, wybierz w Aranżerze **Mikser bieżący**. Przy przywoływaniu miksera zapisanego w scenach to scena określa poziomy. Sceny decydują o aktywności partii; MUTE pozwala dodatkowo odejmować instrumenty podczas występu.

## Snapshot / Return

**SNAPSHOT** zapamiętuje bieżące parametry brzmienia, generatorów, efektów, gridu perkusji i miksu. Możesz eksperymentować, a **RETURN** wielokrotnie przywraca ten punkt.

Return zachowuje transport, zegar, źródło MIDI/JAM/Aranżer i wybór sceny. Nie przewija występu. Przy zmianach zapisanej sceny silnik synchronizuje jej plan, co może ponownie pobudzić głosy. Snapshot nie jest kopią dokumentu scen ani zapisem projektu; jest jednym tymczasowym punktem powrotu na czas działania instancji. Zamknięcie samego GUI go nie kasuje. Zmiany struktury efektów podlegają zwykłemu przygotowaniu buforów i mogą zerować ich ogony.

## REC LIVE

1. **WYBIERZ WAV** — wybierz miejsce nowego nagrania. Sam wybór pliku uzbraja zapis.
2. **REC LIVE** rozpoczyna zapis aktualnego wyjścia **MAIN**, stereo WAV 24-bit z częstotliwością działającego urządzenia / hosta.
3. Graj, zmieniaj sceny, miks i regulatory. To nagranie rzeczywistego audio, a nie ponowne generowanie songu offline.
4. **STOP REC** kończy plik i zapisuje nagłówek WAV. Zatrzymanie transportu nie zatrzymuje REC — możesz nagrać ciszę lub końcowe wybrzmiewanie.

Następne podejście otrzymuje nową nazwę: wcześniejsze nagrania nie są nadpisywane. Nagranie trwa po przejściu na inne strony i po zamknięciu okna edytora. Zatrzymanie / zmiana urządzenia audio oraz usunięcie instancji finalizują plik. Stan „REC” i ścieżka pliku nie przywracają automatycznie nagrywania po ponownym otwarciu projektu.

Dane trafiają do bufora i są zapisywane w oddzielnym wątku. Przepełnienie kolejki zatrzymuje nagranie z komunikatem błędu, zachowując dotychczasowy WAV. Zwykły WAV ma limit 4 GB: nagrywanie zatrzymuje się przed nim. Plik zostanie poprawnie zamknięty przy normalnym STOP / zamknięciu aplikacji; przerwanie procesu lub utrata zasilania może pozostawić nieukończony nagłówek.

**Ważne dla routingu:** zapis obejmuje MAIN wtyczki, nie cały mikser FL Studio. Gdy SPLIT usuwa instrumenty z MAIN, tych instrumentów nie będzie w tym nagraniu; LIVE pokazuje o tym komunikat. Zewnętrzne efekty FL i gitara podłączona do innego kanału wymagają nagrywania w DAW. Ten etap nie obejmuje jednoczesnego nagrywania osobnych stemów.

## Pedały i kontrolery MIDI

Przyciski reagują na naciśnięcie: wartość 64–127. Wartość 0–63 zwalnia przycisk; ponowne 127 bez zwolnienia nie powtarza akcji. Dowolny kanał MIDI.

| CC | Akcja |
| --- | --- |
| 35–42 | Sceny 1–8, niezależnie od widocznego banku |
| 43 / 44 | Następna / poprzednia scena, z zawijaniem listy |
| 45 / 46 | Snapshot / Return |
| 47 / 48 | Stop / Play |
| 49 | REC / STOP REC; najpierw wybierz plik w LIVE |

Dotychczasowe CC20–34 pozostają bez zmian. Własne przypisanie **MIDI Learn ma pierwszeństwo** przed domyślną akcją LIVE. Akcje plikowe, Snapshot i obsługa padów są wykonywane poza callbackiem audio. Fadery korzystają z istniejących parametrów hosta, więc można je przypisać przez MIDI Learn / Browse parameters.

## Zakres tej wersji

To pierwszy etap: LIVE, stereo REC, Snapshot/Return i sceny MIDI. Zapis całej improwizacji jako edytowalny song, zapis ruchów regulatorów, dogrywanie klipów, osobne ścieżki REC, setlista utworów, odsłuch słuchawkowy i chwilowe efekty performance wymagają kolejnego etapu. Nie są przedstawiane jako dostępne w 0.8.

Testy automatyczne obejmują zgodność WAV z rzeczywistym MAIN i liczbą próbek, nagrywanie z zamkniętym GUI, zachowanie wcześniejszych podejść, MIDI z wykrywaniem naciśnięcia, pierwszeństwo Learn, Return bez utraty rootu MIDI i serię przełączeń scen podczas ponad pięciu minut symulowanego odtwarzania. Przed występem potrzebny jest odsłuch i długi test konkretnego komputera, sterownika, kontrolera oraz projektu.
