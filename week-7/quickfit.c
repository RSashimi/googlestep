//
// Quick Fit malloc implementation
// Maintains separate free lists for commonly used sizes for O(1) allocation
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

// Quick fit sizes - most commonly requested sizes
#define NUM_QUICK_SIZES 8
static const size_t quick_sizes[] = {16, 32, 48, 64, 80, 96, 112, 128};

typedef struct my_heap_t {
  my_metadata_t *quick_lists[NUM_QUICK_SIZES];  // Fast lists for common sizes
  my_metadata_t *general_free_head;             // General free list
  my_metadata_t dummy;
} my_heap_t;

my_heap_t my_heap;

// Check if size is a quick fit size
int get_quick_fit_index(size_t size) {
  for (int i = 0; i < NUM_QUICK_SIZES; i++) {
    if (quick_sizes[i] == size) {
      return i;
    }
  }
  return -1;
}

void add_to_quick_list(my_metadata_t *metadata, int index) {
  metadata->next = my_heap.quick_lists[index];
  my_heap.quick_lists[index] = metadata;
}

void add_to_general_list(my_metadata_t *metadata) {
  metadata->next = my_heap.general_free_head;
  my_heap.general_free_head = metadata;
}

my_metadata_t *remove_from_quick_list(int index) {
  my_metadata_t *metadata = my_heap.quick_lists[index];
  if (metadata) {
    my_heap.quick_lists[index] = metadata->next;
    metadata->next = NULL;
  }
  return metadata;
}

my_metadata_t *remove_from_general_list(size_t size) {
  my_metadata_t *metadata = my_heap.general_free_head;
  my_metadata_t *prev = NULL;
  
  while (metadata && metadata->size < size) {
    prev = metadata;
    metadata = metadata->next;
  }
  
  if (metadata) {
    if (prev) {
      prev->next = metadata->next;
    } else {
      my_heap.general_free_head = metadata->next;
    }
    metadata->next = NULL;
  }
  
  return metadata;
}

void my_initialize() {
  for (int i = 0; i < NUM_QUICK_SIZES; i++) {
    my_heap.quick_lists[i] = NULL;
  }
  my_heap.general_free_head = &my_heap.dummy;
  my_heap.dummy.size = 0;
  my_heap.dummy.next = NULL;
}

void *my_malloc(size_t size) {
  my_metadata_t *metadata = NULL;
  
  // Check quick fit first
  int quick_index = get_quick_fit_index(size);
  if (quick_index != -1) {
    metadata = remove_from_quick_list(quick_index);
  }
  
  // If not found in quick fit, check general list
  if (!metadata) {
    metadata = remove_from_general_list(size);
  }

  if (!metadata) {
    // No suitable free slot found, request new memory
    size_t buffer_size = 4096;
    my_metadata_t *new_metadata = (my_metadata_t *)mmap_from_system(buffer_size);
    new_metadata->size = buffer_size - sizeof(my_metadata_t);
    new_metadata->next = NULL;
    add_to_general_list(new_metadata);
    return my_malloc(size);
  }

  void *ptr = metadata + 1;
  size_t remaining_size = metadata->size - size;

  if (remaining_size > sizeof(my_metadata_t)) {
    metadata->size = size;
    my_metadata_t *new_metadata = (my_metadata_t *)((char *)ptr + size);
    new_metadata->size = remaining_size - sizeof(my_metadata_t);
    new_metadata->next = NULL;
    
    // Add remainder to appropriate list
    int remainder_quick_index = get_quick_fit
