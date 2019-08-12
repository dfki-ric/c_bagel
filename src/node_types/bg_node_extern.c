#include "../bg_impl.h"
#include "../bg_node.h"

#include <stdlib.h>
#include <assert.h>


static bg_error init_extern(bg_node_t *node) {
  node->input_port_cnt = 0;
  node->output_port_cnt = 0;
  node->input_ports = NULL;
  node->output_ports = NULL;
  return bg_SUCCESS;
}

static bg_error deinit_extern(bg_node_t *node) {
  return bg_SUCCESS;
  (void)node;
}

static bg_error eval_extern(bg_node_t *node) {
  return bg_ERR_NOT_IMPLEMENTED;
  (void)node;
}

static bg_error eval_extern_interval(bg_node_t *node) {
  return bg_ERR_NOT_IMPLEMENTED;
  (void)node;
}

static node_type_t extern_types[] = {
/*{type_id, name, input_cnt, output_cnt, init, deinit, eval}*/
  {bg_NODE_TYPE_EXTERN, "EXTERN", 0, 0, init_extern, deinit_extern, eval_extern, eval_extern_interval},
  /* sentinel */
  {0, NULL, 0, 0, NULL, NULL, NULL, NULL}
};

void bg_register_extern_types() {
  bg_node_type_register(extern_types);
}



