
#include "DestructibleMapChunk.h"
#include <cassert>
#include "clipper.hpp"
#include <iostream>
#include <glad/glad.h>
#include "ShaderResource.h"
#include "MeshResource.h"
#include <random>

int map_chunks_drawn;

ClipperLib::Path make_rect(const glm::ivec2 pos, const glm::ivec2 size);
void triangulate(const ClipperLib::PolyTree &poly_tree, std::vector<glm::vec2> &vertices);

DestructibleMapChunk::DestructibleMapChunk(DestructibleMapChunk *parent, const glm::vec2 begin, const glm::vec2 end)
{
	assert(begin.x < end.x && begin.y < end.y);
	this->begin_ = begin;
	this->end_ = end;
	this->north_west_ = nullptr;
	this->north_east_ = nullptr;
	this->south_west_ = nullptr;
	this->south_east_ = nullptr;
	this->mesh_ = nullptr;
	this->parent_ = parent;
	this->last_modify_ = 0.0;
	this->is_cached_ = false;
	this->initialized_ = false;
}

DestructibleMapChunk::DestructibleMapChunk()
{
	this->north_west_ = nullptr;
	this->north_east_ = nullptr;
	this->south_west_ = nullptr;
	this->south_east_ = nullptr;
	this->mesh_ = nullptr;
	this->parent_ = nullptr;
	this->last_modify_ = 0.0;
	this->is_cached_ = false;
	this->initialized_ = false;
}

DestructibleMapChunk::~DestructibleMapChunk()
{
	if (this->north_west_)
	{
		delete this->north_west_;
		delete this->north_east_;
		delete this->south_west_;
		delete this->south_east_;
	}
	if (this->mesh_)
	{
		delete this->mesh_;
	}
}

void DestructibleMapChunk::init(RenderingEngine* engine)
{
	if (this->mesh_ != nullptr)
	{
		this->mesh_->init();
	}

	if (this->north_west_)
	{
		this->north_west_->init(engine);
		this->north_east_->init(engine);
		this->south_west_->init(engine);
		this->south_east_->init(engine);
	}

	this->initialized_ = true;
}


void DestructibleMapChunk::get_lines(std::vector<glm::vec2>& lines) const
{
	if (this->north_west_)
	{
		this->north_west_->get_lines(lines);
		this->north_east_->get_lines(lines);
		this->south_west_->get_lines(lines);
		this->south_east_->get_lines(lines);
	}
	else
	{
		const glm::vec2 size = this->end_ - this->begin_;
		lines.push_back(this->begin_);
		lines.push_back(this->begin_ + glm::vec2(size.x, 0));

		lines.push_back(this->begin_);
		lines.push_back(this->begin_ + glm::vec2(0, size.y));

		lines.push_back(this->end_);
		lines.push_back(this->end_ - glm::vec2(size.x, 0));

		lines.push_back(this->end_);
		lines.push_back(this->end_ - glm::vec2(0, size.y));
	}
}

bool DestructibleMapChunk::insert(const glm::vec2& point, const int max_points)
{
	if (point.x < this->begin_.x || point.y < this->begin_.y || point.x > this->end_.x || point.y > this->end_.y)
	{
		return false;
	}

	if (this->points_.size() < max_points)
	{
		this->points_.push_back(point);
		return true;
	}

	if (this->north_west_ == nullptr)
	{
		const glm::vec2 size = (this->end_ - this->begin_) / float(2.0);
		const glm::vec2 size_x = glm::vec2(size.x, 0);
		const glm::vec2 size_y = glm::vec2(0, size.y);

		this->north_west_ = new DestructibleMapChunk(this, this->begin_, this->begin_ + size);
		this->north_east_ = new DestructibleMapChunk(this, this->begin_ + size_x, this->begin_ + size_x + size);
		this->south_west_ = new DestructibleMapChunk(this, this->begin_ + size_y, this->begin_ + size_y + size);
		this->south_east_ = new DestructibleMapChunk(this, this->begin_ + size, this->end_);

		for (auto& old_points : this->points_)
		{
			this->insert(old_points, max_points);
		}
		this->points_.clear();
	}

	if (this->north_west_->insert(point, max_points))
	{
		return true;
	}
	if (this->north_east_->insert(point, max_points))
	{
		return true;
	}
	if (this->south_west_->insert(point, max_points))
	{
		return true;
	}
	if (this->south_east_->insert(point, max_points))
	{
		return true;
	}
	return false;
}


void DestructibleMapChunk::apply_polygon(const double time, const ClipperLib::Paths &input_paths)
{
	const glm::ivec2 quad_pos = glm::ivec2(this->begin_* SCALE_FACTOR);
	const glm::ivec2 quad_size = glm::ivec2((this->end_ - this->begin_) * SCALE_FACTOR);
	const ClipperLib::Path quad = make_rect(
		quad_pos,
		quad_size
	);
	ClipperLib::PolyTree result_poly_tree;
	ClipperLib::Clipper c;
	c.StrictlySimple(true);
	c.AddPaths(input_paths, ClipperLib::ptSubject, true);
	c.AddPath(quad, ClipperLib::ptClip, true);
	if (!c.Execute(ClipperLib::ctIntersection, result_poly_tree, ClipperLib::pftNonZero))
	{
		std::cout << "Could not create Polygon Tree" << std::endl;
	}

	if (result_poly_tree.Total() == 0)
	{
		return;
	}


	this->last_modify_ = time;

	ClipperLib::Paths result_paths;
	ClipperLib::PolyTreeToPaths(result_poly_tree, result_paths);

	if (this->north_west_)
	{
		this->north_west_->apply_polygon(time, result_paths);
		this->north_east_->apply_polygon(time, result_paths);
		this->south_west_->apply_polygon(time, result_paths);
		this->south_east_->apply_polygon(time, result_paths);
	}
	else
	{
		this->set_paths(result_paths, result_poly_tree);
	}
}

void DestructibleMapChunk::query_range(const glm::vec2& query_begin, const glm::vec2& query_end, std::vector<DestructibleMapChunk*> &leaves)
{
	assert(query_begin.x < query_end.x && query_begin.y < query_end.y);

	if (query_end.x < this->begin_.x || query_end.y < this->begin_.y || query_begin.x > this->end_.x || query_begin.x > this->end_.y)
	{
		return;
	}

	if (this->north_west_)
	{
		this->north_west_->query_range(query_begin, query_end, leaves);
		this->north_east_->query_range(query_begin, query_end, leaves);
		this->south_west_->query_range(query_begin, query_end, leaves);
		this->south_east_->query_range(query_begin, query_end, leaves);
	}
	else
	{
		leaves.push_back(this);
	}
}

DestructibleMapChunk *DestructibleMapChunk::query_random()
{
	if (this->north_west_ && !this->is_cached_)
	{
		DestructibleMapChunk *chunks[] = {
			this->north_west_,
			this->north_east_,
			this->south_west_,
			this->south_east_
		};

		return chunks[rand() % 4];
	}
	else
	{
		return this;
	}
}


void DestructibleMapChunk::remove_paths()
{
	this->paths_.clear();
	this->vertices_.clear();

	if (this->mesh_)
	{
		delete this->mesh_;
	}
}

void DestructibleMapChunk::set_paths(const ClipperLib::Paths &paths, const ClipperLib::PolyTree &poly_tree)
{
	this->paths_ = paths;
	this->vertices_.clear();
	triangulate(poly_tree, this->vertices_);

	Material mat;
	mat.set_diffuse_color(glm::vec3(0.5, 0.5, 0.5));
	mat.set_ambient_color(glm::vec3(0.0, 1.0, 0.1));

	// TODO: reuse old mesh
	if (this->mesh_)
	{
		delete this->mesh_;
	}

	this->mesh_ = new MeshResource(this->vertices_, mat);

	if (this->initialized_) 
	{
		this->mesh_->init();
	}
}

void DestructibleMapChunk::simplify()
{
	// TODO
}

void DestructibleMapChunk::draw() const
{
	if (this->north_west_ && !this->is_cached_)
	{
		this->north_west_->draw();
		this->north_east_->draw();
		this->south_west_->draw();
		this->south_east_->draw();
	}
	else if (this->mesh_ != nullptr)
	{
		map_chunks_drawn++;
		glBindVertexArray(this->mesh_->get_resource_id());
		glDrawArrays(GL_TRIANGLES, 0, this->vertices_.size());
	}
}

void DestructibleMapChunk::remove()
{
	// TODO: proper removing
	if (this->mesh_)
	{
		delete this->mesh_;
	}

	this->mesh_ = nullptr;

	const auto parent = this->parent_;
	if (parent &&
		parent->north_west_->mesh_ == nullptr &&
		parent->north_east_->mesh_ == nullptr &&
		parent->south_west_->mesh_ == nullptr &&
		parent->south_east_->mesh_ == nullptr)
	{
		delete parent->north_west_;
		delete parent->north_east_;
		delete parent->south_west_;
		delete parent->south_east_;

		parent->north_west_ = nullptr;
		parent->north_east_ = nullptr;
		parent->south_west_ = nullptr;
		parent->south_east_ = nullptr;
	}
}
