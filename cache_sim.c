#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum { dm, fa } cache_map_t;
typedef enum { uc, sc } cache_org_t;
typedef enum { instruction, data } access_t;


typedef struct {
  uint32_t address;
  access_t accesstype;
} mem_access_t;

typedef struct {
  uint64_t accesses;
  uint64_t hits;
  uint64_t evicts;
} cache_stat_t;

//strucure for block
typedef struct {
    uint32_t tag;   //address
    int valid;      //used to check validity of the block, 1-0
} cache_block_t;

//Pointers to array for split cache:
cache_block_t* data_cache; 
cache_block_t* instruction_cache;
//Pointer to array for unified cache
cache_block_t* unified_cache;

// DECLARE CACHES AND COUNTERS FOR THE STATS HERE

uint32_t cache_size;
uint32_t block_size = 64;
cache_map_t cache_mapping;
cache_org_t cache_org;

// USE THIS FOR YOUR CACHE STATISTICS
cache_stat_t cache_statistics;

/* Reads a memory access from the trace file and returns
 * 1) access type (instruction or data access
 * 2) memory address
 */
mem_access_t read_transaction(FILE* ptr_file) {
  char type;
  mem_access_t access;

  if (fscanf(ptr_file, "%c %x\n", &type, &access.address) == 2) {
    if (type != 'I' && type != 'D') {
      printf("Unkown access type\n");
      exit(0);
    }
    access.accesstype = (type == 'I') ? instruction : data;
    return access;
  }

  /* If there are no more entries in the file,
   * return an address 0 that will terminate the infinite loop in main
   */
  access.address = 0;
  return access;
}

void access_cache_dm(cache_block_t cache[], uint32_t address, int size) {
  uint32_t tag = address / block_size;    //find tag from address, used to see if the data stored is correct
  uint32_t i = tag % size;      //index in the cache, wraps around

  if (cache[i].valid && cache[i].tag == tag) {   //if the block is valid and the calculated tag matches the stored tag it's a hit
    //cache hit
    cache_statistics.hits++;      //adds hit to statistics
    return;
  } else {
    //cache miss
    cache[i].valid = 1;           //sets the bloct to valid
    cache[i].tag = tag;           //updates the tag
  }
}

void access_cache_fa(cache_block_t cache[], uint32_t address, int size) {
  uint32_t tag = address / block_size; 
  static int replace_counter = 0;   //used to see what was first in

  //Search entire cache for tag
    for (int i = 0; i < size; i++) {
        if (cache[i].valid && cache[i].tag == tag) {    //hit works the same as in dm
          //cache hit
          cache_statistics.hits++;
          return;
        }
    }
    //cache miss, replaces the next block
  cache_statistics.evicts++;                      //didn't get this to work correctly, according to testcases
  cache[replace_counter].valid = 1;               //now contains correct data
  cache[replace_counter].tag = tag;               //gives it the appropiate tag

  replace_counter = (replace_counter + 1) % size; //increments the counter, and wraps around

}
//initializing, iterates through the cache and sets all blocks invalid and without tags
void initialize_cache(cache_block_t cache[], int size) {
  for (int i = 0; i < size; i++) {
    cache[i].valid = 0;
    cache[i].tag = 0;
  }
}


void main(int argc, char** argv) {
  // Reset statistics:
  memset(&cache_statistics, 0, sizeof(cache_stat_t));

  /* Read command-line parameters and initialize:
   * cache_size, cache_mapping and cache_org variables
   */
  /* IMPORTANT: *IF* YOU ADD COMMAND LINE PARAMETERS (you really don't need to),
   * MAKE SURE TO ADD THEM IN THE END AND CHOOSE SENSIBLE DEFAULTS SUCH THAT WE
   * CAN RUN THE RESULTING BINARY WITHOUT HAVING TO SUPPLY MORE PARAMETERS THAN
   * SPECIFIED IN THE UNMODIFIED FILE (cache_size, cache_mapping and cache_org)
   */
  if (argc != 4) { /* argc should be 2 for correct execution */
    printf(
        "Usage: ./cache_sim [cache size: 128-4096] [cache mapping: dm|fa] "
        "[cache organization: uc|sc]\n");
    exit(0);
  } else {
    /* argv[0] is program name, parameters start with argv[1] */

    /* Set cache size */
    cache_size = atoi(argv[1]);

    /* Set Cache Mapping */
    if (strcmp(argv[2], "dm") == 0) {
      cache_mapping = dm;
    } else if (strcmp(argv[2], "fa") == 0) {
      cache_mapping = fa;
    } else {
      printf("Unknown cache mapping\n");
      exit(0);
    }

    /* Set Cache Organization */
    if (strcmp(argv[3], "uc") == 0) {
      cache_org = uc;
    } else if (strcmp(argv[3], "sc") == 0) {
      cache_org = sc;
    } else {
      printf("Unknown cache organization\n");
      exit(0);
    }
  }

  /* Open the file mem_trace.txt to read memory accesses */
  FILE* ptr_file;
  ptr_file = fopen("d100hit.txt", "r"); ///////////////////////////////////////////filnavn
  if (!ptr_file) {
    printf("Unable to open the trace file\n");
    exit(1);
  }

  uint32_t num_of_blocks = cache_size / block_size;
  //Allocating memory based on number of blocks and the size, points to the array.
  //Split cache:
  data_cache = (cache_block_t*)malloc(num_of_blocks * sizeof(cache_block_t));
  instruction_cache = (cache_block_t*)malloc(num_of_blocks * sizeof(cache_block_t));
  //unified cache:
  unified_cache = (cache_block_t*)malloc(num_of_blocks * sizeof(cache_block_t));

  // Initializing all caches, split the size between data and instructions for sc
  initialize_cache(data_cache, num_of_blocks/2);
  initialize_cache(instruction_cache, num_of_blocks/2);
  initialize_cache(unified_cache, num_of_blocks);

  /* Loop until whole trace file has been read */
  mem_access_t access;
  while (1) {
    access = read_transaction(ptr_file);
    // If no transactions left, break out of loop
    if (access.address == 0) break;
    cache_statistics.accesses++;
    printf("%d %x\n", access.accesstype, access.address);
    /* Do a cache access */
    //Straight forward if else(even though it is a bit messy), that chooses between dm/uc, and then data/instruction if sc
    if (cache_mapping == dm) {
        if (cache_org == uc) {
            access_cache_dm(unified_cache, access.address, num_of_blocks);
        } else {
            if (access.accesstype == data) {
                access_cache_dm(data_cache, access.address, num_of_blocks);
            } else {
                access_cache_dm(instruction_cache, access.address, num_of_blocks);
            }
        }
    } else {
        if (cache_org == uc) {
            access_cache_fa(unified_cache, access.address, num_of_blocks);
        } else {
            if (access.accesstype == data) {
                access_cache_fa(data_cache, access.address, num_of_blocks);
            } else {
                access_cache_fa(instruction_cache, access.address, num_of_blocks);
            }
        }
    }
}
  /* Print the statistics */
  // DO NOT CHANGE THE FOLLOWING LINES!
  printf("\nCache Statistics\n");
  printf("-----------------\n\n");
  printf("Accesses: %ld\n", cache_statistics.accesses);
  printf("Hits:     %ld\n", cache_statistics.hits);
  printf("Hit Rate: %.4f\n",
         (double)cache_statistics.hits / cache_statistics.accesses);
  // DO NOT CHANGE UNTIL HERE
  // You can extend the memory statistic printing if you like!
  printf("Evicts:     %ld\n", cache_statistics.evicts);


  //freeing up memory allocated
  free(data_cache);
  free(instruction_cache);
  free(unified_cache);

  /* Close the trace file */
  fclose(ptr_file);
}
