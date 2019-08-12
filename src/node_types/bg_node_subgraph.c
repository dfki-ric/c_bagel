#include "bg_node_subgraph.h"

#include <stdlib.h>
#include <assert.h>


static bg_error init_subgraph(bg_node_t *node) {
  subgraph_data_t *subgraph_data;
  node->input_port_cnt = 0;
  node->output_port_cnt = 0;
  node->input_ports = NULL;
  node->output_ports = NULL;
  subgraph_data = (subgraph_data_t*)calloc(1, sizeof(subgraph_data_t));
  if(!subgraph_data) {
    return bg_error_set(bg_ERR_NO_MEMORY);
  }
  node->_priv_data = subgraph_data;
  return bg_SUCCESS;
}

static bg_error deinit_subgraph(bg_node_t *node) {
  bg_graph_t *subgraph = ((subgraph_data_t*)node->_priv_data)->subgraph;
  bg_error err = bg_SUCCESS;
  if(subgraph) {
    err = bg_graph_free(subgraph);
  }
  free(node->_priv_data);
  return err;
}

static bg_error eval_subgraph(bg_node_t *node) {
  bg_graph_t *subgraph = ((subgraph_data_t*)node->_priv_data)->subgraph;
  if(subgraph) {
    return bg_graph_evaluate(subgraph);
  } else {
    return bg_SUCCESS;
  }
}

static bg_error eval_subgraph_interval(bg_node_t *node) {
#ifdef INTERVAL_SUPPORT
  bg_graph_t *subgraph = ((subgraph_data_t*)node->_priv_data)->subgraph;
  if(subgraph) {
    return bg_graph_evaluate(subgraph);
  } else {
    return bg_SUCCESS;
  }
#else
  return bg_ERR_NOT_IMPLEMENTED;
  (void)node;
#endif
}

static node_type_t subgraph_types[] = {
/*{type_id, name, input_cnt, output_cnt, init, deinit, eval}*/
  {bg_NODE_TYPE_SUBGRAPH, "SUBGRAPH", 0, 0, init_subgraph, deinit_subgraph, eval_subgraph, eval_subgraph_interval},
  /* sentinel */
  {0, NULL, 0, 0, NULL, NULL, NULL, NULL}
};

void bg_register_subgraph_types() {
  bg_node_type_register(subgraph_types);
}



