#ifndef C_BAGEL_IMPL_H
#define C_BAGEL_IMPL_H

#if defined(_WIN32) || defined(WIN32)
  #define WIN32
#endif

#include "bagel.h"
#include <stdio.h> /* for debugging */

#ifdef INTERVAL_SUPPORT
#  include <mpfi.h>
#  include <mpfi_io.h>
#else
#  ifndef mpfi_t
typedef void* mpfi_t;
#  endif
#endif

#define bg_MAX_EDGES 256
#define bg_MAX_PORTS 512

typedef struct input_port_t input_port_t;
typedef struct output_port_t output_port_t;
typedef struct node_type_t node_type_t;
typedef struct merge_type_t merge_type_t;
typedef struct bg_node_t bg_node_t;

struct bg_list_t;
struct bg_list_t;


int bg_min(int a, int b);

struct input_port_t {
  const char *name;
  bg_real defaultValue;
  bg_real bias;
  bg_real value;
  mpfi_t value_intv;
  merge_type_t *merge;
  bg_edge_t *edges[bg_MAX_EDGES];
  size_t num_edges;
};

struct output_port_t {
  const char *name;
  bg_real value;
  mpfi_t value_intv;
  bg_edge_t *edges[bg_MAX_EDGES];
  size_t num_edges;
};

struct node_type_t {
  bg_node_type id;
  const char *name;
  size_t input_port_cnt;
  size_t output_port_cnt;
  bg_error (*init)(bg_node_t *n);
  bg_error (*deinit)(bg_node_t *n);
  bg_error (*eval)(bg_node_t *n);
  bg_error (*eval_intv)(bg_node_t *n);
};

struct merge_type_t {
  bg_merge_type id;
  const char *name;
  bg_error (*merge)(struct input_port_t *input_port);
  bg_error (*merge_intv)(struct input_port_t *input_port);
};

struct bg_graph_t {
  const char *name;
  const char *load_path;
  size_t input_port_cnt;
  size_t output_port_cnt;
  input_port_t *input_ports[bg_MAX_PORTS];
  output_port_t *output_ports[bg_MAX_PORTS];
  struct bg_list_t *edge_list;
  struct bg_list_t *evaluation_order;
  struct bg_list_t *input_nodes;
  struct bg_list_t *output_nodes;
  struct bg_list_t *hidden_nodes;
  bool eval_order_is_dirty;
  unsigned long next_id;
  unsigned long id;
};

struct bg_node_t {
  const char *name;
  size_t input_port_cnt;
  size_t output_port_cnt;
  input_port_t **input_ports;
  output_port_t **output_ports;
  node_type_t *type;
  void *_priv_data;
  bg_graph_t *_parent_graph;
  bg_node_id_t id;
};

struct bg_edge_t {
  bg_node_t *source_node;
  size_t source_port_idx;
  bg_node_t *sink_node;
  size_t sink_port_idx;
  bg_real weight;
  bg_real value;
  mpfi_t value_intv;
  bg_edge_id_t id;
  unsigned long ignore_for_sort;
};


void bg_node_type_register(node_type_t *types);
void bg_extern_node_type_register(node_type_t *types);
void bg_merge_type_register(merge_type_t *types);
bg_error bg_error_set(bg_error err);


node_type_t *node_types[bg_NUM_OF_NODE_TYPES];
merge_type_t *merge_types[bg_NUM_OF_MERGE_TYPES];

node_type_t **extern_node_types;
int num_extern_node_types;

static const bg_real bg_EPSILON = 1e-6;

#endif /* C_BAGEL_IMPL_H */
