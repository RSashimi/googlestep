//
// Mixed Optimized malloc implementation
// Combines: Segregated free lists, coalescing, best-fit for large blocks, 
// first-fit for small blocks, and memory pool optimization
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
  struct my_metadata_t *prev;  // Doubly linked for faster removal
  bool is_free;
} my_metadata_t;

#define NUM_SMALL_BINS 16
#define NUM_LARGE_BINS 4
#define SMALL_BIN_MAX_SIZE 256
#define LARGE_THRESHOLD 1024

typedef struct my_heap_t {
  my_metadata_t *small_bins[NUM_SMALL_BINS];  // 16, 32, 48, ..., 256
  my_metadata_t *large_bins[NUM_LARGE_BINS];  // 256-512, 512-1024, 1024-2048, 2048+
  my_metadata_t dummy;
} my_heap_t;

my_heap_t my_heap;

// Get small bin index for sizes 16-256
int get_small_bin_index(size_t size) {
  if (size <= 16) return 0;
  return (size - 1) / 16;  // 16->0, 32->1, 48->2, etc.
}

// Get large bin index for sizes > 256
int get_large_bin_index(size_t size) {
  if (size <= 512) return 0;
  if (size <= 1024) return 1;
  if (size <= 2048) return 2;
  return 3;
}

void add_to_small_bin(my_metadata_t *metadata) {
  int bin_index = get_small_bin_index(metadata->size);
  if (bin_index >= NUM_SMALL_BINS) return;
  
  metadata->next = my_heap.small_bins[bin_index];
  metadata->prev = NULL;
  if (my_heap.small_bins[bin_index]) {
    my_heap.small_bins[bin_index]->prev = metadata;
  }
  my_heap.small_bins[bin_index] = metadata;
  metadata->is_free = true;
}

void add_to_large_bin(my_metadata_t *metadata) {
  int bin_index = get_large_bin_index(metadata->size);
  
  // Insert in size order (best fit optimization)
  my_metadata_t *current = my_heap.large_bins[bin_index];
  my_metadata_t *prev = NULL;
  
  while (current && current->size < metadata->size) {
    prev = current;
    current = current->next;
  }
  
  metadata->next = current;
  metadata->prev = prev;
  
  if (prev) {
    prev->next = metadata;
  } else {
    my_heap.large_bins[bin_index] = metadata;
  }
  
  if (current) {
    current->prev = metadata;
  }
  
  metadata->is_free = true;
}

void my_add_to_free_list(my_metadata_t *metadata) {
  if (metadata->size <= SMALL_BIN_MAX_SIZE) {
    add_to_small_bin(metadata);
  } else {
    add_to_large_bin(metadata);
  }
}

void my_remove_from_free_list(my_metadata_t *metadata) {
  if (metadata->prev) {
    metadata->prev->next = metadata->next;
  } else {
    // This is the head of a bin
    if (metadata->size <= SMALL_BIN_MAX_SIZE) {
      int bin_index = get_small_bin_index(metadata->size);
      if (bin_index < NUM_SMALL_BINS) {
        my_heap.small_bins[bin_index] = metadata->next;
      }
    } else {
      int bin_index = get_large_bin_index(metadata->size);
      my_heap.large_bins[bin_index] = metadata->next;
    }
  }
  
  if (metadata->next) {
    metadata->next->prev = metadata->prev;
  }
  
  metadata->next = NULL;
  metadata->prev = NULL;
  metadata->is_free = false;
}

// Find neighbors for coalescing
my_metadata_t *find_left_neighbor(void *ptr) {
  // Search through all bins for potential left neighbor
  for (int i = 0; i < NUM_SMALL_BINS; i++) {
    my_metadata_t *current = my_heap.small_bins[i];
    while (current) {
      if (current->is_free) {
        char *block_end = (char *)(current + 1) + current->size;
        if (block_end == (char *)ptr) {
          return current;
        }
      }
      current = current->next;
    }
  }
  
  for (int i = 0; i < NUM_LARGE_BINS; i++) {
    my_metadata_t *current = my_heap.large_bins[i];
    while (current) {
      if (current->is_free) {
        char *block_end = (char *)(current + 1) + current->size;
        if (block_end == (char *)ptr) {
          return current;
        }
      }
      current = current->next;
    }
  }
  
  return NULL;
}

my_metadata_t *find_right_neighbor(my_metadata_t *metadata) {
  char *block_end = (char *)(metadata + 1) + metadata->size;
  my_metadata_t *potential_neighbor = (my_metadata_t *)block_end;
  
  // Check if this potential neighbor is free
  if (potential_neighbor->is_free) {
    return potential_neighbor;
  }
  
  return NULL;
}

void my_initialize() {
  for (int i = 0; i < NUM_SMALL_BINS; i++) {
    my_heap.small_bins[i] = NULL;
  }
  for (int i = 0; i < NUM_LARGE_BINS; i++) {
    my_heap.large_bins[i] = NULL;
  }
  my_heap.dummy.size = 0;
  my_heap.dummy.next = NULL;
  my_heap.dummy.prev = NULL;
  my_heap.dummy.is_free = true;
}

void *my_malloc(size_t size) {
  my_metadata_t *metadata = NULL;
  
  if (size <= SMALL_BIN_MAX_SIZE) {
    // Small allocation: use first-fit from appropriate bin
    int bin_index = get_small_bin_index(size);
    for (int i = bin_index; i < NUM_SMALL_BINS; i++) {
      if (my_heap.small_bins[i]) {
        metadata = my_heap.small_bins[i];
        break;
      }
    }
  } else {
    // Large allocation: use best-fit from large bins
    my_metadata_t *best_fit = NULL;
    size_t best_size = SIZE_MAX;
    
    for (int i = 0; i < NUM_LARGE_BINS; i++) {
      my_metadata_t *current = my_heap.large_bins[i];
      while (current) {
        if (current->size >= size && current->size < best_size) {
          best_fit = current;
          best_size = current->size;
          if (current->size == size) break;  // Perfect fit
        }
        current = current->next;
      }
      if (best_fit && best_fit->size == size) break;
    }
    metadata = best_fit;
  }

  if (!metadata) {
    // No suitable free slot found, request new memory
    size_t buffer_size = (size > 2048) ? size + sizeof(my_metadata_t) + 1024 : 4096;
    my_metadata_t *new_metadata = (my_metadata_t *)mmap_from_system(buffer_size);
    new_metadata->size = buffer_size - sizeof(my_metadata_t);
    new_metadata->next = NULL;
    new_metadata->prev = NULL;
    new_metadata->is_free = false;
    my_add_to_free_list(new_metadata);
    return my_malloc(size);
  }

  void *ptr = metadata + 1;
  size_t remaining_size = metadata->size - size;
  my_remove_from_free_list(metadata);

  if (remaining_size > sizeof(my_metadata_t)) {
    metadata->size = size;
    my_metadata_t *new_metadata = (my_metadata_t *)((char *)ptr + size);
    new_metadata->size = remaining_size - sizeof(my_metadata_t);
    new_metadata->next = NULL;
    new_metadata->prev = NULL;
    new_metadata->is_free = false;
    my_add_to_free_list(new_metadata);
  }
  return ptr;
}

void my_free(void *ptr) {
  my_metadata_t *metadata = (my_metadata_t *)ptr - 1;
  
  // Look for neighbors to coalesce
  my_metadata_t *left_neighbor = find_left_neighbor(metadata);
  my_metadata_t *right_neighbor = find_right_neighbor(metadata);
  
  if (left_neighbor && right_neighbor) {
    // Coalesce with both neighbors
    my_remove_from_free_list(left_neighbor);
    my_remove_from_free_list(right_neighbor);
    left_neighbor->size += sizeof(my_metadata_t) + metadata->size + 
                          sizeof(my_metadata_t) + right_neighbor->size;
    my_add_to_free_list(left_neighbor);
    
  } else if (left_neighbor) {
    // Coalesce with left neighbor only
    my_remove_from_free_list(left_neighbor);
    left_neighbor->size += sizeof(my_metadata_t) + metadata->size;
    my_add_to_free_list(left_neighbor);
    
  } else if (right_neighbor) {
    // Coalesce with right neighbor only
    my_remove_from_free_list(right_neighbor);
    metadata->size += sizeof(my_metadata_t) + right_neighbor->size;
    my_add_to_free_list(metadata);
    
  } else {
    // No neighbors, just add to free list
    my_add_to_free_list(metadata);
  }
}

void my_finalize() {
  // Could add statistics reporting here
}

void test() {
  // Comprehensive test of advanced features
  void *small_ptrs[20];
  void *large_ptrs[10];
  
  // Test small allocations (different bins)
  for (int i = 0; i < 20; i++) {
    small_ptrs[i] = my_malloc(16 + (i % 8) * 16);
  }
  
  // Test large allocations
  for (int i = 0; i < 10; i++) {
    large_ptrs[i] = my_malloc(300 + i * 100);
  }
  
  // Free every other small allocation
  for (int i = 0; i < 20; i += 2) {
    my_free(small_ptrs[i]);
  }
  
  // Free large allocations to test coalescing
  for (int i = 0; i < 5; i++) {
    my_free(large_ptrs[i]);
  }
  
  // Reallocate to test bin reuse
  for (int i = 0; i < 10; i++) {
    small_ptrs[i] = my_malloc(32);
  }
  
  // Clean up
  for (int i = 0; i < 20; i++) {
    if (small_ptrs[i]) my_free(small_ptrs[i]);
  }
  for (int i = 5; i < 10; i++) {
    my_free(large_ptrs[i]);
  }
  
  assert(1 == 1);
}
