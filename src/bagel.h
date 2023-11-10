/**
 * \mainpage
 * \author Lorenz Quack (code@lorenzquack.de), Malte Langosz (malte.langosz@dfki.de)
 * \brief Bagel (Biologically inspired Graph-Based Language) is a cross-platform graph-based dataflow language developed at the [Robotics Innovation Center of the German Research Center for Artificial Intelligence (DFKI-RIC)](http://robotik.dfki-bremen.de/en/startpage.html) and the [University of Bremen](http://www.informatik.uni-bremen.de/robotik/index_en.php). It runs on (Ubuntu) Linux, Mac and Windows. c_bagel is an implementation of Bagel in pure C.
 *
 * \version 0.1
 *
 * The API interface of the c_bagel library is bagel.h.
 * The central data type is bg_graph_t which is an opaque pointer.
 * Further more you will need to handle \link bg_node_id_t \endlink and
 * \link bg_edge_id_t \endlink which are
 * essentially numeric types (e.g., \c unsigned \c long).
 *
 * Before you start working with the c_bagel Library you should call
 * bg_initialize(). Once you are done, you should call bg_terminate() so that
 * the library can clean up after itself.
 *
 * At any time you may call bg_error_occurred() to check if there was an error.
 * bg_error_get() will retrieve the \em first error that occurred since the last
 * call to bg_error_clear(). Virtually all functions return a
 * \link bg_error \endlink so that
 * you can either check after every call or collectivly via bg_error_occurred().
 *
 * \section documentation Documentation
 *
 * You can find Bagel' documentation on its [GitHub Wiki](https://github.com/dfki-ric/bagel_wiki/wiki).
 *
 * \section compiling Compile Time Options
 * The source code comes with CMakeLists.txt for compilation via
 * <a href="http://www.cmake.org/">CMake</a>.  There are a few compile time
 * options that you can pass to cmake. By default, they are all enabled:
 *  \arg YAML_SUPPORT
 *       Support \link bg_graph_from_yaml_file loading from \endlink and
 *       \link bg_graph_to_yaml_file saving to \endlink
 *       <a href="http://www.yaml.org/">YAML</a> files. This adds
 *       <a href="http://pyyaml.org/wiki/LibYAML">libyaml</a> as a
 *       dependency.
 *  \arg INTERVAL_SUPPORT
 *       Support for interval arithmetic on the graphs via the
 *       \link interval_api Interval API \endlink. This adds
 *       <a href="http://gforge.inria.fr/">MPFI</a> as a dependency.
 *  \arg UNIT_TESTS
 *       Compile unit tests into a test program.
 *       This adds <a href="http://check.sourceforge.net/">Check</a> as a
 *       dependency.
 *
 * \section license License
 *
 * Bagel is distributed under the
 * [3-clause BSD license](https://opensource.org/licenses/BSD-3-Clause).
 *
 */

#ifndef C_BAGEL_H
#define C_BAGEL_H

#ifdef _PRINT_HEADER_
#  warning "bagel.h"
#endif

#include <stddef.h>
#include "bool.h"
/*#include <stdbool.h>*/


#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"

#define bg_MAX_STRING_LENGTH 255

typedef struct bg_graph_t bg_graph_t;
typedef struct bg_edge_t bg_edge_t;

typedef unsigned long bg_node_id_t;
typedef unsigned long bg_edge_id_t;

typedef enum { bg_SUCCESS = 0,
               bg_ERR_UNKNOWN = 1,
               bg_ERR_NOT_INITIALIZED = 2,
               bg_ERR_NOT_IMPLEMENTED = 3,
               bg_ERR_WRONG_TYPE = 4,
               bg_ERR_IS_CONNECTED = 5,
               bg_ERR_OUT_OF_RANGE = 6,
               bg_ERR_DO_NOT_OWN = 7,
               bg_ERR_PORT_FULL = 8,
               bg_ERR_NUM_PORTS_EXCEEDED = 9,
               bg_ERR_INVALID_CONNECTION = 10,
               bg_ERR_NODE_NOT_FOUND = 11,
               bg_ERR_EDGE_NOT_FOUND = 12,
               bg_ERR_DUPLICATE_NODE_ID = 13,
               bg_ERR_DUPLICATE_EDGE_ID = 14,
               bg_ERR_NO_MEMORY = 15,
               bg_ERR_WRONG_ARG_COUNT = 16,
               bg_ERR_YAML_ERROR = 17,
               bg_ERR_EXTERN_FILE_NOT_OPENED = 18,
               bg_ERR_EXTERN_SYMBOL_NOT_FOUND = 19,
               bg_ERR_EXTERN_NODE_NOT_FOUND = 20,
               bg_NUM_OF_ERRORS = 21 } bg_error;

typedef enum { bg_NODE_TYPE_SUBGRAPH,
               bg_NODE_TYPE_INPUT,
               bg_NODE_TYPE_OUTPUT,
               bg_NODE_TYPE_PIPE,
               bg_NODE_TYPE_DIVIDE,
               bg_NODE_TYPE_SIN,
               bg_NODE_TYPE_COS,
               bg_NODE_TYPE_TAN,
               bg_NODE_TYPE_ACOS,
               bg_NODE_TYPE_ATAN2,
               bg_NODE_TYPE_POW,
               bg_NODE_TYPE_MOD,
               bg_NODE_TYPE_ABS,
               bg_NODE_TYPE_SQRT,
               bg_NODE_TYPE_FSIGMOID,
               bg_NODE_TYPE_GREATER_THAN_0,
               bg_NODE_TYPE_EQUAL_TO_0,
               bg_NODE_TYPE_EXTERN,
               bg_NODE_TYPE_TANH,
               bg_NODE_TYPE_ASIN,
               bg_NUM_OF_NODE_TYPES } bg_node_type;

typedef enum { bg_MERGE_TYPE_SUM,
               bg_MERGE_TYPE_WEIGHTED_SUM,
               bg_MERGE_TYPE_PRODUCT,
               bg_MERGE_TYPE_MIN,
               bg_MERGE_TYPE_MAX,
               bg_MERGE_TYPE_MEDIAN,
               bg_MERGE_TYPE_MEAN,
               bg_MERGE_TYPE_NORM,
               bg_NUM_OF_MERGE_TYPES } bg_merge_type;

/*****************************//**
 * \defgroup lib_api General API
 * @{
 *********************************/

/**
 * \brief Initializes the c_bagel library. This have to be done once
 * for every application. If the library is already initialized the
 * function has no influence and also retruns with \link bg_SUCCESS
 * \endlink.
 */
bg_error bg_initialize(void);

/**
 * \brief Deinitializes the c_bagel library. This have to be done once
 * when an application is closed to cleanup the c_bagel.
 * \warning Currently the memory is not cleared by this call!
 */
void bg_terminate(void);

/**
 * \brief Returns if the c_bagel library is already initialized.
 *
 * \return True if the library is initialized, False otherwise.
 */
bool bg_is_initialized(void);

/**
 * \brief Load an external node type.
 *
 * \param filname The path to the external node file
 * \return \link bg_SUCCESS \endlink on successfully initializing
 * the new external node types.
 */
bg_error load_extern_nodes(const char* filename);

/**
 * \brief The function loads all external node types found recursively
 * in the folder structure of the given path.
 *
 * \param path The path to the external node library
 * \return \link bg_SUCCESS \endlink on successfully initializing
 * the new external node types.
 */
bg_error getExternNodes(const char* path);

/**
 * \brief Indicates if the library is in an error state.
 *
 * \return True if the internal error state is set
 * \return False if everthing is fine
 */
bool bg_error_occurred(void);

/**
 * \brief Clear the internal error state.
 *
 */
void bg_error_clear(void);

/**
 * \brief Read the internal error state.
 *
 * \return The last occurred error state.
 */
bg_error bg_error_get(void);

/**
 * \brief Reads the corresponding error message of a given error state.
 *
 * \param err The error to read the message for.
 * \param error_message A pointer to a char buffer to write the message into.
 */
void bg_error_message_get(bg_error err, char error_message[bg_MAX_STRING_LENGTH]);

/**
 * @}
 */


/*****************************//**
 * \defgroup graph_api Graph API
 * @{
 *********************************/

/**
 * \brief Allocate memory for a new graph.
 *
 * \param **graph A pointer pointer to a graph struct. *graph will be the pointer to
 * the newly allocated bg_graph_t type.
 * \param *char The name of the new graph instance.
 * \return \link bg_SUCCESS \endlink or error state.
 */
bg_error bg_graph_alloc(bg_graph_t **graph, const char *name);

/**
 * \brief Free the memory of a graph instance.
 *
 * \param *graph A pointer to a graph struct to be freed.
 * \return \link bg_SUCCESS \endlink or error state.
 */
bg_error bg_graph_free(bg_graph_t *graph);


/**
 * \brief Set the path to which is used to load sub-graph files
 * if they are given by relative pathes.
 *
 * \param *graph A pointer to a graph struct to store the load path.
 * \param *path The load path.
 * \return \link bg_SUCCESS \endlink or error state.
 */
bg_error bg_graph_set_load_path(bg_graph_t *graph, const char *path);

/**
 * \brief Clone one graph into another one. \warning Currently the
 * function doesn't clone the current state (edge values).
 *
 * \param *dest The target graph.
 * \param *src The source graph to be cloned.
 * \return \link bg_SUCCESS \endlink or error state.
 */
bg_error bg_graph_clone(bg_graph_t *dest, const bg_graph_t *src);

/**
 * \brief Create an input node of a graph.
 *
 * \param *graph The graph to create a node for.
 * \param *name The name of the new node.
 * \param *node_id A unique identifier for the new node.
 * \return \link bg_SUCCESS \endlink or error state.
 */
bg_error bg_graph_create_input(bg_graph_t *graph, const char *name,
                               bg_node_id_t node_id);

/**
 * \brief Create an output node of a graph.
 *
 * \param *graph The graph to create a node for.
 * \param *name The name of the new node.
 * \param *node_id A unique identifier for the new node.
 * \return \link bg_SUCCESS \endlink or error state.
 */
bg_error bg_graph_create_output(bg_graph_t *graph, const char *name,
                                bg_node_id_t node_id);

/**
 * \brief Create an output node of a graph.
 *
 * \param *graph The graph to create a node for.
 * \param *name The name of the new node.
 * \param *node_id A unique identifier for the new node.
 * \param *node_type The type of the node to be created.
 * \return \link bg_SUCCESS \endlink or error state.
 */
bg_error bg_graph_create_node(bg_graph_t *graph, const char *name,
                              bg_node_id_t node_id, bg_node_type node_type);

/**
 * \brief Create a connection between two nodes.
 *
 * Connects an output of a given node to the input of another node.
 * Both nodes must be in the same graph.
 *
 * You may give 0 as either the source or sink ID. This allows you to create
 * semi-connected edges for either introspection or data input.
 *
 * \returns \link bg_SUCCESS \endlink
 *   If the connection was established successfully.
 * \returns \link bg_ERR_DO_NOT_OWN \endlink
 *   If at least one node is not in the given graph.
 * \returns \link bg_ERR_OUT_OF_RANGE \endlink
 *   If the port indices are out of range.
 * \returns \link bg_ERR_INVALID_CONNECTION \endlink
 *   If you try to specify a graph input as sink or a graph output as source.
 * \returns \link bg_ERR_PORT_FULL \endlink
 *   If either the input_port or output_port has to many connections.
 */
bg_error bg_graph_create_edge(bg_graph_t *graph,
                              bg_node_id_t source_node_id,
                              size_t source_output_port_idx,
                              bg_node_id_t sink_node_id,
                              size_t sink_input_port_idx,
                              bg_real weight, bg_edge_id_t edge_id);

/**
 * \brief Remove input node of graph.
 *
 * \param *graph The graph where the node is removed.
 * \param *node_id The id of the node to remove.
 * \return \link bg_SUCCESS \endlink or error state.
 */
bg_error bg_graph_remove_input(bg_graph_t *graph, bg_node_id_t input_id);

/**
 * \brief Remove output node of graph.
 *
 * \param *graph The graph where the node is removed.
 * \param *node_id The id of the node to remove.
 * \return \link bg_SUCCESS \endlink or error state.
 */
bg_error bg_graph_remove_output(bg_graph_t *graph, bg_node_id_t output_id);

/**
 * \brief Remove node of graph.
 *
 * \param *graph The graph where the node is removed.
 * \param *node_id The id of the node to remove.
 * \return \link bg_SUCCESS \endlink or error state.
 */
bg_error bg_graph_remove_node(bg_graph_t *graph, bg_node_id_t node_id);

/**
 * \brief Remove edge of graph.
 *
 * \param *graph The graph where the edge is removed.
 * \param *node_id The id of the edge to remove.
 * \return \link bg_SUCCESS \endlink or error state.
 */
bg_error bg_graph_remove_edge(bg_graph_t *graph, bg_edge_id_t edge_id);

/**
 * \brief Reset the state of a graph.
 *
 * \param *graph The graph to operate on.
 * \param *recursive If true all sub-graphs are reset too.
 * \return \link bg_SUCCESS \endlink or error state.
 */
bg_error bg_graph_reset(bg_graph_t *graph, bool recursive);

/**
 * \brief Set the instance of a sub-graph.
 *
 * \param *graph The graph to operate on.
 * \param *graph_name Identifies the sub-graph node by the name.
 * \param *subgraph The sub-graph instance to link to the selected node.
 * \return \link bg_SUCCESS \endlink or error state.
 */
bg_error bg_graph_set_subgraph(bg_graph_t *graph, const char *graph_name,
                               bg_graph_t *subgraph);

/**
 * \brief Remove all connected edges from the given node.
 *
 * \param *graph The graph to operate on.
 * \param *node_id The id of the node to remove the connected edges.
 * \return \link bg_SUCCESS \endlink or error state.
 */
bg_error bg_graph_disconnect_node(bg_graph_t *graph, bg_node_id_t node_id);

/**
 * \brief Evaluates the given graph and calculates a new state.
 *
 * \param *graph The graph to evaluate.
 * \return \link bg_SUCCESS \endlink or error state.
 */
bg_error bg_graph_evaluate(bg_graph_t *graph);

/* introspection */
bg_error bg_graph_get_output(const bg_graph_t *graph, size_t output_port_idx,
                             bg_real *value);
bg_error bg_graph_get_node_cnt(const bg_graph_t *graph, bool recursive,
                               size_t *node_cnt);
bg_error bg_graph_get_edge_cnt(const bg_graph_t *graph, bool recursive,
                               size_t *edge_cnt);
bg_error bg_graph_get_input_nodes(const bg_graph_t *graph,
                                  bg_node_id_t *input_ids, size_t *input_cnt);
bg_error bg_graph_get_output_nodes(const bg_graph_t *graph,
                                   bg_node_id_t *output_ids,
                                   size_t *output_cnt);
bg_error bg_graph_get_subgraph(bg_graph_t *graph, const char* name,
                               bg_graph_t **subgraph);
bg_error bg_graph_get_subgraph_list(bg_graph_t *graph, char **path,
                                    char **graph_name, size_t *subgraph_cnt,
                                    bool recursive);
bg_error bg_graph_has_node(bg_graph_t *graph, bg_node_id_t node_id,
                           bool *has_node);

/* YAML support */
/**
 * \returns \link bg_ERR_NOT_IMPLEMENTED \endlink if the library was
 *          compiled without YAML_SUPPORT.
 */
bg_error bg_graph_from_yaml_file(const char *filename, bg_graph_t *g);

/**
 * \returns \link bg_ERR_NOT_IMPLEMENTED \endlink if the library was
 *          compiled without YAML_SUPPORT.
 */
bg_error bg_graph_to_yaml_file(const char *filename, const bg_graph_t *g);


bg_error bg_graph_from_yaml_string(const unsigned char *string, bg_graph_t *g);
bg_error bg_graph_to_yaml_string(unsigned char *buffer, size_t buffer_size,
                                 const bg_graph_t *g, size_t *bytes_written);
/*bg_error bg_graph_from_parser(yaml_parser_t *parser, bg_graph_t *g);*/




/**
 * @}
 */


/*****************************//**
 * \defgroup node_api Node API
 * @{
 *********************************/

bg_error bg_node_set_input(bg_graph_t *graph,
                           bg_node_id_t node_id, size_t input_port_idx,
                           bg_merge_type merge_type,
                           bg_real default_value, bg_real bias,
                           const char *name);
bg_error bg_node_set_output(bg_graph_t *graph, bg_node_id_t node_id,
                            size_t output_port_idx, const char *name);
bg_error bg_node_set_merge(bg_graph_t *graph,
                           bg_node_id_t node_id, size_t input_port_idx,
                           bg_merge_type merge_type,
                           bg_real default_value, bg_real bias);
bg_error bg_node_set_default(bg_graph_t *graph,
                             bg_node_id_t node_id, size_t input_port_idx,
                             bg_real default_value);
bg_error bg_node_set_bias(bg_graph_t *graph,
                          bg_node_id_t node_id, size_t input_port_idx,
                          bg_real bias);
bg_error bg_node_set_subgraph(bg_graph_t *graph, bg_node_id_t node_id,
                              bg_graph_t *subgraph);
bg_error bg_node_set_extern(bg_graph_t *graph, bg_node_id_t node_id,
                            const char *extern_node_name);

/* introspection */
bg_error bg_node_get_output(const bg_graph_t *graph,
                            bg_node_id_t node_id, size_t output_port_idx,
                            bg_real *value);
bg_error bg_node_get_name(const bg_graph_t *graph,
                          bg_node_id_t node_id, const char **name);
bg_error bg_node_get_input_name(const bg_graph_t *graph,
                                bg_node_id_t node_id, size_t input_port_idx,
                                char *name);
bg_error bg_node_get_output_name(const bg_graph_t *graph,
                                 bg_node_id_t node_id, size_t output_port_idx,
                                 char *name);
bg_error bg_node_get_type(const bg_graph_t *graph,
                          bg_node_id_t node_id, bg_node_type *node_type);
bg_error bg_node_get_merge(const bg_graph_t *graph,
                           bg_node_id_t node_id, size_t input_port_idx,
                           bg_merge_type *merge_type);
bg_error bg_node_get_default(const bg_graph_t *graph,
                             bg_node_id_t node_id, size_t input_port_idx,
                             bg_real *default_value);
bg_error bg_node_get_bias(const bg_graph_t *graph,
                          bg_node_id_t node_id, size_t input_port_idx,
                          bg_real *bias);
bg_error bg_node_get_input_edges(const bg_graph_t *graph,
                                 bg_node_id_t node_id, size_t input_port_idx,
                                 bg_edge_id_t *edge_ids, size_t *edge_cnt);
bg_error bg_node_get_output_edges(const bg_graph_t *graph,
                                  bg_node_id_t node_id, size_t output_port_idx,
                                  bg_edge_id_t *edge_ids, size_t *edge_cnt);
bg_error bg_node_get_port_cnt(const bg_graph_t *graph, bg_node_id_t node_id,
                              size_t *input_port_cnt, size_t *output_port_cnt);
bg_error bg_node_get_id(const bg_graph_t *graph, const char *name,
                        unsigned long *id);
bg_error bg_node_get_output_idx(bg_graph_t *graph, bg_node_id_t node_id,
                                const char *name, size_t *idx);
bg_error bg_node_get_input_idx(bg_graph_t *graph, bg_node_id_t node_id,
                               const char *name, size_t *idx);


/**
 * @}
 */

/***************************//**
 * \defgroup edge_api Edge API
 * @{
 *******************************/
bg_error bg_edge_set_weight(bg_graph_t *graph,
                            bg_edge_id_t edge_id, bg_real weight);
bg_error bg_edge_set_value(bg_graph_t *graph, bg_edge_id_t edge_id, bg_real value);

/* introspection */
bg_error bg_edge_get_weight(const bg_graph_t *graph,
                            bg_edge_id_t edge_id, bg_real *weight);
bg_error bg_edge_get_value(const bg_graph_t *graph,
                           bg_edge_id_t edge_id, bg_real *value);
bg_error bg_edge_get_nodes(const bg_graph_t *graph, bg_edge_id_t edge_id,
                           bg_node_id_t *source_node_id, size_t *source_port_idx,
                           bg_node_id_t *sink_node_id, size_t *sink_port_idx);

bg_error bg_edge_set_value_p(bg_edge_t *edge, bg_real value);
bg_error bg_edge_get_pointer(bg_graph_t *graph, bg_edge_id_t edge_id,
                             bg_edge_t **edge);
/**
 * @}
 */


/**********************************//**
 * \defgroup interval_api Interval API
 * @{
 * The Interval API depends on the
 * <a href="http://gforge.inria.fr/">MPFI library</a> which in turn
 * depends on <a href="http://www.mpfr.org/">MPFR</a> and
 * <a href="http://gmplib.org/">GMP</a>.
 * Interval support can be disabled at compile time by passing
 * \c -DINTERVAL_SUPPORT=no to cmake. If interval support was disabled
 * all functions will return \link bg_ERR_NOT_IMPLEMENTED \endlink.
 **************************************/

bg_error bg_interval_evaluate_graph(bg_graph_t *graph);

/**
 * \brief Find those input_intervals that produce \c Inf or \c NaN in the graph.
 * \param[in] graph
 *        The graph that will be searched for \c Inf and \c NaN.
 * \param[in] resolution
 *        The size at which bisection of the input intervals will stop.
 * \param[in] input_intervals
 *        An array of intervals that will be fed into the graph and bisected
 *        to find those intervals that produce \c Inf or \c NaN.
 * \param[in] n
 *        The length of the first dimension of \c input_intervals.
 * \param[out] result_intervals
 *        The intervals that cause the graph to produce Inf or Nan will be
 *        saved here.  The first dimension will hold the different sets of
 *        input arguments.  The size of the first dimension will be saved
 *        in \c m.  The second dimension has length \c n and contains the
 *        arguments to the graph.  The third dimension has always length 2
 *        and contains the lower and upper bound of the interval.  The
 *        function will allocate the necessary space.  You should later
 *        call bg_interval_free_inf_nan to avoid memory leaks.
 * \param[out] m
 *        The length of the first dimension of \c result_intervals.
 *
 * \sa bg_interval_free_inf_nan
 */
bg_error bg_interval_find_inf_nan(bg_graph_t *graph, bg_real resolution,
                                  bg_real input_intervals[][2], size_t n,
                                  bg_real ****result_intervals, size_t *m);

/**
 * \sa bg_interval_find_inf_nan
 */
bg_error bg_interval_free_inf_nan(bg_real ***intervals, size_t n, size_t m);
bg_error bg_interval_detect_inf_nan(bg_graph_t *graph,
                                    bg_real input_intervals[][2],
                                    const size_t n, bool *detected);
bg_error bg_interval_set_edge_value(bg_graph_t *graph, bg_edge_id_t edge_id,
                                    const bg_real interval[2]);
bg_error bg_interval_get_edge_value(const bg_graph_t *graph,
                                    bg_edge_id_t edge_id,
                                    bg_real interval[2]);
bg_error bg_interval_get_node_output(const bg_graph_t *graph,
                                     bg_node_id_t node_id,
                                     size_t output_port_idx,
                                     bg_real interval[2]);
bg_error bg_interval_get_graph_output(const bg_graph_t *graph,
                                      size_t output_port_idx,
                                      bg_real interval[2]);
/**
 * @}
 */


#ifdef __cplusplus
}
#endif

#endif /* C_BAGEL_H */
