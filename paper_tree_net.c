#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <unistd.h>

#define PI 3.1415926
#define HEIGHT_CUT_PERCENTAGE 70.00
#define GLUE_PADDING 10.00

char *const tree_part_name[] = { "Top", "Middle", "Bottom", "Log" };
enum TreeParts { TOP, MIDDLE, BOTTOM, LOG };

typedef struct Prism {
  int tree_piece;
  bool is_triangular;

  double top_width;
  double bottom_width;
  double height;
  double depth;

  double enclosed_width;
  double enclosed_height;
  double enclosed_depth;

  struct PrismNet *net;
} prism_t;

typedef struct PrismNet {
  double total_width;
  double total_width_center;
  double total_height;
  double glue_padding_top_bottom;
  double glue_padding_left_right;
  double x_center_offset;
} prismNet_t;

void initPrism(prism_t *prism, int tree_piece, char *opts);
void initPrismNet(prism_t *prism);
void printPrism(prism_t *prism);
void saveAsSvg(prism_t *prism);

int main(int argv, char **argc) {
  int option;
  int prism_count = 0;
  bool include_log = false;
  bool output_svg = false;

  prism_t **prisms;

  while ((option = getopt(argv, argc, "t:m:b:ls")) != -1) {
    if (option == 's') {
      output_svg = true;
    }

    if (option == 'l') {
      include_log = true;
    }

    if (option == 't' || option == 'm' || option == 'b') {
      prism_t **tmp;
      if((tmp = realloc(prism_count == 0 ? NULL : prisms, sizeof(prism_t*) * (prism_count + 1))) == NULL) {
        fprintf(stderr, "Failed to allocate memory in %s on line %d.\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
      }

      prisms = tmp;
      tmp = NULL;

      if ((prisms[prism_count] = malloc(sizeof(prism_t))) == NULL) {
        fprintf(stderr, "Failed to allocate memory in %s on line %d.\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
      }

      int tree_piece_code;
      switch(option) {
        case 't': tree_piece_code = TOP; break;
        case 'm': tree_piece_code = MIDDLE; break;
        case 'b': tree_piece_code = BOTTOM; break;
      }

      initPrism(prisms[prism_count], tree_piece_code, optarg);

      if ((prisms[prism_count]->net = malloc(sizeof(prismNet_t))) == NULL) {
        fprintf(stderr, "Failed to allocate memory in %s on line %d.\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
      }
      
      prism_count++;
    }
  }

  double largest_depth = 0;
  for (int i = 0; i < prism_count; i++) {
    if (prisms[i]->depth > largest_depth) {
      largest_depth = prisms[i]->depth;
    }
  }

  for (int i = 0; i < prism_count; i++) {
    prisms[i]->depth = largest_depth;
    initPrismNet(prisms[i]);
  }

  for (int i = 0; i < prism_count; i++) {
    if (prisms[i]->tree_piece == BOTTOM) {
      prism_t **tmp;

      if((tmp = realloc(prisms, sizeof(prism_t*) * (prism_count + 1))) == NULL) {
        fprintf(stderr, "Failed to allocate memory in %s on line %d.\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
      }

      prisms = tmp;
      tmp = NULL;

      if ((prisms[prism_count] = malloc(sizeof(prism_t))) == NULL) {
        fprintf(stderr, "Failed to allocate memory in %s on line %d.\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
      }

      prisms[prism_count]->tree_piece = LOG;
      prisms[prism_count]->is_triangular = false;
      prisms[prism_count]->top_width = prisms[i]->top_width;
      prisms[prism_count]->bottom_width = prisms[i]->top_width;
      prisms[prism_count]->height = prisms[i]->top_width * 1.382;
      prisms[prism_count]->depth = largest_depth;
      prisms[prism_count]->net = malloc(sizeof(prismNet_t));
      initPrismNet(prisms[prism_count]);

      prism_count++;
      break;
    }
  }

  for (int i = 0; i < prism_count; i++) {
    if (output_svg) {
      saveAsSvg(prisms[i]);
    } else {
      printPrism(prisms[i]);
      printf("\n");
    }

    free(prisms[i]->net);
    free(prisms[i]);
  }

  free(prisms);
}

void initPrism(prism_t *prism, int tree_piece, char *opts) {
  char *options;
  char *buffer;

  if ((options = malloc(sizeof(*options) * (strlen(opts) + 1))) != NULL) {
    strcpy(options, opts);
  } else {
    fprintf(stderr, "Failed to allocate memory in %s on line %d.\n", __FILE__, __LINE__);
    exit(EXIT_FAILURE);
  }

  if ((buffer = strtok(options, ",")) != NULL)
    prism->enclosed_width = strtod(buffer, NULL);

  if ((buffer = strtok(NULL, ",")) != NULL)
    prism->enclosed_height = strtod(buffer, NULL);

  if ((buffer = strtok(NULL, ",")) != NULL)
    prism->enclosed_depth = strtod(buffer, NULL);

  prism->tree_piece = tree_piece;
  prism->is_triangular = tree_piece == TOP ? true : false;
  prism->depth = prism->enclosed_depth;
  prism->bottom_width = prism->enclosed_width + (tan(30.00 * (PI / 180)) * prism->enclosed_height) * 2;
  prism->height = ((prism->enclosed_width / 2) / tan(30.00 * (PI / 180))) + prism->enclosed_height;

  if (prism->tree_piece == TOP) {
    prism->top_width = 0;
  }

  if (prism->tree_piece == MIDDLE || prism->tree_piece == BOTTOM) {
    double cut_height = prism->height / 100 * HEIGHT_CUT_PERCENTAGE;
    prism->top_width = (tan(30.00 * (PI / 180)) * (prism->height - cut_height)) * 2;

    if (cut_height >= prism->enclosed_height) {
      prism->height = cut_height;
    } else {
      prism->height = prism->enclosed_height;
      prism->bottom_width = prism->top_width + (tan(30.00 * (PI / 180)) * prism->height) * 2;
    }
  }

  free(options);
}

void initPrismNet(prism_t *prism) {
  double *total_width = &prism->net->total_width;
  double *total_width_center = &prism->net->total_width_center;
  double *total_height = &prism->net->total_height;
  double *glue_padding_top_bottom = &prism->net->glue_padding_top_bottom;
  double *glue_padding_left_right = &prism->net->glue_padding_left_right;
  double *x_center_offset = &prism->net->x_center_offset;

  if (prism->is_triangular) {
    *glue_padding_top_bottom = 0.0;
    *glue_padding_left_right = GLUE_PADDING;
    *x_center_offset = *glue_padding_left_right;
    *total_width = prism->bottom_width * 3 + *glue_padding_left_right;
    *total_height = prism->height * 2 + prism->depth;
  } else {
    *glue_padding_top_bottom = GLUE_PADDING;
    *glue_padding_left_right = prism->top_width;
    *x_center_offset = 0.0;
    *total_width = prism->bottom_width + *glue_padding_left_right * 2 + (prism->height / cos(30.00 * (PI / 180))) * 2;
    *total_height = prism->height * 2 + prism->depth + *glue_padding_top_bottom * 2;
  }

  *total_width_center = (*total_width - *x_center_offset) / 2;
}

void printPrism(prism_t *prism) {
  printf("%s:\n", tree_part_name[prism->tree_piece]);
  printf("  Bounding Border\n");
  printf("    * Width: %.2fmm (Center: %.2fmm)\n", prism->net->total_width, prism->net->total_width_center);
  printf("    * Height: %.2fmm (Center: %.2fmm)\n", prism->net->total_height, prism->net->total_height / 2);
  printf("    * Padding %s: %.2fmm\n", prism->is_triangular ? "Right" : "Top/Bottom", GLUE_PADDING);
  printf("  Prism Measurements\n");
  printf("    * Top Width: %.2fmm (Center: %.2fmm)\n", prism->top_width, prism->top_width / 2);
  printf("    * Height: %.2fmm\n", prism->height);
  printf("    * Bottom Width: %.2fmm (Center: %.2fmm)\n", prism->bottom_width, prism->bottom_width / 2);
  printf("    * Fold: %.2fmm", prism->is_triangular ? prism->bottom_width : prism->height / cos(30.00 * (PI / 180)));
}

void saveAsSvg(prism_t *prism) {
  char *filename;
  char *file_suffix = ".svg";
  FILE *svg;

  filename = malloc((strlen(tree_part_name[prism->tree_piece]) + strlen(file_suffix)) * sizeof(char) + 1);
  if (filename == NULL) {
    fprintf(stderr, "Failed to allocate memory in %s on line %d.\n", __FILE__, __LINE__);
    exit(EXIT_FAILURE);
  }

  strcpy(filename, tree_part_name[prism->tree_piece]);
  strcat(filename, file_suffix);

  if ((svg = fopen(filename, "r")) != NULL) {
    fprintf(stderr, "SVG file `%s' already exists.\n", filename);
    fclose(svg);
    return;
  }

  if ((svg = fopen(filename, "a")) == NULL) {
    fprintf(stderr, "SVG file could not be created.\n");
    exit(EXIT_FAILURE);
  }

 fprintf(svg,
    "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"%.5fmm\" height=\"%.5fmm\" viewBox=\"0 0 %.5f %.5f\">\n"
    "\t<defs>\n"
    "\t<pattern id=\"stroked\" x=\"0\" y=\"0\" width=\"12\" height=\"12\" patternUnits=\"userSpaceOnUse\">"
    "\t\t<path d=\"M1,1 L5,5 M5,1 L1,5 M1,7 L5,11 M5,7 L1,11 M7,2 L9,0 L11,2 M7,10 L9,12 L11,10 M7,4 L11,8 M7,8 L11,4\" fill=\"none\" stroke=\"black\" stroke-width=\".25\" />"
    "\t</pattern>\n"
    "\t\t<pattern id=\"enclosed_object\" x=\"0\" y=\"0\" width=\"5\" height=\"5\" patternUnits=\"userSpaceOnUse\">\n"
    "\t\t\t<line x1=\"0\" y1=\"0\" x2=\"5\" y2=\"5\" stroke=\"red\" stroke-width=\".25\" />\n"
    "\t\t</pattern>\n"
    "\t</defs>\n"
    "\t<rect x=\"0\" y=\"0\" width=\"%.5f\" height=\"%.5f\" fill=\"url(#stroked)\" stroke=\"black\" stroke-width=\".25\" />\n"
    "\t<path\n\t\td=\"M%.5f,%.5fL%.5f,%.5fL%.5f,%.5fL%.5f,%.5fL%.5f,%.5fL%.5f,%.5fL%.5f,%.5f"
    "L%.5f,%.5fL%.5f,%.5fL%.5f,%.5fL%.5f,%.5fL%.5f,%.5fL%.5f,%.5f"
    "L%.5f,%.5fL%.5f,%.5fL%.5f,%.5fZ\"\n\t\tfill=\"white\"\n\t\tstroke=\"black\"\n\t\tstroke-width=\".25\" />\n"
    "\t<rect x=\"%.5f\" y=\"%.5f\" width=\"%.5f\" height=\"%.5f\" fill=\"none\" stroke=\"black\" stroke-width=\".25\" stroke-dasharray=\"2.5\" />\n"
    "\t<line x1=\"%.5f\" y1=\"%.5f\" x2=\"%.5f\" y2=\"%.5f\" stroke=\"black\" stroke-width=\".25\" stroke-dasharray=\"2.5\" />\n"
    "\t<line x1=\"%.5f\" y1=\"%.5f\" x2=\"%.5f\" y2=\"%.5f\" stroke=\"black\" stroke-width=\".25\" stroke-dasharray=\"2.5\" />\n"
    "\t<line x1=\"%.5f\" y1=\"%.5f\" x2=\"%.5f\" y2=\"%.5f\" stroke=\"black\" stroke-width=\".25\" stroke-dasharray=\"2.5\" />\n"
    "\t<line x1=\"%.5f\" y1=\"%.5f\" x2=\"%.5f\" y2=\"%.5f\" stroke=\"black\" stroke-width=\".25\" stroke-dasharray=\"2.5\" />\n"
    "\t<rect x=\"%.5f\" y=\"%.5f\" width=\"%.5f\" height=\"%.5f\" fill=\"url(#enclosed_object)\" stroke=\"red\" stroke-width=\".25\" stroke-dasharray=\"2.5\" />\n"
    "\t<rect x=\"%.5f\" y=\"%.5f\" width=\"%.5f\" height=\"%.5f\" fill=\"url(#enclosed_object)\" stroke=\"red\" stroke-width=\".25\" stroke-dasharray=\"2.5\" />\n"
    "</svg>",

    prism->net->total_width, prism->net->total_height,
    prism->net->total_width, prism->net->total_height,

    prism->net->total_width, prism->net->total_height,

    0.0, prism->net->glue_padding_top_bottom + prism->height,
    prism->net->total_width_center - prism->bottom_width / 2, prism->net->glue_padding_top_bottom + prism->height,
    prism->net->total_width_center - prism->top_width / 2, prism->net->glue_padding_top_bottom,
    prism->net->total_width_center - prism->top_width / 2, 0.0,
    prism->net->total_width_center + prism->top_width / 2, 0.0,
    prism->net->total_width_center + prism->top_width / 2, prism->net->glue_padding_top_bottom,
    prism->net->total_width_center + prism->bottom_width / 2, prism->net->glue_padding_top_bottom + prism->height,
    prism->net->total_width, prism->net->glue_padding_top_bottom + prism->height,
    prism->net->total_width, prism->net->total_height - prism->net->glue_padding_top_bottom - prism->height,
    prism->net->total_width_center + prism->bottom_width / 2, prism->net->total_height - prism->net->glue_padding_top_bottom - prism->height,
    prism->net->total_width_center + prism->top_width / 2, prism->net->total_height - prism->net->glue_padding_top_bottom,
    prism->net->total_width_center + prism->top_width / 2, prism->net->total_height,
    prism->net->total_width_center - prism->top_width / 2, prism->net->total_height,
    prism->net->total_width_center - prism->top_width / 2, prism->net->total_height - prism->net->glue_padding_top_bottom,
    prism->net->total_width_center - prism->bottom_width / 2, prism->net->total_height - prism->net->glue_padding_top_bottom - prism->height,
    0.0, prism->net->total_height - prism->net->glue_padding_top_bottom - prism->height,

    prism->net->total_width_center - prism->bottom_width / 2, prism->net->glue_padding_top_bottom + prism->height,
    prism->bottom_width, prism->net->total_height - prism->height * 2 - prism->net->glue_padding_top_bottom * 2,

    prism->net->total_width_center - prism->top_width / 2, prism->net->glue_padding_top_bottom,
    prism->net->total_width_center + prism->top_width / 2, prism->net->glue_padding_top_bottom,

    prism->net->total_width_center + prism->bottom_width / 2 + (prism->is_triangular ? prism->bottom_width : prism->height / cos(30.00 * (PI / 180))), prism->net->glue_padding_top_bottom + prism->height,
    prism->net->total_width_center + prism->bottom_width / 2 + (prism->is_triangular ? prism->bottom_width : prism->height / cos(30.00 * (PI / 180))), prism->net->total_height - prism->net->glue_padding_top_bottom - prism->height,

    prism->net->total_width_center - prism->bottom_width / 2 - (prism->is_triangular ? prism->bottom_width : prism->height / cos(30.00 * (PI / 180))), prism->net->glue_padding_top_bottom + prism->height,
    prism->net->total_width_center - prism->bottom_width / 2 - (prism->is_triangular ? prism->bottom_width : prism->height / cos(30.00 * (PI / 180))), prism->net->total_height - prism->net->glue_padding_top_bottom - prism->height,

    prism->net->total_width_center - prism->top_width / 2, prism->net->total_height - prism->net->glue_padding_top_bottom,
    prism->net->total_width_center + prism->top_width / 2, prism->net->total_height - prism->net->glue_padding_top_bottom,

    prism->net->total_width_center - prism->enclosed_width / 2, prism->net->glue_padding_top_bottom + prism->height - prism->enclosed_height,
    prism->enclosed_width, prism->enclosed_height,

    prism->net->total_width_center - prism->enclosed_width / 2, prism->net->total_height / 2 - prism->enclosed_depth / 2,
    prism->enclosed_width, prism->enclosed_depth
  );

  fclose(svg);
  free(filename);
}
