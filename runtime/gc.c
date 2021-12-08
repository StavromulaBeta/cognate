#include <setjmp.h>
#include <stdint.h>
#include <string.h>
#include <signal.h>
#include <sys/mman.h>
#include <limits.h>
#include "runtime.h"

#define PAGE_SIZE 4096

#define MAP_SIZE 0x10000000000

#define BITMAP_EMPTY 0x0
#define BITMAP_FREE  0x1
#define BITMAP_ALLOC 0x3

#define BITMAP_INDEX(ptr) bitmap[ptr - heap_start]

/*
 * Cognate's Garbage Collector
 *
 * This is a simplistic tracing conservative gc, using a bitmap allocator.
 * It has no dependencies on malloc() or free().
 * Well under 100 SLOC is also ridiculously small for a garbage collector.
 *
 * TODO:
 *  - Make the bitmap only use 2 bits per long like it should, instead of a byte.
 *  - Use vector intrinsics to speed up bitmap checking/modifying.
 *  - Make valgrind shut up.
 */

thread_local static uintptr_t* restrict heap_start;
thread_local static uintptr_t* restrict heap_top;

thread_local static uint8_t* restrict bitmap;
thread_local static uint8_t* restrict free_start;

static void handle_segfault()
{
  mprotect((void*)((uintptr_t)heap_top & ~(PAGE_SIZE-1)), PAGE_SIZE, PROT_READ | PROT_WRITE);
  gc_collect();
}

void gc_init()
{
  bitmap = free_start   = mmap(0, MAP_SIZE/32, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE | MAP_NORESERVE, -1, 0);
  heap_start = heap_top = mmap(0, MAP_SIZE,    PROT_NONE, MAP_ANONYMOUS | MAP_PRIVATE | MAP_NORESERVE, -1, 0);
  if (heap_start == MAP_FAILED || bitmap == MAP_FAILED)
    throw_error("memory map failure - are you trying to use valgrind?");
  signal(SIGSEGV, handle_segfault);
  mprotect(heap_start, PAGE_SIZE, PROT_READ | PROT_WRITE);
  BITMAP_INDEX(heap_start) = BITMAP_FREE;
}

void show_heap_usage()
{
  char state;
  for (uintptr_t* i = heap_start; i < heap_top; ++i)
  {
    switch(BITMAP_INDEX(i))
    {
      case BITMAP_ALLOC: state = '#'; break;
      case BITMAP_FREE:  state = '-'; break;
    }
    putc(state, stdout);
  }
  putc('\n', stdout);
}

__attribute__((malloc, hot, assume_aligned(sizeof(uint64_t)), alloc_size(1), returns_nonnull))
void* gc_malloc(size_t bytes)
{
<<<<<<< HEAD
=======
  thread_local static int byte_count = 0;
  byte_count += bytes;
  if unlikely(byte_count > 1024 * 1024) gc_collect(), byte_count = 0;
>>>>>>> e3c996d (Made the garbage collector thread local)
  const size_t longs = (bytes + 7) / sizeof(uintptr_t);
  free_start = memchr(free_start, BITMAP_FREE, LONG_MAX);
  for (uint8_t* restrict free_end; unlikely(free_end = memchr(free_start + 1, BITMAP_ALLOC, longs - 1)); )
    free_start = memchr(free_end + 1, BITMAP_FREE, LONG_MAX);
  memset(free_start + 1, BITMAP_EMPTY, longs - 1);
  uint8_t* restrict free_end = free_start + longs;
  uintptr_t* buf = heap_start + (free_start - bitmap);
  *free_start = BITMAP_ALLOC;
  if (heap_top < buf + longs) heap_top = buf + longs;
  if (*free_end != BITMAP_ALLOC) *free_end = BITMAP_FREE;
  free_start = free_end;
  return buf;
}

static void gc_collect_root(uintptr_t object)
{
  uintptr_t* restrict ptr = (uintptr_t*)(object & PTR_MASK);
  if ((object & ~PTR_MASK && (object & NAN_MASK) != NAN_MASK)
   || ptr < heap_start || ptr >= heap_top
   || BITMAP_INDEX(ptr) != BITMAP_FREE) return;
  BITMAP_INDEX(ptr) = BITMAP_ALLOC;
  // No need to optimize the bitmap addressing, since it's O(n) anyways.
  for (uintptr_t* p = ptr + 1; BITMAP_INDEX(p) == BITMAP_EMPTY; ++p)
    gc_collect_root(*p);
  gc_collect_root(*ptr);
}

__attribute__((noinline)) void gc_collect()
{
  for (uintptr_t* restrict p = (uintptr_t*)bitmap; (uint8_t*)p < bitmap + (heap_top - heap_start); ++p)
    *p &= 0x5555555555555555;
  __attribute__((unused)) volatile ANY s = (ANY)stack.start;
  jmp_buf a;
  setjmp(a);
  uintptr_t* sp = (uintptr_t*)&sp;
  for (uintptr_t* root = sp + 1; root <= (uintptr_t*)function_stack_start; ++root)
    gc_collect_root(*root);
  free_start = bitmap;
}

char* gc_strdup(char* src)
{
  const size_t len = strlen(src);
  return memcpy(gc_malloc(len + 1), src, len + 1);
}

char* gc_strndup(char* src, size_t bytes)
{
  const size_t len = strlen(src);
  if (len < bytes) bytes = len;
  char* dest = gc_malloc(bytes + 1);
  dest[bytes] = '\0';
  return memcpy(dest, src, bytes);
}
