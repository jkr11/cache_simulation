#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include "../include/types.h"
#include "../include/csv_parser.h"
// #include "simulation.h"


// is printing to stderr correct? NETBSD does it
static void usage(const char *prog_name) {
    fprintf(stderr, "Usage: %s [options] <input_file>\n", prog_name);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "-c <Zahl> / --cycles <Zahl>     Die Anzahl der Zyklen, die simuliert werden sollen.\n");
    fprintf(stderr, "--cacheline-size <Zahl>         Die Größe einer Cachezeile in Byte.\n");
    fprintf(stderr, "--l1-lines <Zahl>               Die Anzahl der Cachezeilen des L1 Caches.\n");
    fprintf(stderr, "--l2-lines <Zahl>               Die Anzahl der Cachezeilen des L2 Caches.\n");
    fprintf(stderr, "--l1-latency <Zahl>             Die Latenzzeit des L1 Caches in Zyklen. Latenzzeit ist unabhängig von Treffer oder Miss, sowie von Lese oder Schreiboperationen.\n");
    fprintf(stderr, "--l2-latency <Zahl>             Die Latenzzeit des L2 Caches in Zyklen. Latenzzeit ist unabhängig von Treffer oder Miss, sowie von Lese oder Schreiboperationen.\n");
    fprintf(stderr, "--memory-latency <Zahl>         Die Latenzzeit des Hauptspeichers in Zyklen. Lese und schreiboperationen haben die gleiche Latenz.\n");
    fprintf(stderr, "--tf=<Dateiname>                Ausgabedatei für ein Tracefile mit allen Signalen. Wenn diese Option nicht gesetzt wird, soll kein Tracefile erstellt werden.\n");
    fprintf(stderr, "<Dateiname>                     Positional Argument: Die Eingabedatei, die die zu verarbeitenden Daten enthält.\n");
    fprintf(stderr, "-h/--help                       Eine Beschreibung aller Optionen des Programms und Verwendungsbeispiele werden ausgegeben und das Programm danach beendet.\n");

}

int main(int argc, char* argv[]) {
    usage(argv[0]);
    exit(EXIT_SUCCESS);
}