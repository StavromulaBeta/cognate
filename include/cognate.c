#ifndef COGNATE_C
#define COGNATE_C

#include "cognate.h"
#include "types.h"

static const char *stack_start;
static __attribute__((unused)) char if_status = 2;

static void run_program();
static cognate_object check_block(cognate_object);
static void copy_blocks();
static void check_call_stack();

#include "table.c"
#include "stack.c"
#include "io.c"
#include "error.c"
#include "type.c"
#include "func.c"

#include <time.h>
#include <gc/gc.h>
#include <sys/resource.h>
#include <libgen.h>
#include <Block.h>
#include <locale.h>

static struct rlimit stack_max;

int main(int argc, char** argv)
{
  // Get return stack limit
  char a;
  stack_start = &a;
  if unlikely(getrlimit(RLIMIT_STACK, &stack_max) == -1)
  {
    throw_error("Cannot get return stack limit!");
  }
  // Set locale for strings.
  if unlikely(setlocale(LC_ALL, "C.UF-8") == NULL)
  {
    throw_error("Cannot set locale!");
  }
  // Init GC
#ifndef noGC
  GC_INIT();
#endif
  // Seed the random number generator properly.
  struct timespec ts;
  if unlikely(timespec_get(&ts, TIME_UTC) == 0)
  {
    throw_error("Cannot get system time!");
  }
  srandom(ts.tv_nsec ^ ts.tv_sec); // TODO make random more random.
  // Load parameters
  params.start = (cognate_object*) cognate_malloc (sizeof(cognate_object) * (argc-1));
  params.top = params.start + argc - 1;
  while (argc --> 1)
  {
    char* str = argv[argc];
    params.start[argc-1] = (cognate_object){.type=string, .string=str};
  }
  // Bind error signals.
  signal(SIGABRT, handle_signal);
  signal(SIGFPE, handle_signal);
  signal(SIGILL, handle_signal);
  signal(SIGINT, handle_signal);
  signal(SIGTERM, handle_signal);
  signal(SIGSEGV, handle_signal); // Will only sometimes work.
  // Generate a stack.
  init_stack();
  // Actually run the program.
  run_program();
  // Clean up.
  if unlikely(stack.items.top != stack.items.start)
  {
    throw_error("Program exiting with non-empty stack of length %ti", stack.items.top - stack.items.start);
  }
}

static cognate_object check_block(cognate_object obj)
{
  if unlikely(obj.type==block) obj.block = Block_copy(obj.block);
  return obj;
}

static void copy_blocks()
{
  while (stack.modified)
  {
    cognate_object* const obj = stack.items.top - stack.modified;
    if unlikely(obj->type==block) obj->block = Block_copy(obj->block); // Copy block to heap.
    --stack.modified;
  }
}

static void check_call_stack()
{
  // Performance here is not great.
  static unsigned short function_calls = 1024;
  if unlikely(!--function_calls)
  {
    function_calls = 1024;
    static long old_stack_size = 0;
    const char stack_end;
    // if (how much stack left < stack change between checks)
    if unlikely(stack_max.rlim_cur + &stack_end - stack_start < stack_start - &stack_end - old_stack_size)
    {
      throw_error("Call stack overflow - too much recursion! (call stack is %ti bytes)", stack_start - &stack_end);
    }
    old_stack_size = stack_start - &stack_end;
  }
}

#endif
