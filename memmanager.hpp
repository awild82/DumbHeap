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
    return static_cast<size_t *>(*(static_cast<void **>(ptr) + 1));
  };

  inline void check_alignment(void * ptr) {
    if (reinterpret_cast<size_t>(ptr) % align_size != 0)
      throw "Bad alignment of pointer in MemManager";
  };

  void * get_prev(void * ptr) {
    void * ret = top;

    while ( get_next(ret) ) {
      if ( get_next(ret) > ptr ) 
        break;
      ret = get_next(ret);
    }

    return ret;
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


template <typename AlignType>
void MemManager<AlignType>::add_block_fast(void * block, size_t block_size) {
  if ( block_size == 0 )
    return;

  check_alignment(block);
  get_next(block) = top;
  *get_size(block) = block_size;
  top = block;
};

template <typename AlignType>
void MemManager<AlignType>::add_block(void * block, size_t block_size) {
  if ( block_size == 0 )
    return;

  check_alignment(block);
  void * prev = get_prev(block);
  void * next = get_next(prev);
  size_t prev_size = *get_size(prev);
  size_t next_size = *get_size(next);

  // Append or link next to block
  if ( static_cast<char*>(block) + block_size == next ) {
    get_next(block) = get_next(next);
    *get_size(block) = block_size + *get_size(next);
  }
  else {
    get_next(block) = next;
    *get_size(block) = block_size;
  }

  // Append or link block to prev
  if ( static_cast<char*>(prev) + prev_size == block ) {
    *get_size(prev) += *get_size(block);
    get_next(prev) = get_next(block);
  }
  else {
    get_next(prev) = block;
  };

};

#endif
