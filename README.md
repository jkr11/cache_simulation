

# GRA TEAM 192

## Aufgabestellung 

Das Ziel des Projektes war die Implementierung eines zweistufigen direkt assoziativen Caches mithilfe von SystemC und C++ und einem C-Rahmenprogramm. 

## Implementierung


Darstellung der Cachelines |tag|data|empty als struct
```C++
struct CacheLine {
  int tag;
  uint8_t *bytes;  
  int empty;
};
```

Implementierung von 3 SystemC Modulen:

```C++
SC_MODULE(CACHEL1)
SC_MODULE(CACHEL2)
SC_MODULE(MEMORY)
```
mit Speichern
```C++
Cachelines* internal;
```
 und
```C++
std::unordered_map<uint32_t,uint8_t>  internal;
```
Hier wird außerdem unaligned access verwendet.

### Berechnung von Hit und Miss, cycles

Die totale Hit und Miss ist eine Summe von den Zählern in L1 und L2.

Bei Zeilenübergreifenden Zugriffen haben wir uns entschieden, dass die latency des L1-Caches doppelt gezählt wird, da unsere Implementierung dies in 2 Zugriffe aufteilt.

Das Zählen der Cycles selbst findet in run_simulation statt, die Latenzen werden mithilfe von wait(2*latency,SC_NS) jeweils in 3 Modulen realisiert.


## Literaturrecherche

Die Literaturrecherche ergab, dass die Latenzen standardmäßig bei L1 = 4 cycles, L2 = 16 und memory = 400 cycles liegen. Weiter wird in standartmäßigen Implementierungen unaligned memory access verwendet. Direkt assoziative Caches ermöglichen eine einfache Implementierung der Zugriffe und Verwaltung. Write-through vereinfacht die Implementierung und Kohärenz.
## Methodik und Messumgebung

Die Simulation würde in SystemC durchgeführt, kleine Beispiele mithilfe von gdb und GTKWave (auch für Latenz auf cycle-ebene) analysiert und verifiziert, große Beispiele über .csv Dateien. 

Die Access csvs sind entweder in test.ipynb oder per Hand generiert, test und analyse mithilfe von .sh und .ipynb.


## Ergebnisse des Projekts

Das Projekt verifiziert das aus der Literatur erwartete Verhalten.


## Beitrag

Der Commit-Graph ist in gitgraph zu finden.

### Xuanqi Meng
Entwicklung von einer Version von 3 Module und die Simulation, die später von anderen Teamgelied als abzugebene Vorlage gewählt worde. 

Teilweise Debuggen und Logikoptimierung. 

Entwurf von Schaltung der entsprechenden Programmlogik.
### Jeremias Rieser 
Erstellen von Tests und acccess-csvs, verifizierung und analyse der Ergebnisse. Debugging und teilweise Memory safety. 

Implementierung von Rahmenprogramm, Makefiles, Include
Literaturrecherche, Slides und Grafiken.

Entwickeln einer nur in Struktur verwendeten Cache-Simulation. 


### Artem Bilovol
Erstellen und Ausführen von Tests (manuell/zufällig generiert) zur Überprüfung der korrekten Funktionalität des Cache (test_data_validity in tests/)

Debuggen, insbesondere:
    - korrekte Behandlung der Pfade für Tracefile und Inputfile (Funktionen expand_path und create_dir in main.c)
    - Randfälle und Fehlermeldung für Kommandozeilenparameter (negative Werte, Cache größer als Speicher usw)

Zusammenfassung und Ausblick (Analyse von Erweiterung und Verbesserung des Projekts)

Entwicklung von einer Version von 3 Modulen und Simulation (nicht als abzugebene Vorlage ausgewählt)