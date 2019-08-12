#include "bg_edge.h"
#include "bg_graph.h"

bg_error bg_edge_set_weight(bg_graph_t *graph, bg_edge_id_t edge_id,
                            bg_real weight) {
  bg_edge_t *edge = NULL;
  bg_error err = bg_graph_find_edge((bg_graph_t*)graph, edge_id, &edge);
  if(err == bg_SUCCESS) {
    if(edge) {
      edge->weight = weight;
    } else {
      err = bg_error_set(bg_ERR_EDGE_NOT_FOUND);
    }
  }
  return err;
}

bg_error bg_edge_set_value(bg_graph_t *graph, bg_edge_id_t edge_id,
                           bg_real value) {
  bg_edge_t *edge = NULL;
  bg_error err = bg_graph_find_edge((bg_graph_t*)graph, edge_id, &edge);
  if(err == bg_SUCCESS) {
    if(edge) {
      edge->value = value;
    } else {
      err = bg_error_set(bg_ERR_EDGE_NOT_FOUND);
    }
  }
  return err;
}

bg_error bg_edge_set_value_p(bg_edge_t *edge, bg_real value) {
  edge->value = value;
  return bg_SUCCESS;
}

bg_error bg_edge_get_pointer(bg_graph_t *graph, bg_edge_id_t edge_id,
                             bg_edge_t **edge) {

  bg_error err = bg_graph_find_edge((bg_graph_t*)graph, edge_id, edge);
  if(err == bg_SUCCESS) {
    if(!*edge) {
      err = bg_error_set(bg_ERR_EDGE_NOT_FOUND);
    }
  }
  return err;  
}



/* introspection */
bg_error bg_edge_get_weight(const bg_graph_t *graph, bg_edge_id_t edge_id,
                            bg_real *weight) {
  bg_edge_t *edge = NULL;
  bg_error err = bg_graph_find_edge((bg_graph_t*)graph, edge_id, &edge);
  if(err == bg_SUCCESS) {
    if(edge) {
      *weight = edge->weight;
    } else {
      err = bg_error_set(bg_ERR_EDGE_NOT_FOUND);
    }
  }
  return err;
}

bg_error bg_edge_get_value(const bg_graph_t *graph, bg_edge_id_t edge_id,
                           bg_real *value) {
  bg_edge_t *edge = NULL;
  bg_error err = bg_graph_find_edge((bg_graph_t*)graph, edge_id, &edge);
  if(err == bg_SUCCESS) {
    if(edge) {
      *value = edge->value;
    } else {
      err = bg_error_set(bg_ERR_EDGE_NOT_FOUND);
    }
  }
  return err;
}

bg_error bg_edge_get_nodes(const bg_graph_t *graph, bg_edge_id_t edge_id,
                           bg_node_id_t *sourceNode, size_t *sourcePortIdx,
                           bg_node_id_t *sinkNode, size_t *sinkPortIdx) {
  bg_edge_t *edge = NULL;
  bg_error err = bg_graph_find_edge((bg_graph_t*)graph, edge_id, &edge);
  if(err != bg_SUCCESS) {
    return err;
  } else if(edge == NULL) {
    return bg_error_set(bg_ERR_EDGE_NOT_FOUND);
  }
  if(sourceNode) {
    if(edge->source_node) {
      *sourceNode = edge->source_node->id;
    } else {
      *sourceNode = 0;
    }
  }
  if(sourcePortIdx) {
    *sourcePortIdx = edge->source_port_idx;
  }
  if(sinkNode) {
    if(edge->sink_node) {
      *sinkNode = edge->sink_node->id;
    } else  {
      *sinkNode = 0;
    }
  }
  if(sinkPortIdx) {
    *sinkPortIdx = edge->sink_port_idx;
  }
  return bg_SUCCESS;
}





bg_error bg_edge_init(bg_edge_t *edge,
                      bg_node_t *sourceNode, size_t sourcePortIdx,
                      bg_node_t *sinkNode, size_t sinkPortIdx,
                      bg_real weight) {
  edge->source_node = sourceNode;
  edge->source_port_idx = sourcePortIdx;
  edge->sink_node = sinkNode;
  edge->sink_port_idx = sinkPortIdx;
  edge->weight = weight;
  edge->value = 0.;
  edge->ignore_for_sort = 0;
#ifdef INTERVAL_SUPPORT
  mpfi_init_set_d(edge->value_intv, 0.);
#endif
  return bg_SUCCESS;
}

bg_error bg_edge_deinit(bg_edge_t *edge) {
#ifdef INTERVAL_SUPPORT
  mpfi_clear(edge->value_intv);
#endif
  return bg_SUCCESS;
  (void)edge;
}
