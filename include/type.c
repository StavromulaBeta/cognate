#ifndef TYPE_C
#define TYPE_C

#include "cognate.h"
#include "types.h"

static cognate_object check_type(char, cognate_object);
static const char* lookup_type(cognate_type);
static _Bool compare_objects(cognate_object, cognate_object);
static _Bool compare_lists(cognate_list, cognate_list);
static _Bool compare_tables(cognate_table, cognate_table);

#include "error.c"
#include "table.c"
#include <string.h>

static cognate_object check_type(char expected_type, cognate_object object)
{
  if likely(object.type & expected_type)
    return object;
  // TODO: Print the object itself here.
  throw_error("Type Error! Expected type '%s' but recieved type '%s'", lookup_type(expected_type), lookup_type(object.type));
}

static const char* lookup_type(cognate_type type)
{
  //const char* const types = "NOTHING\0Block\0\0\0Boolean\0String\0\0Number\0\0List\0\0\0\0Table\0";
  //return types+(type << 3);
  switch(type)
  {
    case NOTHING: return "NOTHING";
    case block  : return "Block";
    case boolean: return "Boolean";
    case string : return "String";
    case number : return "Number";
    case list   : return "List";
    case table  : return "Table";
  }
}

static _Bool compare_lists(cognate_list lst1, cognate_list lst2)
{
  ptrdiff_t len = lst1.top - lst1.start;
  if (len != lst2.top - lst2.start) return 0; // Not equal if differing length.
  while (len --> 0)
  {
    if (!compare_objects(lst1.start[len], lst2.start[len])) // Compare each list object.
    {
      return 0;
    }
  }
  return 1;
}

static _Bool compare_tables(const cognate_table tab1, const cognate_table tab2)
{
  // We can't just use compare_lists, since tables do not have guaranteed order.
  const long table_size = tab1.items.top - tab1.items.start;
  if (table_size != tab2.items.top - tab2.items.start) return 0; // If tables are different sizes, they're probably different.
  for (long i = 0; i < table_size; ++i)
  {
    // Iterate over each key in tab1, finding the corresponding one in tab2
    if (tab1.items.start[i].type == NOTHING) continue;
    if (!compare_objects(tab1.items.start[i], table_get_hash(tab1.confirmation_hash[i], tab2))) return 0;
  }
  return 1;
}

static _Bool compare_objects(cognate_object ob1, cognate_object ob2)
{
  if (ob1.type != ob2.type)
  {
    return 0; // Not equal if differing types. None of this javascript rubbish.
  }
  switch (ob1.type)
  {
    case number:  return ob1.number  == ob2.number;
    case boolean: return ob1.boolean == ob2.boolean;
    case string:  return strcoll(ob1.string, ob2.string) == 0;
    case list:    return compare_lists(*ob1.list, *ob2.list);
    case table:   return compare_tables(*ob1.table, *ob2.table);
    case block:   throw_error("Cannot compare blocks!");
    case NOTHING: throw_error("Cognate should not be in this state - compiler bug!");
  }
}

static size_t mbstrlen(const char* str)
{
  // Get the number of characters in a multibyte string.
  // Normal strlen() gets number of bytes for some reason.
  size_t i = 0;
  for (; *str != '\0'; str += mblen(str, MB_CUR_MAX), ++i);
  return i;
}

/*
static size_t fast_strlen(const char* str)
{
  // It's only O(n) once.
  static const char* buf = NULL;
  static size_t len = 0;
  if (str == buf) return len;
  return (len = strlen((buf = str)));
}
*/

#endif
