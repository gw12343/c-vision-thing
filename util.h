//
// Created by gabed on 2/19/2025.
//

#ifndef UTIL_H
#define UTIL_H

#define GET_PIXEL_COLOR(img, x, y, component) img[(x) * 3 + (y) * width * 3 + (component)]
#define GET_PIXEL(img, x, y) img[(x) + (y) * width]

#define SET_PIXEL(img, x, y, c) img[(x) + (y) * width] = (c);
#define CHANGE_PIXEL(img, x, y, dc) img[(x) + (y) * width] += c;

#define SET_PIXEL_COLOR_COMPONENTS(img, x, y, r, g, b) img[(x) * 3 + (y) * width * 3 + 0] = r; \
img[(x) * 3 + (y) * width * 3 + 1] = g; \
img[(x) * 3 + (y) * width * 3 + 2] = b;
#define SET_PIXEL_COLOR(img, x, y, c) img[(x) * 3 + (y) * width * 3 + 0] = (c).red; \
img[(x) * 3 + (y) * width * 3 + 1] = (c).green; \
img[(x) * 3 + (y) * width * 3 + 2] = (c).blue;

#define GET_TAG_PIXEL(tag, x, y) (tags[tag])[x + y * 10]

#define APRIL_TAG_AMOUNT 587


#define max(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })


#define min(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })

typedef struct Vec2 {
    int x;
    int y;
} vec2;

typedef struct Color {
    unsigned char red;
    unsigned char green;
    unsigned char blue;
} color;


void draw_line(unsigned char* image, vec2 pos0, vec2 pos1, unsigned char col);
void draw_line_color(unsigned char* image, vec2 pos0, vec2 pos1, color col);

void visualizeTagData(int tag);

void load_tags(void);

void free_tags(void);

void load_image(const char* path, unsigned char** d);

void save_double_array_as_png(const double *data, int width, int height, const char *filename);

#endif //UTIL_H
