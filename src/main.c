#include <getopt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../include/csv_parser.h"
#include "../include/simulation.h"
#include "../include/types.h"
#include "../include/util.h"
// #include "simulation.h"

#define _DEBUG
#define _OUT
// is printing to stderr correct? NETBSD does it


static void usage(const char *prog_name) {
  fprintf(stderr, "Usage: %s [options] <input_file>\n", prog_name);
  fprintf(stderr, "Options:\n");
  fprintf(stderr,
          "-c <Zahl> / --cycles <Zahl>     Die Anzahl der Zyklen, die "
          "simuliert x^ sollen.\n");
  fprintf(
      stderr,
      "--cacheline-size <Zahl>         Die Größe einer Cachezeile in Byte.\n");
  fprintf(stderr,
          "--l1-lines <Zahl>               Die Anzahl der Cachezeilen des L1 "
          "Caches.\n");
  fprintf(stderr,
          "--l2-lines <Zahl>               Die Anzahl der Cachezeilen des L2 "
          "Caches.\n");
  fprintf(stderr,
          "--l1-latency <Zahl>             Die Latenzzeit des L1 Caches in "
          "Zyklen. Latenzzeit ist unabhängig von Treffer oder Miss, sowie von "
          "Lese oder Schreiboperationen.\n");
  fprintf(stderr,
          "--l2-latency <Zahl>             Die Latenzzeit des L2 Caches in "
          "Zyklen. Latenzzeit ist unabhängig von Treffer oder Miss, sowie von "
          "Lese oder Schreiboperationen.\n");
  fprintf(stderr,
          "--memory-latency <Zahl>         Die Latenzzeit des Hauptspeichers "
          "in Zyklen. Lese und schreiboperationen haben die gleiche Latenz.\n");
  fprintf(stderr,
          "--tf=<Dateiname>                Ausgabedatei für ein Tracefile mit "
          "allen Signalen. Wenn diese Option nicht gesetzt wird, soll kein "
          "Tracefile erstellt werden.\n");
  fprintf(stderr,
          "<Dateiname>                     Positional Argument: Die "
          "Eingabedatei, die die zu verarbeitenden Daten enthält.\n");
  fprintf(stderr,
          "-h/--help                       Eine Beschreibung aller Optionen "
          "des Programms und Verwendungsbeispiele werden ausgegeben und das "
          "Programm danach beendet.\n");
}

// check that input is a valid csv
int is_valid_csv(const char *filename) {
  char *ext = strchr(filename, '.');
  return ext && strcmp("csv", ext + 1) == 0;
}


int is_power_of_two(int n) { return n > 0 && ((n & (n - 1)) == 0); }

#ifdef _OUT
void print_inputs(FILE *file, int cycles, unsigned l1CacheLines,
                  unsigned l2CacheLines, unsigned cacheLineSize,
                  unsigned l1CacheLatency, unsigned l2CacheLatency,
                  unsigned memoryLatency) {
  fprintf(file, "Simulation Inputs:\n");
  fprintf(file, "Cycles: %d\n", cycles);
  fprintf(file, "L1 Cache Lines: %u\n", l1CacheLines);
  fprintf(file, "L2 Cache Lines: %u\n", l2CacheLines);
  fprintf(file, "Cache Line Size: %u\n", cacheLineSize);
  fprintf(file, "L1 Cache Latency: %u\n", l1CacheLatency);
  fprintf(file, "L2 Cache Latency: %u\n", l2CacheLatency);
  fprintf(file, "Memory Latency: %u\n", memoryLatency);
  fprintf(file, "\n");
}

void print_result_to_file(FILE *file, const Result *result) {
  fprintf(file, "Simulation Result:\n");
  fprintf(file, "cycles: %ld\n", result->cycles);
  fprintf(file, "misses: %ld\n", result->misses);
  fprintf(file, "hits: %ld\n", result->hits);
  fprintf(file, "gates: %ld\n", result->primitiveGateCount);
  fprintf(file, "\n");
}

#endif

int sc_main(int argc, char *argv[]) {
  if (argc == 1) {
    usage(argv[0]);
    exit(EXIT_FAILURE);
  }
  // all of these have to be changed;
  int cycles = 10000000;
  unsigned l1CacheLines = 32;
  unsigned l2CacheLines = 64;
  unsigned cacheLineSize = 8;
  unsigned l1CacheLatency = 5;
  unsigned l2CacheLatency = 15;
  unsigned memoryLatency = 30;
  const char *tracefile =
      NULL;  // ???   Is this done in systemC or in simulation.cpp
  char *inputfile;

  int opt;
  int option_ind = 0;

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
      {0, 0, 0, 0}};

  while ((opt = getopt_long(argc, argv, "c:h", long_options, &option_ind)) !=
         -1) {
    switch (opt) {
      case 'c':
        cycles = atoi(optarg);
        printf("cycles: %d\n", cycles);
        break;
      case 1:
        cacheLineSize = atoi(optarg);
        if (cacheLineSize < 4) {
          HANDLE_ERROR("cache line size must be greater than 4");
        }
        if (!is_power_of_two(cacheLineSize)) {
          HANDLE_ERROR("cache line size must be power of 2");
        }
        printf("cs: %d\n", cacheLineSize);
        break;
      case 2:
        l1CacheLines = atoi(optarg);
        if (!is_power_of_two(l1CacheLines)) {
          HANDLE_ERROR("l1 cache lines must be power of 2");
        }
        printf("l1l: %d\n", l1CacheLines);
        break;
      case 3:
        l2CacheLines = atoi(optarg);
        if (!is_power_of_two(l1CacheLines)) {
          HANDLE_ERROR("l2 cache lines must be power of 2");
        }
        if (l2CacheLines < l1CacheLines) {
          HANDLE_ERROR("l2 cache lines must be greater than that of l1");
        }
        printf("l2l: %d\n", l2CacheLines);
        break;
      case 4:
        l1CacheLatency = atoi(optarg);
        printf("l1la: %d\n", l1CacheLatency);
        break;
      case 5:
        l2CacheLatency = atoi(optarg);
        if (l2CacheLatency < l1CacheLatency) {
          HANDLE_ERROR("l2 Latency must be greater than that of l1");
        }
        printf("l2la: %d\n", l2CacheLatency);
        break;
      case 6:
        memoryLatency = atoi(optarg);
        if (memoryLatency < l2CacheLatency) {
          HANDLE_ERROR("memory Latency must be greater than that of l2");
        }
        printf("ml: %d\n", memoryLatency);
        break;
      case 7:
        tracefile = optarg;
        printf("tf: %s\n", tracefile);
        break;
      case 'h':
        usage(argv[0]);
        exit(EXIT_SUCCESS);
        break;
      default:
        usage(argv[0]);
        exit(EXIT_FAILURE);
        break;
    }
  }
  // this is bugged you need at least one cmd option to run example.csv -- bug fixed!
  //  now only the .csv is missing
  //  so we can do this by optind
  if (optind >= argc && optind != 1) {
    usage(argv[0]);
    HANDLE_ERROR("Missing filename");
  } else {
    inputfile = argv[optind];
    printf("%s\n", inputfile);
    if (!is_valid_csv(inputfile)) {
      HANDLE_ERROR("Input file must be csv");
    }
    // https://stackoverflow.com/questions/230062/whats-the-best-way-to-check-if-a-file-exists-in-c
    // TODO: what platform does this even run on?
    if (access(inputfile, F_OK) != 0) {
      HANDLE_ERROR("Input file does not exist");
    }
  }

  size_t numRequests;
  Request *requests = parse_csv(inputfile, &numRequests);
#ifdef _DEBUG
  print_requests(requests, numRequests);
#endif
  Result result;
  result = run_simulation(cycles, l1CacheLines, l2CacheLines, cacheLineSize,
                          l1CacheLatency, l2CacheLatency, memoryLatency,
                          numRequests, requests, tracefile);

#ifdef _DEBUG
  print_requests(requests,numRequests); // print requests after execution
  print_result(&result);
#endif

#ifdef _OUT
  FILE *output_file = fopen("output.txt", "a");
  if (!output_file) {
    HANDLE_ERROR("Could not open file for writing");
  }

  print_inputs(output_file, cycles, l1CacheLines, l2CacheLines, cacheLineSize,
               l1CacheLatency, l2CacheLatency, memoryLatency);

  print_result_to_file(output_file, &result);
  fclose(output_file);
#endif
  free(requests);
  exit(EXIT_SUCCESS);
}