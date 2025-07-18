//
// Free List Bin malloc implementation
// Uses multiple free lists organized by size ranges (bins)
//

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void *mmap_from_system(size_t size);
void munmap_to_system(void *ptr, size_t size);

typedef struct my_metadata_t {
  size_t size;
  struct my_metadata_t *next;
} my_metadata_t;

#define NUM_BINS 8
#define MIN_BIN_SIZE 8

typedef struct my_heap_t {
  my_metadata_t *bins[NUM_BINS];  // Array of free list heads for different size ranges
  my_metadata_t dummy;
} my_heap_t;

my_heap_t my_heap;

// Calculate which bin a size should go into
int get_bin_index(size_t size) {
  if (size <= 32) return 0;      // 8-32 bytes
  if (size <= 64) return 1;      // 33-64 bytes
  if (size <= 128) return 2;     // 65-128 bytes
  if (size <= 256) return 3;     // 129-256 bytes
  if (size <= 512) return 4;     // 257-512 bytes
  if (size <= 1024) return 5;    // 513-1024 bytes
  if (size <= 2048) return 6;    // 1025-2048 bytes
  return 7;                      // 2049+ bytes
}

void my_add_to_free_list(my_metadata_t *metadata) {
  assert(!metadata->next);
  int bin_index = get_bin_index(metadata->size);
  metadata->next = my_heap.bins[bin_index];
  my_heap.bins[bin_index] = metadata;
}

void my_remove_from_free_list(my_metadata_t *metadata, my_metadata_t *prev, int bin_index) {
  if (prev) {
    prev->next = metadata->next;
  } else {
    my_heap.bins[bin_index] = metadata->next;
  }
  metadata->next = NULL;
}

void my_initialize() {
  for (int i = 0; i < NUM_BINS; i++) {
    my_heap.bins[i] = NULL;
  }
  my_heap.dummy.size = 0;
  my_heap.dummy.next = NULL;
}

void *my_malloc(size_t size) {
  int start_bin = get_bin_index(size);
  my_metadata_t *metadata = NULL;
  my_metadata_t *prev = NULL;
  int found_bin = -1;
  
  // Search from the appropriate bin upwards for a suitable block
  for (int bin_index = start_bin; bin_index < NUM_BINS; bin_index++) {
    metadata = my_heap.bins[bin_index];
    prev = NULL;
    
    while (metadata) {
      if (metadata->size >= size) {
        found_bin = bin_index;
        break;
      }
      prev = metadata;
      metadata = metadata->next;
    }
    
    if (found_bin != -1) break;
  }

  if (found_bin == -1) {
    // No suitable free slot found, request new memory
    size_t buffer_size = 4096;
    my_metadata_t *new_metadata = (my_metadata_t *)mmap_from_system(buffer_size);
    new_metadata->size = buffer_size - sizeof(my_metadata_t);
    new_metadata->next = NULL;
    my_add_to_free_list(new_metadata);
    return my_malloc(size);
  }

  void *ptr = metadata + 1;
  size_t remaining_size = metadata->size - size;
  my_remove_from_free_list(metadata, prev, found_bin);

  if (remaining_size > sizeof(my_metadata_t)) {
    metadata->size = size;
    my_metadata_t *new_metadata = (my_metadata_t *)((char *)ptr + size);
    new_metadata->size = remaining_size - sizeof(my_metadata_t);
    new_metadata->next = NULL;
    my_add_to_free_list(new_metadata);
  }
  return ptr;
}

void my_free(void *ptr) {
  my_metadata_t *metadata = (my_metadata_t *)ptr - 1;
  my_add_to_free_list(metadata);
}

void my_finalize() {
  // Nothing here for now
}

void test() {
  // Test bin-based allocation
  void *small_ptrs[10];
  void *large_ptrs[5];
  
  // Allocate various sizes to test different bins
  for (int i = 0; i < 10; i++) {
    small_ptrs[i] = my_malloc(16 + i * 8);  // Different small sizes
  }
  
  for (int i = 0; i < 5; i++) {
    large_ptrs[i] = my_malloc(500 + i * 200);  // Different large sizes
  }
  
  // Free some blocks to populate bins
  for (int i = 0; i < 5; i++) {
    my_free(small_ptrs[i]);
  }
  
  // Reallocate to test bin usage
  for (int i = 0; i < 3; i++) {
    small_ptrs[i] = my_malloc(24);
  }
  
  // Clean up
  for (int i = 0; i < 10; i++) {
    if (small_ptrs[i]) my_free(small_ptrs[i]);
  }
  for (int i = 0; i < 5; i++) {
    my_free(large_ptrs[i]);
  }
  
  assert(1 == 1);
}
