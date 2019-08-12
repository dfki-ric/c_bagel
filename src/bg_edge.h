#ifndef C_BAGEL_EDGE_H
#define C_BAGEL_EDGE_H

#include "bg_impl.h"

bg_error bg_edge_init(bg_edge_t *edge,
                      bg_node_t *sourceNode, size_t sourcePortIdx,
                      bg_node_t *sinkNode, size_t sinkPortIdx,
                      bg_real weight);

bg_error bg_edge_deinit(bg_edge_t *edge);

#endif /* C_BAGEL_EDGE_H */
