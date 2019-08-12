#ifndef C_BAGEL_NODE_H
#define C_BAGEL_NODE_H

#include "bg_impl.h"

bg_error bg_node_init(bg_node_t *node, const char *name, bg_node_id_t id,
                      bg_node_type node_type);
bg_error bg_node_reset(bg_node_t *node, bool recursive);
bg_error bg_node_create_input_ports(bg_node_t *node, size_t cnt);
bg_error bg_node_create_output_ports(bg_node_t *node, size_t cnt);
bg_error bg_node_remove_input_ports(bg_node_t *node);
bg_error bg_node_remove_output_ports(bg_node_t *node);
bg_error bg_node_is_connected(const bg_node_t *node, bool *is_connected);
bg_error bg_node_set_input_intern(bg_node_t *node, size_t inputPortIdx,
                                  bg_merge_type mergeType,
                                  bg_real default_value, bg_real bias,
                                  const char *name, bool clearName);
bg_error bg_node_set_output_intern(bg_node_t *node, size_t outputPortIdx,
                                   const char *name, bool clearName);
bg_error bg_node_evaluate(bg_node_t *node);
bg_error bg_node_evaluate_interval(bg_node_t *node);

#endif /* C_BAGEL_NODE_H */
