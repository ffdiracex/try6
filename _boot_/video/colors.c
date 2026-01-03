/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/types.h>
#include <holy/gui.h>
#include <holy/gui_string_util.h>
#include <holy/misc.h>
#include <holy/dl.h>
#include <holy/i18n.h>
#include <holy/color.h>

holy_MOD_LICENSE ("GPLv2+");

struct named_color
{
  const char *name;
  holy_video_rgba_color_t color;
};

/*
   Named color list generated from the list of SVG color keywords from
   <http://www.w3.org/TR/css3-color/#svg-color>,
   processed through the following Perl command:
   perl -ne 'chomp;split;print "{ \"$_[0]\", RGB_COLOR($_[2]) },\n"'
 */

#define RGB_COLOR(r,g,b) {.red = r, .green = g, .blue = b, .alpha = 255}

static struct named_color named_colors[] =
{
    { "aliceblue", RGB_COLOR(240,248,255) },
    { "antiquewhite", RGB_COLOR(250,235,215) },
    { "aqua", RGB_COLOR(0,255,255) },
    { "aquamarine", RGB_COLOR(127,255,212) },
    { "azure", RGB_COLOR(240,255,255) },
    { "beige", RGB_COLOR(245,245,220) },
    { "bisque", RGB_COLOR(255,228,196) },
    { "black", RGB_COLOR(0,0,0) },
    { "blanchedalmond", RGB_COLOR(255,235,205) },
    { "blue", RGB_COLOR(0,0,255) },
    { "blueviolet", RGB_COLOR(138,43,226) },
    { "brown", RGB_COLOR(165,42,42) },
    { "burlywood", RGB_COLOR(222,184,135) },
    { "cadetblue", RGB_COLOR(95,158,160) },
    { "chartreuse", RGB_COLOR(127,255,0) },
    { "chocolate", RGB_COLOR(210,105,30) },
    { "coral", RGB_COLOR(255,127,80) },
    { "cornflowerblue", RGB_COLOR(100,149,237) },
    { "cornsilk", RGB_COLOR(255,248,220) },
    { "crimson", RGB_COLOR(220,20,60) },
    { "cyan", RGB_COLOR(0,255,255) },
    { "darkblue", RGB_COLOR(0,0,139) },
    { "darkcyan", RGB_COLOR(0,139,139) },
    { "darkgoldenrod", RGB_COLOR(184,134,11) },
    { "darkgray", RGB_COLOR(169,169,169) },
    { "darkgreen", RGB_COLOR(0,100,0) },
    { "darkgrey", RGB_COLOR(169,169,169) },
    { "darkkhaki", RGB_COLOR(189,183,107) },
    { "darkmagenta", RGB_COLOR(139,0,139) },
    { "darkolivegreen", RGB_COLOR(85,107,47) },
    { "darkorange", RGB_COLOR(255,140,0) },
    { "darkorchid", RGB_COLOR(153,50,204) },
    { "darkred", RGB_COLOR(139,0,0) },
    { "darksalmon", RGB_COLOR(233,150,122) },
    { "darkseagreen", RGB_COLOR(143,188,143) },
    { "darkslateblue", RGB_COLOR(72,61,139) },
    { "darkslategray", RGB_COLOR(47,79,79) },
    { "darkslategrey", RGB_COLOR(47,79,79) },
    { "darkturquoise", RGB_COLOR(0,206,209) },
    { "darkviolet", RGB_COLOR(148,0,211) },
    { "deeppink", RGB_COLOR(255,20,147) },
    { "deepskyblue", RGB_COLOR(0,191,255) },
    { "dimgray", RGB_COLOR(105,105,105) },
    { "dimgrey", RGB_COLOR(105,105,105) },
    { "dodgerblue", RGB_COLOR(30,144,255) },
    { "firebrick", RGB_COLOR(178,34,34) },
    { "floralwhite", RGB_COLOR(255,250,240) },
    { "forestgreen", RGB_COLOR(34,139,34) },
    { "fuchsia", RGB_COLOR(255,0,255) },
    { "gainsboro", RGB_COLOR(220,220,220) },
    { "ghostwhite", RGB_COLOR(248,248,255) },
    { "gold", RGB_COLOR(255,215,0) },
    { "goldenrod", RGB_COLOR(218,165,32) },
    { "gray", RGB_COLOR(128,128,128) },
    { "green", RGB_COLOR(0,128,0) },
    { "greenyellow", RGB_COLOR(173,255,47) },
    { "grey", RGB_COLOR(128,128,128) },
    { "honeydew", RGB_COLOR(240,255,240) },
    { "hotpink", RGB_COLOR(255,105,180) },
    { "indianred", RGB_COLOR(205,92,92) },
    { "indigo", RGB_COLOR(75,0,130) },
    { "ivory", RGB_COLOR(255,255,240) },
    { "khaki", RGB_COLOR(240,230,140) },
    { "lavender", RGB_COLOR(230,230,250) },
    { "lavenderblush", RGB_COLOR(255,240,245) },
    { "lawngreen", RGB_COLOR(124,252,0) },
    { "lemonchiffon", RGB_COLOR(255,250,205) },
    { "lightblue", RGB_COLOR(173,216,230) },
    { "lightcoral", RGB_COLOR(240,128,128) },
    { "lightcyan", RGB_COLOR(224,255,255) },
    { "lightgoldenrodyellow", RGB_COLOR(250,250,210) },
    { "lightgray", RGB_COLOR(211,211,211) },
    { "lightgreen", RGB_COLOR(144,238,144) },
    { "lightgrey", RGB_COLOR(211,211,211) },
    { "lightpink", RGB_COLOR(255,182,193) },
    { "lightsalmon", RGB_COLOR(255,160,122) },
    { "lightseagreen", RGB_COLOR(32,178,170) },
    { "lightskyblue", RGB_COLOR(135,206,250) },
    { "lightslategray", RGB_COLOR(119,136,153) },
    { "lightslategrey", RGB_COLOR(119,136,153) },
    { "lightsteelblue", RGB_COLOR(176,196,222) },
    { "lightyellow", RGB_COLOR(255,255,224) },
    { "lime", RGB_COLOR(0,255,0) },
    { "limegreen", RGB_COLOR(50,205,50) },
    { "linen", RGB_COLOR(250,240,230) },
    { "magenta", RGB_COLOR(255,0,255) },
    { "maroon", RGB_COLOR(128,0,0) },
    { "mediumaquamarine", RGB_COLOR(102,205,170) },
    { "mediumblue", RGB_COLOR(0,0,205) },
    { "mediumorchid", RGB_COLOR(186,85,211) },
    { "mediumpurple", RGB_COLOR(147,112,219) },
    { "mediumseagreen", RGB_COLOR(60,179,113) },
    { "mediumslateblue", RGB_COLOR(123,104,238) },
    { "mediumspringgreen", RGB_COLOR(0,250,154) },
    { "mediumturquoise", RGB_COLOR(72,209,204) },
    { "mediumvioletred", RGB_COLOR(199,21,133) },
    { "midnightblue", RGB_COLOR(25,25,112) },
    { "mintcream", RGB_COLOR(245,255,250) },
    { "mistyrose", RGB_COLOR(255,228,225) },
    { "moccasin", RGB_COLOR(255,228,181) },
    { "navajowhite", RGB_COLOR(255,222,173) },
    { "navy", RGB_COLOR(0,0,128) },
    { "oldlace", RGB_COLOR(253,245,230) },
    { "olive", RGB_COLOR(128,128,0) },
    { "olivedrab", RGB_COLOR(107,142,35) },
    { "orange", RGB_COLOR(255,165,0) },
    { "orangered", RGB_COLOR(255,69,0) },
    { "orchid", RGB_COLOR(218,112,214) },
    { "palegoldenrod", RGB_COLOR(238,232,170) },
    { "palegreen", RGB_COLOR(152,251,152) },
    { "paleturquoise", RGB_COLOR(175,238,238) },
    { "palevioletred", RGB_COLOR(219,112,147) },
    { "papayawhip", RGB_COLOR(255,239,213) },
    { "peachpuff", RGB_COLOR(255,218,185) },
    { "peru", RGB_COLOR(205,133,63) },
    { "pink", RGB_COLOR(255,192,203) },
    { "plum", RGB_COLOR(221,160,221) },
    { "powderblue", RGB_COLOR(176,224,230) },
    { "purple", RGB_COLOR(128,0,128) },
    { "red", RGB_COLOR(255,0,0) },
    { "rosybrown", RGB_COLOR(188,143,143) },
    { "royalblue", RGB_COLOR(65,105,225) },
    { "saddlebrown", RGB_COLOR(139,69,19) },
    { "salmon", RGB_COLOR(250,128,114) },
    { "sandybrown", RGB_COLOR(244,164,96) },
    { "seagreen", RGB_COLOR(46,139,87) },
    { "seashell", RGB_COLOR(255,245,238) },
    { "sienna", RGB_COLOR(160,82,45) },
    { "silver", RGB_COLOR(192,192,192) },
    { "skyblue", RGB_COLOR(135,206,235) },
    { "slateblue", RGB_COLOR(106,90,205) },
    { "slategray", RGB_COLOR(112,128,144) },
    { "slategrey", RGB_COLOR(112,128,144) },
    { "snow", RGB_COLOR(255,250,250) },
    { "springgreen", RGB_COLOR(0,255,127) },
    { "steelblue", RGB_COLOR(70,130,180) },
    { "tan", RGB_COLOR(210,180,140) },
    { "teal", RGB_COLOR(0,128,128) },
    { "thistle", RGB_COLOR(216,191,216) },
    { "tomato", RGB_COLOR(255,99,71) },
    { "turquoise", RGB_COLOR(64,224,208) },
    { "violet", RGB_COLOR(238,130,238) },
    { "wheat", RGB_COLOR(245,222,179) },
    { "white", RGB_COLOR(255,255,255) },
    { "whitesmoke", RGB_COLOR(245,245,245) },
    { "yellow", RGB_COLOR(255,255,0) },
    { "yellowgreen", RGB_COLOR(154,205,50) },
    { 0, { 0, 0, 0, 0 } }  /* Terminator.  */
};

/* Get the color named NAME.  If the color was found, returns 1 and
   stores the color into *COLOR.  If the color was not found, returns 0 and
   does not modify *COLOR.  */
int
holy_video_get_named_color (const char *name,
                            holy_video_rgba_color_t *color)
{
  int i;
  for (i = 0; named_colors[i].name; i++)
    {
      if (holy_strcmp (named_colors[i].name, name) == 0)
        {
          *color = named_colors[i].color;
          return 1;
        }
    }
  return 0;
}

static int
parse_hex_color_component (const char *s, unsigned start, unsigned end)
{
  unsigned len;
  char buf[3];

  len = end - start;
  /* Check the limits so we don't overrun the buffer.  */
  if (len < 1 || len > 2)
    return 0;

  if (len == 1)
    {
      buf[0] = s[start];   /* Get the first and only hex digit.  */
      buf[1] = buf[0];     /* Duplicate the hex digit.  */
    }
  else if (len == 2)
    {
      buf[0] = s[start];
      buf[1] = s[start + 1];
    }

  buf[2] = '\0';

  return holy_strtoul (buf, 0, 16);
}

/* Parse a color string of the form "r, g, b", "#RGB", "#RGBA",
   "#RRGGBB", or "#RRGGBBAA".  */
holy_err_t
holy_video_parse_color (const char *s, holy_video_rgba_color_t *color)
{
  holy_video_rgba_color_t c;
  const char *s0;

  /* Skip whitespace.  */
  while (*s && holy_isspace (*s))
    s++;

  s0 = s;

  if (*s == '#')
    {
      /* HTML-style.  Number if hex digits:
         [6] #RRGGBB     [3] #RGB
         [8] #RRGGBBAA   [4] #RGBA  */

      s++;  /* Skip the '#'.  */
      /* Count the hexits to determine the format.  */
      int hexits = 0;
      const char *end = s;
      while (holy_isxdigit (*end))
        {
          end++;
          hexits++;
        }

      /* Parse the color components based on the format.  */
      if (hexits == 3 || hexits == 4)
        {
          c.red = parse_hex_color_component (s, 0, 1);
          c.green = parse_hex_color_component (s, 1, 2);
          c.blue = parse_hex_color_component (s, 2, 3);
          if (hexits == 4)
            c.alpha = parse_hex_color_component (s, 3, 4);
          else
            c.alpha = 255;
        }
      else if (hexits == 6 || hexits == 8)
        {
          c.red = parse_hex_color_component (s, 0, 2);
          c.green = parse_hex_color_component (s, 2, 4);
          c.blue = parse_hex_color_component (s, 4, 6);
          if (hexits == 8)
            c.alpha = parse_hex_color_component (s, 6, 8);
          else
            c.alpha = 255;
        }
      else
        return holy_error (holy_ERR_BAD_ARGUMENT,
                           N_("invalid color specification `%s'"), s0);
    }
  else if (holy_isdigit (*s))
    {
      /* Comma separated decimal values.  */
      c.red = holy_strtoul (s, 0, 0);
      s = holy_strchr (s, ',');
      if (!s)
        return holy_error (holy_ERR_BAD_ARGUMENT,
                           N_("invalid color specification `%s'"), s0);
      s++;
      c.green = holy_strtoul (s, 0, 0);
      s = holy_strchr (s, ',');
      if (!s)
        return holy_error (holy_ERR_BAD_ARGUMENT,
                           N_("invalid color specification `%s'"), s0);
      s++;
      c.blue = holy_strtoul (s, 0, 0);
      s = holy_strchr (s, ',');
      if (!s)
        c.alpha = 255;
      else
        {
          s++;
          c.alpha = holy_strtoul (s, 0, 0);
        }
    }
  else
    {
      if (! holy_video_get_named_color (s, &c))
        return holy_error (holy_ERR_BAD_ARGUMENT,
                           N_("invalid color specification `%s'"), s0);
    }

  if (holy_errno == holy_ERR_NONE)
    *color = c;
  return holy_errno;
}
