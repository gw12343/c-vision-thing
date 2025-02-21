//
// Created by gabed on 2/19/2025.
//
#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "vendor/stb_image.h"
#include "vendor/stb_image_write.h"

extern int width;
extern int height;
extern int channels;
extern unsigned char* tags [APRIL_TAG_AMOUNT];

// Modified from https://www.geeksforgeeks.org/dda-line-generation-algorithm-computer-graphics/
void draw_line(unsigned char* image, vec2 pos0, vec2 pos1, unsigned char col) {
    // calculate dx & dy
    int dx = pos1.x - pos0.x;
    int dy = pos1.y - pos0.y;

    int steps = abs(dx) > abs(dy) ? abs(dx) : abs(dy);

    double Xinc = dx / (double)steps;
    double Yinc = dy / (double)steps;

    double X = pos0.x;
    double Y = pos0.y;
    for (int i = 0; i <= steps; i++) {
        SET_PIXEL(image, (int)round(X), (int)round(Y), col);
        X += Xinc;
        Y += Yinc;
    }
}


// Modified from https://www.geeksforgeeks.org/dda-line-generation-algorithm-computer-graphics/
void draw_line_color(unsigned char* image, vec2 pos0, vec2 pos1, color col) {
    // calculate dx & dy
    int dx = pos1.x - pos0.x;
    int dy = pos1.y - pos0.y;

    int steps = abs(dx) > abs(dy) ? abs(dx) : abs(dy);

    double Xinc = dx / (double)steps;
    double Yinc = dy / (double)steps;

    double X = pos0.x;
    double Y = pos0.y;
    for (int i = 0; i <= steps; i++) {
        SET_PIXEL_COLOR(image, (int)round(X), (int)round(Y), col);
        X += Xinc;
        Y += Yinc;
    }
}

void visualizeTagData(const int tag) {
    for (int y = 0; y < 10; y++) {
        for (int x = 0; x < 10; x++) {
            printf("%s  ", GET_TAG_PIXEL(tag, x, y) == 0 ? "." : "#");
        }
        printf("\n");
    }
}



void load_tags(void) {
    for (int i = 0; i < APRIL_TAG_AMOUNT; i++) {
        char buff[50];
        sprintf(buff, "tags/tag36_11_%05d.png", i);
        int _;
        unsigned char *tag = stbi_load(buff, &_, &_, &_, 1);
        if (tag == NULL) {
            printf("Failed to load tag\n");
        }
        tags [i] = tag;
    }
}

void free_tags(void) {
    for (int i = 0; i < APRIL_TAG_AMOUNT; i++) {
        stbi_image_free(tags [i]);
    }
}

void load_image(const char* path, unsigned char** d) {
    *d = stbi_load(path, &width, &height, &channels, 0);
    if(*d == NULL) {
        printf("Error in loading the image\n");
        exit(1);
    }
}




void save_double_array_as_png(const double *data, int width, int height, const char *filename) {
    unsigned char *image_data = malloc(width * height * sizeof (unsigned char));
    if (!image_data) {
        printf("Failed to allocate memory.\n");
        return;
    }

    for (int i = 0; i < width * height; i++) {
        image_data[i] = (unsigned char)(data[i] * 255.0);
    }

    stbi_write_png(filename, width, height, 1, image_data, width);
    free(image_data);
}