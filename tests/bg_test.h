#ifndef C_BAGEL_TEST_H
#define C_BAGEL_TEST_H


#include "../src/bagel.h"
#include <check.h>
#include <math.h>
#include <float.h>

#define EPSILON 1e-8
#define MAX_STRING_SIZE 1024

#ifndef isnan
#  define isnan(x) ((x) != (x))
#endif

#define ck_assert_flt_almost_eq(X, Y) ck_assert_msg(fabs((X) - (Y)) < EPSILON, "Assertion '"#X"=="#Y"' failed: "#X"==%g, "#Y"==%g", (X), (Y))
#define ck_assert_flt_nan(X) ck_assert_msg(isnan((X)), "Assertion '"#X"==NaN' failed: "#X"==%g", (X))
#define ck_assert_flt_inf(X) ck_assert_msg(((X)==1.f/0.f), "Assertion '"#X"==Inf' failed: "#X"==%g", (X))



#endif /* C_BAGEL_TEST_H */
