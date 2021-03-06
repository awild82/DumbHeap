#ifndef __MEMMANAGER_INCLUDE_DEFINED__
#define __MEMMANAGER_INCLUDE_DEFINED__

#define MEMMANAGER_DEBUG

#ifdef MEMMANAGER_DEBUG
#include <iostream>
#endif

#include <new>
#include <utility>
#include <unordered_map>

class MemManager {
  
  void * top;
  size_t align_size;
  std::unordered_map<void*,size_t> alloc_blocks;

  static inline void * & get_next(void * ptr) {
    return *(static_cast<void **>(ptr));
  };

  static inline size_t & get_size(void * ptr) {
    return reinterpret_cast<size_t&>(*(static_cast<void **>(ptr) + 1));
  };

  inline void check_alignment(void * ptr) {
    if (reinterpret_cast<size_t>(ptr) % align_size != 0)
      throw "Bad alignment of pointer in MemManager";
  };

  void * get_prev(void * ptr) {

    // Handle edge case that all blocks are after ptr
    if ( top > ptr )
      return nullptr;

    void * ret = top;

    while ( get_next(ret) ) {
      if ( get_next(ret) > ptr ) 
        break;
      ret = get_next(ret);
    }

    return ret;
  };


  public:

  MemManager(size_t align_size_ = sizeof(size_t)) :
    top(nullptr), align_size(align_size_) { };

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


  // free is a wrapper for add_block that checks if ptr is in the alloc_blocks 
  void free(void * ptr);

  // free_fast is a wrapper for add_block_fast and checks if ptr is in alloc_blocs
  void free_fast(void * ptr);


  // defrag sorts and remerges the free list so it is ordered and contains no
  //   adjacent blocks
  void defrag();

#ifdef MEMMANAGER_DEBUG
  void print_free() {
    void * ptr(top);

    std::cout << "                 MemManager Free List  \n";
    std::cout << "---------------------------------------------------------\n";
    while (ptr) {
      std::cout << ptr << " | " << get_size(ptr) << '\n';
      ptr = get_next(ptr);
    }
    std::cout << "---------------------------------------------------------\n";
  };
#endif

}; // MemManager


void MemManager::add_block_fast(void * block, size_t block_size) {
  // block_size is in bytes
  if ( block_size == 0 )
    return;
  if ( block_size < sizeof(void*) + sizeof(size_t) )
    throw "Blocks need to be large enough to hold the linked list header";
  check_alignment(block);

  get_next(block) = top;
  get_size(block) = block_size;
  top = block;
};

void MemManager::add_block(void * block, size_t block_size) {
  // block_size is in bytes
  if ( block_size == 0 )
    return;
  if ( block_size < sizeof(void*) + sizeof(size_t) )
    throw "Blocks need to be large enough to hold the linked list header";
  check_alignment(block);

  // Handle edge case with empty free list
  if ( !top ) {
    get_next(block) = top;
    get_size(block) = block_size;
    top = block;
    return;
  }

  void * prev = get_prev(block);
  void * next = prev ? get_next(prev) : top;

  // Append or link next to block
  if ( static_cast<char*>(block) + block_size == next ) {
    get_next(block) = get_next(next);
    get_size(block) = block_size + get_size(next);
  }
  else {
    get_next(block) = next;
    get_size(block) = block_size;
  }

  // Append or link block to prev
  if ( !prev ) {
    top = block;
  }
  else if ( static_cast<char*>(prev) + get_size(prev) == block ) {
    get_next(prev) = get_next(block);
    get_size(prev) += get_size(block);
  }
  else {
    get_next(prev) = block;
  }

};


template <typename T>
T* MemManager::malloc(size_t N) {
  size_t mod_size = N * sizeof(T) % align_size;
  size_t req_size = mod_size == 0 ?
                    N * sizeof(T) : N*sizeof(T) + align_size - mod_size;

  if ( !top )
    throw std::bad_alloc();

  void* ptr(top);
  void* prev(nullptr);
  size_t block_size(get_size(ptr));
  while ( ptr && block_size < req_size ) {
    prev = ptr;
    ptr = get_next(ptr);
    block_size = ptr ? get_size(ptr) : 0;
  };

  if (!ptr)
    throw std::bad_alloc();

  if (req_size == block_size) {
    if ( prev ){
      get_next(prev) = get_next(ptr);
    }
    else if (ptr) {
      top = get_next(ptr);
    }
    else {
      top = nullptr;
    }
  }
  else {
    void * new_free = static_cast<void*>(static_cast<char*>(ptr) + req_size);
    get_next(new_free) = get_next(ptr);
    get_size(new_free) = get_size(ptr) - req_size;
    if (top == ptr) {
      top = new_free;
    }
    else {
      get_next(prev) = new_free;
    }
  }

  alloc_blocks[ptr] = req_size;

  return static_cast<T*>(ptr);
}; 


void MemManager::free(void * ptr) {
  auto entry = alloc_blocks.find(ptr);

  if (entry == alloc_blocks.end())
    throw "Pointer was not allocated by MemManager";

  add_block(ptr, entry->second);

  alloc_blocks.erase(entry);
};


void MemManager::free_fast(void * ptr) {
  auto entry = alloc_blocks.find(ptr);

  if (entry == alloc_blocks.end())
    throw "Pointer was not allocated by MemManager";

  add_block_fast(ptr, entry->second);

  alloc_blocks.erase(entry);
};

void MemManager::defrag() {
  void * ptr(top);
  void * next;
  top = nullptr;

  while ( ptr ) {
    next = get_next(ptr);
    add_block(ptr, get_size(ptr));
    ptr = next;
  }
};

#endif
