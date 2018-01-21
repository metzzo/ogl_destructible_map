#pragma once
#include "DestructibleMap.h"
#include "clipper.hpp"

ClipperLib::Path make_rect(const glm::ivec2 pos, const glm::ivec2 size);
void get_bounding_box(const ClipperLib::Path& polygon, glm::ivec2& begin, glm::ivec2& end);
void generate_point_cloud(float triangle_area_ratio, const std::vector<glm::vec2> &vertices, std::vector<glm::vec2> &points);
void triangulate(const ClipperLib::PolyTree &poly_tree, std::vector<glm::vec2> &vertices);
void triangulate_fast(const ClipperLib::PolyTree &poly_tree, std::vector<glm::vec2> &vertices);
void generate_aabb(const std::vector<glm::vec2> &vertices, glm::vec2& boundary_begin, glm::vec2& boundary_end);
void paths_to_polytree(const ClipperLib::Paths &paths, ClipperLib::PolyTree &poly_tree);
