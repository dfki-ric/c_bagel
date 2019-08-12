#ifndef C_BAGEL_GRAPH_H
#define C_BAGEL_GRAPH_H


#include "bg_impl.h"

bg_error bg_graph_find_node(bg_graph_t *graph, bg_node_id_t node_id,
                            bg_node_t **node);
bg_error bg_graph_find_edge(bg_graph_t *graph, bg_edge_id_t edge_id,
                            bg_edge_t **edge);
bg_error bg_graph_get_max_node_id(bg_graph_t *graph, size_t *max_id);
bg_error bg_graph_get_max_edge_id(bg_graph_t *graph, size_t *max_id);
void determine_evaluation_order(bg_graph_t *graph);



#endif /* C_BAGEL_GRAPH_H */
