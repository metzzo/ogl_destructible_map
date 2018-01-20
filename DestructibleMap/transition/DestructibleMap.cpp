#include "DestructibleMap.h"
#include "clipper.hpp"
#include <iostream>
#include "poly2tri/sweep/cdt.h"
#include <glad/glad.h>
#include "ShaderResource.h"
#include "MeshResource.h"
#include <random>
#include <limits>
#include <glm/gtc/quaternion.hpp>
#include <GLFW/glfw3.h>
#include "DestructibleMapShader.h"
#include "RenderingEngine.h"
#include "DestructibleMapDrawingBatch.h"

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

std::vector<p2t::Point*>* path_to_polyline(ClipperLib::PolyNode *node)
{
	auto polyline = new std::vector<p2t::Point*>(node->Contour.size());
	for (auto j = 0; j < node->Contour.size(); j++)
	{
		(*polyline)[j] = new p2t::Point(double(node->Contour[j].X), double(node->Contour[j].Y));
	}
	return polyline;
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

void triangulate(const ClipperLib::PolyTree &poly_tree, std::vector<glm::vec2> &vertices)
{
	auto current_node = poly_tree.GetFirst()->Parent;
	while (current_node != nullptr)
	{
		if (!current_node->IsHole()) //  && current_node->Contour.size() >= 3
		{
			// convert to Poly2Tri Polygon
			std::vector<std::vector<p2t::Point*> *> holes_registry;
			const auto polyline = path_to_polyline(current_node);

			p2t::CDT* cdt = new p2t::CDT(*polyline);

			for (auto &child_node : current_node->Childs)
			{
				if (!child_node->IsHole())
				{
					std::cout << "Expected only Holes." << std::endl;
					continue;
				}
				/*if (!child_node->Childs.size())
				{
					std::cout << "Expected Nodes with Children." << std::endl;
					continue;
				}*/

				if (child_node->Contour.size() >= 3) {
					const auto hole_polyline = path_to_polyline(child_node);
					cdt->AddHole(*hole_polyline);
					holes_registry.push_back(hole_polyline);
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

			remove_polyline(polyline);
			for (auto &hole : holes_registry)
			{
				remove_polyline(hole);
			}
			delete cdt;
		}

		current_node = current_node->GetNext();
	}
}

void generate_aabb(const std::vector<glm::vec2> &vertices, glm::vec2& boundary_begin, glm::vec2& boundary_end)
{
	for (auto i = 0; i < vertices.size(); i += 3)
	{
		const auto& v0 = vertices[i];
		const auto& v1 = vertices[i + 1];
		const auto& v2 = vertices[i + 2];

		include_point_in_begin_boundary(boundary_begin, v0);
		include_point_in_begin_boundary(boundary_begin, v1);
		include_point_in_begin_boundary(boundary_begin, v2);

		include_point_in_end_boundary(boundary_end, v0);
		include_point_in_end_boundary(boundary_end, v1);
		include_point_in_end_boundary(boundary_end, v2);
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

void DestructibleMap::load(ClipperLib::Paths paths)
{
	std::cout << "Load Map" << std::endl;

	const auto max_float = std::numeric_limits<float>::max();
	glm::vec2 boundary_begin = glm::vec2(max_float, max_float);
	glm::vec2 boundary_end = glm::vec2(-max_float, -max_float);
	ClipperLib::PolyTree poly_tree;

	paths_to_polytree(paths, poly_tree);
	triangulate(poly_tree, this->vertices_);
	generate_aabb(this->vertices_, boundary_begin, boundary_end);

	this->quad_tree_ = DestructibleMapChunk(nullptr, boundary_begin, boundary_end);

	std::cout << "Generating Point Cloud" << std::endl;
	generate_point_cloud(this->triangle_area_ratio_, this->vertices_, this->points_);

	std::cout << "Generating Quad Tree" << std::endl;
	const int count = std::max(this->points_.size() * points_per_leaf_ratio_, 5.0f);
	for (auto& point : this->points_)
	{
		// 0.1% of points are allowed per quad tree
		this->quad_tree_.insert(point, count);
	}

	std::cout << "Applying Polygon" << std::endl;
	this->quad_tree_.apply_polygon(glfwGetTime(), paths);

	this->point_distribution_resource_ = new MeshResource(this->points_);

	this->update_quadtree_representation();
}

void DestructibleMap::update_batches()
{
	std::vector<DestructibleMapChunk*> dirty_chunks;

	this->quad_tree_.query_dirty(dirty_chunks);

	//std::cout << "Dirty Chunks: " << dirty_chunks.size() << std::endl;

	for (auto &chunk : dirty_chunks)
	{
		DestructibleMapDrawingBatch *batch = nullptr;
		if (chunk->get_batch_info())
		{
			batch = chunk->get_batch_info()->batch;
			batch->dealloc_chunk(chunk);
			if (!batch->is_free(chunk->vertices_.size())) {
				batch = nullptr;
			}
		}

		if (batch == nullptr) {
			for (auto &batch_candidate : this->batches_)
			{
				if (batch_candidate->is_free(chunk->vertices_.size()))
				{
					batch = batch_candidate;
					break;
				}
			}
		}

		if (batch == nullptr)
		{
			batch = new DestructibleMapDrawingBatch();
			batch->init();
			this->batches_.push_back(batch);
		}

		if (!batch->is_free(chunk->vertices_.size()))
		{
			std::cout << "Chunk is too big for batch" << std::endl;
		}
		else {
			batch->alloc_chunk(chunk);
		}
	}
}

DestructibleMap::DestructibleMap(float triangle_area_ratio, float points_per_leaf_ratio)
{
	this->rendering_engine_ = nullptr;
	this->point_distribution_resource_ = nullptr;
	this->quadtree_resource_ = nullptr;
	this->triangle_area_ratio_ = triangle_area_ratio;
	this->points_per_leaf_ratio_ = points_per_leaf_ratio;

	this->map_shader_ = new DestructibleMapShader();
	map_shader_->init();
}


DestructibleMap::~DestructibleMap()
{
	for (auto &batch : this->batches_)
	{
		delete batch;
	}
}

void DestructibleMap::load_sample()
{
	const int num_rects = 50;
	const int num_circle = 50;
	const int width = 3000;
	const int height = 3000;

	const int rect_min_size = 10;
	const int rect_max_size = 200;

	ClipperLib::Paths paths;

	for (auto i = 0; i < num_rects; i++)
	{
		auto pos_x = rand() % width;
		auto pos_y = rand() % height;
		auto w = rect_min_size + rand() % (rect_max_size - rect_min_size);
		auto h = rect_min_size + rand() % (rect_max_size - rect_min_size);
		paths.push_back(make_rect(
			glm::ivec2(pos_x*SCALE_FACTOR, pos_y*SCALE_FACTOR),
			glm::ivec2(w*SCALE_FACTOR, h*SCALE_FACTOR)
		));
	}

	for (auto i = 0; i < num_circle; i++)
	{
		auto pos_x = rand() % width;
		auto pos_y = rand() % height;
		auto radius = rect_min_size + rand() % (rect_max_size - rect_min_size);
		paths.push_back(make_circle(
			glm::ivec2(pos_x*SCALE_FACTOR, pos_y*SCALE_FACTOR),
			radius*SCALE_FACTOR,
			32
		));
	}

	this->load(paths);
}


void DestructibleMap::init(RenderingEngine* rendering_engine)
{
	this->rendering_engine_ = rendering_engine;

	this->point_distribution_resource_->init();
	this->quadtree_resource_->init();

	glPointSize(8);
}

void DestructibleMap::draw()
{
	map_draw_calls = 0;

	// batch update
	if (this->quad_tree_.mesh_dirty_)
	{
		update_batches();
	}

	this->map_shader_->use();
	this->map_shader_->set_camera_uniforms(this->rendering_engine_->get_view_matrix(), this->rendering_engine_->get_projection_matrix());
	this->map_shader_->set_base_color(glm::vec3(1.0, 0.0, 0.0));

	glBindVertexArray(this->quadtree_resource_->get_resource_id());
	glDrawArrays(GL_LINES, 0, this->lines_.size());

	glBindVertexArray(this->point_distribution_resource_->get_resource_id());
	glDrawArrays(GL_POINTS, 0, this->points_.size());

	if (glfwGetKey(this->rendering_engine_->get_window(), GLFW_KEY_1))
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	} else
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	this->map_shader_->set_base_color(glm::vec3(0.0, 1.0, 0.0));

	for (auto &batch : batches_)
	{
		batch->draw();
	}

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	glBindVertexArray(0);
}

void DestructibleMap::apply_polygon_operation(const ClipperLib::Path polygon, ClipperLib::ClipType clip_type)
{
	glm::ivec2 begin, end;

	get_bounding_box(polygon, begin, end);

	std::vector<DestructibleMapChunk*> affected_leaves;
	this->quad_tree_.query_range(glm::vec2(begin) * SCALE_FACTOR_INV, glm::vec2(end) * SCALE_FACTOR_INV, affected_leaves);

	for (auto &leave : affected_leaves)
	{
		ClipperLib::PolyTree result_poly_tree;
		ClipperLib::Clipper c;
		c.StrictlySimple(true);
		c.AddPaths(leave->paths_, ClipperLib::ptSubject, true);
		c.AddPath(polygon, ClipperLib::ptClip, true);
		if (!c.Execute(clip_type, result_poly_tree, ClipperLib::pftNonZero))
		{
			std::cout << "Could not create Polygon Tree" << std::endl;
		}

		ClipperLib::Paths result_paths;

		if (result_poly_tree.Total() == 0)
		{
			leave->remove();
		}
		else {
			ClipperLib::PolyTreeToPaths(result_poly_tree, result_paths);

			leave->set_paths(result_paths, result_poly_tree);
		}
	}
}


void DestructibleMap::update_quadtree_representation()
{
	this->lines_.clear();
	this->quad_tree_.get_lines(this->lines_);

	if (this->quadtree_resource_)
	{
		delete this->quadtree_resource_;
	}
	this->quadtree_resource_ = new MeshResource(this->lines_);
}
