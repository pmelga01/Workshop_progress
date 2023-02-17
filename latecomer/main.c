#include <stdlib.h>
#include <stdio.h>
#include <curses.h>
#include <time.h>
#include <math.h>
#include "vec.h"
#include "helpers.h"

/* brightness < 0 corresponds to the background using the '`' character */
char  colors[] = {'`', '@', '%', '#', '*', '+', '=', ':', '-', '.', ' '};
char  dark_mode[] = {'_', ' ', '.', '-', ':', '=', '+', '*', '#', '%', '@'};
float levels[] = {0.0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0};

/* N_LEVELS adapts automatically if you add more levels */
const int N_LEVELS = sizeof(levels) / sizeof(*levels); 

/* program settings macros (change freely) */
#define MAX_TIME 5.0
#define MAX_WINDOW_W 300
#define MAX_WINDOW_H 120
#define WINDOW_STRETCH_FACTOR 2.0 
#define MAX_STEPS 100       /* Steps to run before giving up, assume miss object*/
#define MAX_DIST 12.0
#define SURFACE_DIST 0.01   /* Minimal distance from an object so we know that we hit the object */

/* rendering performance macros */
/* @TODO */

/* camera setting macros */
/* @TODO */
            // x, y, z      where z is forward (negative) backward (positive), x is side-to-side, y is up/down
                            //we look down the negative z axis
#define RAY_ORIGIN 0, 2, -6
#define LOOKAT_PT 0, 0, 0
#define LIGHT_ORIGIN 6*cos(3.0 * time), 10, 0


/* rendering functions */
/* @TODO */
float get_distance(float3 position, float time) {
    // make move here by changing position
    position.y += sin(time);
    position = rotate3Z(position, sin(time));
    float distance = torus_SDF(position, 2.0, 1.0);

    // float torus_distance = torus_SDF()
    // float cube_distance = cube_SDF()
    return distance;
}


float raymarch(float3 origin, float3 direction, float time) {
    float distance_to_origin = 0.0;
    for (int i = 0; i < MAX_STEPS; i++) {
        // Moving our point along the direction.
        float3 position = add3(origin, scale3(direction, distance_to_origin));
        float dist_to_scene = get_distance(position, time);
        distance_to_origin += dist_to_scene;
        
        if (distance_to_origin >= MAX_DIST || fabsf(dist_to_scene) < SURFACE_DIST) {
            return distance_to_origin;
        }
    }
    return distance_to_origin;
}

float3 get_normal( float3 position, float time) {
    float3 p = position;
    float d = get_distance(p, time);
    float e = 0.001;

    // moving the position a little bit on each axis, get those changes to create gradient
    float3 gradient = {
        d - get_distance(f3new(p.x - e, p.y, p.z), time),
        d - get_distance(f3new(p.x , p.y - e, p.z), time),
        d - get_distance(f3new(p.x , p.y, p.z - e), time),
    };

    return normalize3(gradient);
}

float get_light(float3 position, float3 normal, float3 light_origin) {
    // ray bounces off the object and in some direction, we want to see how far that direction is to the position of our light
    // where if direction closer to light, brighter, if direction is farther from light, darker
    float3 light_direction = normalize3(subtract3(light_origin, position));
    float diffuse = dot3(light_direction, normal);


    //float down_angle = -normal.y * 0.5 + 0.5;
    //float reflected_brightness = fmax(down_angle * 0.4, 0.001);
    
    return diffuse; 
    //clamp(diffuse, reflected_brightness, 1.0);
}

/* drawing functions */
float coordinate_to_brightness(float x, float y, float time) {
    float2 xy = f2new(x, y);
    float3 ray_origin = f3new(RAY_ORIGIN);
    float3 lookat_pt = f3new(LOOKAT_PT);
    float3 light_origin = f3new(LIGHT_ORIGIN);
    float3 ray_direction = get_ray_direction(xy, ray_origin, lookat_pt, 1.0);
    float distance = raymarch(ray_origin, ray_direction, time);

    if (distance >= MAX_DIST) {
        return -1.0;
        // if hit background
    } 

    //float3 hit_position = add3(ray_origin, scale3(ray_direction, distance));
    //float hit_normal = get_normal(hit_position, hit_normal, light_origin); 
    float3 position = add3(ray_origin, scale3(ray_direction, distance));
    float3 normal = get_normal(position, time);
    float brightness = get_light(position, normal, light_origin);
    return brightness;
    
}

char brightness_to_ascii(float brightness) {
    int i = 0;
    while (brightness > levels[i] && i < N_LEVELS)
        i++;
    return colors[i];
}

void draw_scene(int width, int height, float time) {

    float aspect_ratio = (float)width / ((float)height);
    
    for (int i = 0; i < width; i++) {
        for (int j = 0; j < height; j++) {
            move(j, i);
            float x = 2.0f * (float)i / (float)width - 1.0f;
            float y = 2.0f * (float)j / (float)height - 1.0f;
            x *= aspect_ratio / WINDOW_STRETCH_FACTOR;
            y *= -1.0;
            float brightness = coordinate_to_brightness(x, y, time);
            float ascii_pixel = brightness_to_ascii(brightness);
            addch(ascii_pixel);
        }
    }
}

/* shape functions */

int main()
{
    initscr();
    curs_set(0);
    clock_t time_start = clock();
    float time_elapsed = 0.0;
    int frames = 0;
    do {
        time_elapsed = (float)(clock() - time_start) / CLOCKS_PER_SEC;
        int width = min(COLS, MAX_WINDOW_W);
        int height = min(LINES, MAX_WINDOW_H);
        draw_scene(width, height, time_elapsed);
        refresh();
        frames++;
    } while (time_elapsed < MAX_TIME);
    endwin();
    printf("Framerate %.0f fps\n", frames / time_elapsed);
    return 0;
}