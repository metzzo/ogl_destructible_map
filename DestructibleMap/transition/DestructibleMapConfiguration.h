#pragma once

// how many vertices are allowed per batch?
#define VERTICES_PER_BATCH (4096)

// how many batches are available on start
#define NUM_START_BATCHES (64)

// how many vertices per chunk should be allowed
#define VERTICES_PER_CHUNK (64)

// factor from real coordinates to Clipper coordinates
#define SCALE_FACTOR (1000.0f)

// factor from Clipper coordinates to real coordinates
#define SCALE_FACTOR_INV (1.0f/SCALE_FACTOR)

// how big is the triangulation buffer (used when fast triangulation is performed, this is the maximum number of points allowed)
#define TRIANGULATION_BUFFER (VERTICES_PER_CHUNK*3)

// how many rects should be generated
#define GENERATE_NUM_RECTS (500)

// how many circles should be generated
#define GENERATE_NUM_CIRCLES (500)

// width of the map (in real coordinates)
#define GENERATE_WIDTH (5000)

// height of the map (in real coordinates)
#define GENERATE_HEIGHT (5000)

// minimum size of shapes (in real coordinates)
#define GENERATE_MIN_SIZE (10)

// maximum size of shapes (in real coordinates)
#define GENERATE_MAX_SIZE (300)

// area*MAP_TRIANGLE_AREA_RATIO many points are being used for monte carlo point cloud approximation for the quadtree
#define MAP_TRIANGLE_AREA_RATIO (0.025f)

// x=total_points*MAP_POINTS_PER_LEAF_RATIO is the maximum number of points which should be stored in each leaf in the quad tree (if number is greater than x a new leaf is generated).
#define MAP_POINTS_PER_LEAF_RATIO (0.0005f)

