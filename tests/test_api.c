#include "../src/bagel.h"
#include "bg_test.h"
#include <math.h>



static bg_graph_t *g;

static double test_vals[] = {-37.5, -5.5, -1., -0.99, -0.01,
                             0, 0.3, 1., 3.3, 113.7};
#define test_vals_num (sizeof(test_vals) / sizeof(test_vals[0]))


/********************
 * fixtures
 ********************/

static void setup_graph() {
  bg_initialize();
  bg_graph_alloc(&g, "my graph");
  ck_assert_int_eq(bg_error_get(), bg_SUCCESS);
}

static void teardown_graph() {
  bg_graph_free(g);
  ck_assert_int_eq(bg_error_get(), bg_SUCCESS);
  bg_terminate();
}

static void setup_node() {
  bg_initialize();
  bg_graph_alloc(&g, "my graph");
  bg_graph_create_input(g, "foo", 1);
  bg_graph_create_input(g, "bar", 2);
  bg_graph_create_node(g, "fnord", 3, bg_NODE_TYPE_PIPE);
  bg_graph_create_output(g, "baz", 4);
  bg_graph_create_edge(g, 0, 0, 1, 0, 1., 1);
  bg_graph_create_edge(g, 1, 0, 3, 0, 3., 3);
  bg_graph_create_edge(g, 2, 0, 3, 0, -5., 4);
  bg_graph_create_edge(g, 3, 0, 4, 0, 7., 5);
  bg_edge_set_value(g, 1, 1);
  bg_node_set_default(g, 2, 0, 1);
  ck_assert_int_eq(bg_error_get(), bg_SUCCESS);
}

static void teardown_node() {
  bg_graph_free(g);
  ck_assert_int_eq(bg_error_get(), bg_SUCCESS);
  bg_terminate();
}

static void setup_node_merge() {
  bg_initialize();
  bg_graph_alloc(&g, "my graph");
  bg_graph_create_output(g, "fnord", 1);
  bg_graph_create_edge(g, 0, 0, 1, 0, -5., 1);
  bg_graph_create_edge(g, 0, 0, 1, 0, 3., 2);
  bg_edge_set_value(g, 1, 1);
  bg_edge_set_value(g, 2, 1);
  ck_assert_int_eq(bg_error_get(), bg_SUCCESS);
}

static void teardown_node_merge() {
  bg_graph_free(g);
  ck_assert_int_eq(bg_error_get(), bg_SUCCESS);
  bg_terminate();
}

static void setup_node_type() {
  bg_initialize();
  bg_graph_alloc(&g, "my graph");
  bg_graph_create_output(g, "out", 2);
  ck_assert_int_eq(bg_error_get(), bg_SUCCESS);
}

static void setup_node_type2(bg_node_type type, size_t num_in) {
  size_t i;
  size_t input_cnt, output_cnt;
  bg_node_type test_type;
  bg_graph_create_node(g, "node", 1, type);
  bg_node_get_type(g, 1, &test_type);
  ck_assert_int_eq(type, test_type);
  bg_node_get_port_cnt(g, 1, &input_cnt, &output_cnt);
  ck_assert_int_eq(input_cnt, num_in);
  ck_assert_int_eq(output_cnt, 1);
  /* create input edge */
  for(i = 0; i < num_in; ++i)
  { bg_graph_create_edge(g, 0, 0, 1, i, 1., i+1);}
  /* connect to output */
  bg_graph_create_edge(g, 1, 0, 2, 0, 1., 2);
  ck_assert_int_eq(bg_error_get(), bg_SUCCESS);
}

static void teardown_node_type() {
  bg_graph_free(g);
  ck_assert_int_eq(bg_error_get(), bg_SUCCESS);
  bg_terminate();
}


/********************
 * general API
 ********************/

START_TEST(test_bg_not_initialized) {
  bg_error err;
  ck_assert(bg_error_occurred());
  err = bg_error_get();
  ck_assert_int_eq(err, bg_ERR_NOT_INITIALIZED);
  ck_assert(!bg_is_initialized());
} END_TEST

START_TEST(test_bg_init) {
  bg_error err;
  ck_assert(!bg_is_initialized());
  err = bg_initialize();
  ck_assert_int_eq(err, bg_SUCCESS);
  ck_assert(!bg_error_occurred());
  ck_assert(bg_is_initialized());
  bg_terminate();
  ck_assert(!bg_is_initialized());
} END_TEST

START_TEST(test_bg_graph_alloc) {
  bg_graph_t *graph;
  bg_error err;
  bg_initialize();
  err = bg_graph_alloc(&graph, "my graph");
  ck_assert_int_eq(err, bg_SUCCESS);
  err = bg_graph_free(graph);
  ck_assert_int_eq(err, bg_SUCCESS);
  ck_assert_int_eq(bg_error_occurred(), bg_SUCCESS);
  bg_terminate();
} END_TEST


/********************
 * graph API
 ********************/

START_TEST(test_bg_graph_empty) {
  size_t x;
  
  bg_graph_get_node_cnt(g, false, &x);
  ck_assert_int_eq(x, 0);
  bg_graph_get_node_cnt(g, true, &x);
  ck_assert_int_eq(x, 0);
  bg_graph_get_edge_cnt(g, false, &x);
  ck_assert_int_eq(x, 0);
  bg_graph_get_edge_cnt(g, true, &x);
  ck_assert_int_eq(x, 0);
  bg_graph_get_input_nodes(g, NULL, &x);
  ck_assert_int_eq(x, 0);
  bg_graph_get_output_nodes(g, NULL, &x);
  ck_assert_int_eq(x, 0);
} END_TEST


START_TEST(test_bg_graph_create_remove) {
  size_t x;
  /* create some nodes */
  bg_graph_create_input(g, "foo", 1);
  bg_graph_create_input(g, "bar", 2);
  bg_graph_create_node(g, "baz", 3, bg_NODE_TYPE_PIPE);
  bg_graph_create_output(g, "fnord", 5);
  ck_assert_int_eq(bg_error_get(), bg_SUCCESS);
  /* create some edges */
  bg_graph_create_edge(g, 1, 0, 3, 0, 1.5, 1);
  bg_graph_create_edge(g, 3, 0, 5, 0, 2.5, 2);
  bg_graph_create_edge(g, 2, 0, 5, 0, 3.5, 3);
  ck_assert(!bg_error_occurred());
  /* create input edges */
  bg_graph_create_edge(g, 0, 0, 1, 0, 1., 100);
  bg_graph_create_edge(g, 0, 0, 2, 0, 1., 101);
  ck_assert_int_eq(bg_error_get(), bg_SUCCESS);
  /* check for consistency */
  bg_graph_get_node_cnt(g, false, &x);
  ck_assert_int_eq(x, 4);
  bg_graph_get_node_cnt(g, true, &x);
  ck_assert_int_eq(x, 4);
  bg_graph_get_edge_cnt(g, false, &x);
  ck_assert_int_eq(x, 5);
  bg_graph_get_edge_cnt(g, true, &x);
  ck_assert_int_eq(x, 5);
  bg_graph_get_input_nodes(g, NULL, &x);
  ck_assert_int_eq(x, 2);
  bg_graph_get_output_nodes(g, NULL, &x);
  ck_assert_int_eq(x, 1);
  /* remove edges */
  bg_graph_remove_edge(g, 101);
  bg_graph_remove_edge(g, 100);
  bg_graph_remove_edge(g, 1);
  bg_graph_remove_edge(g, 2);
  bg_graph_remove_edge(g, 3);
  ck_assert_int_eq(bg_error_get(), bg_SUCCESS);
  /* remove nodes */
  bg_graph_remove_node(g, 3);
  bg_graph_remove_output(g, 5);
  bg_graph_remove_input(g, 1);
  bg_graph_remove_input(g, 2);
  ck_assert_int_eq(bg_error_get(), bg_SUCCESS);
  /* check for consistency */
  bg_graph_get_node_cnt(g, false, &x);
  ck_assert_int_eq(x, 0);
  bg_graph_get_node_cnt(g, true, &x);
  ck_assert_int_eq(x, 0);
  bg_graph_get_edge_cnt(g, false, &x);
  ck_assert_int_eq(x, 0);
  bg_graph_get_edge_cnt(g, true, &x);
  ck_assert_int_eq(x, 0);
  bg_graph_get_input_nodes(g, NULL, &x);
  ck_assert_int_eq(x, 0);
  bg_graph_get_output_nodes(g, NULL, &x);
  ck_assert_int_eq(x, 0);
} END_TEST


START_TEST(test_bg_graph_get_nodes) {
  size_t x;
  bg_error err;
  bg_node_id_t ids[2];
  bg_graph_create_input(g, "foo", 3);
  bg_graph_create_node(g, "fnord", 5, bg_NODE_TYPE_PIPE);
  bg_graph_create_output(g, "bar", 2);
  bg_graph_create_output(g, "baz", 1);
  ck_assert_int_eq(bg_error_get(), bg_SUCCESS);
  /* get input nodes */
  err = bg_graph_get_input_nodes(g, NULL, &x);
  ck_assert_int_eq(err, bg_SUCCESS);
  ck_assert_int_eq(x, 1);
  err = bg_graph_get_input_nodes(g, ids, &x);
  ck_assert_int_eq(err, bg_SUCCESS);
  ck_assert_int_eq(x, 1);
  ck_assert_int_eq(ids[0], 3);
  /* get output nodes */
  err = bg_graph_get_output_nodes(g, NULL, &x);
  ck_assert_int_eq(err, bg_SUCCESS);
  ck_assert_int_eq(x, 2);
  err = bg_graph_get_output_nodes(g, ids, &x);
  ck_assert_int_eq(err, bg_SUCCESS);
  ck_assert_int_eq(x, 2);
  ck_assert_int_eq(ids[0], 2);
  ck_assert_int_eq(ids[1], 1);
} END_TEST


/********************
 * node API
 ********************/

START_TEST(test_bg_node_merge_get_set) {
  bg_error err;
  bg_merge_type m_type;
  /* check default merge types */
  err = bg_node_get_merge(g, 1, 0, &m_type);
  ck_assert_int_eq(err, bg_SUCCESS);
  ck_assert_int_eq(m_type, bg_MERGE_TYPE_SUM);
  /* set merge types */
  err = bg_node_set_merge(g, 1, 0, bg_MERGE_TYPE_PRODUCT, 1., 1.);
  ck_assert_int_eq(err, bg_SUCCESS);
  err = bg_node_get_merge(g, 1, 0, &m_type);
  ck_assert_int_eq(m_type, bg_MERGE_TYPE_PRODUCT);
} END_TEST


START_TEST(test_bg_node_merge_sum) {
  double x;
  bg_node_set_merge(g, 1, 0, bg_MERGE_TYPE_SUM, 0., 17.);
  bg_graph_evaluate(g);
  bg_node_get_output(g, 1, 0, &x);
  ck_assert_int_eq(bg_error_get(), bg_SUCCESS);
  ck_assert_flt_almost_eq(x, 3-5.+17);
} END_TEST

START_TEST(test_bg_node_merge_weighted_sum) {
  double x;
  bg_node_set_merge(g, 1, 0, bg_MERGE_TYPE_WEIGHTED_SUM, 0., 17.);
  bg_graph_evaluate(g);
  bg_node_get_output(g, 1, 0, &x);
  ck_assert_int_eq(bg_error_get(), bg_SUCCESS);
  ck_assert_flt_almost_eq(x, (3-5)/8.+17);
} END_TEST

START_TEST(test_bg_node_merge_product) {
  double x;
  bg_node_set_merge(g, 1, 0, bg_MERGE_TYPE_PRODUCT, 1., 17.);
  bg_graph_evaluate(g);
  bg_node_get_output(g, 1, 0, &x);
  ck_assert_int_eq(bg_error_get(), bg_SUCCESS);
  ck_assert_flt_almost_eq(x, 3*-5*17);
} END_TEST

START_TEST(test_bg_node_merge_min) {
  double x;
  bg_node_set_merge(g, 1, 0, bg_MERGE_TYPE_MIN, 0., 17.);
  bg_graph_evaluate(g);
  bg_node_get_output(g, 1, 0, &x);
  ck_assert_int_eq(bg_error_get(), bg_SUCCESS);
  ck_assert_flt_almost_eq(x, -5);
} END_TEST

START_TEST(test_bg_node_merge_max) {
  double x;
  bg_node_set_merge(g, 1, 0, bg_MERGE_TYPE_MAX, 0., 17.);
  bg_graph_evaluate(g);
  bg_node_get_output(g, 1, 0, &x);
  ck_assert_int_eq(bg_error_get(), bg_SUCCESS);
  ck_assert_flt_almost_eq(x, 17);
} END_TEST

START_TEST(test_bg_node_merge_median) {
  double x;
  bg_node_set_merge(g, 1, 0, bg_MERGE_TYPE_MEDIAN, 0., 17.);
  bg_graph_evaluate(g);
  bg_node_get_output(g, 1, 0, &x);
  ck_assert_int_eq(bg_error_get(), bg_SUCCESS);
  ck_assert_flt_almost_eq(x, (3.-5)/2. + 17);
} END_TEST

START_TEST(test_bg_node_merge_mean) {
  double x;
  bg_node_set_merge(g, 1, 0, bg_MERGE_TYPE_MEAN, 0., 17.);
  bg_graph_evaluate(g);
  bg_node_get_output(g, 1, 0, &x);
  ck_assert_int_eq(bg_error_get(), bg_SUCCESS);
  ck_assert_flt_almost_eq(x, (3.-5)/2. + 17);
} END_TEST

START_TEST(test_bg_node_merge_norm) {
  double x;
  bg_node_set_merge(g, 1, 0, bg_MERGE_TYPE_NORM, 0., 17.);
  bg_graph_evaluate(g);
  bg_node_get_output(g, 1, 0, &x);
  ck_assert_int_eq(bg_error_get(), bg_SUCCESS);
  ck_assert_flt_almost_eq(x, sqrt(3.*3 + (-5*-5) + 17*17));
} END_TEST

START_TEST(test_bg_node_type_pipe) {
  double x;
  setup_node_type2(bg_NODE_TYPE_PIPE, 1);
  /* test values */
  bg_edge_set_value(g, 1, test_vals[_i]);
  bg_graph_evaluate(g);
  bg_graph_get_output(g, 0, &x);
  ck_assert_flt_almost_eq(x, test_vals[_i]);
} END_TEST

START_TEST(test_bg_node_type_sin) {
  double x;
  setup_node_type2(bg_NODE_TYPE_SIN, 1);
  /* test values */
  bg_edge_set_value(g, 1, test_vals[_i]);
  bg_graph_evaluate(g);
  bg_graph_get_output(g, 0, &x);
  ck_assert_flt_almost_eq(x, sin(test_vals[_i]));
} END_TEST

START_TEST(test_bg_node_type_cos) {
  double x;
  setup_node_type2(bg_NODE_TYPE_COS, 1);
  /* test values */
  bg_edge_set_value(g, 1, test_vals[_i]);
  bg_graph_evaluate(g);
  bg_graph_get_output(g, 0, &x);
  ck_assert_flt_almost_eq(x, cos(test_vals[_i]));
} END_TEST

START_TEST(test_bg_node_type_tan) {
  double x;
  setup_node_type2(bg_NODE_TYPE_TAN, 1);
  /* test values */
  bg_edge_set_value(g, 1, test_vals[_i]);
  bg_graph_evaluate(g);
  bg_graph_get_output(g, 0, &x);
  ck_assert_flt_almost_eq(x, tan(test_vals[_i]));
} END_TEST

START_TEST(test_bg_node_type_acos) {
  double x;
  setup_node_type2(bg_NODE_TYPE_ACOS, 1);
  /* test values */
  bg_edge_set_value(g, 1, test_vals[_i]);
  bg_graph_evaluate(g);
  bg_graph_get_output(g, 0, &x);
  if(-1 <= test_vals[_i] && test_vals[_i] <= 1) {
    ck_assert_flt_almost_eq(x, acos(test_vals[_i]));
  } else {
    ck_assert_flt_nan(x);
  }
} END_TEST

START_TEST(test_bg_node_type_atan2) {
  static size_t old_i = test_vals_num - 1;
  double x;
  setup_node_type2(bg_NODE_TYPE_ATAN2, 2);
  /* test values */
  bg_edge_set_value(g, 1, test_vals[_i]);
  bg_edge_set_value(g, 2, test_vals[old_i]);
  bg_graph_evaluate(g);
  bg_graph_get_output(g, 0, &x);
  ck_assert_flt_almost_eq(x, atan2(test_vals[_i], test_vals[old_i]));
  old_i = _i;
} END_TEST

START_TEST(test_bg_node_type_pow) {
  static size_t old_i = test_vals_num - 1;
  double x, ref_result;
  setup_node_type2(bg_NODE_TYPE_POW, 2);
  /* test values */
  bg_edge_set_value(g, 1, test_vals[_i]);
  bg_edge_set_value(g, 2, test_vals[old_i]);
  bg_graph_evaluate(g);
  bg_graph_get_output(g, 0, &x);
  ref_result = pow(test_vals[_i], test_vals[old_i]);
  ck_assert(x == ref_result || (isnan(ref_result) && isnan(x)));
  old_i = _i;
} END_TEST

START_TEST(test_bg_node_type_mod) {
  static size_t old_i = test_vals_num - 1;
  double x;
  setup_node_type2(bg_NODE_TYPE_MOD, 2);
  /* test values */
  bg_edge_set_value(g, 1, test_vals[_i]);
  bg_edge_set_value(g, 2, test_vals[old_i]);
  bg_graph_evaluate(g);
  bg_graph_get_output(g, 0, &x);
  ck_assert_flt_almost_eq(x, fmod(test_vals[_i], test_vals[old_i]));
  old_i = _i;
} END_TEST

START_TEST(test_bg_node_type_abs) {
  double x;
  setup_node_type2(bg_NODE_TYPE_ABS, 1);
  /* test values */
  bg_edge_set_value(g, 1, test_vals[_i]);
  bg_graph_evaluate(g);
  bg_graph_get_output(g, 0, &x);
  ck_assert_flt_almost_eq(x, fabs(test_vals[_i]));
} END_TEST

START_TEST(test_bg_node_type_sqrt) {
  double x;
  setup_node_type2(bg_NODE_TYPE_SQRT, 1);
  /* test values */
  bg_edge_set_value(g, 1, test_vals[_i]);
  bg_graph_evaluate(g);
  bg_graph_get_output(g, 0, &x);
  if(0. <= test_vals[_i]) {
    ck_assert_flt_almost_eq(x, sqrt(test_vals[_i]));
  } else {
    ck_assert_flt_nan(x);
  }
} END_TEST


START_TEST(test_bg_node_type_fsigmoid) {
  double x;
  setup_node_type2(bg_NODE_TYPE_FSIGMOID, 1);
  /* test values */
  bg_edge_set_value(g, 1, -4);
  bg_graph_evaluate(g);
  bg_graph_get_output(g, 0, &x);
  ck_assert_flt_almost_eq(x, 0);
  bg_edge_set_value(g, 1, 0);
  bg_graph_evaluate(g);
  bg_graph_get_output(g, 0, &x);
  ck_assert_flt_almost_eq(x, 0.5);
  bg_edge_set_value(g, 1, 4);
  bg_graph_evaluate(g);
  bg_graph_get_output(g, 0, &x);
  ck_assert_flt_almost_eq(x, 1);
} END_TEST


START_TEST(test_div_zero) {
  double x;
  setup_node_type2(bg_NODE_TYPE_DIVIDE, 1);
  /* test values */
  bg_edge_set_value(g, 1, 1);
  bg_graph_evaluate(g);
  bg_graph_get_output(g, 0, &x);
  ck_assert_flt_almost_eq(x, 1);
  bg_edge_set_value(g, 1, 0);
  bg_graph_evaluate(g);
  bg_graph_get_output(g, 0, &x);
  ck_assert_flt_inf(x);
} END_TEST

START_TEST(test_mod_zero) {
  double x;
  setup_node_type2(bg_NODE_TYPE_MOD, 2);
  /* test values */
  bg_edge_set_value(g, 1, 1);
  bg_edge_set_value(g, 2, 0);
  bg_graph_evaluate(g);
  bg_graph_get_output(g, 0, &x);
  ck_assert_flt_nan(x);
} END_TEST

START_TEST(test_acos_err) {
  double x;
  setup_node_type2(bg_NODE_TYPE_ACOS, 1);
  /* test values */
  bg_edge_set_value(g, 1, 1.1);
  bg_graph_evaluate(g);
  bg_graph_get_output(g, 0, &x);
  ck_assert_flt_nan(x);
} END_TEST

START_TEST(test_sqrt_neg) {
  double x;
  setup_node_type2(bg_NODE_TYPE_SQRT, 1);
  /* test values */
  bg_edge_set_value(g, 1, -1.);
  bg_graph_evaluate(g);
  bg_graph_get_output(g, 0, &x);
  ck_assert_flt_nan(x);
} END_TEST



START_TEST(test_simple_net) {
  double x, y, result;
  bg_graph_create_input(g, "x", 1);
  bg_graph_create_input(g, "y", 2);
  bg_graph_create_node(g, "x*x", 3, bg_NODE_TYPE_PIPE);
  bg_graph_create_node(g, "y*y", 4, bg_NODE_TYPE_PIPE);
  bg_graph_create_output(g, "x*x + y*y", 5);
  bg_node_set_merge(g, 3, 0, bg_MERGE_TYPE_PRODUCT, 1, 1);
  bg_node_set_merge(g, 4, 0, bg_MERGE_TYPE_PRODUCT, 1, 1);
  bg_graph_create_edge(g, 0, 0, 1, 0, 1., 1);
  bg_graph_create_edge(g, 0, 0, 2, 0, 1., 2);
  bg_graph_create_edge(g, 1, 0, 3, 0, 1., 3);
  bg_graph_create_edge(g, 2, 0, 4, 0, 1., 4);
  bg_graph_create_edge(g, 1, 0, 3, 0, 1., 5);
  bg_graph_create_edge(g, 2, 0, 4, 0, 1., 6);
  bg_graph_create_edge(g, 3, 0, 5, 0, 1., 7);
  bg_graph_create_edge(g, 4, 0, 5, 0, 1., 8);
  ck_assert_int_eq(bg_error_get(), bg_SUCCESS);
  x = 7.;
  y = 5.;
  bg_edge_set_value(g, 1, x);
  bg_edge_set_value(g, 2, y);
  bg_graph_evaluate(g);
  bg_graph_get_output(g, 0, &result);
  ck_assert_flt_almost_eq(result, x*x + y*y);
} END_TEST


Suite* bg_suite() {
  Suite *s = suite_create("c_bagel");
  TCase *tc_general, *tc_graph, *tc_node, *tc_node_merge, *tc_node_type;
  TCase *tc_float_exc, *tc_networks;

  tc_general = tcase_create("General");
  tcase_add_test(tc_general, test_bg_not_initialized);
  tcase_add_test(tc_general, test_bg_init);
  tcase_add_test(tc_general, test_bg_graph_alloc);
  suite_add_tcase(s, tc_general);

  tc_graph = tcase_create("Graph");
  tcase_add_checked_fixture(tc_graph, setup_graph, teardown_graph);
  tcase_add_test(tc_graph, test_bg_graph_empty);
  tcase_add_test(tc_graph, test_bg_graph_create_remove);
  tcase_add_test(tc_graph, test_bg_graph_get_nodes);
  suite_add_tcase(s, tc_graph);

  tc_node = tcase_create("Node");
  tcase_add_checked_fixture(tc_node, setup_node, teardown_node);
  suite_add_tcase(s, tc_node);

  tc_node_merge = tcase_create("Node Merges");
  tcase_add_checked_fixture(tc_node_merge, setup_node_merge,
                            teardown_node_merge);
  tcase_add_test(tc_node_merge, test_bg_node_merge_get_set);
  tcase_add_test(tc_node_merge, test_bg_node_merge_sum);
  tcase_add_test(tc_node_merge, test_bg_node_merge_weighted_sum);
  tcase_add_test(tc_node_merge, test_bg_node_merge_product);
  tcase_add_test(tc_node_merge, test_bg_node_merge_min);
  tcase_add_test(tc_node_merge, test_bg_node_merge_max);
  tcase_add_test(tc_node_merge, test_bg_node_merge_median);
  tcase_add_test(tc_node_merge, test_bg_node_merge_mean);
  tcase_add_test(tc_node_merge, test_bg_node_merge_norm);
  suite_add_tcase(s, tc_node_merge);

  tc_node_type = tcase_create("Node Types");
  tcase_add_checked_fixture(tc_node_type, setup_node_type, teardown_node_type);
  tcase_add_loop_test(tc_node_type, test_bg_node_type_pipe, 0, test_vals_num);
  tcase_add_loop_test(tc_node_type, test_bg_node_type_sin, 0, test_vals_num);
  tcase_add_loop_test(tc_node_type, test_bg_node_type_cos, 0, test_vals_num);
  tcase_add_loop_test(tc_node_type, test_bg_node_type_tan, 0, test_vals_num);
  tcase_add_loop_test(tc_node_type, test_bg_node_type_acos, 0, test_vals_num);
  tcase_add_loop_test(tc_node_type, test_bg_node_type_atan2, 0, test_vals_num);
  tcase_add_loop_test(tc_node_type, test_bg_node_type_pow, 0, test_vals_num);
  tcase_add_loop_test(tc_node_type, test_bg_node_type_mod, 0, test_vals_num);
  tcase_add_loop_test(tc_node_type, test_bg_node_type_abs, 0, test_vals_num);
  tcase_add_loop_test(tc_node_type, test_bg_node_type_sqrt, 0, test_vals_num);
  tcase_add_test(tc_node_type, test_bg_node_type_fsigmoid);
  suite_add_tcase(s, tc_node_type);

  tc_float_exc = tcase_create("Float Exceptions");
  tcase_add_checked_fixture(tc_float_exc, setup_node_type, teardown_node_type);
  tcase_add_test(tc_float_exc, test_div_zero);
  tcase_add_test(tc_float_exc, test_mod_zero);
  tcase_add_test(tc_float_exc, test_acos_err);
  tcase_add_test(tc_float_exc, test_sqrt_neg);
  suite_add_tcase(s, tc_float_exc);

  tc_networks = tcase_create("Networks");
  tcase_add_checked_fixture(tc_networks, setup_graph, teardown_graph);
  tcase_add_test(tc_networks, test_simple_net);
  suite_add_tcase(s, tc_networks);

  return s;
}
