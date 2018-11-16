#ifndef __MEMMANAGER_INCLUDE_DEFINED__
#define __MEMMANAGER_INCLUDE_DEFINED__

#include <vector>
#include <utility>

template <typename AlignType>
class MemManager {
  
  void * top;
  size_t align_size;
  std::vector<std::pair<void*,void*>> block_bounds;

  static inline void * get_next(void * ptr) {
    return *(static_cast<void **>(ptr));
  };

  static inline size_t * get_size(void * ptr) {
    return static_cast<size_t *>(static_cast<void **>(ptr) + 1);
  };

  static inline void check_alignment(void * ptr) {
    if (ptr % align_size != 0) throw "Bad alignment of pointer in MemManager";
  };

  public:

  MemManager() : top(nullptr), align_size(sizeof(AlignType)) { };

  // add_block searches the free list for the appropriate location to store
  //   this block to maintain order. Merges blocks together if they are
  //   contiguous
  void add_block(void * block, size_t block_size);

  // add_block_fast adds the block to the top of the free list. Can cause
  //   fragmentation
  void add_block_fast(void * block, size_t block_size);


  // malloc searches the free list for a block with the size required to hold
  //   N objects of type T. 
  template <typename T>
  T * malloc(size_t N);


  // free is a wrapper for add_block that checks if ptr is in the bounds of any
  //   added blocks
  template <typename T>
  void free(T * ptr);

  // free_fast is a wrapper for add_block_fast
  template <typename T>
  void free_fast(T * ptr);


  // defrag sorts and remerges the free list so it is ordered and contains no
  //   adjacent blocks
  void defrag();

};

#endif
