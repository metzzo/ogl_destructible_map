#include "DestructibleMap.h"
#include "clipper.hpp"
#include <iostream>
#include "poly2tri/sweep/cdt.h"
#include "ShaderResource.h"
#include "MeshResource.h"
#include <random>
#include <glm/gtc/quaternion.hpp>
#include <GLFW/glfw3.h>
#include "RenderingEngine.h"
#include "DestructibleMapDrawingBatch.h"
#include <omp.h>


float triangle_area(const float d_x0, const float d_y0, const float d_x1, const float d_y1, const float d_x2, const float d_y2)
{
	return abs(((d_x1 - d_x0)*(d_y2 - d_y0) - (d_x2 - d_x0)*(d_y1 - d_y0)) / 2.0);
}

void get_bounding_box(const ClipperLib::Path& polygon, glm::ivec2& begin, glm::ivec2& end)
{
	end = glm::ivec2(INT_MIN, INT_MIN);
	begin = glm::ivec2(INT_MAX, INT_MAX);
	for (const auto &point : polygon)
	{
		begin.x = std::min(begin.x, int(point.X));
		begin.y = std::min(begin.y, int(point.Y));
		end.x = std::max(end.x, int(point.X));
		end.y = std::max(end.y, int(point.Y));
	}
}

ClipperLib::Path make_rect(const glm::ivec2 pos, const glm::ivec2 size)
{
	ClipperLib::Path rect;
	rect <<
		ClipperLib::IntPoint(pos.x, pos.y) <<
		ClipperLib::IntPoint(pos.x + size.x, pos.y) <<
		ClipperLib::IntPoint(pos.x + size.x, pos.y + size.y) <<
		ClipperLib::IntPoint(pos.x, pos.y + size.y);
	return rect;
}

ClipperLib::Path make_circle(const glm::ivec2 pos, const float radius, const int num_of_points)
{
	auto angle_step = glm::radians(360.0f) / num_of_points;
	auto quat = glm::quat(glm::vec3(0.0f, 0.0f, angle_step));
	auto points = std::vector<glm::vec3>(num_of_points + 1);
	points.push_back(glm::vec3(0, 0, 0));
	points.push_back(glm::vec3(0, radius, 0));
	points.push_back(quat * glm::vec3(0, radius, 0));

	for (int i = 0; i < num_of_points - 1; i++)
	{
		points.push_back(quat * points[points.size() - 1]);
	}

	ClipperLib::Path circle;
	for (auto &p : points)
	{
		circle.push_back(ClipperLib::IntPoint(p.x + pos.x, p.y + pos.y));
	}
	return circle;
}

void path_to_polyline(std::vector<p2t::Point*> &polyline, ClipperLib::PolyNode *node, p2t::Point *points, int &num_points)
{
	for (auto j = 0; j < node->Contour.size(); j++)
	{
		points[num_points] = p2t::Point(double(node->Contour[j].X), double(node->Contour[j].Y));
		polyline.push_back(&points[num_points]);
		num_points++;
	}
}

void remove_polyline(std::vector<p2t::Point*>* polyline)
{
	for (auto& point : *polyline)
	{
		delete point;
	}
	delete polyline;
}

void include_point_in_begin_boundary(glm::vec2& begin_boundary, const glm::vec2& pos)
{
	begin_boundary.x = std::min(begin_boundary.x, pos.x);
	begin_boundary.y = std::min(begin_boundary.y, pos.y);
}

void include_point_in_end_boundary(glm::vec2& end_boundary, const glm::vec2& pos)
{
	end_boundary.x = std::max(end_boundary.x, pos.x);
	end_boundary.y = std::max(end_boundary.y, pos.y);
}

void generate_point_cloud(float triangle_area_ratio, const std::vector<glm::vec2> &vertices, std::vector<glm::vec2> &points)
{
	std::random_device seeder;
	std::mt19937 engine(seeder());
	std::uniform_real_distribution<float> uniform_dist(0.0, 1.0);

	points.clear();
	for (auto i = 0; i < vertices.size(); i += 3)
	{
		const auto& v0 = vertices[i];
		const auto& v1 = vertices[i + 1];
		const auto& v2 = vertices[i + 2];
		const auto center = (v0 + v1 + v2) * float(1.0 / 3.0);

		points.push_back(v0);
		points.push_back(v1);
		points.push_back(v2);
		points.push_back(center);

		// get approximate density distribution
		const auto area = int(triangle_area(v0.x, v0.y, v1.x, v1.y, v2.x, v2.y) * triangle_area_ratio + 0.5);

		for (auto distr = 0; distr < area; distr++) {
			// Uniformly distribute points on triangle:
			// https://math.stackexchange.com/questions/18686/uniform-random-point-in-triangle
			const float r1 = sqrt(uniform_dist(engine));
			const float r2 = uniform_dist(engine);

			const auto point = (1 - r1)*v0 + (r1*(1 - r2))*v1 + (r2*r1)*v2;
			points.push_back(point);
		}
	}

	// shuffle points to avoid degenerate quadtree
	std::random_shuffle(points.begin(), points.end());
}

void ensure_points_not_overlapping(ClipperLib::Path &path)
{
	auto prev = path.size() - 1;
	for (auto i = 0; i < path.size(); i++)
	{
		path[i].X - path[prev].X > 0 ? path[i].X-- : path[i].X++;
		path[i].Y - path[prev].Y > 0 ? path[i].Y-- : path[i].Y++;
		prev = i;
	}
}

void triangulate(const ClipperLib::PolyTree &poly_tree, std::vector<glm::vec2> &vertices)
{
	if (poly_tree.Total() == 0)
	{
		return;
	}
	auto needed_num_points = 0;
	auto current_node = poly_tree.GetFirst()->Parent;
	while (current_node != nullptr)
	{
		if (!current_node->IsHole())
		{
			// convert to Poly2Tri Polygon
			needed_num_points += current_node->Contour.size();
			for (auto &child_node : current_node->Childs)
			{
				if (!child_node->IsHole())
				{
					continue;
				}

				if (child_node->Contour.size() >= 3) {
					needed_num_points += child_node->Contour.size();
				}
			}
		}
		current_node = current_node->GetNext();
	}


	p2t::Point *points = new p2t::Point[needed_num_points];
	std::vector<p2t::Point*> polyline;
	polyline.reserve(needed_num_points);
	std::vector<p2t::Point*> hole_polyline;
	polyline.reserve(needed_num_points);

	current_node = poly_tree.GetFirst()->Parent;
	while (current_node != nullptr)
	{
		if (!current_node->IsHole())
		{
			// convert to Poly2Tri Polygon

			int num_points = 0;
			polyline.clear();
			path_to_polyline(polyline, current_node, points, num_points);

			p2t::CDT* cdt = new p2t::CDT(polyline);

			for (auto &child_node : current_node->Childs)
			{
				if (!child_node->IsHole())
				{
					std::cout << "Expected only Holes." << std::endl;
					continue;
				}

				if (child_node->Contour.size() >= 3) {
					hole_polyline.clear();
					ensure_points_not_overlapping(child_node->Contour);
					path_to_polyline(hole_polyline, child_node, points, num_points);
					cdt->AddHole(hole_polyline);
				}

			}

			cdt->Triangulate();

			auto triangles = cdt->GetTriangles();
			for (auto& triangle : triangles) {
				const auto p0 = triangle->GetPoint(0);
				const auto p1 = triangle->GetPoint(1);
				const auto p2 = triangle->GetPoint(2);

				const auto v0 = glm::vec2(float(p0->x), float(p0->y)) * SCALE_FACTOR_INV;
				const auto v1 = glm::vec2(float(p1->x), float(p1->y)) * SCALE_FACTOR_INV;
				const auto v2 = glm::vec2(float(p2->x), float(p2->y)) * SCALE_FACTOR_INV;

				vertices.push_back(v0);
				vertices.push_back(v1);
				vertices.push_back(v2);
			}

			delete cdt;
		}

		current_node = current_node->GetNext();
	}
	delete[] points;
}

void triangulate_fast(const ClipperLib::PolyTree &poly_tree, std::vector<glm::vec2> &vertices)
{
	if (poly_tree.Total() == 0)
	{
		return;
	}

	p2t::Point points[TRIANGULATION_BUFFER];
	std::vector<p2t::Point*> polyline;
	polyline.reserve(TRIANGULATION_BUFFER);
	std::vector<p2t::Point*> hole_polyline;
	polyline.reserve(TRIANGULATION_BUFFER);

	auto current_node = poly_tree.GetFirst()->Parent;
	while (current_node != nullptr)
	{
		if (!current_node->IsHole())
		{
			// convert to Poly2Tri Polygon

			int num_points = 0;
			polyline.clear();
			path_to_polyline(polyline, current_node, points, num_points);
			assert(num_points < TRIANGULATION_BUFFER);

			p2t::CDT* cdt = new p2t::CDT(polyline);

			for (auto &child_node : current_node->Childs)
			{
				if (!child_node->IsHole())
				{
					std::cout << "Expected only Holes." << std::endl;
					continue;
				}

				if (child_node->Contour.size() >= 3) {
					hole_polyline.clear();
					ensure_points_not_overlapping(child_node->Contour);
					path_to_polyline(hole_polyline, child_node, points, num_points);
					assert(num_points < TRIANGULATION_BUFFER);
					cdt->AddHole(hole_polyline);
				}

			}

			cdt->Triangulate();

			auto triangles = cdt->GetTriangles();
			for (auto& triangle : triangles) {
				const auto p0 = triangle->GetPoint(0);
				const auto p1 = triangle->GetPoint(1);
				const auto p2 = triangle->GetPoint(2);

				const auto v0 = glm::vec2(float(p0->x), float(p0->y)) * SCALE_FACTOR_INV;
				const auto v1 = glm::vec2(float(p1->x), float(p1->y)) * SCALE_FACTOR_INV;
				const auto v2 = glm::vec2(float(p2->x), float(p2->y)) * SCALE_FACTOR_INV;

				vertices.push_back(v0);
				vertices.push_back(v1);
				vertices.push_back(v2);
			}

			delete cdt;
		}

		current_node = current_node->GetNext();
	}
}

void generate_aabb(const std::vector<glm::vec2> &vertices, glm::vec2& boundary_begin, glm::vec2& boundary_end)
{
	for (auto i = 0; i < vertices.size(); i++)
	{
		const auto& v = vertices[i];

		include_point_in_begin_boundary(boundary_begin, v);
		include_point_in_end_boundary(boundary_end, v);
	}
}

void print_vertices(const std::vector<glm::vec2> &vertices)
{
	std::cout << "Vertices: " << std::endl;
	for (auto &vertex : vertices)
	{
		std::cout << vertex.x << " " << vertex.y << std::endl;
	}
}

void paths_to_polytree(const ClipperLib::Paths &paths, ClipperLib::PolyTree &poly_tree)
{
	ClipperLib::Clipper c;
	c.StrictlySimple(true);
	c.AddPaths(paths, ClipperLib::ptSubject, true);
	if (!c.Execute(ClipperLib::ctUnion, poly_tree, ClipperLib::pftNonZero))
	{
		std::cout << "Could not create Polygon Tree" << std::endl;
	}
}
