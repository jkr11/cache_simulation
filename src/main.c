#include <getopt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <libgen.h>
#include <limits.h>

#include "../include/csv_parser.h"
#include "../include/simulation.h"
#include "../include/types.h"
#include "../include/util.h"
// #include "simulation.h"

#define _DEBUG
#define _OUT
// is printing to stderr correct? NETBSD does it

static void usage(const char* prog_name)
{
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

// Function to expand the path if starts with '~'
char* expand_path(char* path)
{
    if (path[0] != '~')
    {
        return path;
    }

    // Should not exceed maximal path length
    char exp[pathconf("/", _PC_PATH_MAX)];

    // Getting the home directory
    const char* home = getenv("HOME");
    if (home == NULL)
    {
        HANDLE_ERROR("Failed to create a tracefile");
    }

    // Constructing the new path
    snprintf(exp, sizeof(exp), "%s%s", home, path + 1);
    char* expPath = (char*)malloc(strlen(exp) + 1);
    if (expPath == NULL)
    {
        HANDLE_ERROR("Failed to create a tracefile");
    }

    strcpy(expPath, exp);

    return expPath;
}

// Function to create directories along the path if needed
void create_dir(const char* path)
{
    char tmp[pathconf("/", _PC_PATH_MAX)];
    strncpy(tmp, path, sizeof(tmp) - 1);
    tmp[sizeof(tmp) - 1] = '\0';

    // Processing directories in the path
    char* directories = dirname(tmp);

    // This variable is required to check directory properties later
    struct stat s;

    char dirPath[256];
    strncpy(dirPath, directories, sizeof(dirPath) - 1);
    dirPath[sizeof(dirPath) - 1] = '\0';

    char* curr = strtok(dirPath, "/");
    char fullPath[256] = "";

    // Turning a realtive path into full
    if (path[0] == '.' && path[1] == '/')
    {
        strcpy(fullPath, ".");
    }

    // Iterating and creating each required directory
    while (curr != NULL)
    {
        strcat(fullPath, "/");
        strcat(fullPath, curr);

        if (stat(fullPath, &s) != 0)
        {
            if (mkdir(fullPath, 0777) != 0)
            {
                HANDLE_ERROR("Failed to create directories for the tracefile");
            }
        }
        curr = strtok(NULL, "/");
    }

    // Checking if directories exist and writable
    if (access(directories, F_OK) == -1 || access(directories, W_OK) == -1)
    {
        HANDLE_ERROR("Failed to create directories for the tracefile or no permissions to write there");
    }
}

// check that input is a valid csv
int is_valid_csv(const char* filename)
{
    char* ext = strrchr(filename, '.');
    return ext && strcmp("csv", ext + 1) == 0;
}

int is_power_of_two(int n) { return n > 0 && ((n & (n - 1)) == 0); }


int main(int argc, char* argv[])
{
    if (argc == 1)
    {
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }
    // can we estimate this at the beginning? at most 2^32 acesses * rough_count
    int cycles = 10000000;
    // https://www.cs.princeton.edu/courses/archive/fall15/cos217/reading/x86-64-opt.pdf // 2012
    // https://www.intel.com/content/dam/doc/manual/64-ia-32-architectures-optimization-manual.pdf#G9.83205 section 2.1.5  // 2024
    // often l1CacheLines and L2CacheLines will around 32 KB or 256KB which would correspoint to around 4000 Cache lines
    // and we have to scale down here a little bit
    // testing this is the goal anyways
    unsigned cacheLineSize = 64;
    unsigned l1CacheLines = 32;

    unsigned l2CacheLines = 128; // usually 4 to 8 times l1CacheLines
    // and
    // https://colin-scott.github.io/personal_website/research/interactive_latency.html
    // https://www.anandtech.com/show/14664/testing-intel-ice-lake-10nm/2
    //
    // assuming 4 Ghz CPU
    unsigned l1CacheLatency = 4;
    unsigned l2CacheLatency = 16;
    unsigned memoryLatency = 400;

    char* tracefile = NULL;
    char* inputfile;

    int opt;
    int option_ind = 0;

    // Additional variables for "not a negative number check"
    char* endptr;
    long valueLong;

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
        {0, 0, 0, 0}
    };

    while ((opt = getopt_long(argc, argv, "c:h", long_options, &option_ind)) !=
        -1)
    {
        switch (opt)
        {
        case 'c':
            cycles = atoi(optarg);
            if (cycles < 0)
            {
                HANDLE_ERROR("cycles must be not negative");
            }
            printf("cycles: %d\n", cycles);
            break;
        case 1:
            // Not a negative number check + checking that it is at least 4
            valueLong = strtol(optarg, &endptr, 10);

            if (*endptr != '\0' || valueLong < 4)
            {
                HANDLE_ERROR("cache line size must be at least 4");
            }

            cacheLineSize = (unsigned int)valueLong;
            if (!is_power_of_two(cacheLineSize))
            {
                HANDLE_ERROR("cache line size must be power of 2");
            }
            printf("cacheLineSize: %d\n", cacheLineSize);
            break;
        case 2:
            // Not a negative number check
            valueLong = strtol(optarg, &endptr, 10);

            if (*endptr != '\0' || valueLong <= 0)
            {
                HANDLE_ERROR("l1 cache lines must be positive");
            }

            l1CacheLines = (unsigned int)valueLong;
            if (l1CacheLines < 2)
            {
                HANDLE_ERROR("l1 cache lines must be greater than 2 to handel unaligned access sensible");
            }
            if (!is_power_of_two(l1CacheLines))
            {
                HANDLE_ERROR("l1 cache lines must be power of 2");
            }
            printf("l1CacheLines: %d\n", l1CacheLines);
            break;
        case 3:
            // Not a negative number check
            valueLong = strtol(optarg, &endptr, 10);

            if (*endptr != '\0' || valueLong < 0)
            {
                HANDLE_ERROR("l2 cache lines must be positive");
            }

        // Cache is smaller than memory (2^32 bytes) check
            if (valueLong * (long)cacheLineSize >= 0xFFFFFFFF)
            {
                HANDLE_ERROR("cache size should be smaller than memory");
            }

            l2CacheLines = (unsigned int)valueLong;
            if (!is_power_of_two(l1CacheLines))
            {
                HANDLE_ERROR("l2 cache lines must be power of 2");
            }
            if (l2CacheLines < l1CacheLines)
            {
                HANDLE_ERROR("l2 cache lines must be greater than that of l1");
            }
            printf("l2CacheLines: %d\n", l2CacheLines);
            break;
        case 4:
            // Not a negative number check
            valueLong = strtol(optarg, &endptr, 10);

            if (*endptr != '\0' || valueLong < 0)
            {
                HANDLE_ERROR("l1 Latency must be not negative");
            }

            l1CacheLatency = (unsigned int)valueLong;
            printf("l1CacheLatency: %d\n", l1CacheLatency);
            break;
        case 5:
            // Not a negative number check
            valueLong = strtol(optarg, &endptr, 10);

            if (*endptr != '\0' || valueLong < 0)
            {
                HANDLE_ERROR("l2 Latency must be not negative");
            }

            l2CacheLatency = (unsigned int)valueLong;
            if (l2CacheLatency < l1CacheLatency)
            {
                HANDLE_ERROR("l2 Latency must be greater than that of l1");
            }
            printf("l2CacheLatency: %d\n", l2CacheLatency);
            break;
        case 6:
            // Not a negative number check
            valueLong = strtol(optarg, &endptr, 10);

            if (*endptr != '\0' || valueLong < 0)
            {
                HANDLE_ERROR("memory Latency must be not negative");
            }

            memoryLatency = (unsigned int)valueLong;
            if (memoryLatency < l2CacheLatency)
            {
                HANDLE_ERROR("memory Latency must be greater than that of l2");
            }
            printf("memoryLatency: %d\n", memoryLatency);
            break;
        case 7:
            // If filepath starts with '~', it is expanded
            tracefile = expand_path(optarg);

            if (strlen(tracefile) > 256)
            {
                HANDLE_ERROR("Tracefile path is too long");
            }

            printf("tracefile: %s\n", tracefile);
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

    // Creating directories on the path of the tracefile if needed
    create_dir(tracefile);

    if (optind >= argc && optind != 1)
    {
        usage(argv[0]);
        HANDLE_ERROR("Missing filename");
    }
    else
    {
        // If filepath starts with '~', it is expanded
        inputfile = expand_path(argv[optind]);

        if (strlen(inputfile) > 256)
        {
            HANDLE_ERROR("Inputfile path is too long");
        }

        printf("%s\n", inputfile);
        if (!is_valid_csv(inputfile))
        {
            HANDLE_ERROR("Input file must be csv");
        }
        // https://stackoverflow.com/questions/230062/whats-the-best-way-to-check-if-a-file-exists-in-c
        if (access(inputfile, F_OK) != 0)
        {
            HANDLE_ERROR("Input file does not exist");
        }
    }

    size_t numRequests;
    Request* requests = parse_csv(inputfile, &numRequests);
#ifdef _DEBUG
    print_requests(requests, numRequests);
#endif
    Result result;
    result = run_simulation(cycles, l1CacheLines, l2CacheLines, cacheLineSize,
                            l1CacheLatency, l2CacheLatency, memoryLatency,
                            numRequests, requests, tracefile);

#ifdef _DEBUG
    print_requests(requests, numRequests); // print requests after execution with updated data from read
    print_result(&result);
#endif
#ifdef _OUT
    FILE *file = fopen("results.csv", "w+");
    if (file == NULL) {
        HANDLE_ERROR("Test file does not exist");
    }

    fprintf(file, "%zu,%zu,%zu,%zu\n", result.cycles, result.misses,
          result.hits, result.primitiveGateCount);

    fclose(file);
#endif

    free(requests);
    exit(EXIT_SUCCESS);
}
