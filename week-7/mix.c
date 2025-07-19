//
// Mixed Optimized malloc implementation
// Combines: Segregated free lists, coalescing, best-fit for large blocks, 
// first-fit for small blocks, and memory pool optimization
// HELLA TOO LONG (and has some serious performance issues, but we'll get to that)
//

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// --- System Memory Interface ---
// These are just declarations. You'd need to implement these, probably using
// sbrk() on older Unix systems or mmap() on modern ones. mmap is generally better.
void *mmap_from_system(size_t size);
void munmap_to_system(void *ptr, size_t size);

// --- Metadata Structure ---
// This struct is placed right before every block of memory we manage.
// It holds all the info we need to know about a chunk of memory.
typedef struct my_metadata_t {
  size_t size;                // How big is the USER'S data block (doesn't include this struct's size).
  struct my_metadata_t *next; // Points to the next FREE block in this bin.
  struct my_metadata_t *prev; // Points to the previous FREE block. Doubly linked makes removal O(1).
  bool is_free;               // A flag to see if the block is currently free.
} my_metadata_t;
// - The 'is_free' flag is somewhat redundant. If a block is in a free list,
//   it's obviously free. If it's not, it's allocated. You could remove this to save a few bytes
//   but it might make debugging or certain operations (like the current find_right_neighbor) easier.
// - For coalescing, a common trick is to add a 'size' field at the END of the block too (a "footer").
//   This lets you find the metadata of the block to your left in O(1) time, which your
//   current implementation desperately needs.

// --- Heap and Bin Configuration ---
// These constants define our binning strategy for segregated free lists.
#define NUM_SMALL_BINS 16       // For small, fast allocations.
#define NUM_LARGE_BINS 4        // For larger, less frequent allocations.
#define SMALL_BIN_MAX_SIZE 256  // Anything this size or smaller goes in a small bin.
#define LARGE_THRESHOLD 1024    // Just a marker, not really used consistently.

// This is the main struct for our heap. It just holds the heads of all our free lists.
typedef struct my_heap_t {
  my_metadata_t *small_bins[NUM_SMALL_BINS]; // Bins for sizes 16, 32, ..., 256.
  my_metadata_t *large_bins[NUM_LARGE_BINS]; // Bins for 257-512, 512-1024, etc.
  my_metadata_t dummy; // Not really used, seems like a leftover idea.
} my_heap_t;

// The one and only global heap instance.
my_heap_t my_heap;


// --- Binning Logic ---
// Purpose: Figures out which small bin a given size belongs to.
// A simple linear mapping. size 1-16 -> bin 0, 17-32 -> bin 1, etc.
int get_small_bin_index(size_t size) {
  if (size <= 16) return 0;
  return (size - 1) / 16;
}

// Purpose: Figures out which large bin a given size belongs to.
// Hardcoded ranges. Not very flexible.
int get_large_bin_index(size_t size) {
  if (size <= 512) return 0;
  if (size <= 1024) return 1;
  if (size <= 2048) return 2;
  return 3; // The "everything else" bin.
}
// - The hardcoded values are brittle. If you change NUM_BINS, you have to rewrite this logic.
//   A more programmatic or logarithmic approach could be more scalable.
// - The mapping could be more granular to reduce internal fragmentation.

// --- Free List Management ---
// Purpose: Adds a free block to the front of the appropriate small bin list (LIFO).
// This is for First-Fit allocation. Quick and dirty.
void add_to_small_bin(my_metadata_t *metadata) {
  int bin_index = get_small_bin_index(metadata->size);
  if (bin_index >= NUM_SMALL_BINS) return; // Should not happen if logic is correct.

  metadata->next = my_heap.small_bins[bin_index];
  metadata->prev = NULL;
  if (my_heap.small_bins[bin_index]) {
    my_heap.small_bins[bin_index]->prev = metadata;
  }
  my_heap.small_bins[bin_index] = metadata;
  metadata->is_free = true;
}

// Purpose: Adds a free block to the correct large bin, but keeps the list SORTED by size.
// This is the key to making the Best-Fit search for large blocks faster.
void add_to_large_bin(my_metadata_t *metadata) {
  int bin_index = get_large_bin_index(metadata->size);

  // Walk the list to find the correct insertion point.
  my_metadata_t *current = my_heap.large_bins[bin_index];
  my_metadata_t *prev = NULL;
  while (current && current->size < metadata->size) {
    prev = current;
    current = current->next;
  }

  // Standard doubly-linked list insertion.
  metadata->next = current;
  metadata->prev = prev;
  if (prev) {
    prev->next = metadata;
  } else {
    my_heap.large_bins[bin_index] = metadata; // It's the new head.
  }
  if (current) {
    current->prev = metadata;
  }
  metadata->is_free = true;
}

// Purpose: A simple dispatcher. Decides whether to call the small or large bin function.
void my_add_to_free_list(my_metadata_t *metadata) {
  if (metadata->size <= SMALL_BIN_MAX_SIZE) {
    add_to_small_bin(metadata);
  } else {
    add_to_large_bin(metadata);
  }
}

// Purpose: Removes a block from whatever free list it's in.
// This is why we use a doubly-linked list. Unlinking is O(1).
void my_remove_from_free_list(my_metadata_t *metadata) {
  if (metadata->prev) {
    // It's not the head of the list.
    metadata->prev->next = metadata->next;
  } else {
    // It IS the head of the list. We need to update the heap's bin pointer.
    if (metadata->size <= SMALL_BIN_MAX_SIZE) {
      int bin_index = get_small_bin_index(metadata->size);
      if (bin_index < NUM_SMALL_BINS) { // Safety check
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
  // Clean up pointers to prevent weird bugs.
  metadata->next = NULL;
  metadata->prev = NULL;
  metadata->is_free = false;
}

// --- Coalescing (Merging Free Blocks) ---
// Purpose: Find the block physically to the LEFT of the given pointer in memory.
// !!! CRITICAL PERFORMANCE BOTTLENECK !!!
my_metadata_t *find_left_neighbor(void *ptr) {
  // This is HORRIBLY inefficient. It iterates through EVERY SINGLE free block in the
  // entire heap just to find the one that happens to end right where `ptr` begins.
  // This turns your free() operation from a potential O(1) into an O(N) disaster,
  // where N is the number of free blocks.
  for (int i = 0; i < NUM_SMALL_BINS; i++) {
    my_metadata_t *current = my_heap.small_bins[i];
    while (current) {
      if (current->is_free) { // Redundant check, they should all be free.
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
// - FIX THIS. The standard solution is to store the size of the block in a "footer"
//   at the very end of the allocated chunk. When freeing `ptr`, you can look at the bytes
//   immediately before `ptr`'s metadata to find the size of the preceding block.
//   Then you can calculate its starting address in O(1) time. No searching needed.

// Purpose: Find the block physically to the RIGHT of the given block in memory.
// This implementation is much better because memory is contiguous.
my_metadata_t *find_right_neighbor(my_metadata_t *metadata) {
  // Calculate the address immediately after our current block ends.
  char *block_end = (char *)(metadata + 1) + metadata->size;
  my_metadata_t *potential_neighbor = (my_metadata_t *)block_end;

  // Now, how do we know this is a valid block and not just random memory?
  // Here, we're checking its 'is_free' flag. risky tho. If that memory
  // belongs to another allocated block, its 'is_free' would be false.
  // could lead to a segfault.
  if (potential_neighbor->is_free) {
    return potential_neighbor;
  }
  return NULL;
}
// - To make this safer, the heap needs to know its boundaries (the start and end
//   addresses of the memory it got from `mmap`). You should check if `potential_neighbor`
//   is within those boundaries before trying to dereference it.

// --- Core Allocator Functions ---
// Purpose: Initialize the heap. Sets all bin heads to NULL.
void my_initialize() {
  for (int i = 0; i < NUM_SMALL_BINS; i++) {
    my_heap.small_bins[i] = NULL;
  }
  for (int i = 0; i < NUM_LARGE_BINS; i++) {
    my_heap.large_bins[i] = NULL;
  }
  my_heap.dummy.size = 0; // The dummy isn't used. Can be removed.
  my_heap.dummy.next = NULL;
  my_heap.dummy.prev = NULL;
  my_heap.dummy.is_free = true;
}

// Purpose: The main allocation function. The heart of the allocator.
void *my_malloc(size_t size) {
  my_metadata_t *metadata = NULL;

  if (size <= SMALL_BIN_MAX_SIZE) {
    // Small allocation: Use First-Fit. It's fast.
    // We search for a block in the "perfect" bin first, then check larger small bins.
    int bin_index = get_small_bin_index(size);
    for (int i = bin_index; i < NUM_SMALL_BINS; i++) {
      if (my_heap.small_bins[i]) {
        metadata = my_heap.small_bins[i]; // Found one!
        break;
      }
    }
  } else {
    // Large allocation: Use Best-Fit. Slower, but reduces fragmentation.
    // Our sorted large-bin lists help speed this up.
    my_metadata_t *best_fit = NULL;
    size_t best_size = SIZE_MAX;

    for (int i = 0; i < NUM_LARGE_BINS; i++) {
      my_metadata_t *current = my_heap.large_bins[i];
      // Since the list is sorted, we only need to find the first block that fits.
      while (current) {
        if (current->size >= size) { // The first one that fits is the best fit in its bin.
            if (current->size < best_size) {
                best_fit = current;
                best_size = current->size;
            }
            break; // Since list is sorted, no need to check further in this bin.
        }
        current = current->next;
      }
    }
    metadata = best_fit;
  }

  if (!metadata) {
    // No suitable free block was found. We must ask the OS for more memory.
    // The buffer gives us extra room to avoid calling mmap too often.
    size_t buffer_size = (size > 2048) ? size + sizeof(my_metadata_t) + 1024 : 4096;
    my_metadata_t *new_metadata = (my_metadata_t *)mmap_from_system(buffer_size);
    new_metadata->size = buffer_size - sizeof(my_metadata_t);
    new_metadata->next = NULL;
    new_metadata->prev = NULL;
    new_metadata->is_free = false; // It's about to be used, but we free it first.

    // Add this new giant block to our free lists.
    my_add_to_free_list(new_metadata);

    // This recursion is a bit weird. It's generally safer to use a loop or a goto
    // to restart the allocation attempt. A deep recursion could blow the stack,
    // though it's very unlikely here.
    return my_malloc(size);
  }

  // We found a block! Now let's prepare it for the user.
  void *ptr = metadata + 1; // The user pointer is AFTER the metadata.
  size_t remaining_size = metadata->size - size;
  my_remove_from_free_list(metadata); // It's no longer free.

  // Block Splitting: If the block is much bigger than requested, split it.
  // This is key to reducing internal fragmentation.
  if (remaining_size > sizeof(my_metadata_t)) {
    metadata->size = size; // Shrink the original block.

    // Create a new metadata for the leftover piece.
    my_metadata_t *new_metadata = (my_metadata_t *)((char *)ptr + size);
    new_metadata->size = remaining_size - sizeof(my_metadata_t);
    new_metadata->next = NULL;
    new_metadata->prev = NULL;
    new_metadata->is_free = false; // Will be set to true by add_to_free_list
    
    // Add the new, smaller leftover block back to the free lists.
    my_add_to_free_list(new_metadata);
  }
  return ptr;
}


// Purpose: Frees a previously allocated block of memory.
void my_free(void *ptr) {
  // Get our metadata header from the user's pointer.
  my_metadata_t *metadata = (my_metadata_t *)ptr - 1;

  // Coalescing logic: Try to merge with adjacent free blocks.
  // This is where the HORRIBLE find_left_neighbor is called.
  my_metadata_t *left_neighbor = find_left_neighbor(metadata);
  my_metadata_t *right_neighbor = find_right_neighbor(metadata);

  if (left_neighbor && right_neighbor) {
    // Case 1: Merge with both left and right.
    my_remove_from_free_list(left_neighbor);
    my_remove_from_free_list(right_neighbor);
    left_neighbor->size += sizeof(my_metadata_t) + metadata->size +
                           sizeof(my_metadata_t) + right_neighbor->size;
    my_add_to_free_list(left_neighbor);

  } else if (left_neighbor) {
    // Case 2: Merge with left only.
    my_remove_from_free_list(left_neighbor);
    left_neighbor->size += sizeof(my_metadata_t) + metadata->size;
    my_add_to_free_list(left_neighbor);

  } else if (right_neighbor) {
    // Case 3: Merge with right only.
    my_remove_from_free_list(right_neighbor);
    metadata->size += sizeof(my_metadata_t) + right_neighbor->size;
    my_add_to_free_list(metadata);

  } else {
    // Case 4: No free neighbors. Just add this block back to the free list.
    my_add_to_free_list(metadata);
  }
}
// - The biggest issue is performance due to find_left_neighbor. Fixing that
//   with footers would make this function vastly more efficient.
// - This implementation never returns memory to the system (munmap is never called).
//   A complete implementation should track large, empty chunks at the end of the heap
//   and return them to the OS to reduce the program's memory footprint.

// Purpose: Cleanup function.
// It's empty, but you could add stats reporting here (e.g., total memory used,
// fragmentation statistics, number of mmap calls, etc.).
void my_finalize() {
  // Could add statistics reporting here
}

// --- Test Function ---
// A basic smoke test to see if the allocator crashes immediately.
void test() {
  void *small_ptrs[20] = {NULL}; // Initialize to NULL to be safe
  void *large_ptrs[10] = {NULL};

  // Test small allocations across different bins.
  for (int i = 0; i < 20; i++) {
    small_ptrs[i] = my_malloc(16 + (i % 8) * 16);
  }

  // Test large allocations.
  for (int i = 0; i < 10; i++) {
    large_ptrs[i] = my_malloc(300 + i * 100);
  }

  // Free some blocks to create holes.
  for (int i = 0; i < 20; i += 2) {
    my_free(small_ptrs[i]);
    small_ptrs[i] = NULL; // Good practice to avoid double-freeing later.
  }

  // Free some large blocks to test coalescing.
  for (int i = 0; i < 5; i++) {
    my_free(large_ptrs[i]);
    large_ptrs[i] = NULL;
  }

  // Reallocate to see if it reuses the freed spots.
  for (int i = 0; i < 10; i++) {
    // This reuses the same array slots, overwriting old pointers.
    small_ptrs[i] = my_malloc(32);
  }

  // Clean up remaining allocations.
  for (int i = 0; i < 20; i++) {
    if (small_ptrs[i]) my_free(small_ptrs[i]);
  }
  for (int i = 0; i < 10; i++) { // You freed 0-4 already, so this double-frees NULL
    if (large_ptrs[i]) my_free(large_ptrs[i]);
  }
  
  // The original test had a bug. It tried to free large_ptrs[0] through [9]
  // after already freeing [0] through [4]. The check `if (large_ptrs[i])`
  // after setting them to NULL fixes this potential double-free.

  assert(1 == 1); // "If it didn't crash, it must be working." - Famous last words.
}
// - This test is okay, but not comprehensive. It doesn't check for:
//   - Correctness of data (write data to a block, free it, reallocate, see if data is gone).
//   - Edge cases like my_malloc(0).
//   - Alignment issues.
//   - Large-scale stress testing to measure performance and fragmentation over time.
