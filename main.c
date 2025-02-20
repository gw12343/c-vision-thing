#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "util.h"
#define STB_IMAGE_IMPLEMENTATION
#include "vendor/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "vendor/stb_image_write.h"

#define RED 0
#define GREEN 1
#define BLUE 2

#define HIGH_THRESHOLD 90
#define LOW_THRESHOLD 50

#define STRONG_EDGE 255
#define WEAK_EDGE 100
#define NO_EDGE 0

#define NMS_TOLERANCE 0.00 // not needed

unsigned char* tags [APRIL_TAG_AMOUNT];

int width;
int height;
int channels;



typedef struct ConvolutionKernel {
    double m00, m01, m02;
    double m10, m11, m12;
    double m20, m21, m22;

    double normalization_factor;
} ck;




#define DEFAULT_VAL 0

void convolve(const ck * kernel, const unsigned char* source, unsigned char* out) {
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            double val = GET_PIXEL(source, x, y);
            double ul, u, ur;
            double l,      r;
            double dl, d, dr;

            bool b_up = y > 0;
            bool b_down = y < height - 1;
            bool b_left = x > 0;
            bool b_right = x < width - 1;



            ul = ((b_up && b_left)    ? (double)GET_PIXEL(source, x - 1, y - 1) :   DEFAULT_VAL) * kernel->m00;
            u = ((b_up)               ? (double)GET_PIXEL(source, x, y - 1) :       DEFAULT_VAL) * kernel->m01;
            ur = ((b_up && b_right)   ? (double)GET_PIXEL(source, x + 1, y - 1) :   DEFAULT_VAL) * kernel->m02;

            l = ((b_left)             ? (double)GET_PIXEL(source, x - 1, y) :       DEFAULT_VAL) * kernel->m10;
            double c =                                                             val * kernel->m11;
            r = ((b_right)            ? (double)GET_PIXEL(source, x + 1, y) :       DEFAULT_VAL) * kernel->m12;


            dl = ((b_down && b_left)  ? (double)GET_PIXEL(source, x - 1, y + 1) :   DEFAULT_VAL) * kernel->m20;
            d = ((b_down)             ? (double)GET_PIXEL(source, x, y + 1) :       DEFAULT_VAL) * kernel->m21;
            dr = ((b_down && b_right) ? (double)GET_PIXEL(source, x + 1, y + 1) :   DEFAULT_VAL) * kernel->m22;

            double total = ul + u + ur + l + c +  r + dl + d + dr;
            double avg = total  * 0.0625;

            SET_PIXEL(out, x, y, avg);
        }
    }
}

void grayscale(const unsigned char* source, unsigned char* out) {
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            const uint8_t r = GET_PIXEL_COLOR(source, x, y, RED);
            const uint8_t g = GET_PIXEL_COLOR(source, x, y, GREEN);
            const uint8_t b = GET_PIXEL_COLOR(source, x, y, BLUE);
            const double luminance = 0.2126 * r + 0.7152 * g + 0.0722 * b;

            SET_PIXEL(out, x, y, (uint8_t)luminance);
        }
    }
}

void double_threshold(const unsigned char* source, unsigned char* out) {
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            const uint8_t v = GET_PIXEL(source, x, y);

            if (v > HIGH_THRESHOLD) {
                SET_PIXEL(out, x, y, STRONG_EDGE);
            } else if (v > LOW_THRESHOLD) {
                SET_PIXEL(out, x, y, WEAK_EDGE);
            } else {
                SET_PIXEL(out, x, y, NO_EDGE);
            }
        }
    }
}


int Gx[3][3] = {
    {-1,  0,  1},
    {-2,  0,  2},
    {-1,  0,  1}
};

int Gy[3][3] = {
    {-1, -2, -1},
    { 0,  0,  0},
    { 1,  2,  1}
};


void compute_gradient(const unsigned char* img, double* gradient, double* direction) {
    if (gradient == NULL || direction == NULL || img == NULL) {
        return;
    }
    for (int i = 1; i < height - 1; i++) {
        for (int j = 1; j < width - 1; j++) {
            double gx = 0, gy = 0;
            for (int x = -1; x <= 1; x++) {
                for (int y = -1; y <= 1; y++) {
                    gx += Gx[x + 1][y + 1] * img[(i + x) * width + j + y];
                    gy += Gy[x + 1][y + 1] * img[(i + x) * width + j + y];
                }
            }
            gradient[i * width + j] = sqrt(gx * gx + gy * gy);
            direction[i * width + j] = atan2(gy, gx) * (180.0 / M_PI);
        }
    }
}

void non_maximum_suppression(double* gradient, double* direction, unsigned char* edges) {
    for (int y = 1; y < height - 1; y++) {
        for (int x = 1; x < width - 1; x++) {
            // Constrain arctan angle
            double angle = fmod(GET_PIXEL(direction, x, y) + 180, 180);

            int q = 0;
            int r = 0;



            if ((0 <= angle && angle < 22.5) || (157.5 <= angle && angle <= 180)) {
                // left and right
                q = GET_PIXEL(gradient, x + 1, y);
                r = GET_PIXEL(gradient, x - 1, y);
            } else if (22.5 <= angle && angle < 67.5) {
                // up left and down right
                q = GET_PIXEL(gradient, x - 1, y + 1);
                r = GET_PIXEL(gradient, x + 1, y - 1);
            } else if (67.5 <= angle && angle < 112.5) {
                //
                q = GET_PIXEL(gradient, x, y + 1);
                r = GET_PIXEL(gradient, x, y - 1);
            } else if (112.5 <= angle && angle < 157.5) {
                q = GET_PIXEL(gradient, x - 1, y - 1);
                r = GET_PIXEL(gradient, x + 1, y + 1);
            }

            double p = GET_PIXEL(gradient, x, y);

            // Remove non-maximums
            if ((p >= q || (p / q) >= (1 - NMS_TOLERANCE)) && (p >= r || (p / r) >= (1 - NMS_TOLERANCE))) {
                SET_PIXEL(edges, x, y, (int)GET_PIXEL(gradient, x, y));
            } else {
                SET_PIXEL(edges, x, y, 0);
            }
        }
    }
}



int main(void) {

    ck gaussian_blur = (ck)
    {
        1, 2, 1,
        2, 4, 2,
        1, 2, 1,

        (1.0/16.0)
    };




    unsigned char *img;
    load_image("pic2.png", &img);
    load_tags();

    unsigned char* outImage = malloc(sizeof(unsigned char) * width * height);
    unsigned char* outImage2 = malloc(sizeof(unsigned char) * width * height);
    unsigned char* outImage3 = malloc(sizeof(unsigned char) * width * height);
    double* gradient = malloc(sizeof(double) * width * height);
    double* dirs = malloc(sizeof(double) * width * height);
    unsigned char* nms = malloc(sizeof(unsigned char) * width * height);

    printf("Image size: %dx%d\n", width, height);
    printf("Pixel color: %d\n", GET_PIXEL_COLOR(img, 1, 1, RED));
    printf("Pixel color: %d\n", GET_PIXEL_COLOR(img, 1, 1, GREEN));
    printf("Pixel color: %d\n", GET_PIXEL_COLOR(img, 1, 1, BLUE));


    //visualizeTagData(55);

    grayscale(img, outImage);
    convolve(&gaussian_blur, outImage, outImage2);
    double_threshold(outImage2, outImage3);
    compute_gradient(outImage3, gradient, dirs);
    non_maximum_suppression(gradient, dirs, nms);

    stbi_write_png("blur.png", width, height, 1, outImage2, width);
    stbi_write_png("threshold.png", width, height, 1, outImage3, width);
    save_double_array_as_png(gradient, width, height, "gradient.png");
    stbi_write_png("nms.png", width, height, 1, nms, width);


    //draw_line(img, (vec2){0, 0}, (vec2){100, 275},  (color){200, 255, 0});

    // Free memory
    stbi_image_free(img);
    free_tags();

    free(outImage);
    free(outImage2);
    free(outImage3);
    free(gradient);
    free(dirs);
    free(nms);
    return 0;
}