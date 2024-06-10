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
    fprintf(stderr, "-c <Zahl> / --cycles <Zahl>     Die Anzahl der Zyklen, die simuliert x^ sollen.\n");
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

    int opt;
    int c,cs,l1l,l2l,l1la,l2la,ml; // TODO: refactor this into struct
    char *tf;
    // probably forgot something 
    static struct option long_options[] = {
        {"cycles", required_argument, 0, 'c'},
        {"cacheline-size", required_argument, 0, 1},
        {"l1-lines", required_argument, 0, 2},
        {"l2-lines", required_argument, 0, 3},
        {"l1-latency", required_argument, 0, 4},
        {"l2-latency", required_argument, 0, 5},
        {"memory-latency", required_argument, 0, 6},
        {"tf", required_argument, 0, 7},
        {"help", no_argument, 0, 'h'},
        {0,0,0,0}
    };

    while((opt = getopt_long(argc, argv, "c:h", long_options, NULL)) != -1) {
        switch (opt)
        {
        case 'c':
            c = atoi(optarg);
            printf("cycles: %d\n", c);
            break;
        case 1:
            cs = atoi(optarg);
            printf("cs: %d\n", cs);
            break;
        case 2:
            l1l = atoi(optarg);
            printf("l1l: %d\n", l1l);
            break;
        case 3:
            l2l = atoi(optarg);
            printf("l2l: %d\n", l2l);
            break;
        case 4:
            l1la = atoi(optarg);
            printf("l1la: %d\n", l1la);
            break;
        case 5:
            l2la = atoi(optarg);
            printf("l2la: %d\n", l2la);
            break;
        case 6:
            ml = atoi(optarg);
            printf("ml: %d\n", ml);
            break;
        case 7:
            tf = optarg;
            printf("tf: %s\n", tf);
            break;
        case 'h':
            usage(argv[0]);
            exit(EXIT_SUCCESS);
        default:
            usage(argv[0]);
            exit(EXIT_FAILURE);
            break;
        }
    }
    exit(EXIT_SUCCESS);
}