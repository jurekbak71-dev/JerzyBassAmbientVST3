# Referencje arpeggiatora i generowania

Sprawdzone źródła online podczas pracy nad PRO (21.10.2026); porównanie funkcji, bez kopiowania kodu źródłowego.

- **Helm**, implementacja arpeggiatora: [mopo/src/arpeggiator.cpp](https://github.com/mtytel/helm/blob/abdedd527e6e1cf86636f0f1e8a3e75b06ed166a/mopo/src/arpeggiator.cpp). Potwierdzone Up / Down / UpDown / AsPlayed / Random, liczba oktaw, gate oraz włączanie/wyłączanie nut z offsetami w bloku audio. PRO oferuje te podstawowe kierunki, dodatkowo bounded walk, rytm euklidesowy, kontrolowane losowanie długości/wysokości w skali i panoramę poszczególnych nut.
- **Stochas**, teksty pomocy: [src/HelpText.h](https://github.com/surge-synthesizer/stochas/blob/7de0c02bf149184e71f5917d32591fd0dd2d5094/src/HelpText.h). Potwierdzone losowanie wyboru nut, humanizacja velocity/położenia/długości, stabilna lub zmienna losowość, długości warstw, gate powyżej 100%, retrigger, zależności kroków oraz mapowanie MIDI. PRO obecnie realizuje deterministyczną losowość, ziarno/Evolve, velocity, gate i mapowanie CC. Nie implementuje pełnej macierzy prawdopodobieństw, zależności kroków ani mikroprzesunięć per krok.
- **Surge XT**, [modulation routing](https://github.com/surge-synthesizer/surge-synthesizer.github.io/blob/64638dd4a56ac50d0b7a8453467c77c2f8c9a8f9/src/content/manual_xt/06-modulation-routing.mdx): przydatna referencja generowania i modulacji — osobne modulatory głosów/sceny, Sample & Hold, step sequencer, obwiednie oraz tryby wyzwalania. To referencja architektury modulacji, nie twierdzenie, że Surge ma taki sam arpeggiator.

Strony instrukcji Ableton Live, Kirnu Cream i Xfer Cthulhu zwróciły blokadę proxy HTTP 403. Nie przeprowadzono odsłuchowego rankingu komercyjnych produktów i nie uznajemy ich szczegółów za zweryfikowane.

Kolejne kandydatury po odsłuchach PRO: edytowalny pattern nut/gate/velocity, ratchet, probability i rotacja kroków, osobne długości warstw, wyjście/eksport MIDI. Nie są oznaczone jako funkcje obecnego buildu.
