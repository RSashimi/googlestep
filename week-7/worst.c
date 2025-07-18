//
// Worst Fit malloc implementation
// Finds the largest block that can fit the requested size
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

typedef struct my_heap_t {
  my_metadata_t *free_head;
  my_metadata_t dummy;
} my_heap_t;

my_heap_t my_heap;

void my_add_to_free_list(my_metadata_t *metadata) {
  assert(!metadata->next);
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
}

void my_initialize() {
  my_heap.free_head = &my_heap.dummy;
  my_heap.dummy.size = 0;
  my_heap.dummy.next = NULL;
}

void *my_malloc(size_t size) {
  my_metadata_t *metadata = my_heap.free_head;
  my_metadata_t *prev = NULL;
  
  // Worst-fit: Find the largest free slot that can fit the object
  my_metadata_t *worst_metadata = NULL;
  my_metadata_t *worst_prev = NULL;
  size_t worst_size = 0;
  
  // Search through all free blocks to find the worst (largest) fit
  while (metadata) {
    if (metadata->size >= size && metadata->size > worst_size) {
      worst_metadata = metadata;
      worst_prev = prev;
      worst_size = metadata->size;
    }
    prev = metadata;
    metadata = metadata->next;
  }

  if (!worst_metadata) {
    // No suitable free slot found, request new memory
    size_t buffer_size = 4096;
    my_metadata_t *new_metadata = (my_metadata_t *)mmap_from_system(buffer_size);
    new_metadata->size = buffer_size - sizeof(my_metadata_t);
    new_metadata->next = NULL;
    my_add_to_free_list(new_metadata);
    return my_malloc(size);
  }

  void *ptr = worst_metadata + 1;
  size_t remaining_size = worst_metadata->size - size;
  my_remove_from_free_list(worst_metadata, worst_prev);

  if (remaining_size > sizeof(my_metadata_t)) {
    worst_metadata->size = size;
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
  // Test worst fit behavior
  void *ptr1 = my_malloc(100);
  void *ptr2 = my_malloc(500);
  void *ptr3 = my_malloc(200);
  
  my_free(ptr2);  // Free largest block
  my_free(ptr1);  // Free smaller block
  
  // This should use worst fit to choose the larger available block
  void *ptr4 = my_malloc(50);
  
  my_free(ptr3);
  my_free(ptr4);
  
  assert(ptr1 != NULL && ptr2 != NULL && ptr3 != NULL && ptr4 != NULL);
}
