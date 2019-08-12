#include "../src/bagel.h"
#include "bg_test.h"
#include <string.h>
#include <math.h>
#include <assert.h>

extern char base_dir[];

START_TEST(test_div_zero) {
  double in[1][2], ***out;
  double diameter;
  size_t cnt=0;
  bg_graph_t *g;
  char path[MAX_STRING_SIZE];
  double res = 1e-6;
  bg_initialize();
  bg_graph_alloc(&g, "my graph");
  /* get the correct path from the base_dir and a hardcoded part */
  strncpy(path, base_dir, MAX_STRING_SIZE);
  strncat(path, "/test_graphs/nanTest.yml", MAX_STRING_SIZE);
  bg_graph_from_yaml_file(path, g);
  ck_assert_int_eq(bg_error_get(), bg_SUCCESS);

  in[0][0] = -1./0.;
  in[0][1] = 1./0.;
  bg_interval_find_inf_nan(g, res, in, 1, &out, &cnt);
  ck_assert_int_eq(bg_error_get(), bg_SUCCESS);
  ck_assert_int_eq(cnt, 2);
  ck_assert((out[0][0][0] <= 1.5 && 1.5 <= out[0][0][1]) ||
            (out[1][0][0] <= 1.5 && 1.5 <= out[1][0][1]));
  ck_assert((out[0][0][0] <= -0.5 && -0.5 <= out[0][0][1]) ||
            (out[1][0][0] <= -0.5 && -0.5 <= out[1][0][1]));
  diameter = out[0][0][1] - out[0][0][0];
  ck_assert(diameter <= 2*res);
  diameter = out[1][0][1] - out[1][0][0];
  ck_assert(diameter <= 2*res);
  bg_interval_free_inf_nan(out, 1, cnt);
  ck_assert_int_eq(bg_error_get(), bg_SUCCESS);

  in[0][0] = -1./0.;
  in[0][1] = 10.;
  bg_interval_find_inf_nan(g, res, in, 1, &out, &cnt);
  ck_assert_int_eq(bg_error_get(), bg_SUCCESS);
  ck_assert_int_eq(cnt, 2);
  bg_interval_free_inf_nan(out, 1, cnt);
  ck_assert_int_eq(bg_error_get(), bg_SUCCESS);

  in[0][0] = -10.;
  in[0][1] = 1./0.;
  bg_interval_find_inf_nan(g, res, in, 1, &out, &cnt);
  ck_assert_int_eq(bg_error_get(), bg_SUCCESS);
  ck_assert_int_eq(cnt, 2);
  bg_interval_free_inf_nan(out, 1, cnt);
  ck_assert_int_eq(bg_error_get(), bg_SUCCESS);

  in[0][0] = 1.1;
  in[0][1] = 1./0.;
  bg_interval_find_inf_nan(g, res, in, 1, &out, &cnt);
  ck_assert_int_eq(bg_error_get(), bg_SUCCESS);
  ck_assert_int_eq(cnt, 1);
  ck_assert(out[0][0][0] <= 1.5 && 1.5 <= out[0][0][1]);
  diameter = out[0][0][1] - out[0][0][0];
  ck_assert(diameter <= 2*res);
  bg_interval_free_inf_nan(out, 1, cnt);
  ck_assert_int_eq(bg_error_get(), bg_SUCCESS);

  in[0][0] = -1.1;
  in[0][1] = 1.;
  bg_interval_find_inf_nan(g, res, in, 1, &out, &cnt);
  ck_assert_int_eq(bg_error_get(), bg_SUCCESS);
  ck_assert_int_eq(cnt, 1);
  ck_assert(out[0][0][0] <= -0.5 && -0.5 <= out[0][0][1]);
  diameter = out[0][0][1] - out[0][0][0];
  ck_assert(diameter <= 2*res);
  bg_interval_free_inf_nan(out, 1, cnt);
  ck_assert_int_eq(bg_error_get(), bg_SUCCESS);

  bg_graph_free(g);
  ck_assert_int_eq(bg_error_get(), bg_SUCCESS);
  bg_terminate();
} END_TEST


START_TEST(test_div_zero_two_inputs) {
  double in[2][2], ***out;
  size_t cnt=0;
  bg_graph_t *g;
  char path[MAX_STRING_SIZE];
  double res = 1e-1;
  bg_initialize();
  bg_graph_alloc(&g, "my graph");
  /* get the correct path from the base_dir and a hardcoded part */
  strncpy(path, base_dir, MAX_STRING_SIZE);
  strncat(path, "/test_graphs/nan2Test.yml", MAX_STRING_SIZE);
  bg_graph_from_yaml_file(path, g);
  ck_assert_int_eq(bg_error_get(), bg_SUCCESS);

  in[0][0] = -1.;
  in[0][1] = 1.;
  in[1][0] = -1.;
  in[1][1] = 1.;
  bg_interval_find_inf_nan(g, res, in, 2, &out, &cnt);
  ck_assert_int_eq(bg_error_get(), bg_SUCCESS);
  ck_assert_int_eq(cnt, 3);
  bg_interval_free_inf_nan(out, 2, cnt);
  ck_assert_int_eq(bg_error_get(), bg_SUCCESS);

  bg_graph_free(g);
  ck_assert_int_eq(bg_error_get(), bg_SUCCESS);
  bg_terminate();
} END_TEST


START_TEST(test_acos) {
  double in[1][2], ***out;
  size_t cnt=0;
  bg_graph_t *g;
  char path[MAX_STRING_SIZE];
  double res = 1e-6;
  bg_initialize();
  bg_graph_alloc(&g, "my graph");
  /* get the correct path from the base_dir and a hardcoded part */
  strncpy(path, base_dir, MAX_STRING_SIZE);
  strncat(path, "/test_graphs/acosTest.yml", MAX_STRING_SIZE);
  bg_graph_from_yaml_file(path, g);
  ck_assert_int_eq(bg_error_get(), bg_SUCCESS);

  in[0][0] = -1.01;
  in[0][1] = 1.01;
  bg_interval_find_inf_nan(g, res, in, 1, &out, &cnt);
  ck_assert_int_eq(bg_error_get(), bg_SUCCESS);
  ck_assert_int_eq(cnt, 2);
  ck_assert((out[0][0][0] <= 1.0001 && 1.0001 <= out[0][0][1]) ||
            (out[1][0][0] <= 1.0001 && 1.0001 <= out[1][0][1]));
  ck_assert((out[0][0][0] <= -1.0001 && -1.0001 <= out[0][0][1]) ||
            (out[1][0][0] <= -1.0001 && -1.0001 <= out[1][0][1]));
  bg_interval_free_inf_nan(out, 1, cnt);
  ck_assert_int_eq(bg_error_get(), bg_SUCCESS);

  bg_graph_free(g);
  ck_assert_int_eq(bg_error_get(), bg_SUCCESS);
  bg_terminate();
} END_TEST


Suite* bg_interval_suite() {
  Suite *s = suite_create("c_bagel - Intervals");
  TCase *tc_general;

  tc_general = tcase_create("General");

  tcase_add_test(tc_general, test_div_zero);
  tcase_add_test(tc_general, test_div_zero_two_inputs);
  tcase_add_test(tc_general, test_acos);

  suite_add_tcase(s, tc_general);

  return s;
}
