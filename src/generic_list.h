#ifndef C_BAGEL_GENERIC_LIST_H
#define C_BAGEL_GENERIC_LIST_H

/** 
 * @file 
 * @brief Doubly linked list.
 *
 * Values are stored as \c void*. You can use the \ref def_list_type macro to 
 * define lists for a specific type.
 */


#include <stdlib.h>
/*#include <stdbool.h>*/
#include "bool.h"

typedef struct bg_list_t bg_list_t;
typedef struct bg_list_item bg_list_item;
typedef struct bg_list_iterator_t {
  bg_list_t *list;
  bg_list_item *item;
} bg_list_iterator_t;

void bg_list_init(bg_list_t **list);
void bg_list_deinit(bg_list_t *list);
void bg_list_clear(bg_list_t *list);
void bg_list_copy(bg_list_t *dest_list, const bg_list_t *src_list);
void bg_list_append(bg_list_t *list, void *obj);
void bg_list_insert(bg_list_iterator_t *it, void *obj);
void* bg_list_erase(bg_list_iterator_t *it);
void* bg_list_first(bg_list_t *list, bg_list_iterator_t *it);
void* bg_list_next(bg_list_iterator_t *it);
void* bg_list_last(bg_list_t *list, bg_list_iterator_t *it);
void* bg_list_prev(bg_list_iterator_t *it);
bool bg_list_find(bg_list_t *list, const void *obj, bg_list_iterator_t *it);
void* bg_list_get_element(bg_list_iterator_t *it);
size_t bg_list_size(const bg_list_t *list);
bool bg_list_empty(const bg_list_t *list);
bool bg_list_contains(bg_list_t *list, const void *obj);

/** 
 * @brief macro to create lists for a specific type.
 * @param typename A short name describing the type.
 * @param basetype The type of the underlying objects.
 *
 * Putting def_list_type(foo, bar) in your code will define a new list type
 * \c bg_foo_list_t and wrappers around the \c bg_list_* functions following 
 * the name convention \c bg_foo_list_*. The accessor functions will take 
 * care to cast from and to type \c bar for you.
 * E.g., there will be a wrapper 
 * \code bar* bg_foo_list_find(bg_foo_list_t *list, const bar *obj);\endcode
 */
#define LIST_TYPE_DEF(typename, basetype)                               \
  typedef struct bg_list_t bg_##typename##_list_t;                      \
  typedef struct bg_list_iterator_t bg_##typename##_list_iterator_t;    \
                                                                        \
  void bg_##typename##_list_init(bg_##typename##_list_t **list);        \
  void bg_##typename##_list_deinit(bg_##typename##_list_t *list);       \
  void bg_##typename##_list_clear(bg_##typename##_list_t *list);        \
  void bg_##typename##_list_copy(bg_##typename##_list_t *dest_list,     \
                                 const bg_##typename##_list_t *src_list); \
  void bg_##typename##_list_append(bg_##typename##_list_t *list,        \
                                   basetype *obj);                      \
  void bg_##typename##_list_insert(bg_##typename##_list_iterator_t *it, \
                                   basetype *obj);                      \
  basetype * bg_##typename##_list_erase(bg_##typename##_list_iterator_t *it); \
  basetype * bg_##typename##_list_first(bg_##typename##_list_t *list,   \
                                        bg_##typename##_list_iterator_t *it); \
  basetype * bg_##typename##_list_next(bg_##typename##_list_iterator_t *it); \
  basetype * bg_##typename##_list_last(bg_##typename##_list_t *list,    \
                                       bg_##typename##_list_iterator_t *it); \
  basetype * bg_##typename##_list_prev(bg_##typename##_list_iterator_t *it); \
  bool bg_##typename##_list_find(bg_##typename##_list_t *list,          \
                                 const basetype *obj,                   \
                                 bg_##typename##_list_iterator_t *it);  \
  basetype * bg_##typename##_list_get_element(bg_##typename##_list_iterator_t *it); \
  size_t bg_##typename##_list_size(const bg_##typename##_list_t *list); \
  bool bg_##typename##_list_empty(const bg_##typename##_list_t *list);  \
  bool bg_##typename##_list_contains(bg_##typename##_list_t *list,      \
                                     const basetype *obj);              \


#define LIST_TYPE_IMPL(typename, basetype)                              \
                                                                        \
  void bg_##typename##_list_init(bg_##typename##_list_t **list)         \
  { bg_list_init(list); }                                               \
                                                                        \
  void bg_##typename##_list_deinit(bg_##typename##_list_t *list)        \
  { bg_list_deinit(list); }                                             \
                                                                        \
  void bg_##typename##_list_clear(bg_##typename##_list_t *list)         \
  { bg_list_clear(list); }                                              \
                                                                        \
  void bg_##typename##_list_copy(bg_##typename##_list_t *dest_list,     \
                                 const bg_##typename##_list_t *src_list) \
  { bg_list_copy(dest_list, src_list); }                                \
                                                                        \
  void bg_##typename##_list_append(bg_##typename##_list_t *list,        \
                                   basetype *obj)                       \
  { bg_list_append(list, (void*)obj); }                                 \
                                                                        \
  void bg_##typename##_list_insert(bg_##typename##_list_iterator_t *it, \
                                   basetype *obj)                       \
  { bg_list_insert(it, obj); }                                          \
                                                                        \
  basetype * bg_##typename##_list_erase(bg_##typename##_list_iterator_t *it) \
  { return bg_list_erase(it); }                                         \
                                                                        \
  basetype * bg_##typename##_list_first(bg_##typename##_list_t *list,   \
                                        bg_##typename##_list_iterator_t *it) \
  { return bg_list_first(list, it); }                                          \
                                                                        \
  basetype * bg_##typename##_list_next(bg_##typename##_list_iterator_t *it) \
  { return bg_list_next(it); }                                          \
                                                                        \
  basetype * bg_##typename##_list_last(bg_##typename##_list_t *list,    \
                                       bg_##typename##_list_iterator_t *it) \
  { return bg_list_last(list, it); }                                    \
                                                                        \
  basetype * bg_##typename##_list_prev(bg_##typename##_list_iterator_t *it) \
  { return bg_list_prev(it); }                                          \
                                                                        \
  bool bg_##typename##_list_find(bg_##typename##_list_t *list,          \
                                 const basetype *obj,                   \
                                 bg_##typename##_list_iterator_t *it)   \
  { return bg_list_find(list, obj, it); }                               \
                                                                        \
  basetype * bg_##typename##_list_get_element(bg_##typename##_list_iterator_t *it) \
  { return bg_list_get_element(it); }                                   \
                                                                        \
  size_t bg_##typename##_list_size(const bg_##typename##_list_t *list)  \
  { return bg_list_size(list); }                                        \
                                                                        \
  bool bg_##typename##_list_empty(const bg_##typename##_list_t *list)   \
  { return bg_list_empty(list); }                                       \
  bool bg_##typename##_list_contains(bg_##typename##_list_t *list,      \
                                     const basetype *obj)               \
  { return bg_list_contains(list, obj); }                               \


#endif /* C_BAGEL_GENERIC_LIST_H */

