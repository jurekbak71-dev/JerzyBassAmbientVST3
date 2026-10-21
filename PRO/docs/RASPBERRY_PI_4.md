# Raspberry Pi 4 — osobna analiza wykorzystania core

## Wniosek

Core PRO nadaje się do prototypu na Raspberry Pi 4: C++17, brak JUCE i systemu okien w `Core.h` / `ProEngines.h`, brak x86 intrinsics w kodzie projektu. Nie jest to jeszcze potwierdzenie wydajności ani port wykonany na urządzeniu. Dostępna maszyna testowa to x86_64 Linux, nie Pi.

## Proponowana integracja w drugim projekcie

1. Raspberry Pi OS 64-bit, aktywne chłodzenie, zasilacz odpowiedni do Pi, interfejs USB audio/ALSA. Zacząć od 48 kHz i 256 próbek; 128 dopiero po pomiarach underrunów.
2. Przenieść warstwę core wraz z licencjami i zbudować testy `BASS_AMBIENT_CORE_ONLY=ON` natywnie przez CMake/GCC. Nie przenosić GUI VST na mały panel bez osobnego projektu interfejsu.
3. Adapter audio: `prepare(sampleRate)` przed uruchomieniem urządzenia; `configureResources(parameters)` raz na blok na wątku audio; `next(parameters, ppq)` na próbkę. `serviceResources()` wywoływać na jednym oddzielnym wątku. Ten sam obiekt ma jednego konsumenta audio i jednego producenta zasobów. Zatrzymać oba przed zmianą sample rate lub destrukcją.
4. Adapter MIDI: zachować offsety próbek, kanały, note on/off, sustain, bend i komunikaty transportu. Przeliczać PPQ z zegara lub MIDI clock; obsługa MIDI clock/start/stop musi zostać napisana w adapterze. Core sam nie otwiera ALSA, USB ani GPIO.
5. Zmiany parametrów i presetów przekazywać na granicach bloków przez snapshoty/kolejkę SPSC; nie modyfikować wspólnego `Parameters` równocześnie z jego odczytem audio. Nie logować, alokować ani wykonywać I/O w callbacku.

## Co mierzyć i ewentualnie uprościć

- Najdroższe: wielogłosowe pady (unison, dwa silniki, filtry stereo 2x), funkcje tanh/exp/sin oraz trzy racki FX. Zacząć od basu i perkusji, potem dołączać arp, pad i FX pojedynczo.
- Budżet bloku przy 48 kHz: 256 próbek = 5,33 ms; 128 = 2,67 ms. To czas na cały callback, nie gwarantowana latencja wejście–wyjście. Sprawdzać maksimum i percentyle czasu, xruns, temperaturę, throttling, MIDI jitter przez co najmniej 30 minut.
- W razie przeciążenia osobna konfiguracja jakości: mniej głosów pada/arp, rezygnacja z unison i 2x oversamplingu, ograniczenie modulacji do control-rate i tańsze aproksymacje. Wymaga to osobnych testów aliasingu/brzmienia, nie tylko przełączników kompilacji.
- Duże bufory są zwalniane po wyłączeniu racków; szczyt pamięci obejmuje równocześnie stare i przygotowywane nowe bufory. Pamięć ograniczać też krótszymi liniami delay/granular i mniejszą liczbą racków, gdy drugi projekt tego wymaga.

## Granice obecnej analizy

Nie testowano kompilatora ARM, urządzenia audio Pi, wejścia MIDI/GPIO, real-time scheduling ani obciążenia na Pi. Render WAV na x86 dowodzi funkcjonalności przenośnego silnika, ale nie określa dostępnej polifonii na Pi 4. Realizację adaptera należy dopasować do repozytorium i sprzętu Twojego drugiego projektu.
