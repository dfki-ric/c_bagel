#include "bg_impl.h"

#ifdef YAML_SUPPORT


#include "generic_list.h"
#include "node_list.h"
#include "edge_list.h"
#include "node_types/bg_node_subgraph.h"

#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#include <yaml.h>


#define bg_MAX_STRING_LENGTH 255

bg_error bg_graph_to_emitter(yaml_emitter_t *emitter, const bg_graph_t *g);


static int emit_ulong(yaml_emitter_t *emitter, unsigned long x) {
  yaml_event_t event;
  int ok=1;
  char buf[bg_MAX_STRING_LENGTH];
  sprintf(buf, "%lu", x);
  ok &= yaml_scalar_event_initialize(&event, NULL, NULL, (yaml_char_t*)buf,
                                     -1, 1, 0, YAML_PLAIN_SCALAR_STYLE);
  ok &= yaml_emitter_emit(emitter, &event);
  return ok;
}

static int emit_str(yaml_emitter_t *emitter, const char *s) {
  yaml_event_t event;
  int ok=1;
  ok &= yaml_scalar_event_initialize(&event, NULL, NULL, (yaml_char_t*)s,
                                     -1, 1, 0, YAML_PLAIN_SCALAR_STYLE);
  ok &= yaml_emitter_emit(emitter, &event);
  return ok;
}

static int emit_double(yaml_emitter_t *emitter, bg_real x) {
  yaml_event_t event;
  int ok=1;
  char buf[bg_MAX_STRING_LENGTH];
  sprintf(buf, "%g", x);
  ok &= yaml_scalar_event_initialize(&event, NULL, NULL, (yaml_char_t*)buf,
                                     -1, 1, 0, YAML_PLAIN_SCALAR_STYLE);
  ok &= yaml_emitter_emit(emitter, &event);
  return ok;
}

static int emit_node_list(yaml_emitter_t *emitter, bg_node_list_t *node_list) {
  yaml_event_t event;
  bg_node_t *current_node;
  bg_node_list_iterator_t it;
  size_t i;
  int ok = 1;
  for(current_node = bg_node_list_first(node_list, &it);
      current_node; current_node = bg_node_list_next(&it)) {
    ok &= yaml_mapping_start_event_initialize(&event, NULL, NULL, 0,
                                        YAML_BLOCK_MAPPING_STYLE);
    ok &= yaml_emitter_emit(emitter, &event);
    
    ok &= emit_str(emitter, "id");
    ok &= emit_ulong(emitter, current_node->id);
    ok &= emit_str(emitter, "type");
    if(current_node->type->id == bg_NODE_TYPE_EXTERN) {
      ok &= emit_str(emitter, "EXTERN");
      ok &= emit_str(emitter, "extern_name");
      ok &= emit_str(emitter, current_node->type->name);
    }
    else if(current_node->type->id == bg_NODE_TYPE_SUBGRAPH) {
      ok &= emit_str(emitter, "SUBGRAPH");
      ok &= emit_str(emitter, "subgraph_name");
      ok &= emit_str(emitter, ((subgraph_data_t*)current_node->_priv_data)->subgraph->name);
    }
    else {
      ok &= emit_str(emitter, current_node->type->name);
    }
    if(current_node->name) {
      ok &= emit_str(emitter, "name");
      ok &= emit_str(emitter, current_node->name);
    }
    ok &= emit_str(emitter, "inputs");
    ok &= yaml_sequence_start_event_initialize(&event, NULL, NULL, 0,
                                               YAML_BLOCK_SEQUENCE_STYLE);
    ok &= yaml_emitter_emit(emitter, &event);
    if(!ok) {
      return bg_error_set(20);
    }
    for(i = 0; i < current_node->input_port_cnt; ++i) {
      ok &= yaml_mapping_start_event_initialize(&event, NULL, NULL, 0,
                                                YAML_FLOW_MAPPING_STYLE);
      ok &= yaml_emitter_emit(emitter, &event);
      ok &= emit_str(emitter, "idx");
      ok &= emit_ulong(emitter, i);
      ok &= emit_str(emitter, "type");
      ok &= emit_str(emitter, current_node->input_ports[i]->merge->name);
      ok &= emit_str(emitter, "bias");
      ok &= emit_double(emitter, current_node->input_ports[i]->bias);
      ok &= emit_str(emitter, "default");
      ok &= emit_double(emitter, current_node->input_ports[i]->defaultValue);
      if(current_node->input_ports[i]->name) {
        ok &= emit_str(emitter, "name");
        ok &= emit_str(emitter, current_node->input_ports[i]->name);
      }
      ok &= yaml_mapping_end_event_initialize(&event);
      ok &= yaml_emitter_emit(emitter, &event);
    }
    ok &= yaml_sequence_end_event_initialize(&event);
    ok &= yaml_emitter_emit(emitter, &event);

    ok &= emit_str(emitter, "outputs");
    ok &= yaml_sequence_start_event_initialize(&event, NULL, NULL, 0,
                                               YAML_BLOCK_SEQUENCE_STYLE);
    ok &= yaml_emitter_emit(emitter, &event);
    if(!ok) {
      return bg_error_set(20);
    }
    for(i = 0; i < current_node->output_port_cnt; ++i) {
      ok &= yaml_mapping_start_event_initialize(&event, NULL, NULL, 0,
                                                YAML_FLOW_MAPPING_STYLE);
      ok &= yaml_emitter_emit(emitter, &event);
      ok &= emit_str(emitter, "idx");
      ok &= emit_ulong(emitter, i);
      if(current_node->output_ports[i]->name) {
        ok &= emit_str(emitter, "name");
        ok &= emit_str(emitter, current_node->output_ports[i]->name);
      }
      ok &= yaml_mapping_end_event_initialize(&event);
      ok &= yaml_emitter_emit(emitter, &event);
    }
    ok &= yaml_sequence_end_event_initialize(&event);
    ok &= yaml_emitter_emit(emitter, &event);

    /*
    ok &= emit_str(emitter, "outputCount");
    ok &= emit_ulong(emitter, current_node->output_port_cnt);
    */

    ok &= yaml_mapping_end_event_initialize(&event);
    ok &= yaml_emitter_emit(emitter, &event);
  }
  return ok;
}

static int emit_edge_list(yaml_emitter_t *emitter, bg_edge_list_t *edge_list) {
  yaml_event_t event;
  bg_edge_t *current_edge;
  bg_edge_list_iterator_t it;
  int ok = 1;
  for(current_edge = bg_edge_list_first(edge_list, &it);
      current_edge; current_edge = bg_edge_list_next(&it)) {

    if(current_edge->source_node != 0) {

      ok &= yaml_mapping_start_event_initialize(&event, NULL, NULL, 0,
                                                YAML_FLOW_MAPPING_STYLE);
      ok &= yaml_emitter_emit(emitter, &event);
      ok &= emit_str(emitter, "fromNodeId");
      if(current_edge->source_node) {
        ok &= emit_ulong(emitter, current_edge->source_node->id);
      } else {
        ok &= emit_ulong(emitter, 0);
      }
      ok &= emit_str(emitter, "fromNodeOutputIdx");
      ok &= emit_ulong(emitter, current_edge->source_port_idx);
      ok &= emit_str(emitter, "toNodeId");
      if(current_edge->sink_node) {
        ok &= emit_ulong(emitter, current_edge->sink_node->id);
      } else {
        ok &= emit_ulong(emitter, 0);
      }
      ok &= emit_str(emitter, "toNodeInputIdx");
      ok &= emit_ulong(emitter, current_edge->sink_port_idx);

      ok &= emit_str(emitter, "weight");
      ok &= emit_double(emitter, current_edge->weight);
      ok &= emit_str(emitter, "ignore_for_sort");
      ok &= emit_ulong(emitter, current_edge->ignore_for_sort);
    
      ok &= yaml_mapping_end_event_initialize(&event);
      ok &= yaml_emitter_emit(emitter, &event);
    }
  }
  return ok;
}

bg_error bg_graph_to_yaml_file(const char *filename, const bg_graph_t *g) {
  yaml_emitter_t emitter;
  int ok = 1;
  FILE *fp;
  /*bg_error err;*/

  fp = fopen(filename, "w");
  if(!fp) {
    return bg_error_set(bg_ERR_UNKNOWN);
  }
  ok &= yaml_emitter_initialize(&emitter);
  yaml_emitter_set_output_file(&emitter, fp);

  ok &= bg_graph_to_emitter(&emitter, g);
  fclose(fp);
  return ok;
}

bg_error bg_graph_to_yaml_string(unsigned char *buffer, size_t buffer_size,
                                 const bg_graph_t *g, size_t *bytes_written) {
  yaml_emitter_t emitter;
  /*int ok = 1;*/
  yaml_emitter_initialize(&emitter);
  yaml_emitter_set_output_string(&emitter, buffer, buffer_size, bytes_written);
  return bg_graph_to_emitter(&emitter, g);
}

bg_error bg_graph_to_emitter(yaml_emitter_t *emitter, const bg_graph_t *g) {
  yaml_event_t event;
  int ok = 1;
  ok &= yaml_stream_start_event_initialize(&event, YAML_UTF8_ENCODING);
  ok &= yaml_emitter_emit(emitter, &event);
  ok &= yaml_document_start_event_initialize(&event, NULL, NULL, 0, 1);
  ok &= yaml_emitter_emit(emitter, &event);
  ok &= yaml_mapping_start_event_initialize(&event, NULL, NULL, 0,
                                            YAML_BLOCK_MAPPING_STYLE);
  ok &= yaml_emitter_emit(emitter, &event);
  if(!ok) {
    return bg_error_set(bg_ERR_UNKNOWN);
  }
  /* emit nodes */
  ok &= emit_str(emitter, "nodes");
  ok &= yaml_sequence_start_event_initialize(&event, NULL, NULL, 0,
                                             YAML_BLOCK_SEQUENCE_STYLE);
  ok &= yaml_emitter_emit(emitter, &event);
  ok &= emit_node_list(emitter, g->input_nodes);
  ok &= emit_node_list(emitter, g->hidden_nodes);
  ok &= emit_node_list(emitter, g->output_nodes);
  ok &= yaml_sequence_end_event_initialize(&event);
  ok &= yaml_emitter_emit(emitter, &event);
  /* emit edges */
  if(bg_list_size(g->edge_list) > 0) {
    ok &= emit_str(emitter, "edges");
    ok &= yaml_sequence_start_event_initialize(&event, NULL, NULL, 0,
                                               YAML_BLOCK_SEQUENCE_STYLE);
    ok &= yaml_emitter_emit(emitter, &event);
    ok &= emit_edge_list(emitter, g->edge_list);
    ok &= yaml_sequence_end_event_initialize(&event);
    ok &= yaml_emitter_emit(emitter, &event);
  }
  /* finalize */
  if(!ok) {
    return bg_error_set(10);
  }
  ok &= yaml_mapping_end_event_initialize(&event);
  ok &= yaml_emitter_emit(emitter, &event);
  ok &= yaml_document_end_event_initialize(&event, 1);
  if(!ok) {
    return bg_error_set(8);
  }
  ok &= yaml_emitter_emit(emitter, &event);
  ok &= yaml_stream_end_event_initialize(&event);
  ok &= yaml_emitter_emit(emitter, &event);
  yaml_emitter_delete(emitter);
  if(!ok) {
    fprintf(stderr, "YAML Error: %s\n", emitter->problem);
    return bg_error_set(5);
  }
  return bg_SUCCESS;
}



#else /* ifdef YAML_SUPPORT */

bg_error bg_graph_to_yaml_file(const char *filename, const bg_graph_t *g) {
  return bg_ERR_NOT_IMPLEMENTED;
  (void)filename;
  (void)g;
}

#endif  /* YAML_SUPPORT */
