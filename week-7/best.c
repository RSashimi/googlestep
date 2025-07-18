//
// Best Fit malloc implementation
// Finds the smallest block that can fit the requested size
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
  
  // Best-fit: Find the smallest free slot that can fit the object
  my_metadata_t *best_metadata = NULL;
  my_metadata_t *best_prev = NULL;
  size_t best_size = SIZE_MAX;
  
  // Search through all free blocks to find the best fit
  while (metadata) {
    if (metadata->size >= size && metadata->size < best_size) {
      best_metadata = metadata;
      best_prev = prev;
      best_size = metadata->size;
      
      // Perfect fit found, no need to search further
      if (metadata->size == size) {
        break;
      }
    }
    prev = metadata;
    metadata = metadata->next;
  }

  if (!best_metadata) {
    // No suitable free slot found, request new memory
    size_t buffer_size = 4096;
    my_metadata_t *new_metadata = (my_metadata_t *)mmap_from_system(buffer_size);
    new_metadata->size = buffer_size - sizeof(my_metadata_t);
    new_metadata->next = NULL;
    my_add_to_free_list(new_metadata);
    return my_malloc(size);
  }

  void *ptr = best_metadata + 1;
  size_t remaining_size = best_metadata->size - size;
  my_remove_from_free_list(best_metadata, best_prev);

  if (remaining_size > sizeof(my_metadata_t)) {
    best_metadata->size = size;
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
  // Test basic allocation and freeing
  void *ptr1 = my_malloc(100);
  void *ptr2 = my_malloc(200);
  void *ptr3 = my_malloc(50);
  
  my_free(ptr2);  // Free middle block
  
  // This should use best fit to find the most suitable block
  void *ptr4 = my_malloc(150);
  
  my_free(ptr1);
  my_free(ptr3);
  my_free(ptr4);
  
  assert(ptr1 != NULL && ptr2 != NULL && ptr3 != NULL && ptr4 != NULL);
}
