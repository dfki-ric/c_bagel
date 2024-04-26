#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct successor
{
  unsigned long item;
  unsigned long next;
};


struct item
{
  unsigned long id;
  unsigned long top;
  unsigned long checked;
  unsigned long visited;
  unsigned long num_incoming;
};

#define BUFFER_STEPS 100

struct item *node_list = NULL;
unsigned long nodes_buffer_size, next_node_index;

static struct successor *succ_list = NULL;
unsigned long succ_buffer_size, next_succ_index;

unsigned long *ids = NULL;

void init(void) {
  nodes_buffer_size = BUFFER_STEPS;
  node_list = (struct item*)malloc(sizeof(struct item)*nodes_buffer_size);
  next_node_index = 0;
  succ_buffer_size = BUFFER_STEPS;
  succ_list = (struct successor*)malloc(sizeof(struct successor)*succ_buffer_size);
  next_succ_index = 0;
}

void deinit(void) {
  free(node_list);
  node_list = NULL;
  free(succ_list);
  succ_list = NULL;
}


/* return index of existing or newly node */

unsigned long search_item (unsigned long id) {
  unsigned long i;
  unsigned long new_node_index;

  new_node_index = next_node_index;

  for(i=0; i<next_node_index; ++i) {
    if(node_list[i].id == id) {
      return i;
    }
  }

  if(new_node_index == nodes_buffer_size) {
    /* reallocate memory */
    void *buffer;
    buffer = malloc(sizeof(struct item)*(nodes_buffer_size+BUFFER_STEPS));
    memcpy(buffer, node_list, sizeof(struct item)*nodes_buffer_size);
    free(node_list);
    node_list = buffer;
    nodes_buffer_size += BUFFER_STEPS;
  }

  /* add new node */
  node_list[new_node_index].id = id;
  node_list[new_node_index].top = 0;
  node_list[new_node_index].visited = 0;
  node_list[new_node_index].checked = 0;
  node_list[new_node_index].num_incoming = 0;
  ++next_node_index;
  return next_node_index-1;
}


void add_relation(unsigned long id1, unsigned long id2) {
  unsigned long j;
  unsigned long k;

  /* don't add relation if id1 == id2 */
  if(id1 == id2) {
    return;
  }

  /* init tsort if not already done */
  if(node_list == NULL) {
    init();
  }

  j = search_item(id1);
  k = search_item(id2);

  /* reallocate memory if required */
  if(++next_succ_index == succ_buffer_size) {
    void *buffer;
    buffer = malloc(sizeof(struct successor)*(succ_buffer_size+BUFFER_STEPS));
    memcpy(buffer, succ_list, sizeof(struct successor)*succ_buffer_size);
    free(succ_list);
    succ_list = buffer;
    succ_buffer_size += BUFFER_STEPS;
  }

  /* add succ to j */
  succ_list[next_succ_index].item = k;
  succ_list[next_succ_index].next = node_list[j].top;
  node_list[j].top = next_succ_index;

  /* increment incoming count for k */
  node_list[k].num_incoming++;
}


void detect_loops_for_node(struct item *item) {
  unsigned long sid;
  unsigned long *last_link;
  struct successor *s;

  /* set checked */
  item->visited = 1;
  item->checked = 1;

  /* loop over successors */
  sid = item->top;
  last_link = &(item->top);
  while(sid != 0) {
    s = succ_list+sid;
    if(node_list[s->item].checked) {
      /* we have a loop */

      /* remove successor */
      *last_link = s->next;

      /* decrease num incomming for target node */
      node_list[s->item].num_incoming--;

      /* for debugging purpose */
      /*fprintf(stderr, "remove relation: %lu -> %lu\n", item->id, node_list[s->item].id);*/
    }
    else if(node_list[s->item].visited == 0) {
      /* this node wasn't checked before */
      detect_loops_for_node(node_list + s->item);
    }
    /* else it is known that the node doesn't produce a loop */
    last_link = &(s->next);
    sid = s->next;
  }
  item->checked = 0;
}

void detect_loops(void) {
  unsigned long i;

  for(i=0; i<next_node_index; ++i) {
    if(node_list[i].visited == 0) {
      detect_loops_for_node(node_list+i);
    }
  }
}

void debug_graph(void) {
  unsigned long i, sid;

  for(i=0; i<next_node_index; ++i) {
    fprintf(stderr, "node: %lu (%lu)\n", node_list[i].id, node_list[i].num_incoming);
    sid = node_list[i].top;
    while(sid!=0) {
      fprintf(stderr, "    -> %lu\n", node_list[succ_list[sid].item].id);
      sid = succ_list[sid].next;
    }
  }
}

int tsort (void) {
  unsigned long next_id = 0;
  unsigned long *buffer1, *buffer2;
  unsigned long *src_vertices, *cpy_vertices;
  unsigned long num_src_vertices, num_cpy_vertices;
  unsigned long sid, i;

  /* detect loops */
  detect_loops();

  /* allocate memory for return ids */
  if(ids) {
    free(ids);
  }
  ids = (unsigned long*)malloc(sizeof(unsigned long)*(next_node_index+1));
  ids[next_node_index] = 0;

  /* allocate memory for fast sort */
  buffer1 = (unsigned long*)malloc(sizeof(unsigned long)*next_node_index);
  buffer2 = (unsigned long*)malloc(sizeof(unsigned long)*next_node_index);

  /* init buffers */
  num_src_vertices = 0;
  for(i=0; i<next_node_index; ++i) {
    if(node_list[i].num_incoming == 0) {
      /* add to id list */
      ids[next_id++] = node_list[i].id;
      /* decrease incoming of successors */
      sid = node_list[i].top;
      while(sid != 0) {
        node_list[succ_list[sid].item].num_incoming--;
        sid = succ_list[sid].next;
      }
    }
    else {
      buffer1[num_src_vertices++] = i;
    }
  }

  src_vertices = buffer1;
  num_cpy_vertices = 0;
  cpy_vertices = buffer2;

  /* loop over nodes and sort them into ids */
  while(num_src_vertices > 0) {
    for(i=0; i<num_src_vertices; ++i) {
      if(node_list[src_vertices[i]].num_incoming == 0) {
        /* add to id list */
        ids[next_id++] = node_list[src_vertices[i]].id;
        /* decrease incoming of successors */
        sid = node_list[src_vertices[i]].top;
        while(sid != 0) {
          node_list[succ_list[sid].item].num_incoming--;
          sid = succ_list[sid].next;
        }
      }
      else {
        cpy_vertices[num_cpy_vertices++] = src_vertices[i];
      }
    }
    if(src_vertices == buffer1) {
      src_vertices = buffer2;
      cpy_vertices = buffer1;
    }
    else {
      src_vertices = buffer1;
      cpy_vertices = buffer2;
    }
    num_src_vertices = num_cpy_vertices;
    num_cpy_vertices = 0;
  }

  /* free memory */
  free(buffer1);
  free(buffer2);
  deinit();
  return 1;
}

unsigned long* get_sorted_ids(void) {
  return ids;
}
