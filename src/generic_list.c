#include "generic_list.h"

#include <assert.h>

struct bg_list_item {
  void *n;
  struct bg_list_item *prev;
  struct bg_list_item *next;
};

struct bg_list_t {
  bg_list_item *first;
  bg_list_item *last;
  size_t size;
};



void bg_list_init(bg_list_t **list) {
  *list = (bg_list_t*)calloc(1, sizeof(bg_list_t));
  (*list)->first = (*list)->last = NULL;
  (*list)->size = 0;
}

void bg_list_deinit(bg_list_t *list) {
  bg_list_clear(list);
  free(list);
}

void bg_list_clear(bg_list_t *list) {
  bg_list_item *tmp, *current = list->first;
  while(current) {
    tmp = current;
    current = current->next;
    free(tmp);
  }
  list->size = 0;
}

void bg_list_copy(bg_list_t *dest_list, const bg_list_t *src_list) {
  bg_list_iterator_t it;
  void *current;
  bg_list_clear(dest_list);
  for(current = bg_list_first((bg_list_t*)src_list, &it);
      current != NULL; current = bg_list_next(&it)) {
    bg_list_append(dest_list, current);
  }
}

void bg_list_append(bg_list_t *list, void *obj) {
  bg_list_item *item = calloc(1, sizeof(bg_list_item));
  item->n = obj;
  if(list->size) {
    item->prev = list->last;
    list->last->next = item;
    list->last = item;
  } else {
    list->first = list->last = item;
  }
  list->size++;
}

void bg_list_insert(bg_list_iterator_t *it, void *obj) {
  bg_list_item *prev=NULL, *next=NULL;
  bg_list_item *new_item = calloc(1, sizeof(bg_list_item));
  new_item->n = obj;
  next = it->item;
  if(it->item) {
    prev = it->item->prev;
  } else {
    prev = it->list->last;
  }
  new_item->prev = prev;
  if(prev) {
    prev->next = new_item;
  } else {
    it->list->first = new_item;
  }
  new_item->next = next;
  if(next) {
    next->prev = new_item;
  } else {
    it->list->last = new_item;
  }
  it->list->size++;
  it->item = new_item;
}

void* bg_list_erase(bg_list_iterator_t *it) {
  bg_list_item *prev, *next;
  void *ret = NULL;
  prev = it->item->prev;
  next = it->item->next;
  if(prev) {
    prev->next = next;
  } else {
    it->list->first = next;
  }
  if(next) {
    next->prev = prev;
  } else {
    it->list->last = prev;
  }
  it->list->size--;
  free(it->item);
  it->item = next;
  if(it->item) {
    ret = it->item->n;
  }
  return ret;
}

void* bg_list_first(bg_list_t *list, bg_list_iterator_t *it) {
  void *ret = NULL;
  it->list = list;
  it->item = list->first;
  if(it->item) {
    ret = it->item->n;
  }
  return ret;
}

void* bg_list_next(bg_list_iterator_t *it) {
  void *ret = NULL;
  if(it->item) {
    it->item = it->item->next;
  }
  if(it->item) {
    ret = it->item->n;
  }
  return ret;
}

void* bg_list_last(bg_list_t *list, bg_list_iterator_t *it) {
  void *ret = NULL;
  it->list = list;
  it->item = list->last;
  if(it->item) {
    ret = it->item->n;
  }
  return ret;
}

void* bg_list_prev(bg_list_iterator_t *it) {
  void *ret = NULL;
  if(it->item) {
    it->item = it->item->prev;
  }
  if(it->item) {
    ret = it->item->n;
  }
  return ret;
}

bool bg_list_find(bg_list_t *list, const void *obj, bg_list_iterator_t *it) {
  void *current = bg_list_first(list, it);
  while(current && current != obj) {
    current = bg_list_next(it);
  }
  return current != NULL;
}

void* bg_list_get_element(bg_list_iterator_t *it) {
  void *ret = NULL;
  if(it->item) {
    ret = it->item->n;
  }
  return ret;
}

size_t bg_list_size(const bg_list_t *list) {
  return list->size;
}

bool bg_list_empty(const bg_list_t *list) {
  return list->size == 0;
}

bool bg_list_contains(bg_list_t *list, const void *obj) {
  bg_list_iterator_t it;
  return bg_list_find(list, obj, &it);
}


