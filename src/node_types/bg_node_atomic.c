#include "../bg_impl.h"
#include "../bg_node.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <math.h>

static bg_error init_atomic(bg_node_t *node) {
  bg_node_create_input_ports(node, node->type->input_port_cnt);
  bg_node_create_output_ports(node, node->type->output_port_cnt);
  return bg_error_get();
}

static bg_error deinit_atomic(bg_node_t *node) {
  bg_node_remove_input_ports(node);
  bg_node_remove_output_ports(node);
  return bg_error_get();
}

static bg_error eval_pipe(bg_node_t *node) {
  assert(node->input_ports && node->input_port_cnt == 1);
  assert(node->output_ports && node->output_port_cnt == 1);
  node->output_ports[0]->value = node->input_ports[0]->value;
  /*fprintf(stderr, "eval_pipe %lu: %g\n", node->id, node->output_ports[0]->value);*/
  return bg_SUCCESS;
}

static bg_error eval_pipe_interval(bg_node_t *node) {
#ifdef INTERVAL_SUPPORT
  assert(node->input_ports && node->input_port_cnt == 1);
  assert(node->output_ports && node->output_port_cnt == 1);
  mpfi_set(node->output_ports[0]->value_intv, node->input_ports[0]->value_intv);
  return bg_SUCCESS;
#else
  return bg_ERR_NOT_IMPLEMENTED;
  (void)node;
#endif
}


static bg_error eval_divide(bg_node_t *node) {
  assert(node->input_ports && node->input_port_cnt == 1);
  assert(node->output_ports && node->output_port_cnt == 1);
  node->output_ports[0]->value = 1. / node->input_ports[0]->value;
  return bg_SUCCESS;
}

static bg_error eval_divide_interval(bg_node_t *node) {
#ifdef INTERVAL_SUPPORT
  assert(node->input_ports && node->input_port_cnt == 1);
  assert(node->output_ports && node->output_port_cnt == 1);
  mpfi_inv(node->output_ports[0]->value_intv, node->input_ports[0]->value_intv);
  return bg_SUCCESS;
#else
  return bg_ERR_NOT_IMPLEMENTED;
  (void)node;
#endif
}


static bg_error eval_sin(bg_node_t *node) {
  assert(node->input_ports && node->input_port_cnt == 1);
  assert(node->output_ports && node->output_port_cnt == 1);
  node->output_ports[0]->value = sin(node->input_ports[0]->value);
  return bg_SUCCESS;
}

static bg_error eval_sin_interval(bg_node_t *node) {
#ifdef INTERVAL_SUPPORT
  assert(node->input_ports && node->input_port_cnt == 1);
  assert(node->output_ports && node->output_port_cnt == 1);
  mpfi_sin(node->output_ports[0]->value_intv, node->input_ports[0]->value_intv);
  return bg_SUCCESS;
#else
  return bg_ERR_NOT_IMPLEMENTED;
  (void)node;
#endif
}

static bg_error eval_asin(bg_node_t *node) {
  assert(node->input_ports && node->input_port_cnt == 1);
  assert(node->output_ports && node->output_port_cnt == 1);
  node->output_ports[0]->value = asin(node->input_ports[0]->value);
  return bg_SUCCESS;
}

static bg_error eval_asin_interval(bg_node_t *node) {
#ifdef INTERVAL_SUPPORT
  assert(node->input_ports && node->input_port_cnt == 1);
  assert(node->output_ports && node->output_port_cnt == 1);
  mpfi_asin(node->output_ports[0]->value_intv, node->input_ports[0]->value_intv);
  return bg_SUCCESS;
#else
  return bg_ERR_NOT_IMPLEMENTED;
  (void)node;
#endif
}


static bg_error eval_cos(bg_node_t *node) {
  assert(node->input_ports && node->input_port_cnt == 1);
  assert(node->output_ports && node->output_port_cnt == 1);
  node->output_ports[0]->value = cos(node->input_ports[0]->value);
  return bg_SUCCESS;
}

static bg_error eval_cos_interval(bg_node_t *node) {
#ifdef INTERVAL_SUPPORT
  assert(node->input_ports && node->input_port_cnt == 1);
  assert(node->output_ports && node->output_port_cnt == 1);
  mpfi_cos(node->output_ports[0]->value_intv, node->input_ports[0]->value_intv);
  return bg_SUCCESS;
#else
  return bg_ERR_NOT_IMPLEMENTED;
  (void)node;
#endif
}


static bg_error eval_tan(bg_node_t *node) {
  assert(node->input_ports && node->input_port_cnt == 1);
  assert(node->output_ports && node->output_port_cnt == 1);
  node->output_ports[0]->value = tan(node->input_ports[0]->value);
  return bg_SUCCESS;
}

static bg_error eval_tan_interval(bg_node_t *node) {
#ifdef INTERVAL_SUPPORT
  assert(node->input_ports && node->input_port_cnt == 1);
  assert(node->output_ports && node->output_port_cnt == 1);
  mpfi_tan(node->output_ports[0]->value_intv, node->input_ports[0]->value_intv);
  return bg_SUCCESS;
#else
  return bg_ERR_NOT_IMPLEMENTED;
  (void)node;
#endif
}


static bg_error eval_acos(bg_node_t *node) {
  assert(node->input_ports && node->input_port_cnt == 1);
  assert(node->output_ports && node->output_port_cnt == 1);
  node->output_ports[0]->value = acos(node->input_ports[0]->value);
  return bg_SUCCESS;
}

static bg_error eval_acos_interval(bg_node_t *node) {
#ifdef INTERVAL_SUPPORT
  assert(node->input_ports && node->input_port_cnt == 1);
  assert(node->output_ports && node->output_port_cnt == 1);
  mpfi_acos(node->output_ports[0]->value_intv,
            node->input_ports[0]->value_intv);
  return bg_SUCCESS;
#else
  return bg_ERR_NOT_IMPLEMENTED;
  (void)node;
#endif
}


static bg_error eval_atan2(bg_node_t *node) {
  assert(node->input_ports && node->input_port_cnt == 2);
  assert(node->output_ports && node->output_port_cnt == 1);
  node->output_ports[0]->value = atan2(node->input_ports[0]->value,
                                       node->input_ports[1]->value);
  return bg_SUCCESS;
}

static bg_error eval_atan2_interval(bg_node_t *node) {
#ifdef INTERVAL_SUPPORT
  assert(node->input_ports && node->input_port_cnt == 2);
  assert(node->output_ports && node->output_port_cnt == 1);
  mpfi_atan2(node->output_ports[0]->value_intv,
             node->input_ports[0]->value_intv,
             node->input_ports[1]->value_intv);
  return bg_SUCCESS;
#else
  return bg_ERR_NOT_IMPLEMENTED;
  (void)node;
#endif
}


static bg_error eval_pow(bg_node_t *node) {
  assert(node->input_ports && node->input_port_cnt == 2);
  assert(node->output_ports && node->output_port_cnt == 1);
  node->output_ports[0]->value = pow(node->input_ports[0]->value,
                                     node->input_ports[1]->value);
  return bg_SUCCESS;
}

static bg_error eval_pow_interval(bg_node_t *node) {
#ifdef INTERVAL_SUPPORT
  mpfi_t tmp;
  assert(node->input_ports && node->input_port_cnt == 2);
  assert(node->output_ports && node->output_port_cnt == 1);
  /* b**c == (e**ln(b))**c == e**(ln(b)*c) */
  mpfi_init(tmp);
  mpfi_log(tmp, node->input_ports[0]->value_intv);
  mpfi_mul(tmp, tmp, node->input_ports[1]->value_intv);
  mpfi_exp(node->output_ports[0]->value_intv, tmp);
  mpfi_clear(tmp);
  return bg_SUCCESS;
#else
  return bg_ERR_NOT_IMPLEMENTED;
  (void)node;
#endif
}


static bg_error eval_mod(bg_node_t *node) {
  assert(node->input_ports && node->input_port_cnt == 2);
  assert(node->output_ports && node->output_port_cnt == 1);
  node->output_ports[0]->value = fmod(node->input_ports[0]->value,
                                      node->input_ports[1]->value);
  return bg_SUCCESS;
}

static bg_error eval_mod_interval(bg_node_t *node) {
#ifdef INTERVAL_SUPPORT
  mpfr_t tmp1, tmp2, left, right;
  assert(node->input_ports && node->input_port_cnt == 2);
  assert(node->output_ports && node->output_port_cnt == 1);
  if(mpfi_has_zero(node->input_ports[1]->value_intv)) {
    mpfi_set_str(node->output_ports[0]->value_intv, "nan", 10);
  } else {
    /* TODO: this is a rough estimate and can be further refined */
    /* for c = a % b set c = [-min(mag(a),mag(b)), min(mag(a),mag(b))] */
    mpfr_init(tmp1);
    mpfr_init(tmp2);
    mpfr_init(left);
    mpfr_init(right);
    mpfi_mag(tmp1, node->input_ports[0]->value_intv);
    mpfi_mag(tmp2, node->input_ports[1]->value_intv);
    mpfr_min(right, tmp1, tmp2, MPFR_RNDU);
    mpfr_neg(left, right, MPFR_RNDD);
    mpfi_interv_fr(node->output_ports[0]->value_intv, left, right);
    mpfr_clear(tmp1);
    mpfr_clear(tmp2);
    mpfr_clear(left);
    mpfr_clear(right);
  }
  return bg_SUCCESS;
#else
  return bg_ERR_NOT_IMPLEMENTED;
  (void)node;
#endif
}


static bg_error eval_abs(bg_node_t *node) {
  assert(node->input_ports && node->input_port_cnt == 1);
  assert(node->output_ports && node->output_port_cnt == 1);
  node->output_ports[0]->value = fabs(node->input_ports[0]->value);
  return bg_SUCCESS;
}

static bg_error eval_abs_interval(bg_node_t *node) {
#ifdef INTERVAL_SUPPORT
  assert(node->input_ports && node->input_port_cnt == 1);
  assert(node->output_ports && node->output_port_cnt == 1);
  mpfi_abs(node->output_ports[0]->value_intv, node->input_ports[0]->value_intv);
  return bg_SUCCESS;
#else
  return bg_ERR_NOT_IMPLEMENTED;
  (void)node;
#endif
}


static bg_error eval_sqrt(bg_node_t *node) {
  assert(node->input_ports && node->input_port_cnt == 1);
  assert(node->output_ports && node->output_port_cnt == 1);
  node->output_ports[0]->value = sqrt(node->input_ports[0]->value);
  return bg_SUCCESS;
}

static bg_error eval_sqrt_interval(bg_node_t *node) {
#ifdef INTERVAL_SUPPORT
  assert(node->input_ports && node->input_port_cnt == 1);
  assert(node->output_ports && node->output_port_cnt == 1);
  mpfi_sqrt(node->output_ports[0]->value_intv,
            node->input_ports[0]->value_intv);
  return bg_SUCCESS;
#else
  return bg_ERR_NOT_IMPLEMENTED;
  (void)node;
#endif
}



static bg_real doSigmoid(bg_real input) {
  /* taken as in the rtneat */
  bg_real slope = 4.924273;
  /*bg_real constant = 2.4621365;*/
  return (1./(1+(exp(-(slope*input)))));
  /*return (2./(1+(exp(-(2.*input)))))-1.;*/
}

static bg_error eval_fsigmoid(bg_node_t *node) {
  assert(node->input_ports && node->input_port_cnt == 1);
  assert(node->output_ports && node->output_port_cnt == 1);
  node->output_ports[0]->value = doSigmoid(node->input_ports[0]->value);
  return bg_SUCCESS;
}

static bg_error eval_fsigmoid_interval(bg_node_t *node) {
#ifdef INTERVAL_SUPPORT
  bg_real slope = 4.924273;
  mpfi_t tmp;
  assert(node->input_ports && node->input_port_cnt == 1);
  assert(node->output_ports && node->output_port_cnt == 1);
  mpfi_init(tmp);
  mpfi_mul_d(tmp, node->input_ports[0]->value_intv, -slope);
  mpfi_exp(tmp, tmp);
  mpfi_add_ui(tmp, tmp, 1);
  mpfi_inv(node->output_ports[0]->value_intv, tmp);
  mpfi_clear(tmp);
  return bg_SUCCESS;
#else
  return bg_ERR_NOT_IMPLEMENTED;
  (void)node;
#endif
}


static bg_error eval_greater_than_0(bg_node_t *node) {
  bg_real result = 0.0;
  assert(node->input_ports && node->input_port_cnt == 3);
  assert(node->output_ports && node->output_port_cnt == 1);
  if(node->input_ports[0]->value > 0.0) {
    result = node->input_ports[1]->value;
  } else {
    result = node->input_ports[2]->value;
  }
  node->output_ports[0]->value = result;
  return bg_SUCCESS;
}

static bg_error eval_greater_than_0_interval(bg_node_t *node) {
#ifdef INTERVAL_SUPPORT
  assert(node->input_ports && node->input_port_cnt == 3);
  assert(node->output_ports && node->output_port_cnt == 1);
  if(mpfi_is_strictly_pos(node->input_ports[0]->value_intv)) {
    mpfi_set(node->output_ports[0]->value_intv,
             node->input_ports[1]->value_intv);
  } else if(mpfi_is_nonpos(node->input_ports[0]->value_intv)) {
    mpfi_set(node->output_ports[0]->value_intv,
             node->input_ports[2]->value_intv);
  } else {
    mpfi_union(node->output_ports[0]->value_intv,
               node->input_ports[1]->value_intv,
               node->input_ports[2]->value_intv);
  }
  return bg_SUCCESS;
#else
  return bg_ERR_NOT_IMPLEMENTED;
  (void)node;
#endif
}


static bg_error eval_equal_to_0(bg_node_t *node) {
  bg_real result = 0.0;
  assert(node->input_ports && node->input_port_cnt == 3);
  assert(node->output_ports && node->output_port_cnt == 1);
  if(fabs(node->input_ports[0]->value) < bg_EPSILON) {
    result = node->input_ports[1]->value;
  } else {
    result = node->input_ports[2]->value;
  }
  node->output_ports[0]->value = result;
  return bg_SUCCESS;
}

static bg_error eval_equal_to_0_interval(bg_node_t *node) {
#ifdef INTERVAL_SUPPORT
  assert(node->input_ports && node->input_port_cnt == 3);
  assert(node->output_ports && node->output_port_cnt == 1);
  if(mpfi_is_zero(node->input_ports[0]->value_intv)) {
    mpfi_set(node->output_ports[0]->value_intv,
             node->input_ports[1]->value_intv);
  } else if(!mpfi_has_zero(node->input_ports[0]->value_intv)) {
    mpfi_set(node->output_ports[0]->value_intv,
             node->input_ports[2]->value_intv);
  } else {
    mpfi_union(node->output_ports[0]->value_intv,
               node->input_ports[1]->value_intv,
               node->input_ports[2]->value_intv);
  }
  return bg_SUCCESS;
#else
  return bg_ERR_NOT_IMPLEMENTED;
  (void)node;
#endif
}

static bg_error eval_tanh(bg_node_t *node) {
  assert(node->input_ports && node->input_port_cnt == 1);
  assert(node->output_ports && node->output_port_cnt == 1);
  node->output_ports[0]->value = tanh(node->input_ports[0]->value);
  return bg_SUCCESS;
}

static bg_error eval_tanh_interval(bg_node_t *node) {
#ifdef INTERVAL_SUPPORT
  assert(node->input_ports && node->input_port_cnt == 1);
  assert(node->output_ports && node->output_port_cnt == 1);
  mpfi_tanh(node->output_ports[0]->value_intv, node->input_ports[0]->value_intv);
  return bg_SUCCESS;
#else
  return bg_ERR_NOT_IMPLEMENTED;
  (void)node;
#endif
}



static node_type_t atomic_types[] = {
/*{type_id, name, input_cnt, output_cnt, init, deinit, eval, eval_intv}*/
  {bg_NODE_TYPE_PIPE, "PIPE", 1, 1, init_atomic, deinit_atomic, eval_pipe, eval_pipe_interval},
  {bg_NODE_TYPE_DIVIDE, "DIVIDE", 1, 1, init_atomic, deinit_atomic, eval_divide, eval_divide_interval},
  {bg_NODE_TYPE_SIN, "SIN", 1, 1, init_atomic, deinit_atomic, eval_sin, eval_sin_interval},
  {bg_NODE_TYPE_ASIN, "ASIN", 1, 1, init_atomic, deinit_atomic, eval_asin, eval_asin_interval},
  {bg_NODE_TYPE_COS, "COS", 1, 1, init_atomic, deinit_atomic, eval_cos, eval_cos_interval},
  {bg_NODE_TYPE_TAN, "TAN", 1, 1, init_atomic, deinit_atomic, eval_tan, eval_tan_interval},
  {bg_NODE_TYPE_ACOS, "ACOS", 1, 1, init_atomic, deinit_atomic, eval_acos, eval_acos_interval},
  {bg_NODE_TYPE_ATAN2, "ATAN2", 2, 1, init_atomic, deinit_atomic, eval_atan2, eval_atan2_interval},
  {bg_NODE_TYPE_POW, "POW", 2, 1, init_atomic, deinit_atomic, eval_pow, eval_pow_interval},
  {bg_NODE_TYPE_MOD, "MOD", 2, 1, init_atomic, deinit_atomic, eval_mod, eval_mod_interval},
  {bg_NODE_TYPE_ABS, "ABS", 1, 1, init_atomic, deinit_atomic, eval_abs, eval_abs_interval},
  {bg_NODE_TYPE_SQRT, "SQRT", 1, 1, init_atomic, deinit_atomic, eval_sqrt, eval_sqrt_interval},
  {bg_NODE_TYPE_FSIGMOID, "FSIGMOID", 1, 1, init_atomic, deinit_atomic, eval_fsigmoid, eval_fsigmoid_interval},
  {bg_NODE_TYPE_GREATER_THAN_0, ">0", 3, 1, init_atomic, deinit_atomic, eval_greater_than_0, eval_greater_than_0_interval},
  {bg_NODE_TYPE_EQUAL_TO_0, "==0", 3, 1, init_atomic, deinit_atomic, eval_equal_to_0, eval_equal_to_0_interval},
  {bg_NODE_TYPE_TANH, "TANH", 1, 1, init_atomic, deinit_atomic, eval_tanh, eval_tanh_interval},
  /* sentinel */
  {0, NULL, 0, 0, NULL, NULL, NULL, NULL}
};

void bg_register_atomic_types(void);
void bg_register_atomic_types(void) {
  bg_node_type_register(atomic_types);
}
