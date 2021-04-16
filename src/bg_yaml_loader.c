#include "bg_impl.h"
#include "bg_graph.h"

#ifdef YAML_SUPPORT


#include "generic_list.h"

#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <yaml.h>


int indent_level = 0;
/*char *library_path = "graph_library/";*/
char *library_path = NULL;
char bg_yaml_loader_error_message[bg_MAX_STRING_LENGTH];

typedef struct input_struct {
  char merge_str[bg_MAX_STRING_LENGTH];
  bg_merge_type merge;
  bg_real defaultValue;
  bg_real bias;
  char *name;
} input_struct;

typedef struct output_struct {
  char *name;
} output_struct;

typedef struct node_struct {
  unsigned long id;
  char type_str[bg_MAX_STRING_LENGTH];
  bg_node_type type;
  char name[bg_MAX_STRING_LENGTH];
  input_struct inputs[bg_MAX_PORTS];
  output_struct outputs[bg_MAX_PORTS];
  size_t input_cnt;
  size_t output_cnt;
} node_struct;

typedef struct edge_struct {
  unsigned long sourceId;
  size_t sourcePortIdx;
  unsigned long sinkId;
  size_t sinkPortIdx;
  bg_real weight;
  unsigned long ignore_for_sort;
  char from_name[bg_MAX_STRING_LENGTH];
  char from_out_name[bg_MAX_STRING_LENGTH];
  char to_name[bg_MAX_STRING_LENGTH];
  char to_in_name[bg_MAX_STRING_LENGTH];
} edge_struct;


LIST_TYPE_DEF(edge_struct, edge_struct)
LIST_TYPE_IMPL(edge_struct, edge_struct)


typedef enum { bg_PARSE_STATE_UNDEF,
               bg_PARSE_STATE_TOP,
               bg_PARSE_STATE_NODES,
               bg_PARSE_STATE_EDGES,
               bg_PARSE_STATE_INPUTS,
               bg_PARSE_STATE_NODE,
               bg_PARSE_STATE_NODE_INPUTS,
               bg_PARSE_STATE_NODE_OUTPUTS,
               bg_PARSE_STATE_EDGE } bg_parse_state;

static bg_error get_event(yaml_parser_t *parser, yaml_event_t *event,
                          yaml_event_type_t type);
static bg_error parse_nodes(yaml_parser_t *parser, bg_graph_t *g);
static bg_error parse_node(yaml_parser_t *parser, bg_graph_t *g);
static bg_error parse_node_id(yaml_parser_t *parser, unsigned long *id);
static bg_error parse_node_type(yaml_parser_t *parser,
                                bg_node_type *type);
static bg_error parse_node_inputs(yaml_parser_t *parser, input_struct *inputs,
                                  size_t *input_cnt);
static bg_error parse_node_outputs(yaml_parser_t *parser,
                                   output_struct *outputs,
                                   size_t *output_cnt);
static bg_error parse_edges(yaml_parser_t *parser, bg_edge_struct_list_t *edges);

static bg_error get_event(yaml_parser_t *parser, yaml_event_t *event,
                          yaml_event_type_t type) {
  if(!yaml_parser_parse(parser, event)) {
    fprintf(stderr, "ERROR!, YAML parser encountered an error.\n");
    return bg_error_set(bg_ERR_UNKNOWN);
  }
  if(event->type != type) {
    fprintf(stderr, "ERROR!, expected %d got %d.\n",
            type, event->type);

    fprintf(stderr, "YAML_SCALAR_EVENT: %d\n", YAML_SCALAR_EVENT);
    fprintf(stderr, "YAML_SEQUENCE_START_EVENT: %d\n", YAML_SEQUENCE_START_EVENT);
    fprintf(stderr, "YAML_SEQUENCE_END_EVENT: %d\n", YAML_SEQUENCE_END_EVENT);
    fprintf(stderr, "YAML_MAPPING_START_EVENT: %d\n", YAML_MAPPING_START_EVENT);
    fprintf(stderr, "YAML_MAPPING_END_EVENT: %d\n", YAML_MAPPING_END_EVENT);

    return bg_error_set(bg_ERR_UNKNOWN);
  }
  return bg_error_set(bg_SUCCESS);
}

static bg_error get_int(yaml_parser_t *parser, unsigned long *result) {
  bg_error err;
  yaml_event_t event;
  err = get_event(parser, &event, YAML_SCALAR_EVENT);
  if(err != bg_SUCCESS) {
    /*fprintf(stderr, "ERROR: get_int\n");*/
    return bg_error_set(err);
  }
  *result = atol((const char*)event.data.scalar.value);
  yaml_event_delete(&event);
  return bg_error_set(bg_SUCCESS);
}

static bg_error get_double(yaml_parser_t *parser, bg_real *result) {
  bg_error err;
  yaml_event_t event;
  err = get_event(parser, &event, YAML_SCALAR_EVENT);
  if(err != bg_SUCCESS) {
    /*fprintf(stderr, "ERROR: get_double\n");*/
    return bg_error_set(err);
  }
  *result = atof((const char*)event.data.scalar.value);
  yaml_event_delete(&event);
  return bg_error_set(bg_SUCCESS);
}

static bg_error get_string(yaml_parser_t *parser, char s[bg_MAX_STRING_LENGTH]) {
  bg_error err;
  yaml_event_t event;
  err = get_event(parser, &event, YAML_SCALAR_EVENT);
  if(err != bg_SUCCESS) {
    /*fprintf(stderr, "ERROR: get_string\n");*/
    return bg_error_set(err);
  }
  /*strncpy(s, (const char*)event.data.scalar.value, bg_MAX_STRING_LENGTH);*/
  memcpy(s, (const char*)event.data.scalar.value, bg_MAX_STRING_LENGTH*sizeof(char));
  yaml_event_delete(&event);
  return bg_error_set(bg_SUCCESS);
}

static bg_error _skip_node_helper(yaml_parser_t *parser, yaml_event_t *event,
                                  bool is_nested) {
  bg_error err = bg_SUCCESS;
  yaml_event_type_t type;
  yaml_event_t next_event;
  bool done = false;
  type = event->type;
  switch(type) {
  case YAML_SEQUENCE_START_EVENT:
    do {
      if(!yaml_parser_parse(parser, &next_event)) {
        fprintf(stderr, "ERROR!, YAML parser encountered an error.\n");
        return bg_error_set(bg_ERR_UNKNOWN);
      }
      err = _skip_node_helper(parser, &next_event, true);
      if(next_event.type == YAML_SEQUENCE_END_EVENT) {
        done = true;
      }
      yaml_event_delete(&next_event);
    } while(!done);
    break;
  case YAML_MAPPING_START_EVENT:
    do {
      if(!yaml_parser_parse(parser, &next_event)) {
        fprintf(stderr, "ERROR!, YAML parser encountered an error.\n");
        return bg_error_set(bg_ERR_UNKNOWN);
      }
      err = _skip_node_helper(parser, &next_event, true);
      if(next_event.type == YAML_MAPPING_END_EVENT) {
        done = true;
      }
      yaml_event_delete(&next_event);
    } while(!done);
    break;
  case YAML_SEQUENCE_END_EVENT:
  case YAML_MAPPING_END_EVENT:
    break;
  case YAML_ALIAS_EVENT:
  case YAML_SCALAR_EVENT:
    break;
    if(is_nested) {
      if(!yaml_parser_parse(parser, &next_event)) {
        fprintf(stderr, "ERROR!, YAML parser encountered an error.\n");
        return bg_error_set(bg_ERR_UNKNOWN);
      }
      err = _skip_node_helper(parser, &next_event, is_nested);
      yaml_event_delete(&next_event);
    }
    break;
  case YAML_NO_EVENT:
    fprintf(stderr, "What is a YAML NO_EVENT? Let's skip the next node.\n");
    return bg_error_set(bg_ERR_UNKNOWN);
  default:
    fprintf(stderr, "unhandled event: %d.\n", event->type);
    return bg_error_set(bg_ERR_UNKNOWN);
  }
  return bg_error_set(err);
}

static bg_error skip_node(yaml_parser_t *parser, yaml_event_t *event) {
  return bg_error_set(_skip_node_helper(parser, event, false));
}

static bg_error skip_next_node(yaml_parser_t *parser, yaml_event_t *event) {
  bg_error err;
  yaml_event_t next_event;
  if(!yaml_parser_parse(parser, &next_event)) {
    fprintf(stderr, "ERROR!, YAML parser encountered an error.\n");
    return bg_error_set(bg_ERR_UNKNOWN);
  }
  err = skip_node(parser, &next_event);
  yaml_event_delete(&next_event);
  return err;
  (void)event;
}

static bg_error parse_nodes(yaml_parser_t *parser, bg_graph_t *g) {
  bg_error err = bg_SUCCESS;
  yaml_event_t event;
  err = get_event(parser, &event, YAML_SEQUENCE_START_EVENT);
  yaml_event_delete(&event);
  err = get_event(parser, &event, YAML_MAPPING_START_EVENT);
  while(err == bg_SUCCESS && event.type != YAML_SEQUENCE_END_EVENT) {
    switch(event.type) {
    case YAML_MAPPING_START_EVENT:
      err = parse_node(parser, g);
      break;
    default:
      fprintf(stderr, "unexpected sequence \"%d\"\n.", event.type);
      break;
    }
    yaml_event_delete(&event);
    if(!yaml_parser_parse(parser, &event)) {
      fprintf(stderr, "ERROR!, YAML parser encountered an error.\n");
      err = bg_error_set(bg_ERR_UNKNOWN);
    }
  }
  if(err != bg_SUCCESS) {
    fprintf(stderr, "bg error: %d\n", err);
  }
  yaml_event_delete(&event);
  return err;
}

static bg_error parse_node(yaml_parser_t *parser, bg_graph_t *g) {
  size_t i;
  bg_error err;
  yaml_event_t event;
  char done_ID = 0x01;
  char done_TYPE = 0x02;
  char done_INPUTS = 0x04;
  char done_SUBGRAPH_NAME = 0x08;
  char done_EXTERN_NAME = 0x10;
  char done_NODE_NAME = 0x20;
  char done_OUTPUTS = 0x40;
  char done = 0;
  node_struct node;
  char subgraph_name[bg_MAX_STRING_LENGTH];
  char extern_name[bg_MAX_STRING_LENGTH];
  char node_name[bg_MAX_STRING_LENGTH];
  char err_message[bg_MAX_STRING_LENGTH];

  node_name[0] = '\0';
  node.input_cnt = 0;
  node.output_cnt = 0;
  /*fprintf(stderr, "parse node...\n");*/
  err = get_event(parser, &event, YAML_SCALAR_EVENT);
  while(err == bg_SUCCESS && event.type != YAML_MAPPING_END_EVENT) {
    /*fprintf(stderr, "parse node   ...\n");*/
    if(strcmp((const char*)event.data.scalar.value, "id") == 0) {
      if(done & done_ID) {
        fprintf(stderr, "ERROR! multiple \"id\" sections.\n");
      }
      err = parse_node_id(parser, &node.id);
      /*fprintf(stderr, "node id: %lu\n", node.id);*/
      done |= done_ID;
    } else if(strcmp((const char*)event.data.scalar.value, "type") == 0) {
      if(done & done_TYPE) {
        fprintf(stderr, "ERROR! multiple \"type\" sections.\n");
      }
      err = parse_node_type(parser, &node.type);
      done |= done_TYPE;
    } else if(strcmp((const char*)event.data.scalar.value, "inputs") == 0) {
      if(done & done_INPUTS) {
        fprintf(stderr, "ERROR! multiple \"inputs\" sections.\n");
      }
      err = parse_node_inputs(parser, node.inputs, &node.input_cnt);
      done |= done_INPUTS;
    } else if(strcmp((const char*)event.data.scalar.value, "outputs") == 0) {
      if(done & done_OUTPUTS) {
        fprintf(stderr, "ERROR! multiple \"outputs\" sections.\n");
      }
      err = parse_node_outputs(parser, node.outputs, &node.output_cnt);
      done |= done_OUTPUTS;
    } else if(strcmp((const char*)event.data.scalar.value, "subgraph_name") == 0) {
      if(done & done_SUBGRAPH_NAME) {
        fprintf(stderr, "ERROR! multiple \"subgraph_name\" sections.\n");
      }
      err = get_string(parser, subgraph_name);
      done |= done_SUBGRAPH_NAME;
    } else if(strcmp((const char*)event.data.scalar.value, "extern_name") == 0) {
      if(done & done_EXTERN_NAME) {
        fprintf(stderr, "ERROR! multiple \"extern_name\" sections.\n");
      }
      err = get_string(parser, extern_name);
      done |= done_EXTERN_NAME;
    } else if(strcmp((const char*)event.data.scalar.value, "name") == 0) {
      if(done & done_NODE_NAME) {
        fprintf(stderr, "ERROR! multiple \"name\" sections.\n");
      }
      err = get_string(parser, node_name);
      /*fprintf(stderr, "node name: %s\n", node_name);*/
      done |= done_NODE_NAME;
    } else if(strcmp((const char*)event.data.scalar.value, "outputCount") == 0){
      /*fprintf(stderr, "WARNING: ignoring deprecated section \"outputCount\".\n");*/
      skip_next_node(parser, &event);
    } else {
      /*fprintf(stderr, "WARNING: unknown key: \"%s\".\n", event.data.scalar.value);*/
      skip_node(parser, &event);
      skip_next_node(parser, &event);
      /*err = bg_error_set(bg_ERR_UNKNOWN);*/
    }
    yaml_event_delete(&event);
    if(!yaml_parser_parse(parser, &event)) {
      fprintf(stderr, "ERROR!, YAML parser encountered an error.\n");
      err = bg_error_set(bg_ERR_UNKNOWN);
    }
  }
  if(err == bg_SUCCESS) {
    yaml_event_delete(&event);
    /* some node id handling too allow node identification via name */
    if(!(done&done_ID)) {
      node.id = g->next_id++;
    }
    if(!(done&done_NODE_NAME) ) {
      sprintf(node_name, "node_%lu", node.id);
    }
    if(node.type == bg_NODE_TYPE_INPUT) {
      err = bg_graph_create_input(g, node_name, node.id);
      if(err != bg_SUCCESS) {
        printf("error while creating input node: %d %d %lu\n", err, bg_ERR_OUT_OF_RANGE, node.id);
      }
    } else if(node.type == bg_NODE_TYPE_OUTPUT) {
      bg_graph_create_output(g, node_name, node.id);
    } else {
      bg_graph_create_node(g, node_name, node.id, node.type);
      if(node.type == bg_NODE_TYPE_SUBGRAPH) {

        bg_graph_t *sub_g;
        bg_graph_alloc(&sub_g, subgraph_name);
        bg_graph_set_load_path(sub_g, g->load_path);
        err = bg_graph_from_yaml_file(subgraph_name, sub_g);
        if(err != bg_SUCCESS) {
          printf("error while loading subgraph node: %s %d\n", subgraph_name,
                 err);
        }
        /*
        else {
          printf("finished subgraph %s\n", subgraph_name);
        }
        */
        bg_node_set_subgraph(g, node.id, sub_g);
      }
      if(node.type == bg_NODE_TYPE_EXTERN) {
        err = bg_node_set_extern(g, node.id, extern_name);
        /*fprintf(stderr, "set extern node: %lu %s\n", node.id, extern_name);*/
        if(err != bg_SUCCESS) {
          bg_error_message_get(err, err_message);
          printf("error while setting extern node: %d - %s (node.id: %lu, extern_name: %s)\n", err, err_message, node.id, extern_name);
        }
      }
    }
    for(i = 0; i < node.input_cnt; ++i) {
      err = bg_node_set_input(g, node.id, i, node.inputs[i].merge,
                              node.inputs[i].defaultValue,
                              node.inputs[i].bias,
                              node.inputs[i].name);
      free(node.inputs[i].name);
      if(err != bg_SUCCESS) {
        bg_error_message_get(err, bg_yaml_loader_error_message);
        fprintf(stderr, "[parse_node] error in set_input: %s %lu %s\n", node_name,
               (unsigned long) i, bg_yaml_loader_error_message);
      }
    }
    for(i = 0; i < node.output_cnt; ++i) {
      err = bg_node_set_output(g, node.id, i, node.outputs[i].name);
      free(node.outputs[i].name);
      if(err != bg_SUCCESS) {
        bg_error_message_get(err, bg_yaml_loader_error_message);
        fprintf(stderr, "[parse_node] error in set_output: %s %lu %s\n", node_name,
               (unsigned long) i, bg_yaml_loader_error_message);
      }
    }
  }
  return err;
}

static bg_error parse_node_id(yaml_parser_t *parser, unsigned long *id) {
  return get_int(parser, id);
}

static bg_error parse_node_type(yaml_parser_t *parser,
                                bg_node_type *type) {
  bg_error err = bg_SUCCESS;
  char type_str[bg_MAX_STRING_LENGTH];
  bg_error_set(get_string(parser, type_str));
  if(strcmp("SUBGRAPH", type_str) == 0) {
    *type = bg_NODE_TYPE_SUBGRAPH;
  } else if(strcmp("INPUT", type_str) == 0) {
    *type = bg_NODE_TYPE_INPUT;
  } else if(strcmp("OUTPUT", type_str) == 0) {
    *type = bg_NODE_TYPE_OUTPUT;
  } else if(strcmp("PIPE", type_str) == 0) {
    *type = bg_NODE_TYPE_PIPE;
  } else if(strcmp("DIVIDE", type_str) == 0) {
    *type = bg_NODE_TYPE_DIVIDE;
  } else if(strcmp("SIN", type_str) == 0) {
    *type = bg_NODE_TYPE_SIN;
  } else if(strcmp("ASIN", type_str) == 0) {
    *type = bg_NODE_TYPE_ASIN;
  } else if(strcmp("COS", type_str) == 0) {
    *type = bg_NODE_TYPE_COS;
  } else if(strcmp("TAN", type_str) == 0) {
    *type = bg_NODE_TYPE_TAN;
  } else if(strcmp("ACOS", type_str) == 0) {
    *type = bg_NODE_TYPE_ACOS;
  } else if(strcmp("ATAN2", type_str) == 0) {
    *type = bg_NODE_TYPE_ATAN2;
  } else if(strcmp("POW", type_str) == 0) {
    *type = bg_NODE_TYPE_POW;
  } else if(strcmp("MOD", type_str) == 0) {
    *type = bg_NODE_TYPE_MOD;
  } else if(strcmp("ABS", type_str) == 0) {
    *type = bg_NODE_TYPE_ABS;
  } else if(strcmp("SQRT", type_str) == 0) {
    *type = bg_NODE_TYPE_SQRT;
  } else if(strcmp("FSIGMOID", type_str) == 0) {
    *type = bg_NODE_TYPE_FSIGMOID;
  } else if(strcmp("TANH", type_str) == 0) {
    *type = bg_NODE_TYPE_TANH;
  } else if(strcmp(">0", type_str) == 0) {
    *type = bg_NODE_TYPE_GREATER_THAN_0;
  } else if(strcmp("==0", type_str) == 0) {
    *type = bg_NODE_TYPE_EQUAL_TO_0;
  } else if(strcmp("EXTERN", type_str) == 0) {
    *type = bg_NODE_TYPE_EXTERN;
  } else {
    fprintf(stderr, "error parsing node type \"%s\".\n", type_str);
    err = bg_error_set(bg_ERR_UNKNOWN);
  }
  return err;
}

static bg_error parse_merge_type(yaml_parser_t *parser,
                                 bg_merge_type *merge) {
  bg_error err = bg_SUCCESS;
  char merge_str[bg_MAX_STRING_LENGTH];
  err = get_string(parser, merge_str);
  if(strcmp("SUM", merge_str) == 0) {
    *merge = bg_MERGE_TYPE_SUM;
  } else if(strcmp("WEIGHTED_SUM", merge_str) == 0) {
    *merge = bg_MERGE_TYPE_WEIGHTED_SUM;
  } else if(strcmp("PRODUCT", merge_str) == 0) {
    *merge = bg_MERGE_TYPE_PRODUCT;
  } else if(strcmp("MIN", merge_str) == 0) {
    *merge = bg_MERGE_TYPE_MIN;
  } else if(strcmp("MAX", merge_str) == 0) {
    *merge = bg_MERGE_TYPE_MAX;
  } else if(strcmp("MEDIAN", merge_str) == 0) {
    *merge = bg_MERGE_TYPE_MEDIAN;
  } else if(strcmp("MEAN", merge_str) == 0) {
    *merge = bg_MERGE_TYPE_MEAN;
  } else if(strcmp("NORM", merge_str) == 0) {
    *merge = bg_MERGE_TYPE_NORM;
  } else {
    fprintf(stderr, "error parsing merge type \"%s\".\n", merge_str);
    err = bg_error_set(bg_ERR_UNKNOWN);
  }
  return err;
}

static bg_error parse_node_inputs(yaml_parser_t *parser,
                                  input_struct *inputs, size_t *input_cnt) {
  bg_error err;
  yaml_event_t event;
  const char done_TYPE = 1<<1;
  const char done_DEFAULT = 1<<2;
  const char done_BIAS = 1<<3;
  const char done_INPUT_NAME = 1<<4;
  char input_name[bg_MAX_STRING_LENGTH];
  char done = 0;
  size_t cnt = 0;
  bool single_input = false;

  if(!yaml_parser_parse(parser, &event)) {
    fprintf(stderr, "ERROR!, YAML parser encountered an error.\n");
    return bg_error_set(bg_ERR_UNKNOWN);
  }
  if(event.type == YAML_MAPPING_START_EVENT) {
    single_input = true;
    err = bg_SUCCESS;
  }
  else {
    err = get_event(parser, &event, YAML_MAPPING_START_EVENT);
  }
  yaml_event_delete(&event);

  /*
    err = get_event(parser, &event, YAML_SEQUENCE_START_EVENT);
  yaml_event_delete(&event);
  err = get_event(parser, &event, YAML_MAPPING_START_EVENT);
  */

  /*fprintf(stderr, "parse nodes inputs...");*/

  while(err == bg_SUCCESS && event.type != YAML_SEQUENCE_END_EVENT) {
    if(cnt >= bg_MAX_PORTS) {
      fprintf(stderr, "ERROR: to many inputs\n");
      err = bg_error_set(bg_ERR_UNKNOWN);
      break;
    }
    yaml_event_delete(&event);
    err = get_event(parser, &event, YAML_SCALAR_EVENT);
    done = 0;
    while(err == bg_SUCCESS && event.type != YAML_MAPPING_END_EVENT) {
      if(strcmp((const char*)event.data.scalar.value, "default") == 0) {
        if(done & done_DEFAULT) {
          fprintf(stderr, "ERROR! multiple \"default\" sections.\n");
          err = bg_error_set(bg_ERR_UNKNOWN);
        } else {
          err = get_double(parser, &inputs[cnt].defaultValue);
          done |= done_DEFAULT;
        }
      } else if(strcmp((const char*)event.data.scalar.value, "bias") == 0) {
        if(done & done_BIAS) {
          fprintf(stderr, "ERROR! multiple \"bias\" sections.\n");
          err = bg_error_set(bg_ERR_UNKNOWN);
        } else {
          err = get_double(parser, &inputs[cnt].bias);
          done |= done_BIAS;
          /*fprintf(stderr, "read bias: %g\n", inputs[cnt].bias);*/
        }
      } else if(strcmp((const char*)event.data.scalar.value, "type") == 0) {
        if(done & done_TYPE) {
          fprintf(stderr, "ERROR! multiple \"type\" sections.\n");
          err = bg_error_set(bg_ERR_UNKNOWN);
        } else {
          err = parse_merge_type(parser, &inputs[cnt].merge);
          done |= done_TYPE;
        }
      } else if(strcmp((const char*)event.data.scalar.value, "name") == 0) {
        if(done & done_INPUT_NAME) {
          fprintf(stderr, "ERROR! multiple \"name\" sections.\n");
          err = bg_error_set(bg_ERR_UNKNOWN);
        } else {
          err = get_string(parser, input_name);
          /*fprintf(stderr, "node name: %s\n", node_name);*/
          done |= done_INPUT_NAME;
        }
      } else {
        /*fprintf(stderr, "WARNING! Ignoring unexpected section: %s\n",
          (const char*)event.data.scalar.value);*/
        skip_next_node(parser, &event);
        /*skip_node(parser, &event);*/
      }
      yaml_event_delete(&event);
      if(!yaml_parser_parse(parser, &event)) {
        fprintf(stderr, "ERROR!, YAML parser encountered an error.\n");
        err = bg_error_set(bg_ERR_UNKNOWN);
      }
    }
    if(!(done & done_INPUT_NAME)) {
      sprintf(input_name, "in_%05lu", (unsigned long)cnt);
    }
    {
      char *name_copy;
      name_copy = malloc(strlen(input_name)+1);
      strcpy(name_copy, input_name);
      inputs[cnt].name = name_copy;
    }
    if(single_input) {
      event.type = YAML_SEQUENCE_END_EVENT;
    }
    else {
      yaml_event_delete(&event);
      if(!yaml_parser_parse(parser, &event)) {
        fprintf(stderr, "ERROR!, YAML parser encountered an error.\n");
        err = bg_error_set(bg_ERR_UNKNOWN);
      }
    }
    ++cnt;
  }
  yaml_event_delete(&event);
  *input_cnt = cnt;
  return err;
}

static bg_error parse_node_outputs(yaml_parser_t *parser,
                                  output_struct *outputs, size_t *output_cnt) {
  bg_error err;
  yaml_event_t event;
  const char done_OUTPUT_NAME = 1<<1;
  char output_name[bg_MAX_STRING_LENGTH];
  char done = 0;
  size_t cnt = 0;
  bool single_output = false;

  if(!yaml_parser_parse(parser, &event)) {
    fprintf(stderr, "ERROR!, YAML parser encountered an error.\n");
    return bg_error_set(bg_ERR_UNKNOWN);
  }
  if(event.type == YAML_MAPPING_START_EVENT) {
    single_output = true;
    err = bg_SUCCESS;
  }
  else {
    err = get_event(parser, &event, YAML_MAPPING_START_EVENT);
  }
  yaml_event_delete(&event);

  /*fprintf(stderr, "parse nodes outputs...");*/

  while(err == bg_SUCCESS && event.type != YAML_SEQUENCE_END_EVENT) {
    if(cnt >= bg_MAX_PORTS) {
      fprintf(stderr, "ERROR: to many outputs\n");
      err = bg_error_set(bg_ERR_UNKNOWN);
      break;
    }
    yaml_event_delete(&event);
    err = get_event(parser, &event, YAML_SCALAR_EVENT);
    done = 0;
    while(err == bg_SUCCESS && event.type != YAML_MAPPING_END_EVENT) {
      if(strcmp((const char*)event.data.scalar.value, "name") == 0) {
        if(done & done_OUTPUT_NAME) {
          fprintf(stderr, "ERROR! multiple \"name\" sections.\n");
          err = bg_error_set(bg_ERR_UNKNOWN);
        } else {
          err = get_string(parser, output_name);
          /*fprintf(stderr, "node name: %s\n", node_name);*/
          done |= done_OUTPUT_NAME;
        }
      } else {
        /*fprintf(stderr, "WARNING! Ignoring unexpected section: %s\n",
          (const char*)event.data.scalar.value);*/
        skip_next_node(parser, &event);
        /*skip_node(parser, &event);*/
      }
      yaml_event_delete(&event);
      if(!yaml_parser_parse(parser, &event)) {
        fprintf(stderr, "ERROR!, YAML parser encountered an error.\n");
        err = bg_error_set(bg_ERR_UNKNOWN);
      }
    }
    if(!(done & done_OUTPUT_NAME)) {
      sprintf(output_name, "out_%05lu", (unsigned long)cnt);
    }
    {
      char *name_copy;
      name_copy = malloc(strlen(output_name)+1);
      strcpy(name_copy, output_name);
      outputs[cnt].name = name_copy;
    }
    if(single_output) {
      event.type = YAML_SEQUENCE_END_EVENT;
    }
    else {
      yaml_event_delete(&event);
      if(!yaml_parser_parse(parser, &event)) {
        fprintf(stderr, "ERROR!, YAML parser encountered an error.\n");
        err = bg_error_set(bg_ERR_UNKNOWN);
      }
    }
    ++cnt;
  }
  yaml_event_delete(&event);
  *output_cnt = cnt;
  return err;
}


static bg_error parse_edges(yaml_parser_t *parser, bg_edge_struct_list_t *edges) {
  bg_error err = bg_SUCCESS;
  yaml_event_t event;
  int done = 0;
  const int done_FROM_ID = 0x01;
  const int done_FROM_IDX = 0x02;
  const int done_TO_ID = 0x04;
  const int done_TO_IDX = 0x08;
  const int done_FROM_NAME = 0x10;
  const int done_FROM_OUT_NAME = 0x20;
  const int done_TO_NAME = 0x40;
  const int done_TO_IN_NAME = 0x80;
  const int done_WEIGHT = 0x100;
  edge_struct *edge;
  /*fprintf(stderr, "parse edges...");*/
  err = get_event(parser, &event, YAML_SEQUENCE_START_EVENT);
  yaml_event_delete(&event);
  err = get_event(parser, &event, YAML_MAPPING_START_EVENT);

  while(err == bg_SUCCESS && event.type != YAML_SEQUENCE_END_EVENT) {
    yaml_event_delete(&event);
    edge = (edge_struct*)calloc(1, sizeof(edge_struct));
    edge->ignore_for_sort = 0;
    edge->from_name[0] = '\0';
    edge->from_out_name[0] = '\0';
    edge->to_name[0] = '\0';
    edge->to_in_name[0] = '\0';
    done = 0;
    err = get_event(parser, &event, YAML_SCALAR_EVENT);
    while(err == bg_SUCCESS && event.type != YAML_MAPPING_END_EVENT) {
      if(strcmp((const char*)event.data.scalar.value, "fromNodeId") == 0) {
        if(done & done_FROM_ID) {
          fprintf(stderr, "ERROR! multiple \"fromNodeId\" sections.\n");
          err = bg_error_set(bg_ERR_UNKNOWN);
        } else if(done & done_FROM_NAME) {
          fprintf(stderr, "ERROR! cannot have \"fromNodeId\" and \"fromNode\" sections.\n");
          err = bg_error_set(bg_ERR_UNKNOWN);
        } else {
          err = get_int(parser, &edge->sourceId);
          done |= done_FROM_ID;
        }
      } else if(strcmp((const char*)event.data.scalar.value, "fromNode") == 0) {
        if(done & done_FROM_ID) {
          fprintf(stderr, "ERROR! cannot have \"fromNodeId\" and \"fromNode\" sections.\n");
          err = bg_error_set(bg_ERR_UNKNOWN);
        } else if(done & done_FROM_NAME) {
          fprintf(stderr, "ERROR! multiple \"fromNode\" sections.\n");
          err = bg_error_set(bg_ERR_UNKNOWN);
        } else {
          err = get_string(parser, edge->from_name);
          done |= done_FROM_NAME;
        }
      } else if(strcmp((const char*)event.data.scalar.value, "fromNodeOutputIdx") == 0) {
        if(done & done_FROM_IDX) {
          fprintf(stderr, "ERROR! multiple \"fromNodeOutputIdx\" sections.\n");
          err = bg_error_set(bg_ERR_UNKNOWN);
        } else if(done & done_FROM_OUT_NAME) {
          fprintf(stderr, "ERROR! cannot have \"fromNodeOutputIdx\" and \"fromNodeOutput\" sections.\n");
          err = bg_error_set(bg_ERR_UNKNOWN);
        } else {
          unsigned long idx = 0;
          err = get_int(parser, &idx);
          edge->sourcePortIdx = (size_t)idx;
          done |= done_FROM_IDX;
        }
      } else if(strcmp((const char*)event.data.scalar.value, "fromNodeOutput") == 0) {
        if(done & done_FROM_IDX) {
          fprintf(stderr, "ERROR! cannot have \"fromNodeOutputIdx\" and \"fromNodeOutput\" sections.\n");
          err = bg_error_set(bg_ERR_UNKNOWN);
        } else if(done & done_FROM_OUT_NAME) {
          fprintf(stderr, "ERROR! multiple \"fromNodeOutput\" sections.\n");
          err = bg_error_set(bg_ERR_UNKNOWN);
        } else {
          err = get_string(parser, edge->from_out_name);
          done |= done_FROM_OUT_NAME;
        }
      } else if(strcmp((const char*)event.data.scalar.value, "toNodeId") == 0) {
        if(done & done_TO_ID) {
          fprintf(stderr, "ERROR! multiple \"toNodeId\" sections.\n");
          err = bg_error_set(bg_ERR_UNKNOWN);
        } else if(done & done_TO_NAME) {
          fprintf(stderr, "ERROR! cannot have \"toNodeId\" and \"toNode\" sections.\n");
          err = bg_error_set(bg_ERR_UNKNOWN);
        } else {
          err = get_int(parser, &edge->sinkId);
          done |= done_TO_ID;
        }
      } else if(strcmp((const char*)event.data.scalar.value, "toNode") == 0) {
        if(done & done_TO_ID) {
          fprintf(stderr, "ERROR! cannot have \"toNodeId\" and \"toNode\" sections.\n");
          err = bg_error_set(bg_ERR_UNKNOWN);
        } else if(done & done_TO_NAME) {
          fprintf(stderr, "ERROR! multiple \"toNode\" sections.\n");
          err = bg_error_set(bg_ERR_UNKNOWN);
        } else {
          err = get_string(parser, edge->to_name);
          done |= done_TO_NAME;
        }
      } else if(strcmp((const char*)event.data.scalar.value, "toNodeInputIdx") == 0) {
        if(done & done_TO_IDX) {
          fprintf(stderr, "ERROR! multiple \"toNodeInputIdx\" sections.\n");
          err = bg_error_set(bg_ERR_UNKNOWN);
        } else if(done & done_TO_IN_NAME) {
          fprintf(stderr, "ERROR! cannot have \"toNodeInputIdx\" and \"toNodeInput\" sections.\n");
          err = bg_error_set(bg_ERR_UNKNOWN);
        } else {
          unsigned long idx = 0;
          err = get_int(parser, &idx);
          edge->sinkPortIdx = (size_t)idx;
          done |= done_TO_IDX;
        }
      } else if(strcmp((const char*)event.data.scalar.value, "toNodeInput") == 0) {
        if(done & done_TO_IDX) {
          fprintf(stderr, "ERROR! cannot have \"toNodeInputIdx\" and \"toNodeInput\" sections.\n");
          err = bg_error_set(bg_ERR_UNKNOWN);
        } else if(done & done_TO_IN_NAME) {
          fprintf(stderr, "ERROR! multiple \"toNodeInput\" sections.\n");
          err = bg_error_set(bg_ERR_UNKNOWN);
        } else {
          err = get_string(parser, edge->to_in_name);
          done |= done_TO_IN_NAME;
        }
      } else if(strcmp((const char*)event.data.scalar.value, "weight") == 0) {
        if(done & done_WEIGHT) {
          fprintf(stderr, "ERROR! multiple \"weight\" sections.\n");
          err = bg_error_set(bg_ERR_UNKNOWN);
        } else {
          err = get_double(parser, &edge->weight);
          done |= done_WEIGHT;
        }
      } else if(strcmp((const char*)event.data.scalar.value, "ignore_for_sort") == 0) {
        err = get_int(parser, &edge->ignore_for_sort);
      } else {
        /*fprintf(stderr, "WARNING! Ignoring unexpected section: %s\n",
          (const char*)event.data.scalar.value);*/
        skip_next_node(parser, &event);
      }
      yaml_event_delete(&event);
      if(!yaml_parser_parse(parser, &event)) {
        fprintf(stderr, "ERROR!, YAML parser encountered an error.\n");
        err = bg_error_set(bg_ERR_UNKNOWN);
      }
    }
    if(err == bg_SUCCESS) {
      if(done & done_FROM_NAME) {

      }
      bg_edge_struct_list_append(edges, edge);
    } else {
      free(edge);
    }
    yaml_event_delete(&event);
    if(!yaml_parser_parse(parser, &event)) {
      fprintf(stderr, "ERROR!, YAML parser encountered an error.\n");
      err = bg_error_set(bg_ERR_UNKNOWN);
    }
  }
  yaml_event_delete(&event);
  return err;
}


static bg_error connect_nodes(bg_graph_t *g, bg_edge_struct_list_t *edges) {
  edge_struct *edge;
  bg_edge_t *e;
  bg_edge_struct_list_iterator_t it;
  unsigned long edgeId = 1;
  bg_error err;
  for(edge = bg_edge_struct_list_first(edges, &it);
      edge; edge = bg_edge_struct_list_next(&it)) {
    if(strlen(edge->from_name) > 0) {
      err = bg_node_get_id(g, edge->from_name, &edge->sourceId);
      if(err != bg_SUCCESS) {
        fprintf(stderr, "ERROR!, creating edge: cannot find node id for: %s\n",
                edge->from_name);
      }
    }
    if(strlen(edge->from_out_name) > 0) {
      err = bg_node_get_output_idx(g, edge->sourceId, edge->from_out_name,
                                    &edge->sourcePortIdx);
      if(err != bg_SUCCESS) {
        fprintf(stderr, "ERROR!, creating edge: cannot find output idx for: %s\n",
                edge->from_out_name);
      }
    }
    if(strlen(edge->to_name) > 0) {
      err = bg_node_get_id(g, edge->to_name, &edge->sinkId);
      if(err != bg_SUCCESS) {
        fprintf(stderr, "ERROR!, creating edge: cannot find node id for: %s\n",
                edge->to_name);
      }
    }
    if(strlen(edge->to_in_name) > 0) {
      err = bg_node_get_input_idx(g, edge->sinkId, edge->to_in_name,
                                  &edge->sinkPortIdx);
      if(err != bg_SUCCESS) {
        fprintf(stderr, "ERROR!, creating edge: cannot find input idx for: %s\n",
                edge->from_out_name);
      }
    }
    err = bg_graph_create_edge(g, edge->sourceId, edge->sourcePortIdx,
                               edge->sinkId, edge->sinkPortIdx,
                               edge->weight, edgeId++);
    if(err != bg_SUCCESS) {
      fprintf(stderr, "ERROR!, creating edge %lu:%lu -> %lu:%lu\n",
              (unsigned long) edge->sourceId, (unsigned long) edge->sourcePortIdx,
              (unsigned long) edge->sinkId, (unsigned long) edge->sinkPortIdx);
    }
    err = bg_graph_find_edge(g, edgeId-1, &e);
    if(err != bg_SUCCESS) {
      fprintf(stderr, "ERROR!, setting sort option for edge: %lu\n", edgeId-1);
    }
    else {
      e->ignore_for_sort = edge->ignore_for_sort;
    }
    free(edge);
  }
  return err;
}


bg_error bg_graph_from_parser(yaml_parser_t *parser, bg_graph_t *g){
  bg_error err = bg_SUCCESS;
  yaml_event_t event;
  bg_edge_struct_list_t *edge_struct_list;

  bg_edge_struct_list_init(&edge_struct_list);
  if(!yaml_parser_parse(parser, &event)) {
    fprintf(stderr, "ERROR! YAML parser encountered an error 1.\n");
    return bg_error_set(bg_ERR_UNKNOWN);
  }
  if(event.type == YAML_STREAM_START_EVENT) {
    yaml_event_delete(&event);
    if(!yaml_parser_parse(parser, &event)) {
      fprintf(stderr, "ERROR! YAML parser encountered an error 2.\n");
      return bg_error_set(bg_ERR_UNKNOWN);
    }
    if(event.type == YAML_DOCUMENT_START_EVENT) {
      yaml_event_delete(&event);
      if(!yaml_parser_parse(parser, &event)) {
        fprintf(stderr, "ERROR! YAML parser encountered an error 3.\n");
        return bg_error_set(bg_ERR_UNKNOWN);
      }
      if(event.type == YAML_MAPPING_START_EVENT) {
        if(!yaml_parser_parse(parser, &event)) {
          fprintf(stderr, "ERROR! YAML parser encountered an error 4.\n");
          return bg_error_set(bg_ERR_UNKNOWN);
        }
        while(err == bg_SUCCESS && event.type != YAML_MAPPING_END_EVENT) {
          if(event.type == YAML_SCALAR_EVENT) {
            if(strcmp("nodes", (const char*)event.data.scalar.value) == 0) {
              err = parse_nodes(parser, g);
            } else if(strcmp("edges",(const char*)event.data.scalar.value)==0) {
              parse_edges(parser, edge_struct_list);
            } else if(strcmp("networkInputs",
                             (const char*)event.data.scalar.value) == 0) {
              fprintf(stderr,
                      "section \"networkInputs\" deprecated. ignoring it.\n");
            } else if(strcmp("descriptions",(const char*)event.data.scalar.value)==0) {
              /* ignore descriptions */
              err = skip_next_node(parser, &event);
            } else {
              /*fprintf(stderr, "error: unexpected stuff %d (%s)\n", event.type, event.data.scalar.value);*/
              /* ignore section */
              err = skip_next_node(parser, &event);
            }
          }
          yaml_event_delete(&event);
          if(!yaml_parser_parse(parser, &event)) {
            fprintf(stderr, "ERROR! YAML parser encountered an error.\n");
            return bg_error_set(bg_ERR_UNKNOWN);
          }
        }
      }
    }
  }
  connect_nodes(g, edge_struct_list);
  bg_edge_struct_list_deinit(edge_struct_list);
  return err;
}


bg_error bg_graph_from_yaml_file(const char *filename, bg_graph_t *g) {
  yaml_parser_t parser;
  FILE *fp = NULL;
  char *full_path = NULL;
  char *last_slash = NULL;
  char *new_path;
  const char *c_full_path;

  if(g->load_path == 0 || filename[0] == '/') {
    c_full_path = filename;
  }
  else {
    size_t l1 = strlen(g->load_path);
    size_t l2 = strlen(filename);
    if(g->load_path[l1-1] == '/') {
      full_path = malloc(l1 + l2 + 1);
      strcpy(full_path, g->load_path);
      strcpy(full_path + l1, filename);
      full_path[l1+l2] = '\0';
      c_full_path = full_path;
    }
    else {
      full_path = malloc(l1 + l2 + 2);
      strcpy(full_path, g->load_path);
      full_path[l1] = '/';
      strcpy(full_path + l1 +1, filename);
      full_path[l1+l2+1] = '\0';
      c_full_path = full_path;
    }
  }
  /*printf("parsing: %s\n", c_full_path);*/

  /* get the path of the new subgraph and use it as new load path */
  last_slash = strrchr(c_full_path,'/');
  if (last_slash)
  {
      new_path = malloc((last_slash - c_full_path) + 2);
      strncpy(new_path, c_full_path, (last_slash - c_full_path) + 1);
      new_path[(last_slash-c_full_path)+1] = '\0';
      if(g->load_path) {
        free((char*)g->load_path);
      }
      g->load_path = new_path;
  }
  yaml_parser_initialize(&parser);
  fp = fopen(c_full_path, "r");
  if(!fp) {
    fprintf(stderr, "ERROR: could not open file \"%s\".\n", c_full_path);
    return bg_error_set(bg_ERR_UNKNOWN);
  }
  yaml_parser_set_input_file(&parser, fp);

  bg_graph_from_parser(&parser,g);

  yaml_parser_delete(&parser);
  fclose(fp);
  if(full_path) {
    free(full_path);
  }
  return bg_SUCCESS;
}


bg_error bg_graph_from_yaml_string(const unsigned char *string, bg_graph_t *g) {
  yaml_parser_t parser;
  yaml_parser_initialize(&parser);

  yaml_parser_set_input_string(&parser, string,strlen((const char*)string));

  bg_graph_from_parser(&parser,g);

  yaml_parser_delete(&parser);
  return bg_SUCCESS;
}



#else /* ifdef YAML_SUPPORT */

bg_error bg_graph_from_yaml_file(const char *filename, bg_graph_t *g) {
  return bg_ERR_NOT_IMPLEMENTED;
  (void)filename;
  (void)g;
}

#endif /* YAML_SUPPORT */
