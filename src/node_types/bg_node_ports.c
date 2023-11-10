#include "../bg_impl.h"
#include "../bg_node.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <math.h>



static bg_error init_port(bg_node_t *node) {
  bg_node_create_input_ports(node, 1);
  bg_node_create_output_ports(node, 1);
  return bg_error_get();
}

static bg_error deinit_port(bg_node_t *node) {
  bg_node_remove_input_ports(node);
  bg_node_remove_output_ports(node);
  return bg_error_get();
}

static bg_error eval_port(bg_node_t *node) {
  assert(node->input_ports && node->input_port_cnt == 1);
  assert(node->output_ports && node->output_port_cnt == 1);
  node->output_ports[0]->value = node->input_ports[0]->value;
  /*fprintf(stderr, "eval_port %lu: %g\n", node->id, node->output_ports[0]->value);*/
  return bg_SUCCESS;
}

static bg_error eval_port_interval(bg_node_t *node) {
#ifdef INTERVAL_SUPPORT
  assert(node->input_ports && node->input_port_cnt == 1);
  assert(node->output_ports && node->output_port_cnt == 1);
  mpfi_set(node->output_ports[0]->value_intv, node->input_ports[0]->value_intv);
  return bg_SUCCESS;
#else
  return bg_ERR_NOT_IMPLEMENTED;
  (void)node;
#endif
}



static node_type_t port_types[] = {
/*{type_id, name, input_cnt, output_cnt, init, deinit, eval}*/
  {bg_NODE_TYPE_INPUT, "INPUT", 1, 1, init_port, deinit_port, eval_port, eval_port_interval},
  {bg_NODE_TYPE_OUTPUT, "OUTPUT", 1, 1, init_port, deinit_port, eval_port, eval_port_interval},
  /* sentinel */
  {0, NULL, 0, 0, NULL, NULL, NULL, NULL}
};

void bg_register_port_types(void) {
  bg_node_type_register(port_types);
}
