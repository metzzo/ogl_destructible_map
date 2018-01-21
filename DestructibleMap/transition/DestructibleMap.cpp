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
#include <omp.h>
#include "DestructibleMapUtility.h"

DestructibleMap::DestructibleMap(float triangle_area_ratio, float points_per_leaf_ratio)
{
	this->rendering_engine_ = nullptr;
	this->point_distribution_resource_ = nullptr;
	this->quadtree_resource_ = nullptr;
	this->triangle_area_ratio_ = triangle_area_ratio;
	this->points_per_leaf_ratio_ = points_per_leaf_ratio;
	this->startup_displayed_ = false;

	this->map_shader_ = new DestructibleMapShader();
	map_shader_->init();
}


DestructibleMap::~DestructibleMap()
{
	for (auto &batch : this->batches_)
	{
		delete batch;
	}
	delete this->map_shader_;

	if (point_distribution_resource_)
	{
		delete point_distribution_resource_;
	}

	if (quadtree_resource_)
	{
		delete quadtree_resource_;
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

#ifdef ENABLE_MERGING_SUBDIVIDING
	std::cout << "Generating Point Cloud" << std::endl;
	generate_point_cloud(this->triangle_area_ratio_, this->vertices_, this->points_);

	std::cout << "Generating Quad Tree" << std::endl;
	const int count = std::max(this->points_.size() * points_per_leaf_ratio_, 5.0f);
	for (auto& point : this->points_)
	{
		this->quad_tree_.insert(point, count);
	}
#endif

	std::cout << "Applying Polygon" << std::endl;
	this->quad_tree_.apply_polygon(paths);

	this->point_distribution_resource_ = new MeshResource(this->points_);
}

void DestructibleMap::update_batches()
{
	auto time = glfwGetTime();
	while (this->quad_tree_.mesh_dirty_)
	{
		std::vector<DestructibleMapChunk*> dirty_chunks;
		std::vector<DestructibleMapChunk*> merge_chunks;

		this->quad_tree_.query_dirty(dirty_chunks);

		for (auto &chunk : dirty_chunks)
		{
#ifdef ENABLE_MERGING_SUBDIVIDING
			auto parent = chunk->parent_;
			if (parent != nullptr && parent->north_west_ && !parent->north_west_->north_west_ && !parent->north_east_->north_west_ && !parent->south_east_->north_west_ && !parent->south_west_->north_west_)
			{
				auto total_vertices = parent->north_west_->vertices_.size() + parent->north_east_->vertices_.size() + parent->south_west_->vertices_.size() + parent->south_east_->vertices_.size();

				if (parent->mergeable_count_ == 0 && total_vertices < VERTICES_PER_CHUNK) {
					// chunk may be merged with parent

					// => increase mergeable count of leaves
					parent->north_west_->mergeable_count_++;
					parent->north_east_->mergeable_count_++;
					parent->south_west_->mergeable_count_++;
					parent->south_east_->mergeable_count_++;

					// => increase mergeable count to root
					auto current = parent;
					while (current)
					{
						current->mergeable_count_++;
						current = current->parent_;
					}
				} else if (parent->mergeable_count_ > 0 && total_vertices >= VERTICES_PER_CHUNK)
				{
					// chunk was previously marked as being mergable, but it got some vertices, so it is NOT mergeable

					parent->north_west_->mergeable_count_--;
					parent->north_east_->mergeable_count_--;
					parent->south_west_->mergeable_count_--;
					parent->south_east_->mergeable_count_--;

					// => decrease mergeable count to root
					auto current = parent;
					while (current)
					{
						current->mergeable_count_--;
						current = current->parent_;
					}
				}
			}
#endif
			DestructibleMapDrawingBatch *batch = nullptr;
			if (chunk->get_batch_info())
			{
				auto info = chunk->get_batch_info();
				batch = info->batch;

				batch->dealloc_chunk(chunk);
				if (!batch->is_free(chunk->vertices_.size())) {
					batch = nullptr;
				}
			}

#ifdef ENABLE_MERGING_SUBDIVIDING
			if (chunk->vertices_.size() >= VERTICES_PER_CHUNK)
			{
				chunk->subdivide();
			}
			else
#endif
			{
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

				batch->alloc_chunk(chunk);
			}
		}
	}

	// merge parents

#ifdef ENABLE_MERGING_SUBDIVIDING
	auto mergeable = this->quad_tree_.get_best_mergeable();
	if (mergeable != nullptr)
	{
		mergeable->merge();
	}
#endif

	//std::cout << "Time " << (glfwGetTime() - time) * 1000 << std::endl;
}

void DestructibleMap::generate_map(int num_rects, int num_circle, int width, int height, int min_size, int max_size)
{
	this->start_time_ = glfwGetTime();

	std::cout << "Generate Map" << std::endl;

	ClipperLib::Paths paths;

	for (auto i = 0; i < num_rects; i++)
	{
		auto pos_x = rand() % width;
		auto pos_y = rand() % height;
		auto w = min_size + rand() % (max_size - min_size);
		auto h = min_size + rand() % (max_size - min_size);
		paths.push_back(make_rect(
			glm::ivec2(pos_x*SCALE_FACTOR, pos_y*SCALE_FACTOR),
			glm::ivec2(w*SCALE_FACTOR, h*SCALE_FACTOR)
		));
	}

	for (auto i = 0; i < num_circle; i++)
	{
		auto pos_x = rand() % width;
		auto pos_y = rand() % height;
		auto radius = min_size + rand() % (max_size - min_size);
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

	this->update_quadtree_representation();

	this->point_distribution_resource_->init();

	glPointSize(8);

	for (auto i = 0; i < NUM_START_BATCHES; i++)
	{
		auto batch = new DestructibleMapDrawingBatch();
		batch->init();
		this->batches_.push_back(batch);
	}
}

void DestructibleMap::draw()
{
	map_draw_calls = 0;

	update_batches();

	this->map_shader_->use();
	this->map_shader_->set_camera_uniforms(this->rendering_engine_->get_view_matrix(), this->rendering_engine_->get_projection_matrix());
	this->map_shader_->set_base_color(glm::vec3(1.0, 0.0, 0.0));

	glBindVertexArray(this->quadtree_resource_->get_resource_id());
	glDrawArrays(GL_LINES, 0, this->lines_.size());

	if (glfwGetKey(this->rendering_engine_->get_window(), GLFW_KEY_3)) {
		glBindVertexArray(this->point_distribution_resource_->get_resource_id());
		glDrawArrays(GL_POINTS, 0, this->points_.size());
	}

	if (glfwGetKey(this->rendering_engine_->get_window(), GLFW_KEY_1))
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	} else
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	this->map_shader_->set_base_color(glm::vec3(0.0, 1.0, 0.0));
	auto time = glfwGetTime();
	for (auto &batch : batches_)
	{
		
		for (auto &info : batch->infos_)
		{
			if (info->chunk != nullptr && info->chunk->highlighted_)
			{
				this->map_shader_->set_base_color(glm::vec3(1.0, 1.0, 0.0));
			}
		}

		batch->draw(this->map_shader_);

		this->map_shader_->set_base_color(glm::vec3(0.0, 1.0, 0.0));
	}
	//std::cout << "Time: " << (glfwGetTime() - time) * 1000 << std::endl;

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	glBindVertexArray(0);

	if (!startup_displayed_)
	{
		std::cout << "Time from loading until first frame " << (glfwGetTime() - this->start_time_) << std::endl;
		this->startup_displayed_ = true;
	}
}

void DestructibleMap::apply_polygon_operation(const ClipperLib::Path polygon, ClipperLib::ClipType clip_type)
{
	double time = glfwGetTime();
	glm::ivec2 begin, end;

	get_bounding_box(polygon, begin, end);

	std::vector<DestructibleMapChunk*> affected_leaves;
	this->quad_tree_.query_range(glm::vec2(begin) * SCALE_FACTOR_INV, glm::vec2(end) * SCALE_FACTOR_INV, affected_leaves);

#pragma omp parallel for
	for (auto i = 0; i < affected_leaves.size(); i++)
	{
		auto &leave = affected_leaves[i];

		ClipperLib::PolyTree result_poly_tree;
		ClipperLib::Paths path_inside_bounds;
		ClipperLib::Clipper c;
		c.StrictlySimple(true);
		c.AddPath(polygon, ClipperLib::ptSubject, true);
		c.AddPath(leave->quad_, ClipperLib::ptClip, true);

		if (clip_type == ClipperLib::ctIntersection)
		{
			c.AddPaths(leave->paths_, ClipperLib::ptClip, true);

			ClipperLib::Paths result_paths;

			ClipperLib::PolyTreeToPaths(result_poly_tree, result_paths);

			leave->set_paths(result_paths, result_poly_tree, true);
		}
		else {
			if (!c.Execute(ClipperLib::ctIntersection, path_inside_bounds, ClipperLib::pftNonZero))
			{
				std::cout << "Could not create Polygon Tree" << std::endl;
			}

			if (path_inside_bounds.size() == 0)
			{
				continue;
			}

			c.Clear();
			c.AddPaths(leave->paths_, ClipperLib::ptSubject, true);
			c.AddPaths(path_inside_bounds, ClipperLib::ptClip, true);
			if (!c.Execute(clip_type, result_poly_tree, ClipperLib::pftNonZero))
			{
				std::cout << "Could not create Polygon Tree" << std::endl;
			}

			ClipperLib::Paths result_paths;

			ClipperLib::PolyTreeToPaths(result_poly_tree, result_paths);

			leave->set_paths(result_paths, result_poly_tree, true);
		}
	}

	//std::cout << "Time: " << (glfwGetTime() - time) * 1000 << std::endl;
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
	this->quadtree_resource_->init();
}
