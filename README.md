

# GRA TEAM 192

## Überblick

### Aufgabestellung 

Das Ziel des Projektes war die Implementierung eines zweistufigen direkt assoziativen Caches mithilfe von SystemC und einem C-Rahmenprogramm. Der Cache sollte in zwei Ebenen (L1 und L2) arbeiten und eine Speicherhierarchie simulieren.

## Implementierung

### Logik

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
welche jeweils den eigentlichen Cache als
```C++
Cachelines* internal;
```
verwalten und
```C++
std::unordered_map<uint32_t,uint8_t>  internal;
```
da die innere Logik von Memory nicht für die Simulation relevant ist.

### Berechnung von Hit und Miss, cycles

Die totale Hit und Miss ist eine Summe von den Zählern in L1 und L2.

Bei Zeilenübergreifenden Zugriffen haben wir uns entschieden, dass die latency des L1-Caches doppelt gezählt wird, da unsere Implementierung dies in 2 Zugriffe aufteilt.

Das Zählen der Cycles selbst findet in run_simulation statt, die Latenzen werden mithilfe von wait(2*latency,SC_NS) jeweils in 3 Modulen realisiert.


## Literaturrecherche

Die Literaturrecherche ergab, dass die Latenzen (hier normalisiert über einen 4GHz Prozessor) bei L1 = 3 cycles, L2 = 20 und memory = 50 cycles liegen. Weiter wird in standartmäßigen Implementierungen unaligned memory access verwendet. Direkt assoziative Caches ermöglichen eine einfache Implementierung der Zugriffe und Verwaltung, ebenso vereinfacht die Write-through method die Implementierung (und Kohärenz bzgl. Inklusivität) im vergleich zu z.B. write-back.

## Methodik und Messumgebung

Die Simulation würde in SystemC durchgeführt, kleine Beispiele mithilfe von gdb und GTKWave (auch für Latenz auf cycle-ebene) analysiert und verifiziert, große Beispiele über .csv Dateien. 

Die Korrektheit von Ein-/Auslesen durch Cache wurde durch Vergleich mit den Ergebnissen von dem Tester verifiziert, der direkt mit Memory kommuniziert.

Manche der Testdaten wurden über python generiert, um möglichst einfach zufällige dicht- und weitverteilte Zugriffsfolgen zu bekommen (generate.py).

Die Tests selbst wurden mithilfe von .sh files durchgeführt.

## Schaltkreis und Gatteranzahl

der Entwurf von Schaltung ist in Schaltung/ zu finden. Die Berechnung von Gatter ist mit dem Entwurf vergleichbar. 

## Ergebnisse des Projekts

Das Projekt verifiziert die erwartete Reduktion der Zugriffszeiten.


## Beitrag

### Xuanqi Meng
Entwicklung von einer Version von 3 Modulen und der Simulation, die später von anderen Teamgelied als abzugebene Vorlage gewählt worde. 

Teilweise Debuggen und Logikoptimierung (Modulensynchronisation und Hit/Misszähler). 

Entwurf von Schaltung der entsprechenden Programmlogik und Korrektheitprüfung.
### Jeremias Rieser 

Erstellen von Tests und acccess-csvs, verifizierung und analyse der Ergebnisse. Debugging und teilweise Memory safety.

Implementierung von Rahmenprogramm, Makefiles, Include Literaturrecherche, Slides und Grafiken.

Entwickeln einer nur in Struktur verwendeten Cache-Simulation.

### Artem Bilovol
Entwicklung von einer Version von 3 Module und die Simulation (nicht als abzugebene Vorlage ausgewählt)

Erstellen Teste zur korrekten Funktionalität des Cache (tests/)

Debuggen, insbesondere bei Behandlung der Dateipfade und Randfälle

Zusammenfassung und Ausblick von Folien

