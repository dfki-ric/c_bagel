#include "bg_impl.h"

#ifdef INTERVAL_SUPPORT


#include "bg_graph.h"
#include "bg_node.h"
#include "generic_list.h"
#include "node_list.h"
#include <assert.h>
#include <float.h>


LIST_TYPE_DEF(interval, mpfi_t)
LIST_TYPE_IMPL(interval, mpfi_t)
LIST_TYPE_DEF(interval2, bg_interval_list_t)
LIST_TYPE_IMPL(interval2, bg_interval_list_t)

typedef enum { BISECT_NORMAL,
               BISECT_EXP } BisectionMode;

/* forward declarations of static functions */
static bg_error bg_interval_bisect(mpfi_t interval, BisectionMode mode,
                                   mpfi_t *left, mpfi_t *right);
static bg_error bg_interval_merge(bg_interval2_list_t *list);
static void print_list_of_interval_lists(bg_interval2_list_t *l);
static void print_list_of_intervals(bg_interval_list_t *l);
static void free_interval_list(bg_interval_list_t *list);
static bg_error bg_graph_detect_inf_nan_intern(bg_graph_t *graph,
                                               bg_interval_list_t *input_intervals,
                                               bool *detected);
static void bg_interval_get_endpoints(mpfi_t interval,
                                      bg_real *left, bg_real *right);
static bg_error bg_interval_evaluate_node(bg_node_t *node);


bg_error bg_interval_get_node_output(const bg_graph_t *graph,
                                     bg_node_id_t node_id,
                                     size_t output_port_idx,
                                     bg_real interval[2]) {
  bg_node_t *node = NULL;
  output_port_t *port;
  bg_error err = bg_graph_find_node((bg_graph_t*)graph, node_id, &node);
  if(err == bg_SUCCESS) {
    if(node) {
      if(node->output_port_cnt > output_port_idx) {
        port = node->output_ports[output_port_idx];
        bg_interval_get_endpoints(port->value_intv, &interval[0], &interval[1]);
      } else {
        err = bg_error_set(bg_ERR_OUT_OF_RANGE);
      }
    } else {
      err = bg_error_set(bg_ERR_NODE_NOT_FOUND);
    }
  }
  return err;
}

bg_error bg_interval_get_graph_output(const bg_graph_t *graph,
                                      size_t output_port_idx,
                                      bg_real interval[2]) {
  bg_error err = bg_SUCCESS;
  output_port_t *port;
  if(graph->output_port_cnt > output_port_idx) {
    port = graph->output_ports[output_port_idx];
    bg_interval_get_endpoints(port->value_intv, &interval[0], &interval[1]);
  } else {
    err = bg_error_set(bg_ERR_OUT_OF_RANGE);
  }
  return err;

}

bg_error bg_interval_set_edge_value(bg_graph_t *graph, bg_edge_id_t edge_id,
                                    const bg_real value_interval[2]) {
  bg_edge_t *edge = NULL;
  bg_error err = bg_graph_find_edge((bg_graph_t*)graph, edge_id, &edge);
  if(err == bg_SUCCESS) {
    if(edge) {
      mpfi_interv_d(edge->value_intv, value_interval[0], value_interval[1]);
    } else {
      err = bg_error_set(bg_ERR_EDGE_NOT_FOUND);
    }
  }
  return err;
}

bg_error bg_interval_get_edge_value(const bg_graph_t *graph,
                                    bg_edge_id_t edge_id,
                                    bg_real value_interval[2]) {
  bg_edge_t *edge = NULL;
  bg_error err = bg_graph_find_edge((bg_graph_t*)graph, edge_id, &edge);
  if(err == bg_SUCCESS) {
    if(edge) {
      bg_interval_get_endpoints(edge->value_intv,
                                &value_interval[0], &value_interval[1]);
    } else {
      err = bg_error_set(bg_ERR_EDGE_NOT_FOUND);
    }
  }
  return err;
}


bg_error bg_interval_evaluate_graph(bg_graph_t *graph) {
  bg_error err = bg_SUCCESS;
  bg_node_t *current_node;
  bg_node_list_t *node_list = graph->evaluation_order;
  bg_node_list_iterator_t node_it;
  if(graph->eval_order_is_dirty) {
    determine_evaluation_order(graph);
  }
  for(current_node = bg_node_list_first(node_list, &node_it);
      current_node; current_node = bg_node_list_next(&node_it)) {
    err = bg_interval_evaluate_node(current_node);
  }
  return err;
}

bg_error bg_interval_detect_inf_nan(bg_graph_t *graph,
                                    bg_real input_intervals[][2],
                                    const size_t n, bool *detected) {
  bg_error err;
  bg_interval_list_t *list;
  bg_interval_list_iterator_t list_it;
  mpfi_t *interval;
  size_t i;
  bg_interval_list_init(&list);
  for(i = 0; i < n; ++i) {
    interval = (mpfi_t*)calloc(1, sizeof(mpfi_t));
    mpfi_init(*interval);
    mpfi_interv_d(*interval, input_intervals[i][0], input_intervals[i][1]);
    bg_interval_list_append(list, interval);
  }
  err = bg_graph_detect_inf_nan_intern(graph, (bg_interval_list_t*)list,
                                       detected);
  for(interval = bg_interval_list_first(list, &list_it);
      interval; interval = bg_interval_list_next(&list_it)) {
    mpfi_clear(*interval);
    free(interval);
  }
  bg_interval_list_deinit(list);
  return err;
}

/**
 * @param input_intervals Array of intervals that should be test for NaNs
 * @param n The number of input_intervals. Should be the same as the number of inputs of the graph
 * @param output_intervals Pointer to a pointer to a pointer of intervals. This will be filled with an array of grouped intervals. it will represent a two dimensional array with the first dimension being the number of solutions found and the second dimension the number of inputs.
 * @param m the size of the first dimension of output_intervals.
 *
 */
bg_error bg_interval_find_inf_nan(bg_graph_t *graph, bg_real resolution,
                                  bg_real input_intervals[][2], size_t n,
                                  bg_real ****result_intervals, size_t *m) {
  bool found_nan, interval_bisected;
  size_t i, j;
  bg_error err = bg_SUCCESS;
  mpfi_t *intv, *tmp, *tmp2, left, right;
  mpfr_t diameter;
  bg_interval2_list_t *output_list;
  bg_interval2_list_t *test_intervals_list;
  bg_interval2_list_iterator_t interval2_it;
  bg_interval_list_t *intv_list;
  bg_interval_list_t *tmp_list;
  bg_interval_list_iterator_t interval_it, interval_it2;

  /* sanity check */
  if(graph->input_port_cnt != n) {
    fprintf(stderr,
            "ERROR: wrong number of arguments!\n"
            "       graph takes %lu arguments but only %lu were provided.\n",
            graph->input_port_cnt, n);
    return bg_error_set(bg_ERR_WRONG_ARG_COUNT);
  }

  /* Print some information */
  printf("searching for NaN and Inf in the following input intervals:\n");
  {
    bg_real abs_left, abs_right;
    bg_real cnt = 1;
    bool is_bounded = true;
    for(i = 0; i < n; ++i) {
      abs_left = input_intervals[i][0];
      abs_right = input_intervals[i][1];
      cnt *= (abs_right - abs_left) / resolution;
      printf("  [%.6f, %.6f]\n", abs_left, abs_right);
      if(input_intervals[i][0] != input_intervals[i][0] ||
         input_intervals[i][1] != input_intervals[i][1]) {
        fprintf(stderr, "ERROR: invalid interval!\n");
        return bg_error_set(bg_ERR_UNKNOWN);
      }
      if(is_bounded && (input_intervals[i][0] == 1./0. ||
                        input_intervals[i][0] == -1./0. ||
                        input_intervals[i][1] == 1./0. ||
                        input_intervals[i][1] == -1./0.)) {
        fprintf(stderr, "WARNING: searching an unbound interval may take very long and may require a lot of resources. In fact it may not succeed at all.\n");
        is_bounded = false;
      }
    }
    printf("with a granularity of %g\n", resolution);
    if(cnt > 1e6 && is_bounded) {
      printf("Note that searching these intervals with this granularity may require up to %.0f evaluations.\n", 2*cnt);
    }
  }

  /* create a list containing a list for each input with the original input interval as its only element */
  mpfr_init(diameter);
  mpfi_init(left);
  mpfi_init(right);
  bg_interval2_list_init(&test_intervals_list);
  bg_interval2_list_init(&output_list);
  bg_interval_list_init(&intv_list);
  for(i = 0; i < n; ++i) {
    intv = (mpfi_t*)calloc(1, sizeof(mpfi_t));
    mpfi_init(*intv);
    mpfi_interv_d(*intv, input_intervals[i][0], input_intervals[i][1]);
    bg_interval_list_append(intv_list, intv);
  }
  bg_interval2_list_append(test_intervals_list, intv_list);

  /* begin bisection */
  while(bg_interval2_list_size(test_intervals_list) > 0) {
    intv_list = bg_interval2_list_last(test_intervals_list, &interval2_it);
    err = bg_graph_detect_inf_nan_intern(graph, intv_list, &found_nan);
    if(err != bg_SUCCESS) {
      break;
    }
    if(found_nan) {
      /* bisect if any input is larger than resolution */
      interval_bisected = false;
      for(intv = bg_interval_list_first(intv_list, &interval_it);
          intv; intv = bg_interval_list_next(&interval_it)) {
        mpfi_diam_abs(diameter, *intv);
        if(mpfi_inf_p(*intv) || mpfr_cmp_d(diameter, resolution) > 0) {
          bg_interval_list_init(&tmp_list);
          bg_interval_bisect(*intv, BISECT_EXP, &left, &right);
          /* deepcopy the list */
          for(tmp = bg_interval_list_first(intv_list, &interval_it2);
              tmp; tmp = bg_interval_list_next(&interval_it2)) {
            tmp2 = (mpfi_t*)calloc(1, sizeof(mpfi_t));
            mpfi_init_set(*tmp2, *tmp);
            bg_interval_list_append(tmp_list, tmp2);
            if(tmp == intv) {
              /* replace the bisected interval in each copy of the list */
              mpfi_set(*tmp, left);
              mpfi_set(*tmp2, right);
            }
          }
          bg_interval2_list_append(test_intervals_list, tmp_list);
          interval_bisected = true;
          break;
        }
      }
      /* if the interval was not bisected we reached the final resolution */
      if(!interval_bisected) {
        bg_interval2_list_append(output_list, intv_list);
        bg_interval2_list_erase(&interval2_it);
        bg_interval_merge(output_list);
      }
    } else {
      /* discard the current intervals as they
       * do not produce NaN in the output */
      free_interval_list(intv_list);
      bg_interval2_list_erase(&interval2_it);
    }
  }

  /* allocate output and fill it and cleanup */
  *m = bg_interval2_list_size(output_list);
  *result_intervals = (double***)calloc(*m, sizeof(double**));
  for(i = 0, intv_list = bg_interval2_list_first(output_list, &interval2_it);
      intv_list; ++i, intv_list = bg_interval2_list_next(&interval2_it)) {
    (*result_intervals)[i] = (double**)calloc(n, sizeof(double*));
    for(j = 0, intv = bg_interval_list_first(intv_list, &interval_it);
        intv; ++j, intv = bg_interval_list_next(&interval_it)) {
      (*result_intervals)[i][j] = (double*)calloc(2, sizeof(double));
      bg_interval_get_endpoints(*intv, &(*result_intervals)[i][j][0],
                                &(*result_intervals)[i][j][1]);
    }
    free_interval_list(intv_list);
  }

  /* under normal circumstances size of test_intervals_list should be zero,
   * but may be != 0 if there was an error. In that case clean up. */
  for(intv_list = bg_interval2_list_first(test_intervals_list, &interval2_it);
      intv_list; intv_list = bg_interval2_list_next(&interval2_it)) {
    free_interval_list(intv_list);
  }
  bg_interval2_list_deinit(test_intervals_list);
  bg_interval2_list_deinit(output_list);
  mpfr_clear(diameter);
  mpfi_clear(left);
  mpfi_clear(right);
  return err;
}

bg_error bg_interval_free_inf_nan(bg_real ***intervals, size_t n, size_t m) {
  size_t i, j;
  for(i = 0; i < m; ++i) {
    for(j = 0; j < n; ++j) {
      free(intervals[i][j]);
    }
    free(intervals[i]);
  }
  free(intervals);
  return bg_SUCCESS;
}




/***********************************************
 * static functions implementation
 ***********************************************/

static void bg_interval_get_endpoints(mpfi_t interval,
                                      bg_real *left, bg_real *right) {
  mpfr_t tmp;
  mpfr_init(tmp);
  mpfi_get_left(tmp, interval);
  *left = mpfr_get_d(tmp, MPFR_RNDD);
  mpfi_get_right(tmp, interval);
  *right = mpfr_get_d(tmp, MPFR_RNDD);
  mpfr_clear(tmp);
}

static void free_interval_list(bg_interval_list_t *list) {
  bg_interval_list_iterator_t it;
  mpfi_t *interval;
  interval = bg_interval_list_first(list, &it);
  while(interval) {
    mpfi_clear(*interval);
    free(interval);
    interval = bg_interval_list_erase(&it);
  }
  bg_interval_list_deinit(list);
}

static bg_error bg_graph_detect_inf_nan_intern(bg_graph_t *graph,
                                               bg_interval_list_t *input_intervals,
                                               bool *detected) {
  bg_error err = bg_SUCCESS;
  bg_interval_list_iterator_t interval_it;
  mpfi_t *interval;
  size_t base_id;
  size_t i, n=bg_interval_list_size(input_intervals);
  if(graph->input_port_cnt != n) {
    fprintf(stderr,
            "ERROR: wrong number of arguments!\n"
            "       graph takes %lu arguments but only %lu were provided.\n",
            graph->input_port_cnt, n);
    return bg_error_set(bg_ERR_WRONG_ARG_COUNT);
  }

  if(bg_SUCCESS != bg_graph_reset(graph, true)) {
    return bg_error_get();
  }

  /* create input connections and set the intervals */
  {
    bg_real tmp[2];
    /* Get first unused edges id */
    bg_graph_get_max_edge_id(graph, &base_id);
    base_id++;
    for(i = 0, interval = bg_interval_list_first(input_intervals, &interval_it);
        i < n && interval; ++i, interval=bg_interval_list_next(&interval_it)) {
      err = bg_graph_create_edge(graph, 0, 0, i+1, 0, 1., base_id + i);
      if(err != bg_SUCCESS) {
        break;
      }
      bg_interval_get_endpoints(*interval, &tmp[0], &tmp[1]);
      bg_interval_set_edge_value(graph, base_id + i, tmp);
    }
  }

  if(err == bg_SUCCESS) {
    /* evaluate the graph */
    err = bg_interval_evaluate_graph(graph);
    if(err == bg_SUCCESS) {
      /* check for NaNs */
      *detected = false;
      for(i = 0; i < graph->output_port_cnt; ++i) {
        if(mpfi_inf_p(graph->output_ports[i]->value_intv) ||
           mpfi_nan_p(graph->output_ports[i]->value_intv)) {
          *detected = true;
          break;
        }
      }
    }
  }

  /* clean up (e.g., input edges) */
  for(i = 0; i < n; ++i) {
    err = bg_graph_remove_edge(graph, base_id + i);
    if(err != bg_SUCCESS) {
      break;
    }
  }

  return err;
}


/** \brief bisects the Interval in to halves.
 * Halves the Interval and sets \c left to the lower half and \c right to
 * the higher half. If the Interval contains \c -Inf or \c +Inf the \c mode
 * will decide how the Interval is bisected. In BISECT_NORMAL mode \c -Inf
 * and \c +Inf will be replaced by \c -DBL_MAX and \c DBL_MAX respectivly.
 * The BISECT_EXP mode will first try to bisect at Zero and then at
 * increasing powers of two (in steps of 10). Here are some examples for
 * the BISECT_EXP mode:
 *   [-Inf,Inf] -> [-Inf,0] [0,Inf]
 *   [-Inf,37] -> [-Inf,0] [0,37]
 *   [37,Inf] -> [37,37*1024] [37*1024,Inf] (1024 == 2**10)
 *   [37*1024,Inf] -> [37*1024,37*1048576] [37*1048576,Inf] (1048576 == 2**20)
 *   [1048576,Inf] -> [1048576,1073741824] [1073741824,Inf]
 * When bisecting very large intervals (say [-Inf,Inf]) the BISECT_EXP mode
 * is recommended.
 * Note: If the Interval is bounded (neither endpoint is +-Inf) the bisection
 *       mode has no influence.
 */
static bg_error bg_interval_bisect(mpfi_t interval, BisectionMode mode,
                                   mpfi_t *left, mpfi_t *right) {
  mpfi_t tmp;
  mpfr_t low, high, left_low, left_high, right_low, right_high, pivot;
  int sign;
  mpfr_init(low);
  mpfr_init(high);
  mpfi_init(tmp);
  mpfi_get_left(low, interval);
  mpfi_get_right(high, interval);

  /* If the interval is bounded we perform normal bisection */
  if(mpfi_bounded_p(interval)) {
    mpfi_bisect(*left, *right, interval);
  } else {
    switch(mode) {
    case BISECT_NORMAL:
      if(mpfr_inf_p(low)) {
        mpfr_set_d(low, -DBL_MAX, MPFR_RNDD);
      }
      if(mpfr_inf_p(high)) {
        mpfr_set_d(high, DBL_MAX, MPFR_RNDU);
      }
      mpfi_interv_fr(tmp, low, high);
      mpfi_bisect(*left, *right, tmp);
      break;
    case BISECT_EXP:
      mpfr_init_set(left_low, low, MPFR_RNDD);
      mpfr_init_set(right_high, high, MPFR_RNDU);
      mpfr_init(left_high);
      mpfr_init(right_low);
      /* interval contains zero, and is not bounded by zero */
      if((mpfr_cmp_ui(low, 0) < 0) && (mpfr_cmp_ui(high, 0) > 0)) {
        mpfr_set_zero(left_high, +1);
        mpfr_set_zero(right_low, -1);
      } else {
        mpfr_init(pivot);
        /* Figure out which side is unbounded.
           The case that both sides are unbounded is handled above. */
        if(mpfr_inf_p(low)) {
          mpfr_set(pivot, high, MPFR_RNDN);
          sign = mpfr_signbit(low);
        } else {
          mpfr_set(pivot, low, MPFR_RNDN);
          sign = mpfr_signbit(high);
        }
        /* Prevent the pivot from being zero */
        if(mpfr_zero_p(pivot)) {
          mpfr_set_ui(pivot, 1024, MPFR_RNDN);
          mpfr_setsign(pivot, pivot, sign, MPFR_RNDN);
        } else {
          /* Increase the exponent of the pivot by 10 */
          mpfr_mul_2ui(pivot, pivot, 10, MPFR_RNDN);
        }
        mpfr_set(left_high, pivot, MPFR_RNDU);
        mpfr_set(right_low, pivot, MPFR_RNDD);
        mpfr_clear(pivot);
      }
      mpfi_interv_fr(*left, left_low, left_high);
      mpfi_interv_fr(*right, right_low, right_high);
      mpfr_clear(left_low);
      mpfr_clear(left_high);
      mpfr_clear(right_low);
      mpfr_clear(right_high);
      break;
    }
  }
  mpfr_clear(low);
  mpfr_clear(high);
  mpfi_clear(tmp);
  return bg_SUCCESS;
}

static bg_error bg_interval_evaluate_node(bg_node_t *node) {
  bg_error err;
  size_t i, j;
  mpfi_t value;
  mpfi_init(value);
  /* merge input ports */
  for(i = 0; i < node->input_port_cnt; ++i) {
    node->input_ports[i]->merge->merge_intv(node->input_ports[i]);
  }
  /* evaluate nodes */
  err = node->type->eval_intv(node);
  /* write to outputs */
  for(i = 0; i < node->output_port_cnt; ++i) {
    mpfi_set(value, node->output_ports[i]->value_intv);
    for(j = 0; j < node->output_ports[i]->num_edges; ++j) {
      mpfi_set(node->output_ports[i]->edges[j]->value_intv, value);
    }
  }
  mpfi_clear(value);
  return err;
}

static bg_error bg_interval_merge(bg_interval2_list_t *list) {
  size_t merge_dim, dim, num_dim;
  bg_interval_list_t *current_list, *cmp_list;
  bg_interval2_list_iterator_t interval2_it, interval2_it2;
  bg_interval_list_iterator_t interval_it, interval_it2;
  mpfi_t *current_intv, *cmp_intv, *merge_intv;
  mpfi_t merged_intv, tmp_intv;
  mpfr_t diameter1, diameter2, diameter3, diff1, diff2;
  bg_real merge_threshold = 1e-10;
  bool did_merge = false;
  mpfi_init(merged_intv);
  mpfi_init(tmp_intv);
  mpfr_init(diameter1);
  mpfr_init(diameter2);
  mpfr_init(diameter3);
  mpfr_init(diff1);
  mpfr_init(diff2);
  current_list = bg_interval2_list_first(list, &interval2_it);
  if(!current_list) {
    return bg_SUCCESS;
  }
  num_dim = bg_interval_list_size(current_list);
  if(num_dim == 0) {
    return bg_SUCCESS;
  }
  do {
    did_merge = false;
    for(merge_dim = 0; merge_dim < num_dim; ++merge_dim) {
      for(current_list = bg_interval2_list_first(list, &interval2_it);
          current_list; current_list = bg_interval2_list_next(&interval2_it)) {
        assert(num_dim == bg_interval_list_size(current_list));

        bg_interval2_list_find(list, current_list, &interval2_it2);
        cmp_list = bg_interval2_list_next(&interval2_it2);
        while(cmp_list) {
          bool can_merge = true;
          current_intv = bg_interval_list_first(current_list, &interval_it);
          cmp_intv = bg_interval_list_first(cmp_list, &interval_it2);
          for(dim = 0; dim < num_dim; ++dim) {
            if(dim == merge_dim) {
              /* tentatively, merge the intervals along the merge dimension */
              mpfi_intersect(tmp_intv, *current_intv, *cmp_intv);
              if(mpfi_is_empty(tmp_intv)) {
                can_merge = false;
                break;
              }
              mpfi_union(merged_intv, *current_intv, *cmp_intv);
              merge_intv = current_intv;
              current_intv = bg_interval_list_next(&interval_it);
              cmp_intv = bg_interval_list_next(&interval_it2);
              continue;
            }
            /* Make sure the other intervals have the same size.
             * A stricter alternative would be:
             * mpfi_is_inside(a, b) && mpfi_is_inside(b, a)
             */
            mpfi_union(tmp_intv, *current_intv, *cmp_intv);
            mpfi_diam_abs(diameter1, tmp_intv);
            mpfi_diam_abs(diameter2, *current_intv);
            mpfi_diam_abs(diameter3, *cmp_intv);
            mpfr_sub(diff1, diameter1, diameter2, MPFR_RNDU);
            mpfr_sub(diff2, diameter1, diameter3, MPFR_RNDU);
            if(!(mpfr_cmp_d(diff1, merge_threshold) < 0) ||
               !(mpfr_cmp_d(diff2, merge_threshold) < 0)) {
              can_merge = false;
              break;
            }
            current_intv = bg_interval_list_next(&interval_it);
            cmp_intv = bg_interval_list_next(&interval_it2);
          }
          if(can_merge) {
            mpfi_set(*merge_intv, merged_intv);
            free_interval_list(bg_interval2_list_get_element(&interval2_it2));
            cmp_list = bg_interval2_list_erase(&interval2_it2);
            did_merge = true;
          } else {
            cmp_list = bg_interval2_list_next(&interval2_it2);
          }
        }
      }
    }
  } while(did_merge);
  mpfi_clear(merged_intv);
  mpfi_clear(tmp_intv);
  mpfr_clear(diameter1);
  mpfr_clear(diameter2);
  mpfr_clear(diameter3);
  mpfr_clear(diff1);
  mpfr_clear(diff2);
  return bg_SUCCESS;
}

static void print_list_of_intervals(bg_interval_list_t *l) {
  bg_interval_list_iterator_t it;
  mpfi_t *intv;
  for(intv = bg_interval_list_first(l, &it);
      intv; intv = bg_interval_list_next(&it)) {
    bg_real left, right;
    bg_interval_get_endpoints(*intv, &left, &right);
    fprintf(stderr, "[%.6g, %.6g] ", left, right);
  }
  fprintf(stderr, "\n");
  (void)print_list_of_interval_lists;
}

static void print_list_of_interval_lists(bg_interval2_list_t *l) {
  bg_interval2_list_iterator_t it2;
  bg_interval_list_t *list;
  return;
  for(list = bg_interval2_list_first(l, &it2);
      list; list = bg_interval2_list_next(&it2)) {
    break;
    print_list_of_intervals(list);
  }
  fprintf(stderr, "=> %lu intervals\n", bg_interval2_list_size(l));
}


#else /* ifdef INTERVAL_SUPPORT */


bg_error bg_interval_set_edge_value(bg_graph_t *graph, bg_edge_id_t edge_id,
                                    const bg_real value_interval[2]) {
  return bg_ERR_NOT_IMPLEMENTED;
  (void)graph;
  (void)edge_id;
  (void)value_interval;
}

bg_error bg_interval_get_edge_value(const bg_graph_t *graph,
                                    bg_edge_id_t edge_id,
                                    bg_real value_interval[2]) {
  return bg_ERR_NOT_IMPLEMENTED;
  (void)graph;
  (void)edge_id;
  (void)value_interval;
}

bg_error bg_interval_evaluate_graph(bg_graph_t *graph) {
  return bg_ERR_NOT_IMPLEMENTED;
  (void)graph;
}

bg_error bg_interval_detect_nans_d(bg_graph_t *graph,
                                   bg_real input_intervals[][2],
                                   const size_t n, bool *nan_detected) {
  return bg_ERR_NOT_IMPLEMENTED;
  (void)graph;
  (void)input_intervals;
  (void)n;
  (void)nan_detected;
}

bg_error bg_interval_find_inf_nan(bg_graph_t *graph, bg_real resolution,
                                  bg_real input_intervals[][2], size_t n,
                                  bg_real ****result_intervals, size_t *m) {
  return bg_ERR_NOT_IMPLEMENTED;
  (void)graph;
  (void)resolution;
  (void)input_intervals;
  (void)n;
  (void)result_intervals;
  (void)m;
}

bg_error bg_interval_free_inf_nan(bg_real ***intervals, size_t n, size_t m) {
  return bg_ERR_NOT_IMPLEMENTED;
  (void)intervals;
  (void)n;
  (void)m;
}

bg_error bg_interval_get_node_output(const bg_graph_t *graph,
                                     bg_node_id_t node_id,
                                     size_t output_port_idx,
                                     bg_real interval[2]) {
  return bg_ERR_NOT_IMPLEMENTED;
  (void)graph;
  (void)node_id;
  (void)output_port_idx;
  (void)interval;
}

bg_error bg_interval_get_graph_output(const bg_graph_t *graph,
                                      size_t output_port_idx,
                                      bg_real interval[2]) {
  return bg_ERR_NOT_IMPLEMENTED;
  (void)graph;
  (void)output_port_idx;
  (void)interval;
}


#endif /* INTERVAL_SUPPORT */
