#include "bg_graph.h"

#include "bg_impl.h"
#include "bg_node.h"
#include "bg_edge.h"
#include "tsort/tsort.h"
#include "node_types/bg_node_subgraph.h"

#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "node_list.h"
#include "edge_list.h"

char bg_graph_error_message[bg_MAX_STRING_LENGTH];

bg_error bg_graph_find_node(bg_graph_t *graph, bg_node_id_t node_id,
                            bg_node_t **node) {
  bg_node_list_t *node_list;
  bg_node_list_iterator_t it;
  bg_node_t *current_node;
  node_list = graph->input_nodes;
  for(current_node = bg_node_list_first(node_list, &it);
      current_node; current_node = bg_node_list_next(&it)) {
    if(current_node->id == node_id) {
      *node = current_node;
      return bg_SUCCESS;
    }
  }
  node_list = graph->hidden_nodes;
  for(current_node = bg_node_list_first(node_list, &it);
      current_node; current_node = bg_node_list_next(&it)) {
    if(current_node->id == node_id) {
      *node = current_node;
      return bg_SUCCESS;
    }
  }
  node_list = graph->output_nodes;
  for(current_node = bg_node_list_first(node_list, &it);
      current_node; current_node = bg_node_list_next(&it)) {
    if(current_node->id == node_id) {
      *node = current_node;
      return bg_SUCCESS;
    }
  }
  *node = NULL;
  return bg_SUCCESS;
}

bg_error bg_graph_get_subgraph(bg_graph_t *graph, const char* name,
                               bg_graph_t **subgraph) {
  bg_node_list_t *node_list;
  bg_node_list_iterator_t it;
  bg_node_t *current_node;
  node_list = graph->hidden_nodes;
  fprintf(stderr, "search for: %s\n", name);
  for(current_node = bg_node_list_first(node_list, &it);
      current_node; current_node = bg_node_list_next(&it)) {
    fprintf(stderr, "  compare: %s\n", current_node->name);
    if(strcmp(current_node->name, name) == 0) {
      fprintf(stderr, "found name ");
    }
    if(strcmp(current_node->name, name) == 0 &&
       current_node->type->id == bg_NODE_TYPE_SUBGRAPH) {
      fprintf(stderr, "...found\n");
      *subgraph = ((subgraph_data_t*)current_node->_priv_data)->subgraph;
      return bg_SUCCESS;
    }
  }
  *subgraph = NULL;
  return bg_SUCCESS;
}


bg_error bg_graph_find_edge(bg_graph_t *graph, bg_edge_id_t edge_id,
                            bg_edge_t **edge) {
  bg_edge_list_t *edge_list;
  bg_edge_list_iterator_t it;
  bg_edge_t *current_edge;

  edge_list = graph->edge_list;
  for(current_edge = bg_edge_list_first(edge_list, &it);
      current_edge; current_edge = bg_edge_list_next(&it)) {
    if(current_edge->id == edge_id) {
      *edge = current_edge;
      return bg_SUCCESS;
    }
  }
  *edge = NULL;
  return bg_SUCCESS;
}

bg_error bg_graph_evaluate(bg_graph_t *graph) {
  bg_error err = bg_SUCCESS;
  bg_node_t *current_node;
  bg_node_list_t *node_list = graph->evaluation_order;
  bg_node_list_iterator_t it;
  if(graph->eval_order_is_dirty) {
    determine_evaluation_order(graph);
    graph->eval_order_is_dirty = false;
  }
  /* first process input nodes, then hidden nodes, and last output nodes */
  for(current_node = bg_node_list_first(graph->input_nodes, &it);
      current_node; current_node = bg_node_list_next(&it)) {
    /*printf("eval \"%s\"\n", current_node->name);*/
    err = bg_node_evaluate(current_node);
    if(err != bg_SUCCESS) {
      break;
    }
  }

  for(current_node = bg_node_list_first(node_list, &it);
      current_node; current_node = bg_node_list_next(&it)) {
    /*printf("eval \"%s\"\n", current_node->name);*/
    err = bg_node_evaluate(current_node);
    if(err != bg_SUCCESS) {
      break;
    }
  }

  for(current_node = bg_node_list_first(graph->output_nodes, &it);
      current_node; current_node = bg_node_list_next(&it)) {
    /*printf("eval \"%s\"\n", current_node->name);*/
    err = bg_node_evaluate(current_node);
    if(err != bg_SUCCESS) {
      break;
    }
  }
  return err;
}


bg_error bg_graph_alloc(bg_graph_t **graph, const char *name) {
  char *name_copy = malloc(strlen(name)+1);
  bg_graph_t *g;
  g = (bg_graph_t*)calloc(1, sizeof(bg_graph_t));
  if(!g) {
    return bg_error_set(bg_ERR_NO_MEMORY);
  }
  strcpy(name_copy, name);
  g->name = name_copy;

  g->input_port_cnt = 0;
  g->output_port_cnt = 0;
  bg_node_list_init(&g->evaluation_order);
  bg_node_list_init(&g->output_nodes);
  bg_node_list_init(&g->input_nodes);
  bg_node_list_init(&g->hidden_nodes);
  bg_edge_list_init(&g->edge_list);
  g->eval_order_is_dirty = false;
  g->next_id = 1;
  g->load_path = NULL;
  *graph = g;
  return bg_SUCCESS;
}

bg_error bg_graph_set_load_path(bg_graph_t *graph, const char *path) {
  char *name_copy;
  if(graph->load_path) {
    free((char*)graph->load_path);
  }
  if(path) {
    name_copy = malloc(strlen(path)+1);
    strcpy(name_copy, path);
    graph->load_path = name_copy;
  }
  else {
    graph->load_path = NULL;
  }
  return bg_error_get();
}

bg_error bg_graph_clone(bg_graph_t *dest, const bg_graph_t *src) {
  size_t i;
  bg_error err;
  bg_node_t *current_node;
  bg_edge_t *current_edge;
  bg_edge_t *e;
  bg_node_list_t *node_list;
  bg_edge_list_t *edge_list;
  bg_node_list_iterator_t node_it;
  bg_edge_list_iterator_t edge_it;

  /* clone load path */
  if(src->load_path) {
    char *name_copy = malloc(strlen(src->load_path)+1);
    strcpy(name_copy, src->load_path);
    if(dest->load_path) {
      free((char*)dest->load_path);
    }
    dest->load_path = name_copy;
  }

  /* clone all nodes */
  node_list = src->input_nodes;
  for(current_node = bg_node_list_first(node_list, &node_it);
      current_node; current_node = bg_node_list_next(&node_it)) {
    err = bg_graph_create_input(dest, current_node->name, current_node->id);
    if(err != bg_SUCCESS) {
      bg_error_message_get(err, bg_graph_error_message);
      printf("error while creating input node: %lu %s\n",
             (unsigned long) current_node->id, bg_graph_error_message);
    }
    for(i = 0; i < current_node->type->input_port_cnt; ++i) {
      err = bg_node_set_input(dest, current_node->id, i,
                              current_node->input_ports[i]->merge->id,
                              current_node->input_ports[i]->defaultValue,
                              current_node->input_ports[i]->bias,
                              current_node->input_ports[i]->name);
      if(err != bg_SUCCESS) {
        bg_error_message_get(err, bg_graph_error_message);
        printf("error in merge: %s %lu %s %lu\n", current_node->name,
               (unsigned long) i,  bg_graph_error_message, (unsigned long) current_node->id);
      }
    }
    for(i = 0; i < current_node->type->output_port_cnt; ++i) {
      err = bg_node_set_output(dest, current_node->id, i,
                               current_node->output_ports[i]->name);
      if(err != bg_SUCCESS) {
        bg_error_message_get(err, bg_graph_error_message);
        printf("error in set_output: %s %lu %s %lu\n", current_node->name,
               (unsigned long) i, bg_graph_error_message, (unsigned long) current_node->id);
      }
    }
  }

  node_list = src->output_nodes;
  for(current_node = bg_node_list_first(node_list, &node_it);
      current_node; current_node = bg_node_list_next(&node_it)) {
    bg_graph_create_output(dest, current_node->name, current_node->id);
    for(i = 0; i < current_node->type->input_port_cnt; ++i) {
      err = bg_node_set_input(dest, current_node->id, i,
                              current_node->input_ports[i]->merge->id,
                              current_node->input_ports[i]->defaultValue,
                              current_node->input_ports[i]->bias,
                              current_node->input_ports[i]->name);
      if(err != bg_SUCCESS) {
        bg_error_message_get(err, bg_graph_error_message);
        fprintf(stderr, "[bg_graph_clone] error in set_input: %s %lu %s %lu\n",
                current_node->name,  (unsigned long) i, bg_graph_error_message,
                (unsigned long) current_node->id);
      }
    }
    for(i = 0; i < current_node->type->output_port_cnt; ++i) {
      err = bg_node_set_output(dest, current_node->id, i,
                              current_node->output_ports[i]->name);
      if(err != bg_SUCCESS) {
        bg_error_message_get(err, bg_graph_error_message);
        fprintf(stderr, "[bg_graph_clone]  error in set_output: %s %lu %s %lu\n",
                current_node->name, (unsigned long) i, bg_graph_error_message,
                (unsigned long) current_node->id);
      }
    }
  }

  node_list = src->hidden_nodes;
  for(current_node = bg_node_list_first(node_list, &node_it);
      current_node; current_node = bg_node_list_next(&node_it)) {
    bg_graph_create_node(dest, current_node->name, current_node->id,
                         current_node->type->id);
    if(current_node->type->id == bg_NODE_TYPE_EXTERN) {
      err = bg_node_set_extern(dest, current_node->id,
                               current_node->type->name);
    }
    if(current_node->type->id == bg_NODE_TYPE_SUBGRAPH) {
      bg_graph_t *subgraph=0;
      bg_graph_t *currentSub;
      currentSub = ((subgraph_data_t*)current_node->_priv_data)->subgraph;
      err = bg_graph_alloc(&subgraph, currentSub->name);
      if(err != bg_SUCCESS) {
        printf("bg_graph_clone: error in alloc subgraph memory\n");
      }
      err = bg_graph_clone(subgraph, currentSub);
      if(err != bg_SUCCESS) {
        bg_error_message_get(err, bg_graph_error_message);
        printf("bg_graph_clone: error in clone subgraph: %s\n", bg_graph_error_message);
      }
      err = bg_node_set_subgraph(dest, current_node->id, subgraph);
      if(err != bg_SUCCESS) {
        bg_error_message_get(err, bg_graph_error_message);
        printf("bg_graph_clone: error in node set subgraph: %s\n", bg_graph_error_message);
      }
    }
    for(i = 0; i < current_node->type->input_port_cnt; ++i) {
      err = bg_node_set_input(dest, current_node->id, i,
                              current_node->input_ports[i]->merge->id,
                              current_node->input_ports[i]->defaultValue,
                              current_node->input_ports[i]->bias,
                              current_node->input_ports[i]->name);
      if(err != bg_SUCCESS) {
        bg_error_message_get(err, bg_graph_error_message);
        printf("error in set_input: %s %lu %s %lu\n", current_node->name,
               (unsigned long) i, bg_graph_error_message,
               (unsigned long) current_node->id);
      }
    }
    for(i = 0; i < current_node->type->output_port_cnt; ++i) {
      err = bg_node_set_output(dest, current_node->id, i,
                              current_node->output_ports[i]->name);
      if(err != bg_SUCCESS) {
        bg_error_message_get(err, bg_graph_error_message);
        printf("error in set_output: %s %lu %s %lu\n", current_node->name,
               (unsigned long) i, bg_graph_error_message, (unsigned long) current_node->id);
      }
    }
    /* todo: need some special handling for subgraph nodes? */
  }

  /* clone all edges */
  edge_list = src->edge_list;
  for(current_edge = bg_edge_list_first(edge_list, &edge_it);
      current_edge; current_edge = bg_edge_list_next(&edge_it)) {

    if (current_edge->source_node) {
      err = bg_graph_create_edge(dest, current_edge->source_node->id,
                                 current_edge->source_port_idx,
                                 current_edge->sink_node->id,
                                 current_edge->sink_port_idx,
                                 current_edge->weight,
                                 current_edge->id);
    }
    else {
      err = bg_graph_create_edge(dest, 0, 0,
                                 current_edge->sink_node->id,
                                 current_edge->sink_port_idx,
                                 current_edge->weight,
                                 current_edge->id);
    }
    if(err != bg_SUCCESS) {
      bg_error_message_get(err, bg_graph_error_message);
      printf("bg_graph_clone: error while creating edge: %s\n", bg_graph_error_message);
      return err;
    }
    err = bg_graph_find_edge(dest, current_edge->id, &e);
    if(err != bg_SUCCESS) {
      fprintf(stderr, "ERROR!, setting sort option for edge: %lu\n", current_edge->id);
    }
    else {
      e->ignore_for_sort = current_edge->ignore_for_sort;
    }
  }

  dest->eval_order_is_dirty = true;

  return bg_error_get();

}

bg_error bg_graph_free(bg_graph_t *graph) {
  size_t i;
  bg_node_t *current_node;
  bg_edge_t *current_edge;
  bg_node_list_t *node_list, *node_lists[3];
  bg_edge_list_t *edge_list;
  bg_node_list_iterator_t node_it;
  bg_edge_list_iterator_t edge_it;
  /* remove all edges */
  edge_list = graph->edge_list;
  for(current_edge = bg_edge_list_first(edge_list, &edge_it);
      current_edge; current_edge = bg_edge_list_next(&edge_it)) {
    bg_edge_deinit(current_edge);
    free(current_edge);
  }
  /* remove all nodes */
  node_lists[0] = graph->input_nodes;
  node_lists[1] = graph->output_nodes;
  node_lists[2] = graph->hidden_nodes;
  for(i = 0; i < 3; ++i) {
    node_list = node_lists[i];
    for(current_node = bg_node_list_first(node_list, &node_it);
        current_node; current_node = bg_node_list_next(&node_it)) {
      current_node->type->deinit(current_node);
      free((void*)(current_node->name));
      free(current_node);
    }
  }
  /* remove input ports */
  graph->input_port_cnt = 0;
  /* remove output ports */
  graph->output_port_cnt = 0;
  /* free private data */
  bg_node_list_deinit(graph->evaluation_order);
  bg_node_list_deinit(graph->output_nodes);
  bg_node_list_deinit(graph->input_nodes);
  bg_node_list_deinit(graph->hidden_nodes);
  bg_edge_list_deinit(graph->edge_list);
  free((char*)graph->name);
  if(graph->load_path) {
    free((char*)graph->load_path);
  }
  free(graph);
  return bg_error_get();
}

bg_error bg_graph_has_node(bg_graph_t *graph, bg_node_id_t node_id,
                           bool *has_node) {
  bg_error err = bg_SUCCESS;
  bg_node_t *tmp_node;
  err = bg_graph_find_node(graph, node_id, &tmp_node);
  if(err != bg_SUCCESS) {
    return err;
  } else if(tmp_node != NULL) {
    *has_node = true;
  }
  else {
    *has_node = false;
  }
  return err;
}

bg_error bg_graph_create_input(bg_graph_t *graph, const char *name,
                               bg_node_id_t input_id) {
  bg_error err = bg_SUCCESS;
  bg_node_t *new_node, *tmp_node;
  err = bg_graph_find_node(graph, input_id, &tmp_node);
  if(err != bg_SUCCESS) {
    return err;
  } else if(tmp_node != NULL) {
    return bg_error_set(bg_ERR_DUPLICATE_NODE_ID);
  }
  if(graph->input_port_cnt+1 >= bg_MAX_PORTS) {
    return bg_error_set(bg_ERR_NUM_PORTS_EXCEEDED);
  }
  new_node = (bg_node_t*)calloc(1, sizeof(bg_node_t));
  if(!new_node) {
    return bg_error_set(bg_ERR_NO_MEMORY);
  }
  err = bg_node_init(new_node, name, input_id, bg_NODE_TYPE_INPUT);
  if(err != bg_SUCCESS) {
    return err;
  }
  graph->input_ports[graph->input_port_cnt] = new_node->input_ports[0];
  graph->input_port_cnt++;
  new_node->_parent_graph = graph;
  bg_node_list_append(graph->input_nodes, new_node);
  graph->eval_order_is_dirty = true;
  return bg_SUCCESS;
}

bg_error bg_graph_create_output(bg_graph_t *graph, const char *name,
                                bg_node_id_t output_id) {
  bg_error err = bg_SUCCESS;
  bg_node_t *new_node, *tmp_node;
  err = bg_graph_find_node(graph, output_id, &tmp_node);
  if(err != bg_SUCCESS) {
    return err;
  } else if(tmp_node != NULL) {
    return bg_error_set(bg_ERR_DUPLICATE_NODE_ID);
  }
  if(graph->output_port_cnt+1 >= bg_MAX_PORTS) {
    return bg_error_set(bg_ERR_NUM_PORTS_EXCEEDED);
  }
  new_node = (bg_node_t*)calloc(1, sizeof(bg_node_t));
  if(!new_node) {
    return bg_error_set(bg_ERR_NO_MEMORY);
  }
  err = bg_node_init(new_node, name, output_id, bg_NODE_TYPE_OUTPUT);
  if(err != bg_SUCCESS) {
    return err;
  }
  graph->output_ports[graph->output_port_cnt] = new_node->output_ports[0];
  graph->output_port_cnt++;
  new_node->_parent_graph = graph;
  bg_node_list_append(graph->output_nodes, new_node);
  graph->eval_order_is_dirty = true;
  return bg_SUCCESS;
}

bg_error bg_graph_create_node(bg_graph_t *graph, const char *name,
                              bg_node_id_t node_id, bg_node_type nodeType) {
  bg_error err = bg_SUCCESS;
  bg_node_t *new_node, *tmp_node;
  err = bg_graph_find_node(graph, node_id, &tmp_node);
  if(err != bg_SUCCESS) {
    return err;
  } else if(tmp_node != NULL) {
    return bg_error_set(bg_ERR_DUPLICATE_NODE_ID);
  }
  new_node = (bg_node_t*)calloc(1, sizeof(bg_node_t));
  if(!new_node) {
    return bg_error_set(bg_ERR_NO_MEMORY);
  }
  err = bg_node_init(new_node, name, node_id, nodeType);
  if(err != bg_SUCCESS) {
    return err;
  }
  new_node->_parent_graph = graph;
  bg_node_list_append(graph->hidden_nodes, new_node);
  graph->eval_order_is_dirty = true;
  return bg_SUCCESS;
}

bg_error bg_graph_create_edge(bg_graph_t *graph,
                              bg_node_id_t source_node_id, size_t source_port_idx,
                              bg_node_id_t sink_node_id, size_t sink_port_idx,
                              bg_real weight, bg_edge_id_t edge_id) {
  bg_error err;
  bg_edge_t *new_edge;
  input_port_t *input_port;
  output_port_t *output_port;
  bg_node_t *sourceNode = NULL, *sinkNode = NULL;
  err = bg_graph_find_node(graph, source_node_id, &sourceNode);
  if(err != bg_SUCCESS) {
    return err;
  }
  err = bg_graph_find_node(graph, sink_node_id, &sinkNode);
  if(err != bg_SUCCESS) {
    return err;
  }
  if((source_node_id != 0 && sourceNode == NULL) ||
     (sink_node_id != 0 && sinkNode == NULL)) {
    return bg_error_set(bg_ERR_NODE_NOT_FOUND);
  }
  if((sourceNode && sourceNode->_parent_graph != graph) ||
     (sinkNode && sinkNode->_parent_graph != graph)) {
    return bg_error_set(bg_ERR_DO_NOT_OWN);
  }
  if((sourceNode && (sourceNode->output_port_cnt <= source_port_idx)) ||
     (sinkNode && (sinkNode->input_port_cnt <= sink_port_idx))) {
    return bg_error_set(bg_ERR_OUT_OF_RANGE);
  }
  if((sourceNode && sinkNode &&
      ((sourceNode->type->id == bg_NODE_TYPE_OUTPUT) ||
       (sinkNode->type->id == bg_NODE_TYPE_INPUT))) ||
     (!sourceNode && !sinkNode)) {
    return bg_error_set(bg_ERR_INVALID_CONNECTION);
  }

  if((sourceNode &&
      (sourceNode->output_ports[source_port_idx]->num_edges >= bg_MAX_EDGES)) ||
     (sinkNode &&
      (sinkNode->input_ports[sink_port_idx]->num_edges >= bg_MAX_EDGES))) {
    return bg_error_set(bg_ERR_PORT_FULL);
  }
  new_edge = (bg_edge_t*)calloc(1, sizeof(bg_edge_t));
  new_edge->id = edge_id;
  if(!new_edge) {
    return bg_error_set(bg_ERR_NO_MEMORY);
  }
  err = bg_edge_init(new_edge, sourceNode, source_port_idx,
                     sinkNode, sink_port_idx, weight);
  if(err != bg_SUCCESS) {
    free(new_edge);
    return err;
  }
  if(sourceNode) {
    output_port = sourceNode->output_ports[source_port_idx];
    output_port->edges[output_port->num_edges++] = new_edge;
  }
  if(sinkNode) {
    input_port = sinkNode->input_ports[sink_port_idx];
    input_port->edges[input_port->num_edges++] = new_edge;
  }
  bg_edge_list_append(graph->edge_list, new_edge);
  graph->eval_order_is_dirty = true;
  return bg_SUCCESS;
}

bg_error bg_graph_remove_node(bg_graph_t *graph, bg_node_id_t node_id) {
  bool is_connected;
  bg_error err = bg_SUCCESS;
  bg_node_list_iterator_t it;
  bg_node_t *node;
  bool found;
  err = bg_graph_find_node(graph, node_id, &node);
  if(err != bg_SUCCESS) {
    return err;
  } else if(node == NULL) {
    return bg_error_set(bg_ERR_NODE_NOT_FOUND);
  }
  if(node->type->id == bg_NODE_TYPE_INPUT ||
     node->type->id == bg_NODE_TYPE_OUTPUT) {
    return bg_error_set(bg_ERR_WRONG_TYPE);
  }
  if(node->_parent_graph != graph) {
    return bg_error_set(bg_ERR_DO_NOT_OWN);
  }
  bg_node_is_connected(node, &is_connected);
  if(is_connected) {
    return bg_error_set(bg_ERR_IS_CONNECTED);
  }
  found = bg_node_list_find(graph->hidden_nodes, node, &it);
  assert(found);
  /* nesseccary since found is not used in release build */
  if(found) {
    bg_node_list_erase(&it);
  }
  node->type->deinit(node);
  free((char*)node->name);
  free(node);
  graph->eval_order_is_dirty = true;
  return bg_SUCCESS;
}

bg_error bg_graph_remove_input(bg_graph_t *graph, bg_node_id_t input_id) {
  bool is_connected;
  bg_error err = bg_SUCCESS;
  bg_node_t *input;
  bg_node_list_iterator_t it;
  bool found;
  err = bg_graph_find_node(graph, input_id, &input);
  if(err != bg_SUCCESS) {
    return err;
  } else if(input == NULL) {
    return bg_error_set(bg_ERR_NODE_NOT_FOUND);
  }
  if(input->type->id != bg_NODE_TYPE_INPUT) {
    return bg_error_set(bg_ERR_WRONG_TYPE);
  }
  if(input->_parent_graph != graph) {
    return bg_error_set(bg_ERR_DO_NOT_OWN);
  }
  bg_node_is_connected(input, &is_connected);
  if(is_connected) {
    return bg_error_set(bg_ERR_IS_CONNECTED);
  }
  found = bg_node_list_find(graph->input_nodes, input, &it);
  assert(found);
  /* nesseccary since found is not used in release build */
  if(found) {
    bg_node_list_erase(&it);
  }
  input->type->deinit(input);
  free(input);
  graph->eval_order_is_dirty = true;
  return bg_SUCCESS;
}

bg_error bg_graph_remove_output(bg_graph_t *graph, bg_node_id_t output_id) {
  bool is_connected;
  bg_error err = bg_SUCCESS;
  bg_node_t *output;
  bg_node_list_iterator_t it;
  bool found;
  err = bg_graph_find_node(graph, output_id, &output);
  if(err != bg_SUCCESS) {
    return err;
  } else if(output == NULL) {
    return bg_error_set(bg_ERR_NODE_NOT_FOUND);
  }
  if(output->type->id != bg_NODE_TYPE_OUTPUT) {
    return bg_error_set(bg_ERR_WRONG_TYPE);
  }
  if(output->_parent_graph != graph) {
    return bg_error_set(bg_ERR_DO_NOT_OWN);
  }
  bg_node_is_connected(output, &is_connected);
  if(is_connected) {
    return bg_error_set(bg_ERR_IS_CONNECTED);
  }
  found = bg_node_list_find(graph->output_nodes, output, &it);
  assert(found);
  /* nesseccary since found is not used in release build */
  if(found) {
    bg_node_list_erase(&it);
  }
  output->type->deinit(output);
  free(output);
  graph->eval_order_is_dirty = true;
  return bg_SUCCESS;
}

bg_error bg_graph_remove_edge(bg_graph_t *graph, bg_edge_id_t edge_id) {
  size_t i;
  bg_edge_list_t *edge_list;
  input_port_t *input_port;
  output_port_t *output_port;
  bg_edge_t *edge;
  bg_edge_list_iterator_t it;
  bg_error err = bg_SUCCESS;
  err = bg_graph_find_edge(graph, edge_id, &edge);
  if(err != bg_SUCCESS) {
    return err;
  } else if(edge == NULL) {
    return bg_error_set(bg_ERR_EDGE_NOT_FOUND);
  }
  /* remove edge from edge_list */
  edge_list = graph->edge_list;
  if(!bg_edge_list_find(edge_list, edge, &it)) {
    return bg_error_set(bg_ERR_DO_NOT_OWN);
  }
  bg_edge_list_erase(&it);
  /* remove edge from source_node */
  if(edge->source_node) {
    i = 0;
    output_port = edge->source_node->output_ports[edge->source_port_idx];
    while((i < output_port->num_edges) && (output_port->edges[i] != edge)) {
      ++i;
    }
    if(i < output_port->num_edges) {
      output_port->edges[i] = output_port->edges[output_port->num_edges-1];
      --output_port->num_edges;
    } else {
      /* for some reason the edge is not in the list */
      return bg_error_set(bg_ERR_UNKNOWN);
    }
  }
  /* remove edge from sink_node */
  if(edge->sink_node) {
    i = 0;
    input_port = edge->sink_node->input_ports[edge->sink_port_idx];
    while((i < input_port->num_edges) && (input_port->edges[i] != edge)) {
      ++i;
    }
    if(i < input_port->num_edges) {
      input_port->edges[i] = input_port->edges[input_port->num_edges-1];
      --input_port->num_edges;
    } else {
      /* for some reason the edge is not in the list */
      return bg_error_set(bg_ERR_UNKNOWN);
    }
  }
  /* delete edge */
  bg_edge_deinit(edge);
  free(edge);
  graph->eval_order_is_dirty = true;
  return bg_SUCCESS;
}

bg_error bg_graph_reset(bg_graph_t *graph, bool recursive) {
  bg_edge_t *current_edge;
  bg_node_t *current_node;
  bg_edge_list_iterator_t edge_it;
  bg_node_list_iterator_t node_it;
  for(current_edge = bg_edge_list_first(graph->edge_list, &edge_it);
      current_edge; current_edge = bg_edge_list_next(&edge_it)) {
    current_edge->value = 0.;
#ifdef INTERVAL_SUPPORT
    mpfi_set_ui(current_edge->value_intv, 0);
#endif
  }
  for(current_node = bg_node_list_first(graph->input_nodes, &node_it);
      current_node; current_node = bg_node_list_next(&node_it)) {
    bg_node_reset(current_node, recursive);
  }
  for(current_node = bg_node_list_first(graph->hidden_nodes, &node_it);
      current_node; current_node = bg_node_list_next(&node_it)) {
    bg_node_reset(current_node, recursive);
  }
  for(current_node = bg_node_list_first(graph->output_nodes, &node_it);
      current_node; current_node = bg_node_list_next(&node_it)) {
    bg_node_reset(current_node, recursive);
  }
  return bg_SUCCESS;
}


bg_error bg_graph_get_output(const bg_graph_t *graph, size_t output_port_idx,
                             bg_real *value) {
  if(graph->output_port_cnt <= output_port_idx) {
    return bg_error_set(bg_ERR_OUT_OF_RANGE);
  }
  *value = graph->output_ports[output_port_idx]->value;
  return bg_SUCCESS;
}

bg_error bg_graph_get_node_cnt(const bg_graph_t *graph, bool recursive,
                             size_t *node_cnt) {
  size_t cnt = 0, subCnt = 0;
  bg_error err;
  bg_node_t *current_node;
  bg_node_list_t *node_list;
  bg_node_list_iterator_t node_it;
  subgraph_data_t *subgraph_data;

  if(recursive) {
    node_list = graph->hidden_nodes;
    current_node = bg_node_list_first(node_list, &node_it);
    while(current_node) {
      if(current_node->type->id == bg_NODE_TYPE_SUBGRAPH) {
        subgraph_data = ((subgraph_data_t*)current_node->_priv_data);
        err = bg_graph_get_node_cnt(subgraph_data->subgraph, recursive, &subCnt);
        if(err != bg_SUCCESS) {
          return err;
        }
        cnt += subCnt;
      } else {
        cnt++;
      }
      current_node = bg_node_list_next(&node_it);
    }
  } else {
    cnt += bg_node_list_size(graph->hidden_nodes);
  }

  cnt += bg_node_list_size(graph->input_nodes);
  cnt += bg_node_list_size(graph->output_nodes);
  *node_cnt = cnt;
  return bg_SUCCESS;
}

bg_error bg_graph_get_edge_cnt(const bg_graph_t *graph, bool recursive, size_t *edge_cnt) {
  size_t cnt = 0, subCnt = 0;
  bg_error err;
  bg_node_t *current_node;
  bg_node_list_iterator_t node_it;
  subgraph_data_t *subgraph_data;

  if(recursive) {
    for(current_node = bg_node_list_first(graph->hidden_nodes, &node_it);
        current_node; current_node = bg_node_list_next(&node_it)){
      if(current_node->type->id == bg_NODE_TYPE_SUBGRAPH) {
        subgraph_data = ((subgraph_data_t*)current_node->_priv_data);
        err = bg_graph_get_edge_cnt(subgraph_data->subgraph, recursive, &subCnt);
        if(err != bg_SUCCESS) {
          return err;
        }
        cnt += subCnt;
      }
    }
  }
  cnt += bg_edge_list_size(graph->edge_list);
  *edge_cnt = cnt;
  return bg_SUCCESS;
}

bg_error bg_graph_get_input_nodes(const bg_graph_t *graph,
                                  bg_node_id_t *input_ids, size_t *input_cnt) {
  int i, cnt;
  bg_node_t *input_node;
  bg_node_list_t *input_nodes;
  bg_node_list_iterator_t node_it;
  input_nodes = graph->input_nodes;
  cnt = bg_node_list_size(input_nodes);
  if(!input_ids) {
    *input_cnt = cnt;
  } else {
    for(i = 0, input_node = bg_node_list_first(input_nodes, &node_it);
        i < bg_min(cnt, *input_cnt);
        ++i, input_node = bg_node_list_next(&node_it)) {
      input_ids[i] = input_node->id;
    }
  }
  return bg_SUCCESS;
}

bg_error bg_graph_get_output_nodes(const bg_graph_t *graph,
                                   bg_node_id_t *output_ids, size_t *output_cnt) {
  int i, cnt;
  bg_node_t *output_node;
  bg_node_list_t *output_nodes;
  bg_node_list_iterator_t node_it;
  output_nodes = graph->output_nodes;
  cnt = bg_node_list_size(output_nodes);
  if(!output_ids) {
    *output_cnt = cnt;
  } else {
    for(i = 0, output_node = bg_node_list_first(output_nodes, &node_it);
        i < bg_min(cnt, *output_cnt);
        ++i, output_node = bg_node_list_next(&node_it)) {
      output_ids[i] = output_node->id;
    }
  }
  return bg_SUCCESS;
}


bg_error bg_graph_get_subgraph_list_r(bg_graph_t *graph, char **path,
                                      char **graph_name, size_t *subgraph_cnt,
                                      char *path_, size_t *index_,
                                      bool recursive) {
  int i, cnt;
  bg_node_t *node;
  bg_node_list_t *nodes;
  bg_node_list_iterator_t node_it;
  char tmp_str[255];
  subgraph_data_t *subgraph_data;
  bg_error err;

  nodes = graph->hidden_nodes;
  cnt = bg_node_list_size(nodes);

  for(i = 0, node = bg_node_list_first(nodes, &node_it);
      i < cnt; ++i, node = bg_node_list_next(&node_it)) {
      if(node->type->id == bg_NODE_TYPE_SUBGRAPH) {
        subgraph_data = ((subgraph_data_t*)node->_priv_data);
        if(path && *subgraph_cnt > *index_) {
          if(strlen(path_) > 0) {
            sprintf(tmp_str, "%s/%s", path_, node->name);
            /*fprintf(stderr, "get_subgraph2: %lu %s\n", *index_, tmp_str);*/
          }
          else {
            sprintf(tmp_str, "%s", node->name);
            /*fprintf(stderr, "get_subgraph1: %lu %s\n", *index_, tmp_str);*/
          }
          strcpy(path[*index_], tmp_str);
          strcpy(graph_name[*index_], subgraph_data->subgraph->name);
          /*fprintf(stderr, "get_subgraph: %lu %s\n", *index_, tmp_str);*/
        }
        *index_ += 1;
        if(recursive) {
          err = bg_graph_get_subgraph_list_r(subgraph_data->subgraph, path,
                                             graph_name, subgraph_cnt,
                                             tmp_str, index_, recursive);
          if(err != bg_SUCCESS) {
            return err;
          }
        }
      }
  }

  return bg_SUCCESS;
}

bg_error bg_graph_get_subgraph_list(bg_graph_t *graph, char **path,
                                    char **graph_name, size_t *subgraph_cnt,
                                    bool recursive) {
  size_t cnt = 0;
  bg_error err;

  err = bg_graph_get_subgraph_list_r(graph, path, graph_name,
                                     subgraph_cnt, "", &cnt,
                                     recursive);
  if(err != bg_SUCCESS) {
    return err;
  }
  if(path == 0) {
    *subgraph_cnt = cnt;
  }
  return bg_SUCCESS;
}

bg_error bg_graph_set_subgraph(bg_graph_t *graph, const char *graph_name,
                               bg_graph_t *subgraph) {
  int i, cnt;
  bg_node_t *node;
  bg_node_list_t *nodes;
  bg_node_list_iterator_t node_it;
  subgraph_data_t *subgraph_data;
  bg_graph_t *old_graph;
  bg_error err;
  char *name_copy;

  nodes = graph->hidden_nodes;
  cnt = bg_node_list_size(nodes);

  for(i = 0, node = bg_node_list_first(nodes, &node_it);
      i < cnt; ++i, node = bg_node_list_next(&node_it)) {
    if(node->type->id == bg_NODE_TYPE_SUBGRAPH) {
      subgraph_data = ((subgraph_data_t*)node->_priv_data);
      if(!strcmp(subgraph_data->subgraph->name, graph_name)) {
        size_t l;
        old_graph = subgraph_data->subgraph;
        err = bg_graph_alloc(&subgraph_data->subgraph, graph_name);
        if(err != bg_SUCCESS) {
          return err;
        }
        err = bg_graph_clone(subgraph_data->subgraph, subgraph);
        if(err != bg_SUCCESS) {
          return err;
        }
        node->input_port_cnt = subgraph_data->subgraph->input_port_cnt;
        node->output_port_cnt = subgraph_data->subgraph->output_port_cnt;
        node->input_ports = subgraph_data->subgraph->input_ports;
        node->output_ports = subgraph_data->subgraph->output_ports;

        for(l=0; l<node->input_port_cnt; ++l) {
          if(node->input_ports[l]->name) {
            free((void*)node->input_ports[l]->name);
          }
          *node->input_ports[l] = *old_graph->input_ports[l];
          if(old_graph->input_ports[l]->name) {
            name_copy = malloc(strlen(old_graph->input_ports[l]->name)+1);
            strcpy(name_copy, old_graph->input_ports[l]->name);
            node->input_ports[l]->name = name_copy;
          }
        }
        for(l=0; l<node->output_port_cnt; ++l) {
          if(node->output_ports[l]->name) {
            free((void*)node->output_ports[l]->name);
          }
          *node->output_ports[l] = *old_graph->output_ports[l];
          if(old_graph->output_ports[l]->name) {
            name_copy = malloc(strlen(old_graph->output_ports[l]->name)+1);
            strcpy(name_copy, old_graph->output_ports[l]->name);
            node->output_ports[l]->name = name_copy;
          }
        }
        err = bg_graph_free(old_graph);
        if(err != bg_SUCCESS) {
          return err;
        }
      }
      else {
        err = bg_graph_set_subgraph(subgraph_data->subgraph, graph_name,
                                    subgraph);
        if(err != bg_SUCCESS) {
          return err;
        }
      }
    }
  }

  return bg_SUCCESS;
}

bg_error bg_graph_get_max_node_id(bg_graph_t *graph, size_t *max_id) {
  bg_node_t *node;
  bg_node_list_iterator_t node_it;
  *max_id = 0;
  for(node = bg_node_list_first(graph->input_nodes, &node_it);
      node; node = bg_node_list_next(&node_it)) {
    if(node->id > *max_id) {
      *max_id = node->id;
    }
  }
  for(node = bg_node_list_first(graph->hidden_nodes, &node_it);
      node; node = bg_node_list_next(&node_it)) {
    if(node->id > *max_id) {
      *max_id = node->id;
    }
  }
  for(node = bg_node_list_first(graph->output_nodes, &node_it);
      node; node = bg_node_list_next(&node_it)) {
    if(node->id > *max_id) {
      *max_id = node->id;
    }
  }
  return bg_SUCCESS;
}

bg_error bg_graph_get_max_edge_id(bg_graph_t *graph, size_t *max_id) {
  bg_edge_t *edge;
  bg_edge_list_iterator_t edge_it;
  *max_id = 0;
  for(edge = bg_edge_list_first(graph->edge_list, &edge_it);
      edge; edge = bg_edge_list_next(&edge_it)) {
    if(edge->id > *max_id) {
      *max_id = edge->id;
    }
  }
  return bg_SUCCESS;
}

void determine_evaluation_order(bg_graph_t *graph) {

  unsigned long *ids=NULL;
  int i=0, relation_cnt=0;
  bg_node_t *current_node, *output_node;
  bg_node_list_t *node_list;
  bg_node_list_t *process_list;
  bg_node_list_iterator_t node_it, node_it2;
  bg_edge_t *current_edge;
  bg_edge_list_t *edge_list;
  bg_edge_list_iterator_t edge_it;

  edge_list = graph->edge_list;
  for(current_edge = bg_edge_list_first(edge_list, &edge_it);
      current_edge; current_edge = bg_edge_list_next(&edge_it)) {
    if(current_edge->source_node && current_edge->sink_node &&
       !current_edge->ignore_for_sort) {
      add_relation(current_edge->source_node->id,
                   current_edge->sink_node->id);
      ++relation_cnt;
    }
  }

  bg_node_list_init(&process_list);
  node_list = graph->input_nodes;
  for(current_node = bg_node_list_first(node_list, &node_it);
      current_node; current_node = bg_node_list_next(&node_it)) {
    bg_node_list_append(process_list, current_node);
  }

  node_list = graph->hidden_nodes;
  for(current_node = bg_node_list_first(node_list, &node_it);
      current_node; current_node = bg_node_list_next(&node_it)) {
    bg_node_list_append(process_list, current_node);
  }

  node_list = graph->output_nodes;
  for(current_node = bg_node_list_first(node_list, &node_it);
      current_node; current_node = bg_node_list_next(&node_it)) {
    bg_node_list_append(process_list, current_node);
  }

  if(relation_cnt) {
    tsort();
    ids = get_sorted_ids();
  }

  node_list = graph->evaluation_order;
  bg_node_list_clear(node_list);

  while(ids && ids[i]) {
    for(current_node = bg_node_list_first(process_list, &node_it);
        current_node; current_node = bg_node_list_next(&node_it)) {
      if(current_node->id == ids[i]) {
        bg_node_list_erase(&node_it);
        if(current_node->type->id != bg_NODE_TYPE_INPUT && current_node->type->id != bg_NODE_TYPE_OUTPUT) {
          bg_node_list_append(node_list, current_node);
        }
        break;
      }
    }
    ++i;
  }
  /* add output nodes that aren't processed yet (like unconnected outputs) */
  for(output_node = bg_node_list_first(graph->output_nodes, &node_it);
      output_node; output_node = bg_node_list_next(&node_it)) {
    for(current_node = bg_node_list_first(process_list, &node_it2);
        current_node; current_node = bg_node_list_next(&node_it2)) {
      if(current_node == output_node) {
        bg_node_list_erase(&node_it2);
        bg_node_list_append(graph->evaluation_order, output_node);
        break;
      }
    }
  }
  bg_node_list_deinit(process_list);
}
