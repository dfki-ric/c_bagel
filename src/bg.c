#include "bg_impl.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>         /* directory search */

#ifdef WIN32
#  include <windows.h>
#  define LibHandle HINSTANCE
#else
#  include <dlfcn.h>
#  define LibHandle void*
# include <stdint.h>
#endif

node_type_t *node_types[bg_NUM_OF_NODE_TYPES];
merge_type_t *merge_types[bg_NUM_OF_MERGE_TYPES];

node_type_t **extern_node_types;
int num_extern_node_types;

typedef void (*init_nodes_t) (void);

static const char* bg_error_messages[21] = { "SUCCESS",
                                  "ERR_UNKNOWN",
                                  "ERR_NOT_INITIALIZED",
                                  "ERR_NOT_IMPLEMENTED",
                                  "ERR_WRONG_TYPE",
                                  "ERR_IS_CONNECTED",
                                  "ERR_OUT_OF_RANGE",
                                  "ERR_DO_NOT_OWN",
                                  "ERR_PORT_FULL",
                                  "ERR_NUM_PORTS_EXCEEDED",
                                  "ERR_INVALID_CONNECTION",
                                  "ERR_NODE_NOT_FOUND",
                                  "ERR_EDGE_NOT_FOUND",
                                  "ERR_DUPLICATE_NODE_ID",
                                  "ERR_DUPLICATE_EDGE_ID",
                                  "ERR_NO_MEMORY",
                                  "ERR_WRONG_ARG_COUNT",
                                  "ERR_YAML_ERROR",
                                  "ERR_EXTERN_FILE_NOT_OPENED",
                                  "ERR_EXTERN_SYMBOL_NOT_FOUND",
                                  "ERR_EXTERN_NODE_NOT_FOUND"};

extern void bg_register_atomic_types(void);
extern void bg_register_subgraph_types(void);
extern void bg_register_extern_types(void);
extern void bg_register_port_types(void);
extern void bg_register_basic_merges(void);


static int is_initialized = 0;
static bg_error bg_err = bg_ERR_NOT_INITIALIZED;


bool bg_is_initialized(void) {
  return is_initialized;
}

bg_error bg_initialize(void) {
  if(!is_initialized) {
    is_initialized = true;
    bg_error_clear();
    bg_register_atomic_types();
    bg_register_subgraph_types();
    bg_register_extern_types();
    bg_register_port_types();
    bg_register_basic_merges();
    extern_node_types = 0;
    num_extern_node_types = 0;
  }
  return bg_SUCCESS;
}


void bg_terminate(void) {
  is_initialized = false;
}

bool bg_error_occurred(void) {
  return bg_err != bg_SUCCESS;
}

void bg_error_clear(void) {
  bg_err = bg_SUCCESS;
}

bg_error bg_error_set(bg_error err) {
  if((err != bg_SUCCESS) && (bg_err == bg_SUCCESS)) {
    bg_err = err;
  }
  return err;
}

bg_error bg_error_get(void) {
  return bg_err;
}

void bg_error_message_get( bg_error err, char error_message[bg_MAX_STRING_LENGTH] ) {
  size_t bg_error_messages_length = sizeof(bg_error_messages) / sizeof(char*);

  if (bg_error_messages_length != bg_NUM_OF_ERRORS || bg_error_messages_length <= err) {
    const char* message = "No string message corresponding to the error. Check message array!";
    strncpy(error_message, message, bg_MAX_STRING_LENGTH-1);
  } else {
    strncpy(error_message, (const char*)bg_error_messages[err], bg_MAX_STRING_LENGTH-1);
  }
}

void bg_node_type_register(node_type_t *types) {
  node_type_t *type = types;

  while(type->eval) {
    /*printf("register node type \"%s\" with ID: %d\n", type->name, type->id);*/
    node_types[type->id] = type;
    ++type;
  }
}

void bg_extern_node_type_register(node_type_t *types) {
  node_type_t *type = types;
  node_type_t **new_extern_node_types;

  while(type->eval) {
    printf("register node type \"%s\" with ID: %d\n", type->name, type->id);
    new_extern_node_types = (node_type_t**)calloc(num_extern_node_types+1,
                                                  sizeof(node_type_t*));
    memcpy(new_extern_node_types, extern_node_types,
           sizeof(node_type_t*)*num_extern_node_types);
    new_extern_node_types[num_extern_node_types++] = type;
    if(extern_node_types != 0) {
      free(extern_node_types);
    }
    extern_node_types = new_extern_node_types;
    ++type;
  }
}

void bg_merge_type_register(merge_type_t *types) {
  merge_type_t *type = types;
  while(type->merge) {
    /*printf("register merge type \"%s\" with ID: %d\n", type->name, type->id);*/
    merge_types[type->id] = type;
    ++type;
  }
}

int bg_min(int a, int b) {
  return (a <= b ? a : b);
}

bg_error load_extern_nodes(const char* filename) {
  LibHandle libHandle;
  init_nodes_t initn;
  char *error;
  char *loadfile;
  int i;
  int l;
  bool found_ending;

  found_ending = false;
  l = strlen(filename);
  loadfile = (char*)malloc(l+7);
  memcpy(loadfile, filename, l*sizeof(char));
  loadfile[l] = '\0';

  for(i=0; i<l; ++i) {
    if(filename[i] == '.') {
      found_ending = true;
    }
    if(filename[i] == '/') {
      found_ending = false;
    }
    if(filename[i] == '\\') {
      found_ending = false;
    }
  }
  if(!found_ending) {
    /* no file ending found */
#ifdef __APPLE__
    strncpy(loadfile+l, ".dylib", 7);
    loadfile[l+6] = '\0';
#else
#ifdef WIN32
    strncpy(loadfile+l, ".dll", 5);
    loadfile[l+4] = '\0';
#else
    strncpy(loadfile+l, ".so", 4);
    loadfile[l+3] = '\0';
#endif
#endif
  }
#ifdef WIN32
  libHandle = LoadLibrary(loadfile);
#else
  libHandle = (dlopen(loadfile, RTLD_LAZY));
#endif
  if(!libHandle) {
    fprintf(stderr, "ERROR: load_extern_nodes: %s\n", loadfile);
    free(loadfile);
    return bg_ERR_EXTERN_FILE_NOT_OPENED;
  }

#ifdef WIN32
  initn = (init_nodes_t)GetProcAddress(libHandle, "init_nodes");
  if(GetLastError()) {
    (void)error;
    fprintf(stderr, "error while loading lib: %lu\n", GetLastError());
    free(loadfile);
    return bg_ERR_EXTERN_SYMBOL_NOT_FOUND;
  }
#else
  initn = (init_nodes_t)(uintptr_t)dlsym(libHandle, "init_nodes");

  if ((error = dlerror()) != NULL)  {
    fprintf(stderr, "%s\n", error);
    free(loadfile);
    return bg_ERR_EXTERN_SYMBOL_NOT_FOUND;
  }
#endif

  printf("call init!\n");
  initn();
  free(loadfile);
  return bg_SUCCESS;
}


bg_error getExternNodes(const char* path)
{
    /*fprintf(stderr, "%s\n", path.c_str());*/
  DIR *dir;
    struct dirent *ent;
    if ((dir = opendir (path)) != NULL) {
        /* go through all entities */
        while ((ent = readdir (dir)) != NULL) {
            char* file = ent->d_name;
#ifdef __APPLE__
            if (strstr(file,".dylib") != NULL){
#else
#ifdef WIN32
            if (strstr(file,".dll") != NULL){
#else
            if (strstr(file,".so") != NULL){
#endif
#endif
                /* try to load the library */
                char newPath[258];
                /* todo: add check wether copy was fine*/
                sprintf(newPath, "%s%s", path, file);
                load_extern_nodes(newPath);
            } else if (strstr(&(file[0]), ".")) {
                /* skip ".*" */
            } else {
                /* go into the next dir */
                char newPath[258];
                /* todo: add check wether copy was fine*/
                sprintf(newPath, "%s%s/", path, file);
                getExternNodes(newPath);
            }
        }
        closedir (dir);
    } else {
        /* this is not a directory */
        fprintf(stderr, "Specified path '%s' is not a valid directory\n", path);
    }
    return bg_SUCCESS;
}
