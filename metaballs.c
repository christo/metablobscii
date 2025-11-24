/*
 * Spinning ASCII Metablobs
 * Inspired by donut.c obfuscated c contest entry by a1kon 
 *
 * This program renders three metablobs (spheroids) that orbit and merge
 * using ASCII characters with z-buffering and lighting.
 */

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdlib.h>

// metablob parameters
#define NUM_METABLOBS 3
#define METABLOB_RADIUS 1.2f
#define THRESHOLD 1.0f  // surface threshold for metablob field
#define K2 8.0f  // distance from viewer to screen

#define BASE_SCALE_X 90.0f 
#define BASE_SCALE_Y 45.0f
#define BASE_WIDTH 80.0f
#define BASE_HEIGHT 22.0f

// rotation speeds (radians per frame)
#define ROTATION_SPEED_A 0.0216f 
#define ROTATION_SPEED_B 0.03312f 

// ray marching parameters
#define MAX_STEPS 64
#define MIN_DISTANCE 0.01f
#define MAX_DISTANCE 20.0f
#define EPSILON 0.001f

// luminance characters (from darkest to brightest)
const char LUMINANCE_CHARS[] = ".:;!=Xs*$M@#";

typedef struct {
    float x, y, z;
} Metablob;

// calculate metablob field strength at a point
float metablob_field(float px, float py, float pz, Metablob *blobs, int num_blobs) {
    float sum = 0.0f;
    for (int i = 0; i < num_blobs; i++) {
        float dx = px - blobs[i].x;
        float dy = py - blobs[i].y;
        float dz = pz - blobs[i].z;
        float dist_sq = dx*dx + dy*dy + dz*dz;
        if (dist_sq > 0.0001f) {
            sum += (METABLOB_RADIUS * METABLOB_RADIUS) / dist_sq;
        }
    }
    return sum;
}

// calculate gradient (surface normal) using central differences
void calculate_normal(float px, float py, float pz, Metablob *blobs, int num_blobs,
                      float *nx, float *ny, float *nz) {
    float fx = metablob_field(px, py, pz, blobs, num_blobs);
    float fx_plus = metablob_field(px + EPSILON, py, pz, blobs, num_blobs);
    float fy_plus = metablob_field(px, py + EPSILON, pz, blobs, num_blobs);
    float fz_plus = metablob_field(px, py, pz + EPSILON, blobs, num_blobs);

    *nx = fx_plus - fx;
    *ny = fy_plus - fx;
    *nz = fz_plus - fx;

    // Normalise
    float len = sqrtf((*nx)*(*nx) + (*ny)*(*ny) + (*nz)*(*nz));
    if (len > 0.0001f) {
        *nx /= len;
        *ny /= len;
        *nz /= len;
    }
}

// ray march to find surface intersection
float ray_march(float ox, float oy, float oz,  // Ray origin
                float dx, float dy, float dz,  // Ray direction
                Metablob *blobs, int num_blobs,
                float *hit_x, float *hit_y, float *hit_z) {
    float t = 0.0f;

    for (int step = 0; step < MAX_STEPS; step++) {
        float px = ox + dx * t;
        float py = oy + dy * t;
        float pz = oz + dz * t;

        float field = metablob_field(px, py, pz, blobs, num_blobs);

        // Check if we hit the surface
        if (field >= THRESHOLD) {
            *hit_x = px;
            *hit_y = py;
            *hit_z = pz;
            return t;
        }

        // Adaptive step size based on field strength
        float step_size = 0.1f / (field + 0.1f);
        t += step_size;

        if (t > MAX_DISTANCE) break;
    }

    return -1.0f;  // No hit
}

// rotate two coordinates in a plane
void rotate(float *a, float *b, float angle) {
    float cos_a = cosf(angle);
    float sin_a = sinf(angle);
    float temp_a = *a * cos_a - *b * sin_a;
    float temp_b = *a * sin_a + *b * cos_a;
    *a = temp_a;
    *b = temp_b;
}

int main() {
    // get terminal size
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

    int SCREEN_WIDTH = w.ws_col;
    int SCREEN_HEIGHT = w.ws_row;
    int SCREEN_SIZE = SCREEN_WIDTH * SCREEN_HEIGHT;

    // calculate scaling factors to maintain aspect ratio
    float scale_factor = fminf((float)SCREEN_WIDTH / BASE_WIDTH,
                               (float)SCREEN_HEIGHT / BASE_HEIGHT);
    float K1_X = BASE_SCALE_X * scale_factor;
    float K1_Y = BASE_SCALE_Y * scale_factor;

    // calculate Y center offset
    int Y_CENTER = (int)(12.0f * SCREEN_HEIGHT / BASE_HEIGHT);

    float angle_A = 0.0f;  // rotation angle around X-axis
    float angle_B = 0.0f;  // rotation angle around Z-axis

    // allocate dynamic buffers
    float *z_buffer = (float *)malloc(SCREEN_SIZE * sizeof(float));
    char *screen_buffer = (char *)malloc(SCREEN_SIZE * sizeof(char));

    if (!z_buffer || !screen_buffer) {
        fprintf(stderr, "Failed to allocate buffers\n");
        return 1;
    }

    // ANSI escape to clear screen
    printf("\x1b[2J");

    while (1) {
        // initialise buffers
        memset(screen_buffer, ' ', SCREEN_SIZE);
        memset(z_buffer, 0, SCREEN_SIZE * sizeof(float));

        // position metablobs in orbital patterns with different frequencies
        Metablob blobs[NUM_METABLOBS];

        // blob 1: inner orbit, fast rotation
        float orbit1_angle = angle_A * 2.3f;
        blobs[0].x = 1.5f * cosf(orbit1_angle);
        blobs[0].y = 1.5f * sinf(orbit1_angle);
        blobs[0].z = 0.8f * sinf(angle_B * 3.1f);

        // blob 2: middle orbit, medium speed, different phase
        float orbit2_angle = angle_A * 1.7f + 1.5f;
        blobs[1].x = 2.2f * cosf(orbit2_angle);
        blobs[1].y = 2.2f * sinf(orbit2_angle);
        blobs[1].z = 1.0f * cosf(angle_B * 2.3f);

        // blob 3: outer orbit, slower, counter-rotating Z
        float orbit3_angle = angle_A * 1.1f + 3.7f;
        blobs[2].x = 2.6f * cosf(orbit3_angle);
        blobs[2].y = 2.6f * sinf(orbit3_angle);
        blobs[2].z = 0.6f * sinf(angle_B * -1.8f);

        // apply different global rotations to each metablob
        rotate(&blobs[0].y, &blobs[0].z, angle_A * 0.9f);
        rotate(&blobs[0].x, &blobs[0].y, angle_B * 0.6f);

        rotate(&blobs[1].y, &blobs[1].z, angle_A * 0.5f + 1.2f);
        rotate(&blobs[1].x, &blobs[1].y, angle_B * 0.8f);

        rotate(&blobs[2].y, &blobs[2].z, angle_A * 0.3f + 2.5f);
        rotate(&blobs[2].x, &blobs[2].y, angle_B * 0.4f);

        // Render each pixel
        for (int screen_y = 0; screen_y < SCREEN_HEIGHT; screen_y++) {
            for (int screen_x = 0; screen_x < SCREEN_WIDTH; screen_x++) {
                // calculate ray direction
                float ray_x = ((float)screen_x - SCREEN_WIDTH / 2.0f) / K1_X;
                float ray_y = ((float)Y_CENTER - screen_y) / K1_Y;
                float ray_z = 1.0f;

                // normalise ray direction
                float ray_len = sqrtf(ray_x*ray_x + ray_y*ray_y + ray_z*ray_z);
                ray_x /= ray_len;
                ray_y /= ray_len;
                ray_z /= ray_len;

                // ray origin (camera position)
                float origin_x = 0.0f;
                float origin_y = 0.0f;
                float origin_z = -K2;

                // ray march to find intersection
                float hit_x, hit_y, hit_z;
                float t = ray_march(origin_x, origin_y, origin_z,
                                   ray_x, ray_y, ray_z,
                                   blobs, NUM_METABLOBS,
                                   &hit_x, &hit_y, &hit_z);

                if (t > 0.0f) {
                    // calculate depth for z-buffer
                    float depth = 1.0f / (t + 1.0f);

                    int buffer_index = screen_x + SCREEN_WIDTH * screen_y;

                    if (depth > z_buffer[buffer_index]) {
                        z_buffer[buffer_index] = depth;

                        // calculate surface normal
                        float nx, ny, nz;
                        calculate_normal(hit_x, hit_y, hit_z, blobs, NUM_METABLOBS,
                                       &nx, &ny, &nz);

                        // calculate luminance (dot product with light direction)
                        // light direction is roughly (-0.3, -0.7, 0.6) normalised 
                        float luminance = nx * -0.3f + ny * -0.7f + nz * 0.6f;

                        int luminance_index = (int)(luminance * 8.0f + 4.0f);
                        if (luminance_index < 0) luminance_index = 0;
                        if (luminance_index > 11) luminance_index = 11;

                        screen_buffer[buffer_index] = LUMINANCE_CHARS[luminance_index];
                    }
                }
            }
        }

        // ANSI escape to move cursor home 
        printf("\x1b[H");

        // print the screen buffer
        for (int y = 0; y < SCREEN_HEIGHT; y++) {
            for (int x = 0; x < SCREEN_WIDTH; x++) {
                putchar(screen_buffer[x + y * SCREEN_WIDTH]);
            }
            // don't print newline after the last line to prevent scroll
            if (y < SCREEN_HEIGHT - 1) {
                putchar('\n');
            }
        }

        // update rotation angles
        angle_A += ROTATION_SPEED_A;
        angle_B += ROTATION_SPEED_B;

        // Small delay to control frame rate
        usleep(33333);
    }

    // Cleanup (unreachable but...)
    free(z_buffer);
    free(screen_buffer);

    return 0;
}
