//
// Right Coalescing malloc implementation
// Merges freed blocks with adjacent free blocks on the right
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
  bool is_free;
} my_metadata_t;

typedef struct my_heap_t {
  my_metadata_t *free_head;
  my_metadata_t dummy;
} my_heap_t;

my_heap_t my_heap;

void my_add_to_free_list(my_metadata_t *metadata) {
  assert(!metadata->next);
  metadata->is_free = true;
  metadata->next = my_heap.free_head;
  my_heap.free_head = metadata;
}

void my_remove_from_free_list(my_metadata_t *metadata, my_metadata_t *prev) {
  if (prev) {
    prev->next = metadata->next;
  } else {
    my_heap.free_head = metadata->next;
  }
  metadata->next = NULL;
  metadata->is_free = false;
}

// Find the metadata that comes immediately after the given block
my_metadata_t *find_right_neighbor(my_metadata_t *metadata) {
  // Calculate where this block ends
  char *block_end = (char *)(metadata + 1) + metadata->size;
  my_metadata_t *potential_neighbor = (my_metadata_t *)block_end;
  
  // Check if this potential neighbor is in our free list
  my_metadata_t *current = my_heap.free_head;
  while (current) {
    if (current == potential_neighbor && current->is_free) {
      return current;
    }
    current = current->next;
  }
  return NULL;
}

void my_initialize() {
  my_heap.free_head = &my_heap.dummy;
  my_heap.dummy.size = 0;
  my_heap.dummy.next = NULL;
  my_heap.dummy.is_free = true;
}

void *my_malloc(size_t size) {
  my_metadata_t *metadata = my_heap.free_head;
  my_metadata_t *prev = NULL;
  
  // First-fit search
  while (metadata && metadata->size < size) {
    prev = metadata;
    metadata = metadata->next;
  }

  if (!metadata) {
    size_t buffer_size = 4096;
    my_metadata_t *new_metadata = (my_metadata_t *)mmap_from_system(buffer_size);
    new_metadata->size = buffer_size - sizeof(my_metadata_t);
    new_metadata->next = NULL;
    new_metadata->is_free = false;
    my_add_to_free_list(new_metadata);
    return my_malloc(size);
  }

  void *ptr = metadata + 1;
  size_t remaining_size = metadata->size - size;
  my_remove_from_free_list(metadata, prev);

  if (remaining_size > sizeof(my_metadata_t)) {
    metadata->size = size;
    my_metadata_t *new_metadata = (my_metadata_t *)((char *)ptr + size);
    new_metadata->size = remaining_size - sizeof(my_metadata_t);
    new_metadata->next = NULL;
    new_metadata->is_free = false;
    my_add_to_free_list(new_metadata);
  }
  return ptr;
}

void my_free(void *ptr) {
  my_metadata_t *metadata = (my_metadata_t *)ptr - 1;
  
  // Look for right neighbor to coalesce
  my_metadata_t *right_neighbor = find_right_neighbor(metadata);
  
  if (right_neighbor) {
    // Coalesce with right neighbor
    // Remove right neighbor from free list first
    my_metadata_t *current = my_heap.free_head;
    my_metadata_t *prev = NULL;
    
    while (current && current != right_neighbor) {
      prev = current;
      current = current->next;
    }
    
    if (current == right_neighbor) {
      my_remove_from_free_list(right_neighbor, prev);
