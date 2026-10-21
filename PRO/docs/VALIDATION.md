# Walidacja PRO 0.3.0 — 21.10.2026

Zweryfikowano na x86_64 Debian Linux, GCC 14.2, CMake 3.31.6, JUCE 9.0.3. Kompilacja Release, LTO wyłączone w chmurze dla krótszych iteracji.

- Pełny build VST3 i Standalone zakończony powodzeniem; helper manifestu załadował wtyczkę i zapisał nazwę Jerzy Bass Ambient PRO, wersję 0.3.0 oraz osobny identyfikator.
- CTest: **3/3 PASS** — BassAmbientCore, BassAmbientPRO, BassAmbientHostGUI. Ostatni pełny przebieg: 4,75 s. Core wykonał 817987 sprawdzeń, PRO 379687 (duża część to kontrola każdej próbki, nie niezależne przypadki testowe).
- Test hosta: offsety MIDI, trzy racki i migracja schematu FX, połączenia GUI, zmiana kolejności, zapis nowych parametrów i kroku 64, MIDI Learn wraz z odtworzeniem przypisania, sample-accurate wyłączenie modułu przez CC20, reset regulatora prawoklikiem i CC120 hard panic. Powstało 25 świeżych PNG: 24 warianty stron/racków w trzech rozmiarach oraz dodatkowy widok efektu. Obejrzano ARPEGGIO i DRUMS; zapisy PNG teraz zastępują poprzedni plik zamiast dopisywać do niego kolejne obrazy.
- Osobny build Debug z AddressSanitizer + UndefinedBehaviorSanitizer: **2/2 PASS**, bez zgłoszonych naruszeń w testach core/PRO. Nie jest to pełny test GUI pod sanitizerami ani dowód braku wyścigów.
- Bufory FX w teście: **18908160 B** aktywne, **0 B** po wyłączeniu FX i obsłużeniu wymiany zasobów. Pamięć małych, stałych struktur oraz całego procesu pozostaje odrębną wielkością.
- Orientacyjny benchmark core, 0,5 s audio bez FX: wszystkie silniki aktywne 0,0845 s, wyłączone 0,000063 s. To wynik konkretnego uruchomienia na tej maszynie i profilu, nie prognoza CPU w FL Studio ani na Raspberry Pi.
- Cztery pliki demonstracyjne: stereo PCM16, 48 kHz, 12 s każdy — Deep House, Stereo Arp, Lush Pad, Krell. Sprawdzono niezerowy sygnał, skończone próbki, poziomy i nagłówki WAV. Nie zastępuje to odsłuchu przez użytkownika.
- Oryginalny projekt 0.2.0: test DSP przeszedł; jego śledzone pliki pozostają niezmienione.

Nie wykonano: kompilacji Windows/MSVC, odsłuchu w FL Studio, testów sterownika audio i rzeczywistej latencji, fizycznego MIDI ani testów ARM/Pi 4. Workflow Windows jest przygotowany do ręcznego uruchomienia w GitHub Actions po udostępnieniu zmian w repozytorium. Wymagania i granice portu Pi opisuje RASPBERRY_PI_4.md.

Skrypt instalacji oraz instrukcje uruchamiania zapisano jako szkic konfiguracji chmury. Skrypt wykonano w bieżącej maszynie; instalacja lokalnych zależności i ponowna konfiguracja/build zostały sprawdzone. Nie wykonano publikacji środowiska ani testu odtworzenia w nowym zadaniu.
