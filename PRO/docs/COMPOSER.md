# Composer / Arranger — PRO 0.7

Bieżąca instrukcja obsługi, eksportu MIDI/WAV i połączeń FL Studio znajduje się w [START_PL.md](START_PL.md). Interfejs 0.7 zastępuje osobne przełączniki COMPOSER / GENERATE / MIDI PARTS wyborem źródła MIDI / JAM / ARANŻER.

Dokument zawiera do 32 scen, każda po 1–32 takty, do 512 ręcznych nut na klip, osiem partii i do 128 pozycji songu po 1–16 powtórzeń. Wystąpienia tej samej sceny współdzielą edycje; WARIANT tworzy niezależną scenę. Metrum sceny: licznik 1–12, mianownik 2/4/8/16. Plan songu ma limit 500000 nut.

Progresje obsługują major, m, 5, sus4, m7, maj7, m9 i maj9. Klip generatora przekazuje rooty do instrumentu; klip manualny gra zapisane nuty. Zamrożenie utrwala wynik generatora. Edycje dokumentu mają lokalną historię 40 zmian. Historia nie jest zapisywana w projekcie i nie obejmuje globalnych parametrów ani wczytywania pliku.

MIDI: format 1, 960 PPQ, tempo, metrum, nazwy partii i markery. Kanały 1 Bass, 2 Pad, 3 Arp, 4 Krell, 10 Drums. Eksport zapisuje wynikowe nuty; do odtwarzania ustaw NUTY. MIDI nie zawiera pełnej syntezy, FX, stereo per-note ani automatyki timbru. Zapis `.jpro` i stan hosta zachowują ustawienia brzmienia.

Edycja przelicza niezmienny plan w wątku pomocniczym. Przyjęcie planu synchronizuje odtwarzanie. Zmiana uruchamianej sceny jest kwantyzowana; nie jest to wielośladowe nagrywanie live. Rendering WAV korzysta z prywatnego silnika i kopii dokumentu, nie porusza transportem sesji.

Źródła: gałąź `pro/composer-arranger`. Natywne Windows EXE/VST3 oraz instrukcje: `windows/pro-0.8/delivery`. Oryginalny projekt bez PRO pozostaje oddzielny.

LIVE używa tych samych scen, z wyborem następnej na granicy taktu. Patrz [LIVE_PL.md](LIVE_PL.md).
