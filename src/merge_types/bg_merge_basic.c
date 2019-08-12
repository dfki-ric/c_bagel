#include "bg_impl.h"

#include <float.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

static bg_error merge_sum(input_port_t *input_port) {
  size_t i;
  bg_real value = input_port->bias;
  /*fprintf(stderr, "input bias: %g\n", input_port->bias);*/
  if(input_port->num_edges == 0) {
    value += input_port->defaultValue;
  } else {
    for(i = 0; i < input_port->num_edges; ++i) {
      value += input_port->edges[i]->value * input_port->edges[i]->weight;
      /*fprintf(stderr, "%lu %lu in->value: %g, in->weight: %g\n", i,
              (size_t)input_port->edges[i],
              input_port->edges[i]->value, input_port->edges[i]->weight);*/
    }
  }
  input_port->value = value;
  return bg_SUCCESS;
}

static bg_error merge_sum_interval(input_port_t *input_port) {
#ifdef INTERVAL_SUPPORT
  size_t i;
  mpfi_set_d(input_port->value_intv, input_port->bias);
  if(input_port->num_edges == 0) {
    mpfi_add_d(input_port->value_intv,
               input_port->value_intv, input_port->defaultValue);
  } else {
    mpfi_t tmp;
    mpfi_init(tmp);
    for(i = 0; i < input_port->num_edges; ++i) {
      mpfi_mul_d(tmp, input_port->edges[i]->value_intv,
                 input_port->edges[i]->weight);
      mpfi_add(input_port->value_intv, input_port->value_intv, tmp);
    }
    mpfi_clear(tmp);
  }
  return bg_SUCCESS;
#else
  return bg_ERR_NOT_IMPLEMENTED;
  (void)input_port;
#endif
}


static bg_error merge_weighted_sum(input_port_t *input_port) {
  size_t i;
  bg_edge_t *edge;
  bg_real value = 0.0;
  bg_real sum_weights = 0.0;

  if(input_port->num_edges == 0) {
    value += input_port->defaultValue;
    sum_weights = 1.0;
  } else {
    for(i = 0; i < input_port->num_edges; ++i) {
      edge = input_port->edges[i];
      value += edge->value * edge->weight;
      sum_weights += fabs(edge->weight);
    }
  }
  if(sum_weights > bg_EPSILON) {
    input_port->value = (value / sum_weights) + input_port->bias;
  }
  else {
    input_port->value = input_port->bias;
  }
  return bg_SUCCESS;
}

static bg_error merge_weighted_sum_interval(input_port_t *input_port) {
#ifdef INTERVAL_SUPPORT
  size_t i;
  bg_edge_t *edge;
  bg_real sum_weights = 0.0;
  mpfi_t value, tmp;
  mpfi_init_set_d(value, 0.);
  mpfi_init_set_d(tmp, 0.);
  if(input_port->num_edges == 0) {
    mpfi_add_d(value, value, input_port->defaultValue);
    sum_weights = 1.0;
  } else {
    for(i = 0; i < input_port->num_edges; ++i) {
      edge = input_port->edges[i];
      mpfi_mul_d(tmp, edge->value_intv, edge->weight);
      mpfi_add(value, value, tmp);
      sum_weights += fabs(edge->weight);
    }
  }
  if(sum_weights > bg_EPSILON) {
    mpfi_div_d(value, value, sum_weights);
    mpfi_add_d(input_port->value_intv, value, input_port->bias);
  } else {
    mpfi_set_d(input_port->value_intv, input_port->bias);
  }
  mpfi_clear(tmp);
  mpfi_clear(value);
  return bg_SUCCESS;
#else
  return bg_ERR_NOT_IMPLEMENTED;
  (void)input_port;
#endif
}


static bg_error merge_product(input_port_t *input_port) {
  size_t i;
  bg_real value = input_port->bias;
  if(input_port->num_edges == 0) {
    value *= input_port->defaultValue;
  } else {
    for(i = 0; i < input_port->num_edges; ++i) {
      value *= input_port->edges[i]->value * input_port->edges[i]->weight;
    }
  }
  input_port->value = value;
  return bg_SUCCESS;
}

static bg_error merge_product_interval(input_port_t *input_port) {
#ifdef INTERVAL_SUPPORT
  size_t i;
  mpfi_t value, tmp;
  mpfi_init(tmp);
  mpfi_init_set_d(value, input_port->bias);
  if(input_port->num_edges == 0) {
    mpfi_mul_d(value, value, input_port->defaultValue);
  } else {
    for(i = 0; i < input_port->num_edges; ++i) {
      mpfi_mul_d(tmp, input_port->edges[i]->value_intv,
                 input_port->edges[i]->weight);
      mpfi_mul(value, value, tmp);
    }
  }
  mpfi_set(input_port->value_intv, value);
  mpfi_clear(tmp);
  mpfi_clear(value);
  return bg_SUCCESS;
#else
  return bg_ERR_NOT_IMPLEMENTED;
  (void)input_port;
#endif
}


static bg_error merge_min(input_port_t *input_port) {
  size_t i;
  bg_edge_t *edge;
  bg_real value = input_port->bias;
  if(input_port->num_edges == 0) {
    if(input_port->defaultValue < value) {
      value = input_port->defaultValue;
    }
  } else {
    for(i = 0; i < input_port->num_edges; ++i) {
      edge = input_port->edges[i];
      if(edge->value * edge->weight < value) {
        value = edge->value * edge->weight;
      }
    }
  }
  input_port->value = value;
  return bg_SUCCESS;
}

static bg_error merge_min_interval(input_port_t *input_port) {
#ifdef INTERVAL_SUPPORT
  size_t i;
  bg_edge_t *edge;
  mpfi_t tmp;
  mpfr_t left, right, low, high;
  mpfi_init(tmp);
  mpfr_init(left);
  mpfr_init(right);
  mpfr_init_set_d(low, input_port->bias, MPFR_RNDD);
  mpfr_init_set_d(high, input_port->bias, MPFR_RNDU);
  if(input_port->num_edges == 0) {
    if(mpfr_cmp_d(low, input_port->defaultValue) > 0) {
      mpfr_set_d(low, input_port->defaultValue, MPFR_RNDD);
      mpfr_set_d(high, input_port->defaultValue, MPFR_RNDU);
    }
  } else {
    for(i = 0; i < input_port->num_edges; ++i) {
      edge = input_port->edges[i];
      mpfi_mul_d(tmp, edge->value_intv, edge->weight);
      mpfi_get_left(left, tmp);
      mpfi_get_right(right, tmp);
      if(mpfr_cmp(low, left) > 0) {
        mpfr_set(low, left, MPFR_RNDD);
      }
      if(mpfr_cmp(high, right) > 0) {
        mpfr_set(high, right, MPFR_RNDU);
      }
    }
  }
  mpfi_interv_fr(input_port->value_intv, low, high);
  mpfi_clear(tmp);
  mpfr_clear(left);
  mpfr_clear(right);
  mpfr_clear(low);
  mpfr_clear(high);
  return bg_SUCCESS;
#else
  return bg_ERR_NOT_IMPLEMENTED;
  (void)input_port;
#endif
}


static bg_error merge_max(input_port_t *input_port) {
  size_t i;
  bg_edge_t *edge;
  bg_real value = input_port->bias;
  if(input_port->num_edges == 0) {
    if(input_port->defaultValue > value) {
      value = input_port->defaultValue;
    }
  } else {
    for(i = 0; i < input_port->num_edges; ++i) {
      edge = input_port->edges[i];
      if(edge->value * edge->weight > value) {
        value = edge->value * edge->weight;
      }
    }
  }
  input_port->value = value;
  return bg_SUCCESS;
}

static bg_error merge_max_interval(input_port_t *input_port) {
#ifdef INTERVAL_SUPPORT
  size_t i;
  bg_edge_t *edge;
  mpfi_t tmp;
  mpfr_t left, right, low, high;
  mpfi_init(tmp);
  mpfr_init(left);
  mpfr_init(right);
  mpfr_init_set_d(low, input_port->bias, MPFR_RNDD);
  mpfr_init_set_d(high, input_port->bias, MPFR_RNDU);
  if(input_port->num_edges == 0) {
    if(mpfr_cmp_d(low, input_port->defaultValue) < 0) {
      mpfr_set_d(low, input_port->defaultValue, MPFR_RNDD);
      mpfr_set_d(high, input_port->defaultValue, MPFR_RNDU);
    }
  } else {
    for(i = 0; i < input_port->num_edges; ++i) {
      edge = input_port->edges[i];
      mpfi_mul_d(tmp, edge->value_intv, edge->weight);
      mpfi_get_left(left, tmp);
      mpfi_get_right(right, tmp);
      if(mpfr_cmp(low, left) < 0) {
        mpfr_set(low, left, MPFR_RNDD);
      }
      if(mpfr_cmp(high, right) < 0) {
        mpfr_set(high, right, MPFR_RNDU);
      }
    }
  }
  mpfi_interv_fr(input_port->value_intv, low, high);
  mpfi_clear(tmp);
  mpfr_clear(left);
  mpfr_clear(right);
  mpfr_clear(low);
  mpfr_clear(high);
  return bg_SUCCESS;
#else
  return bg_ERR_NOT_IMPLEMENTED;
  (void)input_port;
#endif
}


/*
 * kth_smallest function:
 * Algorithm from N. Wirth's book, implementation by N. Devillard.
 * This code in public domain.
 * Taken from http://ndevilla.free.fr/median/median/index.html
 */
static bg_real kth_smallest(bg_real *a, int n, int k) {
  register int i,j,l,m;
  register bg_real x;
  register bg_real tmp;
  l = 0;
  m = n - 1;
  while(l<m) {
    x = a[k];
    i = l;
    j = m;
    do {
      while(a[i] < x) ++i;
      while(x < a[j]) --j;
      if(i <= j) {
        tmp = a[i];
        a[i] = a[j];
        a[j] = tmp;
        ++i;
        --j;
      }
    } while(i <= j);
    if(j < k) l = i;
    if(k < i) m = j;
  }
  return a[k];
}

/*
  We handle the bias differently, documentation needed!
 */
static bg_error merge_median(input_port_t *input_port) {
  size_t i, cnt;
  bg_real *values;
  bg_real value;
  cnt = (input_port->num_edges ? input_port->num_edges : 1);
  values = (bg_real*)malloc(sizeof(bg_real) * cnt);

  if(input_port->num_edges == 0) {
    values[0] = input_port->defaultValue;
  } else {
    for(i = 0; i < input_port->num_edges; ++i) {
      values[i] = input_port->edges[i]->value * input_port->edges[i]->weight;
    }
  }

  value = kth_smallest(values, cnt, cnt / 2);
  /* if even number of elements take mean of the middle two */
  if(!(cnt & 1)) {
    value += kth_smallest(values, cnt, (cnt / 2) - 1);
    value /= 2.;
  }
  free(values);
  input_port->value = value + input_port->bias;
  return bg_SUCCESS;
}

static bg_error merge_median_interval(input_port_t *input_port) {
#ifdef INTERVAL_SUPPORT
  size_t i;
  mpfr_t low, high, left, right;
  mpfi_t tmp;
  mpfi_init(tmp);
  mpfr_init(low);
  mpfr_init(high);
  mpfr_init(left);
  mpfr_init(right);
  if(input_port->num_edges == 0) {
    mpfr_set_d(low, input_port->defaultValue, MPFR_RNDD);
    mpfr_set_d(high, input_port->defaultValue, MPFR_RNDU);
  } else {
    for(i = 0; i < input_port->num_edges; ++i) {
      mpfi_mul_d(tmp, input_port->edges[i]->value_intv,
                 input_port->edges[i]->weight);
      mpfi_get_left(left, tmp);
      mpfi_get_right(right, tmp);
      if(mpfr_cmp(left, low) < 0) {
        mpfr_set(low, left, MPFR_RNDD);
      }
      if(mpfr_cmp(right, high) > 0) {
        mpfr_set(high, right, MPFR_RNDU);
      }
    }
  }
  mpfi_interv_fr(input_port->value_intv, low, high);
  mpfi_clear(tmp);
  mpfr_clear(low);
  mpfr_clear(high);
  mpfr_clear(left);
  mpfr_clear(right);
  return bg_SUCCESS;
#else
  return bg_ERR_NOT_IMPLEMENTED;
  (void)input_port;
#endif
}


/*
  We handle the bias differently, documentation needed!
 */
static bg_error merge_mean(input_port_t *input_port) {
  size_t i, cnt = 1;
  bg_real value = 0.0;
  if(input_port->num_edges == 0) {
    value += input_port->defaultValue;
  } else {
    for(i = 0; i < input_port->num_edges; ++i) {
      value += input_port->edges[i]->value * input_port->edges[i]->weight;
    }
    cnt = input_port->num_edges;
  }
  input_port->value = (value / cnt) + input_port->bias;
  return bg_SUCCESS;
}

static bg_error merge_mean_interval(input_port_t *input_port) {
#ifdef INTERVAL_SUPPORT
  size_t i, cnt = 1;
  mpfi_t value, tmp;
  mpfi_init(tmp);
  mpfi_init_set_d(value, 0.0);
  if(input_port->num_edges == 0) {
    mpfi_add_d(value, value, input_port->defaultValue);
  } else {
    for(i = 0; i < input_port->num_edges; ++i) {
      mpfi_mul_d(tmp, input_port->edges[i]->value_intv,
                 input_port->edges[i]->weight);
      mpfi_add(value, value, tmp);
    }
    cnt = input_port->num_edges;
  }
  mpfi_div_ui(value, value, cnt);
  mpfi_add_d(input_port->value_intv, value, input_port->bias);
  mpfi_clear(value);
  mpfi_clear(tmp);
  return bg_SUCCESS;
#else
  return bg_ERR_NOT_IMPLEMENTED;
  (void)input_port;
#endif
}


static bg_error merge_norm(input_port_t *input_port) {
  size_t i;
  bg_real tmp, value = input_port->bias * input_port->bias;
  if(input_port->num_edges == 0) {
    tmp = input_port->defaultValue;
    value += tmp * tmp;
  }
  for(i = 0; i < input_port->num_edges; ++i) {
    tmp = input_port->edges[i]->value * input_port->edges[i]->weight;
    value += tmp * tmp;
  }
  input_port->value = sqrt(value);
  return bg_SUCCESS;
}

static bg_error merge_norm_interval(input_port_t *input_port) {
#ifdef INTERVAL_SUPPORT
  size_t i;
  mpfi_t value, tmp;
  mpfi_init(tmp);
  mpfi_init_set_d(value, input_port->bias * input_port->bias);
  if(input_port->num_edges == 0) {
    mpfi_add_d(value, value,
               input_port->defaultValue * input_port->defaultValue);
  }
  for(i = 0; i < input_port->num_edges; ++i) {
    mpfi_mul_d(tmp, input_port->edges[i]->value_intv,
               input_port->edges[i]->weight);
    mpfi_sqr(tmp, tmp);
    mpfi_add(value, value, tmp);
  }
  mpfi_sqrt(input_port->value_intv, value);
  mpfi_clear(tmp);
  mpfi_clear(value);
  return bg_SUCCESS;
#else
  return bg_ERR_NOT_IMPLEMENTED;
  (void)input_port;
#endif
}


/* Need to discuss semantics

static bg_error merge_wta(input_port_t *input_port) {
  size_t i;
  bg_real winner_weight = 0.;
  bg_real winner_value = input_port->bias;
  if(input_port->num_edges == 0) {
    winner_value = input_port->defaultValue;
  }
  for(i = 0; i < input_port->num_edges; ++i) {
    if(input_port->edges[i]->weight > winner_weight) {
      winner_weight = input_port->edges[i]->weight;
      winner_value = input_port->edges[i]->value;
    }
  }
  input_port->value = winner_value;
  return bg_SUCCESS;
}
*/

static merge_type_t basic_merges[] = {
/*{ merge_id, name, merge_func } */
  { bg_MERGE_TYPE_SUM, "SUM", &merge_sum, &merge_sum_interval },
  { bg_MERGE_TYPE_WEIGHTED_SUM, "WEIGHTED_SUM", &merge_weighted_sum, &merge_weighted_sum_interval },
  { bg_MERGE_TYPE_PRODUCT, "PRODUCT", &merge_product, &merge_product_interval },
  { bg_MERGE_TYPE_MIN, "MIN", &merge_min, &merge_min_interval },
  { bg_MERGE_TYPE_MAX, "MAX", &merge_max, &merge_max_interval },
  { bg_MERGE_TYPE_MEDIAN, "MEDIAN", &merge_median, &merge_median_interval },
  { bg_MERGE_TYPE_MEAN, "MEAN", &merge_mean, &merge_mean_interval },
  { bg_MERGE_TYPE_NORM, "NORM", &merge_norm, &merge_norm_interval },
  /*  { bg_MERGE_TYPE_WTA, "WTA", &merge_wta }, */
  /* sentinel */
  { 0, NULL, NULL, NULL }
};


void bg_register_basic_merges() {
  bg_merge_type_register(basic_merges);
}
