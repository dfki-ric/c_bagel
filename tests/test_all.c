#include "bg_test.h"
#include <string.h>
#include <check.h>

extern Suite* bg_suite();
#ifdef YAML_SUPPORT
extern Suite* bg_yaml_suite();
#  ifdef INTERVAL_SUPPORT
extern Suite* bg_interval_suite();
#  endif
#endif

char base_dir[MAX_STRING_SIZE];

int main(int argc, const char **argv) {
  int number_failed = 0;
  SRunner *sr = srunner_create(bg_suite());
  strncpy(base_dir, argv[0], MAX_STRING_SIZE);
  strrchr(base_dir, '/')[0] = '\0';
#ifdef YAML_SUPPORT
  srunner_add_suite(sr, bg_yaml_suite());
#  ifdef INTERVAL_SUPPORT
  srunner_add_suite(sr, bg_interval_suite());
#  endif
#endif
  srunner_run_all(sr, CK_NORMAL);
  number_failed += srunner_ntests_failed(sr);
  srunner_free(sr);
  return number_failed;
  (void)argc;
}
