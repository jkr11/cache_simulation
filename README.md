

# GRA TEAM 192

## Überblick

### Aufgabestellung 

Das Ziel des Projektes war die Implementierung eines zweistufigen direkt assoziativen Caches mithilfe von SystemC und einem C-Rahmenprogramm. Der Cache sollte in zwei Ebenen (L1 und L2) arbeiten und eine Speicherhierarchie simulieren.

## Implementierung

### Struktur

```bash
- project/
  - include/ #  enthät globale definierungen von z.B. Request und Result, HANDLE_ERROR etc ...
  - src/ # enthält die gesamte Implementierung des Caches (C++) und des Rahmenprogramms
  - test/ # enthält code und inputs zum testen der implementierung
  ...
- Makefile # make project / make clean
- test.sh # kompiliert und lässt mit kleinem Beispiel laufen
...
```
### Logik

Darstellung der Cachelines |tag|data|empty als struct
```C++
struct CacheLine {
  int tag;
  uint8_t *bytes;  // Einfachere Byte-adressierung
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

Bei Zeilenübergreifenden Zugriffen haben wir uns entschieden, dass die latency des L1-Caches doppelt gezählt wird, da unsere Implementierung dies in 2 Zugriffe aufteilt.

Das Zählen der Cycles selbst findet in run_simulation statt, die Latenzen werden mithilfe von wait(latency) an die simulationsmethode weitergegeben.


## Literaturrecherche

Die Literaturrecherche ergab, dass die Latenzen (hier normalisiert über einen 4GHz Prozessor) bei L1 = 3 cycles, L2 = 20 und memory = 50 cycles liegen. Weiter wird in standartmäßigen Implementierungen unaligned memory access verwendet. Direkt assoziative Caches ermöglichen eine einfache Implementierung der Zugriffe und Verwaltung, ebenso vereinfacht die Write-through method die Implementierung (und Kohärenz bzgl. Inklusivität) im vergleich zu z.B. write-back.

## Methodik und Messumgebung

Die Simulation würde in SystemC durchgeführt, kleine Besipiele mithilfe von gdb und GTKWave (auch für Latenz auf cycle-ebene) analysiert und verifiziert, große Beispiele über .csv Dateien. 

Manche der Testdaten wurden über python generiert, um möglichst einfach zufällige dicht- und weitverteilte Zugriffsfolgen zu bekommen (generate.py).

Die Tests selbst wurden mithilfe von .sh files durchgeführt.

## Ergebnisse des Projekts

Das Projekt verifiziert die erwartete Reduktion der Zugriffszeiten.


## Beitrag

### Xuanqi Meng

### Jeremias Rieser 

### Artem Bilovol