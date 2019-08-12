#include "../src/bagel.h"
#include "bg_test.h"
#include <string.h>
#include <math.h>

extern char base_dir[];

START_TEST(test_simple_graph) {
  double x, y, result;
  bg_graph_t *g;
  char path[MAX_STRING_SIZE];
  bg_initialize();
  bg_graph_alloc(&g, "my graph");
  /* get the correct path from the base_dir and a hardcoded part */
  strncpy(path, base_dir, MAX_STRING_SIZE);
  strncat(path, "/test_graphs/simpleTest.yml", MAX_STRING_SIZE);
  bg_graph_from_yaml_file(path, g);
  ck_assert_int_eq(bg_error_get(), bg_SUCCESS);
  bg_graph_create_edge(g, 0, 0, 2, 0, 1., 10);
  bg_graph_create_edge(g, 0, 0, 4, 0, 1., 11);
  x = 3.;
  y = 5.;
  bg_edge_set_value(g, 10, x);
  bg_edge_set_value(g, 11, y);
  bg_graph_evaluate(g);
  bg_graph_get_output(g, 0, &result);
  ck_assert_int_eq(bg_error_get(), bg_SUCCESS);
  ck_assert_flt_almost_eq(result, x*x + y*y);
  ck_assert_int_eq(bg_error_get(), bg_SUCCESS);
  bg_graph_free(g);
  ck_assert_int_eq(bg_error_get(), bg_SUCCESS);
  bg_terminate();
} END_TEST


Suite* bg_yaml_suite() {
  Suite *s = suite_create("c_bagel - YAML Loader");
  TCase *tc_general;

  tc_general = tcase_create("General");
  tcase_add_test(tc_general, test_simple_graph);
  suite_add_tcase(s, tc_general);

  return s;
}
