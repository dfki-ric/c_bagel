#include "bagel.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

extern bg_error bg_net_from_yaml_file(const char *filename);


int main() {
  bg_edge_id_t *edges;
  bg_node_id_t *inputs;
  bg_graph_t *net;
  bg_node_id_t node1=1, node2=2, node, output=6;
  bg_edge_id_t tmpEdge;
  bg_real outputValue, defaultValue, bias, weight;
  size_t inputCnt, nodeCnt, edgeCnt;
  bg_node_type nodeType;
  bg_merge_type mergeType;
  size_t inputIdx=0, outputIdx=0, inputPortCnt, outputPortCnt;
  bg_node_id_t sourceNode, sinkNode;
  size_t sourcePortIdx, sinkPortIdx;

  bg_error err = bg_SUCCESS;

  bg_initialize();

  /*  bg_net_from_yaml_file("test.yml");*/
  
  
  err |= bg_graph_alloc(&net, "simple_test");
  assert(err == bg_SUCCESS);
  err |= bg_graph_create_input(net, "x", node1);
  err |= bg_graph_create_input(net, "y", node2);
  assert(err == bg_SUCCESS);
  err |= bg_graph_create_output(net, "result", output);
  err |= bg_node_set_merge(net, output, 0, bg_MERGE_TYPE_SUM, 0., 0.);
  assert(err == bg_SUCCESS);
  err |= bg_graph_create_node(net, "square_x", 3, bg_NODE_TYPE_PIPE);
  assert(err == bg_SUCCESS);
  err |= bg_node_set_merge(net, 3, 0, bg_MERGE_TYPE_PRODUCT, 1., 1.);
  assert(err == bg_SUCCESS);
  err |= bg_node_set_default(net, node1, 0, 0);
  assert(err == bg_SUCCESS);
  err |= bg_node_set_bias(net, node1, 0, 1.);
  assert(err == bg_SUCCESS);
  err |= bg_graph_create_node(net, "square_y", 4, bg_NODE_TYPE_PIPE);
  err |= bg_node_set_merge(net, 4, 0, bg_MERGE_TYPE_PRODUCT, 1., 1.);
  assert(err == bg_SUCCESS);
  err |= bg_graph_create_edge(net, node1, 0, 3, 0, 1., 0);
  err |= bg_graph_create_edge(net, node1, 0, 3, 0, 1., 0);
  err |= bg_graph_create_edge(net, node2, 0, 4, 0, 1., 0);
  err |= bg_graph_create_edge(net, node2, 0, 4, 0, 1., 0);
  err |= bg_graph_create_edge(net, 3, 0, output, 0, 1., 1);
  err |= bg_graph_create_edge(net, 4, 0, output, 0, 1., 2);
  assert(err == bg_SUCCESS);

  tmpEdge = 3;
  err |= bg_graph_create_edge(net, 2, 0, output, 0, 1., tmpEdge);
  assert(err == bg_SUCCESS);
  err |= bg_edge_set_weight(net, tmpEdge, 2.5);
  assert(err == bg_SUCCESS);
  err |= bg_graph_remove_edge(net, tmpEdge);
  assert(err == bg_SUCCESS);

  err |= bg_graph_get_node_cnt(net, false, &nodeCnt);
  assert(err == bg_SUCCESS);
  err |= bg_graph_get_edge_cnt(net, false, &edgeCnt);
  assert(err == bg_SUCCESS);
  assert(nodeCnt == 5);
  assert(edgeCnt == 6);

  /* test it */
  err |= bg_graph_create_edge(net, 0, 0, node1, 0, 1., 37);
  assert(err == bg_SUCCESS);
  err |= bg_graph_create_edge(net, 0, 0, node2, 0, 1., 42);
  assert(err == bg_SUCCESS);
  err |= bg_edge_set_value(net, 37, 2.5);
  err |= bg_edge_set_value(net, 42, 0.5);
  assert(err == bg_SUCCESS);
  err |= bg_graph_evaluate(net);
  assert(err == bg_SUCCESS);
  err |= bg_node_get_output(net, output, 0, &outputValue);

  assert(err == bg_SUCCESS);
  printf("output: %g\n", outputValue);
  assert(outputValue == 3.5*3.5 + 0.5*0.5);

  /* introspection */
  node = 2;
  err |= bg_node_get_type(net, node, &nodeType);
  assert(err == bg_SUCCESS);
  err |= bg_node_get_merge(net, node, inputIdx, &mergeType);
  assert(err == bg_SUCCESS);
  err |= bg_node_get_default(net, node, inputIdx, &defaultValue);
  assert(err == bg_SUCCESS);
  err |= bg_node_get_bias(net, node, inputIdx, &bias);
  assert(err == bg_SUCCESS);
  err |= bg_node_get_input_edges(net, node, inputIdx, NULL, &edgeCnt);
  assert(err == bg_SUCCESS);
  edges = (bg_edge_id_t*)malloc(edgeCnt * sizeof(bg_edge_id_t));
  err |= bg_node_get_input_edges(net, node, inputIdx, edges, &edgeCnt);
  assert(err == bg_SUCCESS);
  /* get only the number of connected output edges */
  err |= bg_node_get_output_edges(net, node, outputIdx, NULL, &edgeCnt);
  assert(err == bg_SUCCESS);
  err |= bg_node_get_port_cnt(net, node, &inputPortCnt, &outputPortCnt);
  assert(err == bg_SUCCESS);
  /* introspect an edge */
  err |= bg_edge_get_weight(net, edges[0], &weight);
  assert(err == bg_SUCCESS);
  err |= bg_edge_get_nodes(net, edges[0], &sourceNode, &sourcePortIdx,
                           &sinkNode, &sinkPortIdx);
  assert(err == bg_SUCCESS);

  
  err |= bg_graph_get_input_nodes(net, NULL, &inputCnt);
  assert(err == bg_SUCCESS);
  inputs = (bg_node_id_t*)malloc(inputCnt * sizeof(bg_node_id_t));
  err |= bg_graph_get_input_nodes(net, inputs, &inputCnt);
  assert(err == bg_SUCCESS);

  free(inputs);
  bg_graph_free(net);

  err |= bg_edge_get_weight(net, edges[0], &weight);
  fprintf(stderr, "da %g\n", weight);
  free(edges);

  return 0;
}
