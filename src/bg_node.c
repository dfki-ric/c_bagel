#include "bg_node.h"
#include "bg_impl.h"
#include "bg_graph.h"
#include "node_types/bg_node_subgraph.h"
#include "node_list.h"

#include <stdlib.h>
#include <string.h>

bg_error bg_node_init(bg_node_t *node, const char *name, bg_node_id_t id,
                      bg_node_type type) {
  char *name_copy = malloc(strlen(name)+1);
  strcpy(name_copy, name);
  node->name = name_copy;
  node->id = id;
  node->type = node_types[type];
  return node->type->init(node);
}

bg_error bg_node_reset(bg_node_t *node, bool recursive) {
  size_t i;
  subgraph_data_t *subgraph_data;
  for(i = 0; i < node->input_port_cnt; ++i) {
    node->input_ports[i]->value = 0.;
#ifdef INTERVAL_SUPPORT
    mpfi_set_ui(node->input_ports[i]->value_intv, 0);
#endif
  }
  for(i = 0; i < node->output_port_cnt; ++i) {
    node->output_ports[i]->value = 0.;
#ifdef INTERVAL_SUPPORT
    mpfi_set_ui(node->output_ports[i]->value_intv, 0);
#endif
  }
  if(recursive && node->type->id == bg_NODE_TYPE_SUBGRAPH) {
    subgraph_data = ((subgraph_data_t*)node->_priv_data);
    bg_graph_reset(subgraph_data->subgraph, recursive);
  }
  return bg_SUCCESS;
}

bg_error bg_node_create_input_ports(bg_node_t *node, size_t cnt) {
  size_t i;
  char name[20];
  char *copy_name;
  bg_error err = bg_SUCCESS;
  node->input_port_cnt = cnt;
  node->input_ports = (input_port_t**)calloc(cnt, sizeof(input_port_t*));
  if(node->input_ports) {
    for(i = 0; i < cnt; ++i) {
      node->input_ports[i] = (input_port_t*)calloc(1, sizeof(input_port_t));
      if(!node->input_ports[i]) {
        err = bg_error_set(bg_ERR_NO_MEMORY);
        break;
      }
      node->input_ports[i]->value = 0.;
      sprintf(name, "in%lu", (unsigned long)i+1);
      copy_name = malloc(strlen(name)+1);
      strcpy(copy_name, name);
      node->input_ports[i]->name = copy_name;
#ifdef INTERVAL_SUPPORT
      mpfi_init_set_d(node->input_ports[i]->value_intv, 0.);
#endif
      err = bg_node_set_input_intern(node, i, bg_MERGE_TYPE_SUM, 0., 0., 0, false);
      if(err != bg_SUCCESS) {
        break;
      }
    }
  } else {
    err = bg_error_set(bg_ERR_NO_MEMORY);
  }
  return err;
}

bg_error bg_node_create_output_ports(bg_node_t *node, size_t cnt) {
  size_t i;
  char name[25];
  char *copy_name;
  bg_error err = bg_SUCCESS;
  node->output_port_cnt = cnt;
  node->output_ports = (output_port_t**)calloc(cnt, sizeof(output_port_t*));
  if(node->output_ports) {
    for(i = 0; i < cnt; ++i) {
      node->output_ports[i] = (output_port_t*)calloc(1, sizeof(output_port_t));
      if(!node->output_ports[i]) {
        err = bg_error_set(bg_ERR_NO_MEMORY);
        break;
      }
      sprintf(name, "out%lu", (unsigned long)i+1);
      copy_name = malloc(strlen(name)+1);
      strcpy(copy_name, name);
      node->output_ports[i]->name = copy_name;
#ifdef INTERVAL_SUPPORT
      mpfi_init_set_d(node->output_ports[i]->value_intv, 0.);
#endif
    }
  } else {
    err = bg_error_set(bg_ERR_NO_MEMORY);
  }
  return err;
}

bg_error bg_node_remove_input_ports(bg_node_t *node) {
  size_t i;
  for(i = 0; i < node->input_port_cnt; ++i) {
#ifdef INTERVAL_SUPPORT
    mpfi_clear(node->input_ports[i]->value_intv);
#endif
    if(node->input_ports[i]->name) {
      free((void*)node->input_ports[i]->name);
    }
    free(node->input_ports[i]);
  }
  free(node->input_ports);
  node->input_port_cnt = 0;
  return bg_SUCCESS;
}

bg_error bg_node_remove_output_ports(bg_node_t *node) {
  size_t i;
  for(i = 0; i < node->output_port_cnt; ++i) {
#ifdef INTERVAL_SUPPORT
    mpfi_clear(node->output_ports[i]->value_intv);
#endif
    if(node->output_ports[i]->name) {
      free((void*)node->output_ports[i]->name);
    }
    free(node->output_ports[i]);
  }
  free(node->output_ports);
  node->output_port_cnt = 0;
  return bg_SUCCESS;
}

bg_error bg_node_set_merge(bg_graph_t *graph,
                           bg_node_id_t node_id, size_t inputPortIdx,
                           bg_merge_type mergeType,
                           bg_real default_value, bg_real bias) {
  bg_node_t *node = NULL;
  bg_error err = bg_graph_find_node(graph, node_id, &node);
  if(err == bg_SUCCESS) {
    if(node) {
      err = bg_node_set_input_intern(node, inputPortIdx, mergeType,
                                     default_value, bias, 0, false);
    } else {
      err = bg_error_set(bg_ERR_NODE_NOT_FOUND);
    }
  }
  return err;
}

bg_error bg_node_set_input_intern(bg_node_t *node, size_t inputPortIdx,
                                  bg_merge_type mergeType,
                                  bg_real default_value, bg_real bias,
                                  const char *name, bool clearName) {
  input_port_t *input_port;
  if(node->input_port_cnt <= inputPortIdx) {
    return bg_error_set(bg_ERR_OUT_OF_RANGE);
  }
  input_port = node->input_ports[inputPortIdx];
  input_port->merge = merge_types[mergeType];
  input_port->bias = bias;
  input_port->defaultValue = default_value;
  if(clearName && input_port->name) {
    free((void*)input_port->name);
    input_port->name = 0;
  }
  if(name) {
    char *copy_name;
    copy_name = malloc(strlen(name)+1);
    strcpy(copy_name, name);
    input_port->name = copy_name;
  }
  return bg_SUCCESS;
}

bg_error bg_node_set_input(bg_graph_t *graph,
                           bg_node_id_t node_id, size_t inputPortIdx,
                           bg_merge_type mergeType,
                           bg_real default_value, bg_real bias,
                           const char *name) {
  bg_node_t *node = NULL;
  bg_error err = bg_graph_find_node(graph, node_id, &node);
  if(err == bg_SUCCESS) {
    if(node) {
      err = bg_node_set_input_intern(node, inputPortIdx, mergeType,
                                     default_value, bias, name, true);
    } else {
      err = bg_error_set(bg_ERR_NODE_NOT_FOUND);
    }
  }
  return err;
}

bg_error bg_node_set_output_intern(bg_node_t *node, size_t outputPortIdx,
                                   const char *name, bool clearName) {
  output_port_t *output_port;
  if(node->output_port_cnt <= outputPortIdx) {
    return bg_error_set(bg_ERR_OUT_OF_RANGE);
  }
  output_port = node->output_ports[outputPortIdx];
  if(clearName && output_port->name) {
    free((void*)output_port->name);
    output_port->name = 0;
  }
  if(name) {
    char *copy_name;
    copy_name = malloc(strlen(name)+1);
    strcpy(copy_name, name);
    output_port->name = copy_name;
  }
  return bg_SUCCESS;
}

bg_error bg_node_set_output(bg_graph_t *graph,
                           bg_node_id_t node_id, size_t outputPortIdx,
                           const char *name) {
  bg_node_t *node = NULL;
  bg_error err = bg_graph_find_node(graph, node_id, &node);
  if(err == bg_SUCCESS) {
    if(node) {
      err = bg_node_set_output_intern(node, outputPortIdx, name, true);
    } else {
      err = bg_error_set(bg_ERR_NODE_NOT_FOUND);
    }
  }
  return err;
}

bg_error bg_node_set_default(bg_graph_t *graph,
                             bg_node_id_t node_id, size_t inputPortIdx,
                             bg_real defaultValue) {
  bg_node_t *node = NULL;
  bg_error err = bg_graph_find_node(graph, node_id, &node);
  if(err != bg_SUCCESS) {
    return err;
  } else if(node == NULL) {
    return bg_error_set(bg_ERR_NODE_NOT_FOUND);
  }
  if(node->input_port_cnt <= inputPortIdx) {
    return bg_error_set(bg_ERR_OUT_OF_RANGE);
  }
  node->input_ports[inputPortIdx]->defaultValue = defaultValue;
  return bg_SUCCESS;
}

bg_error bg_node_set_bias(bg_graph_t *graph,
                          bg_node_id_t node_id, size_t inputPortIdx,
                          bg_real bias) {
  bg_node_t *node = NULL;
  bg_error err = bg_graph_find_node(graph, node_id, &node);
  if(err != bg_SUCCESS) {
    return err;
  } else if(node == NULL) {
    return bg_error_set(bg_ERR_NODE_NOT_FOUND);
  }
  if(node->input_port_cnt <= inputPortIdx) {
    return bg_error_set(bg_ERR_OUT_OF_RANGE);
  }
  node->input_ports[inputPortIdx]->bias = bias;
  return bg_SUCCESS;
}

bg_error bg_node_set_subgraph(bg_graph_t *graph, bg_node_id_t node_id,
                              bg_graph_t *subgraph) {
  bg_node_t *node = NULL;
  bg_error err = bg_graph_find_node(graph, node_id, &node);
  if(err != bg_SUCCESS) {
    return err;
  } else if(node == NULL) {
    return bg_error_set(bg_ERR_NODE_NOT_FOUND);
  }
  if(node->type->id != bg_NODE_TYPE_SUBGRAPH) {
    return bg_error_set(bg_ERR_WRONG_TYPE);
  }
  ((subgraph_data_t*)node->_priv_data)->subgraph = subgraph;
  node->input_port_cnt = subgraph->input_port_cnt;
  node->output_port_cnt = subgraph->output_port_cnt;
  node->input_ports = subgraph->input_ports;
  node->output_ports = subgraph->output_ports;
  return bg_SUCCESS;
}

bg_error bg_node_set_extern(bg_graph_t *graph, bg_node_id_t node_id,
                            const char *extern_node_name) {

  bg_node_t *node = NULL;
  int i;
  bg_error err = bg_graph_find_node(graph, node_id, &node);
  if(err != bg_SUCCESS) {
    return err;
  } else if(node == NULL) {
    return bg_error_set(bg_ERR_NODE_NOT_FOUND);
  }
  if(node->type->id != bg_NODE_TYPE_EXTERN) {
    return bg_error_set(bg_ERR_WRONG_TYPE);
  }

  for(i=0; i<num_extern_node_types; ++i) {
    if(strlen(extern_node_name) == strlen(extern_node_types[i]->name)) {
      if(strncmp(extern_node_name, extern_node_types[i]->name,
                 strlen(extern_node_types[i]->name)) == 0) {
        node->type = extern_node_types[i];
        return node->type->init(node);
      }
    }
  }
  return bg_ERR_EXTERN_NODE_NOT_FOUND;
}

bg_error bg_node_get_output(const bg_graph_t *graph,
                            bg_node_id_t node_id, size_t outputPortIdx,
                            bg_real *value) {
  bg_node_t *node = NULL;
  /* cast away const-ness because C does not support overloading */
  bg_error err = bg_graph_find_node((bg_graph_t*)graph, node_id, &node);
  if(err != bg_SUCCESS) {
    return err;
  } else if(node == NULL) {
    return bg_error_set(bg_ERR_NODE_NOT_FOUND);
  }
  if(node->output_port_cnt <= outputPortIdx) {
    return bg_error_set(bg_ERR_OUT_OF_RANGE);
  }
  *value = node->output_ports[outputPortIdx]->value;
  return bg_SUCCESS;
}

bg_error bg_node_get_name(const bg_graph_t *graph,
                          bg_node_id_t node_id, const char **name) {
  bg_node_t *node = NULL;
  bg_error err = bg_graph_find_node((bg_graph_t*)graph, node_id, &node);
  if(err != bg_SUCCESS) {
    return err;
  } else if(node == NULL) {
    return bg_error_set(bg_ERR_NODE_NOT_FOUND);
  }
  *name = node->name;
  return bg_SUCCESS;
}

bg_error bg_node_get_type(const bg_graph_t *graph,
                          bg_node_id_t node_id, bg_node_type *nodeType) {
  bg_node_t *node = NULL;
  bg_error err = bg_graph_find_node((bg_graph_t*)graph, node_id, &node);
  if(err != bg_SUCCESS) {
    return err;
  } else if(node == NULL) {
    return bg_error_set(bg_ERR_NODE_NOT_FOUND);
  }
  *nodeType = node->type->id;
  return bg_SUCCESS;
}

bg_error bg_node_get_merge(const bg_graph_t *graph,
                           bg_node_id_t node_id, size_t inputPortIdx,
                           bg_merge_type *mergeType) {
  bg_node_t *node = NULL;
  bg_error err = bg_graph_find_node((bg_graph_t*)graph, node_id, &node);
  if(err != bg_SUCCESS) {
    return err;
  } else if(node == NULL) {
    return bg_error_set(bg_ERR_NODE_NOT_FOUND);
  }
  if(node->input_port_cnt <= inputPortIdx) {
    return bg_error_set(bg_ERR_OUT_OF_RANGE);
  }
  *mergeType = node->input_ports[inputPortIdx]->merge->id;
  return bg_SUCCESS;
}

bg_error bg_node_get_default(const bg_graph_t *graph,
                             bg_node_id_t node_id, size_t inputPortIdx,
                             bg_real *defaultValue) {
  bg_node_t *node = NULL;
  bg_error err = bg_graph_find_node((bg_graph_t*)graph, node_id, &node);
  if(err != bg_SUCCESS) {
    return err;
  } else if(node == NULL) {
    return bg_error_set(bg_ERR_NODE_NOT_FOUND);
  }
  if(node->input_port_cnt <= inputPortIdx) {
    return bg_error_set(bg_ERR_OUT_OF_RANGE);
  }
  *defaultValue = node->input_ports[inputPortIdx]->defaultValue;
  return bg_SUCCESS;
}

bg_error bg_node_get_bias(const bg_graph_t *graph,
                          bg_node_id_t node_id, size_t inputPortIdx,
                          bg_real *bias) {
  bg_node_t *node = NULL;
  bg_error err = bg_graph_find_node((bg_graph_t*)graph, node_id, &node);
  if(err != bg_SUCCESS) {
    return err;
  } else if(node == NULL) {
    return bg_error_set(bg_ERR_NODE_NOT_FOUND);
  }
  if(node->input_port_cnt <= inputPortIdx) {
    return bg_error_set(bg_ERR_OUT_OF_RANGE);
  }
  *bias = node->input_ports[inputPortIdx]->bias;
  return bg_SUCCESS;
}

bg_error bg_node_get_port_cnt(const bg_graph_t *graph, bg_node_id_t node_id,
                              size_t *inputPortCnt, size_t *outputPortCnt) {
  bg_node_t *node = NULL;
  bg_error err = bg_graph_find_node((bg_graph_t*)graph, node_id, &node);
  if(err != bg_SUCCESS) {
    return err;
  } else if(node == NULL) {
    return bg_error_set(bg_ERR_NODE_NOT_FOUND);
  }
  if(inputPortCnt) {
    *inputPortCnt = node->input_port_cnt;
  }
  if(outputPortCnt) {
    *outputPortCnt = node->output_port_cnt;
  }
  return bg_SUCCESS;
}

bg_error bg_node_get_input_edges(const bg_graph_t *graph,
                                 bg_node_id_t node_id, size_t inputPortIdx,
                                 bg_edge_id_t *inputEdges, size_t *inputEdgeCnt) {
  int i;
  input_port_t *input_port;
  bg_node_t *node = NULL;
  bg_error err = bg_graph_find_node((bg_graph_t*)graph, node_id, &node);
  if(err != bg_SUCCESS) {
    return err;
  } else if(node == NULL) {
    return bg_error_set(bg_ERR_NODE_NOT_FOUND);
  }
  if(node->input_port_cnt <= inputPortIdx) {
    return bg_error_set(bg_ERR_OUT_OF_RANGE);
  }
  input_port = node->input_ports[inputPortIdx];
  if(!inputEdges) {
    *inputEdgeCnt = input_port->num_edges;
  } else {
    for(i = 0; i < bg_min(input_port->num_edges, *inputEdgeCnt); ++i) {
      inputEdges[i] = input_port->edges[i]->id;
    }
  }
  return bg_SUCCESS;
}

bg_error bg_node_get_output_edges(const bg_graph_t *graph,
                                  bg_node_id_t node_id, size_t outputPortIdx,
                                  bg_edge_id_t *outputEdges, size_t *outputEdgeCnt) {
  int i;
  output_port_t *output_port;
  bg_node_t *node = NULL;
  bg_error err = bg_graph_find_node((bg_graph_t*)graph, node_id, &node);
  if(err != bg_SUCCESS) {
    return err;
  } else if(node == NULL) {
    return bg_error_set(bg_ERR_NODE_NOT_FOUND);
  }
  if(node->output_port_cnt <= outputPortIdx) {
    return bg_error_set(bg_ERR_OUT_OF_RANGE);
  }
  output_port = node->output_ports[outputPortIdx];
  if(!outputEdges) {
    *outputEdgeCnt = output_port->num_edges;
  } else {
    for(i = 0; i < bg_min(output_port->num_edges, *outputEdgeCnt); ++i) {
      outputEdges[i] = output_port->edges[i]->id;
    }
  }
  return bg_SUCCESS;
}

bg_error bg_node_is_connected(const bg_node_t *node, bool *is_connected) {
  size_t i;
  for(i = 0; i < node->input_port_cnt; ++i) {
    if(node->input_ports[i]->num_edges) {
      *is_connected = true;
      return bg_SUCCESS;
    }
  }
  for(i = 0; i < node->output_port_cnt; ++i) {
    if(node->output_ports[i]->num_edges) {
      *is_connected = true;
      return bg_SUCCESS;
    }
  }
  *is_connected = false;
  return bg_SUCCESS;
}

bg_error bg_node_evaluate(bg_node_t *node) {
  bg_error err;
  size_t i, j;
  bg_real value;
  /* merge input ports */
  for(i = 0; i < node->input_port_cnt; ++i) {
    node->input_ports[i]->merge->merge(node->input_ports[i]);
    /*printf("\"%s %lu:%lu\" merge result: %g\n", node->name, node->id, i, node->input_ports[i]->value);*/
  }
  /* evaluate nodes */
  err = node->type->eval(node);
  /* write to outputs */
  for(i = 0; i < node->output_port_cnt; ++i) {
    value = node->output_ports[i]->value;
    /*printf("\"%s %lu:%lu\" output result: %g\n", node->name, node->id, i, value);*/
    for(j = 0; j < node->output_ports[i]->num_edges; ++j) {
      node->output_ports[i]->edges[j]->value = value;
    }
  }
  return err;
}

bg_error bg_node_get_id(const bg_graph_t *graph, const char *name,
                        unsigned long *id) {
  bg_node_list_t *node_list;
  bg_node_list_iterator_t it;
  bg_node_t *current_node;
  node_list = graph->input_nodes;
  for(current_node = bg_node_list_first(node_list, &it);
      current_node; current_node = bg_node_list_next(&it)) {
    if(current_node->name != 0 &&
       strcmp(current_node->name, name) == 0) {
      *id = current_node->id;
      return bg_SUCCESS;
    }
  }
  node_list = graph->hidden_nodes;
  for(current_node = bg_node_list_first(node_list, &it);
      current_node; current_node = bg_node_list_next(&it)) {
    if(current_node->name != 0 &&
       strcmp(current_node->name, name) == 0) {
      *id = current_node->id;
      return bg_SUCCESS;
    }
  }
  node_list = graph->output_nodes;
  for(current_node = bg_node_list_first(node_list, &it);
      current_node; current_node = bg_node_list_next(&it)) {
    if(current_node->name != 0 &&
       strcmp(current_node->name, name) == 0) {
      *id = current_node->id;
      return bg_SUCCESS;
    }
  }
  return bg_ERR_UNKNOWN;
}

bg_error bg_node_get_output_idx(bg_graph_t *graph, bg_node_id_t node_id,
                                const char *name, size_t *idx) {
  bg_node_t *node;
  size_t i;

  bg_graph_find_node(graph, node_id, &node);
  if(node == NULL) {
    return bg_ERR_UNKNOWN;
  }
  for(i = 0; i < node->output_port_cnt; ++i) {
    if(node->output_ports[i]->name != 0 &&
       strcmp(node->output_ports[i]->name, name) == 0) {
      *idx = i;
      return bg_SUCCESS;
    }
  }

  return bg_ERR_UNKNOWN;
}

bg_error bg_node_get_input_idx(bg_graph_t *graph, bg_node_id_t node_id,
                               const char *name, size_t *idx) {
  bg_node_t *node;
  size_t i;

  bg_graph_find_node(graph, node_id, &node);
  if(node == NULL) {
    return bg_ERR_UNKNOWN;
  }
  for(i = 0; i < node->input_port_cnt; ++i) {
    if(node->input_ports[i]->name != 0 &&
       strcmp(node->input_ports[i]->name, name) == 0) {
      *idx = i;
      return bg_SUCCESS;
    }
  }

  return bg_ERR_UNKNOWN;
}

bg_error bg_node_get_input_name(const bg_graph_t *graph,
                                bg_node_id_t node_id, size_t input_port_idx,
                                char *name) {
  bg_node_t *node;

  bg_graph_find_node((bg_graph_t*)graph, node_id, &node);
  if(node == NULL) {
    return bg_ERR_UNKNOWN;
  }
  if(node->input_port_cnt <= input_port_idx) {
    return bg_error_set(bg_ERR_OUT_OF_RANGE);
  }

  if(node->input_ports[input_port_idx]->name) {
    strcpy(name, node->input_ports[input_port_idx]->name);
    return bg_SUCCESS;
  }

  return bg_ERR_UNKNOWN;
}

bg_error bg_node_get_output_name(const bg_graph_t *graph,
                                 bg_node_id_t node_id, size_t output_port_idx,
                                 char *name) {
  bg_node_t *node;

  bg_graph_find_node((bg_graph_t*)graph, node_id, &node);
  if(node == NULL) {
    return bg_ERR_UNKNOWN;
  }
  if(node->output_port_cnt <= output_port_idx) {
    return bg_error_set(bg_ERR_OUT_OF_RANGE);
  }

  if(node->output_ports[output_port_idx]->name) {
    strcpy(name, node->output_ports[output_port_idx]->name);
    return bg_SUCCESS;
  }

  return bg_ERR_UNKNOWN;
}
