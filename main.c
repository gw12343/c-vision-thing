#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "util.h"
#define STB_IMAGE_IMPLEMENTATION
#include "vendor/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "vendor/stb_image_write.h"

#define RED 0
#define GREEN 1
#define BLUE 2


#define STRONG_EDGE 255
#define WEAK_EDGE 100
#define NO_EDGE 0

#define NMS_TOLERANCE 0.0 // not needed

#define MALLOC_CHECK(v, cap) v = malloc(cap); if(v == NULL) {printf("Failed to allocate mem."); exit(1);}

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

const ck gaussian_blur = (ck)
    {
            1, 2, 1,
            2, 4, 2,
            1, 2, 1,

            (1.0/16.0)
    };

#define MAX_COMPONENTS 2500

typedef struct Point_t {
    int x, y;
} Point;

typedef struct Component_t{
    Point points[525625];
    int size;
} Component;

bool* visited = NULL;
Component* components = NULL;
int components_count = 0;

#define CLAMP(x, low, high) ((x) < (low) ? (low) : ((x) > (high) ? (high) : (x)))


void convolve(const ck * kernel, const unsigned char* source, unsigned char* out) {
    for (int y = 1; y < height - 1; y++) {
        for (int x = 1; x < width - 1; x++) {
            const double val = GET_PIXEL(source, x, y);




            const double ul =(double)GET_PIXEL(source, x - 1, y - 1) * kernel->m00;
            const double u = (double)GET_PIXEL(source, x, y - 1) * kernel->m01;
            const double ur =(double)GET_PIXEL(source, x + 1, y - 1) * kernel->m02;

            const double l = (double)GET_PIXEL(source, x - 1, y) * kernel->m10;
            const double c =                                                             val * kernel->m11;
            const double r = (double) GET_PIXEL(source, x + 1, y) * kernel->m12;

            const double dl = (double)GET_PIXEL(source, x - 1, y + 1) * kernel->m20;
            const double d = (double)GET_PIXEL(source, x, y + 1) * kernel->m21;
            const double dr = (double) GET_PIXEL(source, x + 1, y + 1) * kernel->m22;

            const double total = ul + u + ur + l + c + r + dl + d + dr;
            const double avg = total  * kernel->normalization_factor;

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
            SET_PIXEL(out, x, y, (unsigned char) (luminance));
        }
    }
}


void double_threshold(const unsigned char* source, unsigned char* out) {
    int w = width;
    int h = height;
    int w2 = (w + 3) / 4;
    int h2 = (h + 3) / 4;
    unsigned char* maxes = malloc(w2 * h2 * sizeof(unsigned char));
    unsigned char* mins = malloc(w2 * h2 * sizeof(unsigned char));

    for(int x = 0; x < w2; x++) {
        for (int y = 0; y < h2; y++) {
            unsigned char min = 255;
            unsigned char max = 0;
            for (int x2 = 0; x2 < 4; x2++){
                for (int y2 = 0; y2 < 4; y2++) {
                    unsigned char v = source[(x * 4 + x2) +  (y * 4 + y2) * width];
                    min = min(min, v);
                    max = max(max, v);
                }
            }
            maxes[y * w2 + x] = max;
            mins[y * w2 + x] = min;
        }
    }


    unsigned char* maxes2 = malloc(w2 * h2);
    unsigned char* mins2 = malloc(w2 * h2);

    for(int x = 0; x < w2; x++) {
        for (int y = 0; y < h2; y++) {
            unsigned char min = 255;
            unsigned char max = 0;
            for(int x2 = -1; x2 <= 1; x2++) {
                for (int y2 = -1; y2 <= 1; y2++) {
                    int x3 = CLAMP(x + x2, 0, w2 - 1);
                    int y3 = CLAMP(y + y2, 0, h2 - 1);
                    min = min(min, mins[y3 * w2 + x3]);
                    max = max(max, maxes[y3 * w2 + x3]);
                }
            }
            maxes2[y * w2 + x] = max;
            mins2[y * w2 + x] = min;
        }
    }

    for(int x = 0; x < w; x++) {
        for (int y = 0; y < h; y++) {
            unsigned char max = maxes2[(y / 4) * w2 + (x / 4)];
            unsigned char min = mins2[(y / 4) * w2 + (x / 4)];

            if (max - min < (int) (0.30f * 255)) {
                out[y * width + x] = WEAK_EDGE;
            } else {
                out[y * width + x] = source[y * width + x] > (max + min) / 2 ? STRONG_EDGE : NO_EDGE;
            }
        }
    }

    free(maxes);
    free(mins);
    free(maxes2);
    free(mins2);
}

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

void non_maximum_suppression(const double* gradient, const double * direction, unsigned char* edges) {
    for (int y = 1; y < height - 1; y++) {
        for (int x = 1; x < width - 1; x++) {
            const double angle = fmod(GET_PIXEL(direction, x, y) + 180, 180);

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

            const double p = GET_PIXEL(gradient, x, y);

            // Remove non-maximums
            if ((p >= q || (p / q) >= (1 - NMS_TOLERANCE)) && (p >= r || (p / r) >= (1 - NMS_TOLERANCE))) {
                SET_PIXEL(edges, x, y, GET_PIXEL(gradient, x, y));
            } else {
                SET_PIXEL(edges, x, y, 0);
            }
        }
    }
}

void edge_tracking(unsigned char* edges) {
    for (int i = 1; i < height - 1; i++) {
        for (int j = 1; j < width - 1; j++) {
            if (GET_PIXEL(edges, j, i)  < 230 && GET_PIXEL(edges, j, i) > 20) {
                // Check if a strong edge is in the 8-neighborhood
                if (GET_PIXEL(edges, j - 1, i - 1) > 230 || GET_PIXEL(edges, j, i - 1) > 230 ||
                    GET_PIXEL(edges, j + 1, i - 1) > 230 || GET_PIXEL(edges, j - 1, i) > 230 ||
                    GET_PIXEL(edges, j + 1, i) > 230 || GET_PIXEL(edges, j - 1, i + 1) > 230 ||
                    GET_PIXEL(edges, j, i + 1) > 230 || GET_PIXEL(edges, j + 1, i + 1) > 230
                ) {
                        SET_PIXEL(edges, j, i, STRONG_EDGE);
                    } else {
                        SET_PIXEL(edges, j, i, NO_EDGE);
                    }
            }
        }
    }
}

void flood_fill(int x, int y, int current, unsigned char* img) { // NOLINT(*-no-recursion)
    if (x < 0 || y < 0 || x >= width || y >= height || visited[y * width + x] || GET_PIXEL(img, x, y) != STRONG_EDGE) return;

    visited[y * width + x] = 1;
    components[current].points[components[current].size++] = (Point){x, y};

    flood_fill(x + 1, y, current, img);
    flood_fill(x - 1, y, current, img);
    flood_fill(x, y + 1, current, img);
    flood_fill(x, y - 1, current, img);
}

void find_connected(unsigned char* img) {
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            if (GET_PIXEL(img, x, y) == STRONG_EDGE && !visited[y * width + x]) {
                components[components_count].size = 0;
                flood_fill(x, y, components_count, img);
                components_count++;
            }
        }
    }
}

#define MIN_COMPONENT_SIZE 250

void visualize_components(unsigned char* out){
    for (int i = 0; i < components_count; i++) {
        color col = (color) { rand() % 256, rand() % 256, rand() % 256};
        Component* comp = &components[i];

        if(comp ->size < MIN_COMPONENT_SIZE)
            continue;

        for(int p = 0; p < comp->size; p++){
            Point point = comp->points[p];
            int px = point.x;
            int py = point.y;
            if(px >= 0 && px < width && py >= 0 && py < height) {
                SET_PIXEL_COLOR(out, px, py, col)
            }
        }
    }
}

int main(void) {
    unsigned char *img;
    load_image("pic2.png", &img);
    MALLOC_CHECK(visited, sizeof(bool) * width * height)
    MALLOC_CHECK(components, sizeof(Component) * MAX_COMPONENTS)
    memset(visited, false, sizeof(bool) * width * height);
    components_count = 0;



    printf("Image size: %dx%d\n", width, height);
    unsigned char* outImage1 = NULL;
    unsigned char* outImage2 = NULL;
    unsigned char *outImage3 = NULL;
    unsigned char* outImage4 = NULL;
    unsigned char* outImage5 = NULL;
    unsigned char* outImage6 = NULL;

    MALLOC_CHECK(outImage1, sizeof(unsigned char) * width * height)
    MALLOC_CHECK(outImage2, sizeof(unsigned char) * width * height)
    MALLOC_CHECK(outImage3, sizeof(unsigned char) * width * height)
    MALLOC_CHECK(outImage4, sizeof(unsigned char) * width * height)
    MALLOC_CHECK(outImage5, sizeof(unsigned char) * width * height)
    MALLOC_CHECK(outImage6, sizeof(unsigned char) * width * height * 3)

    double* gradient = NULL;
    double* dirs = NULL;

    MALLOC_CHECK(gradient, sizeof(double) * width * height)
    MALLOC_CHECK(dirs, sizeof(double) * width * height)

    clock_t start = clock() / (CLOCKS_PER_SEC / 1000);

    grayscale(img, outImage1);
    convolve(&gaussian_blur, outImage1, outImage2);
    double_threshold(outImage2, outImage5);
/*    compute_gradient(outImage5, gradient, dirs);
    non_maximum_suppression(gradient, dirs, outImage5);
    edge_tracking(outImage5);*/


    find_connected(outImage5);
    visualize_components(outImage6);



    clock_t end = clock() / (CLOCKS_PER_SEC / 1000);
    printf("Time taken: %ldms\n", end - start);

    stbi_write_png("debug/grayscale.png", width, height, 1, outImage1, width);
    stbi_write_png("debug/blur.png", width, height, 1, outImage2, width);
    stbi_write_png("debug/threshold.png", width, height, 1, outImage5, width);
    stbi_write_png("debug/viz.png", width, height, 3, outImage6, width * 3);





    //stbi_write_png("grayscale.png", width, height, 1, outImage1, width);
    //save_double_array_as_png(gradient, width, height, "gradient.png");
    //save_double_array_as_png(dirs, width, height, "dirs.png");




    // Free memory
    stbi_image_free(img);
    free_tags();

    free(outImage1);
    free(outImage2);
    free(outImage3);
    free(outImage4);
    free(outImage5);
    free(outImage6);
    free(gradient);
    free(dirs);
    free(components);
    free(visited);
    return 0;
}