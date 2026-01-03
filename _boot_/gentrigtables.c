#define _GNU_SOURCE 1

#include <holy/trig.h>
#include <math.h>
#include <stdio.h>

int
main (int argc __attribute__ ((unused)),
      char **argv __attribute__ ((unused)))
{
  int i;

  printf ("#include <holy/types.h>\n");
  printf ("#include <holy/dl.h>\n");
  printf ("\n");

  printf ("/* Under copyright legislature such automated output isn't\n");
  printf ("covered by any copyright. Hence it's public domain. Public\n");
  printf ("domain works can be dual-licenced with any license. */\n");
  printf ("holy_MOD_LICENSE (\"GPLv2+\");");
  printf ("holy_MOD_DUAL_LICENSE (\"Public Domain\");");

#define TAB(op) \
  printf ("const holy_int16_t holy_trig_" #op "tab[] =\n{"); \
  for (i = 0; i < holy_TRIG_ANGLE_MAX; i++) \
    { \
      double x = i * 2 * M_PI / holy_TRIG_ANGLE_MAX; \
      if (i % 10 == 0) \
	printf ("\n    "); \
      printf ("%d,", (int) (round (op (x) * holy_TRIG_FRACTION_SCALE))); \
    } \
  printf ("\n};\n")

  TAB(sin);
  TAB(cos);

  return 0;
}
