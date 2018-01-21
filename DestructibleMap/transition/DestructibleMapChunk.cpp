
#include "DestructibleMapChunk.h"
#include <cassert>
#include "clipper.hpp"
#include <iostream>
#include <glad/glad.h>
#include "ShaderResource.h"
#include <random>
#include "DestructibleMap.h"
#include "DestructibleMapDrawingBatch.h"
#include "DestructibleMapUtility.h"

int map_draw_calls;


void DestructibleMapChunk::constructor()
{
	this->north_west_ = nullptr;
	this->north_east_ = nullptr;
	this->south_west_ = nullptr;
	this->south_east_ = nullptr;
	this->parent_ = nullptr;
	this->mesh_dirty_ = false;
	this->batch_info_ = nullptr;
	this->mergeable_count_ = false;

}

DestructibleMapChunk::DestructibleMapChunk(DestructibleMapChunk *parent, const glm::vec2 begin, const glm::vec2 end)
{
	constructor();
	assert(begin.x < end.x && begin.y < end.y);
	this->begin_ = begin;
	this->end_ = end;
	this->parent_ = parent;

	const glm::ivec2 quad_pos = glm::ivec2(this->begin_* SCALE_FACTOR);
	const glm::ivec2 quad_size = glm::ivec2((this->end_ - this->begin_) * SCALE_FACTOR);
	this->quad_ = make_rect(
		quad_pos,
		quad_size
	);

	this->vertices_.reserve(VERTICES_PER_CHUNK * 2);
}

DestructibleMapChunk::DestructibleMapChunk()
{
	constructor();
}

DestructibleMapChunk::~DestructibleMapChunk()
{
	if (this->batch_info_ != nullptr)
	{
		this->batch_info_->batch->dealloc_chunk(this);
	}

	if (this->north_west_)
	{
		delete this->north_west_;
		delete this->north_east_;
		delete this->south_west_;
		delete this->south_east_;
	}
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
		this->subdivide();

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

void DestructibleMapChunk::subdivide()
{
	const glm::vec2 size = (this->end_ - this->begin_) / 2.0f;
	assert(size.x >= 0 && size.y >= 0);
	const glm::vec2 size_x = glm::vec2(size.x, 0);
	const glm::vec2 size_y = glm::vec2(0, size.y);

	this->north_west_ = new DestructibleMapChunk(this, this->begin_, this->begin_ + size);
	this->north_east_ = new DestructibleMapChunk(this, this->begin_ + size_x, this->begin_ + size_x + size);
	this->south_west_ = new DestructibleMapChunk(this, this->begin_ + size_y, this->begin_ + size_y + size);
	this->south_east_ = new DestructibleMapChunk(this, this->begin_ + size, this->end_);

	DestructibleMapChunk *directions[] = {
		this->north_west_,
		this->north_east_,
		this->south_west_,
		this->south_east_
	};


#pragma omp parallel for
	for (auto i = 0; i < 4; i++)
	{
		directions[i]->apply_polygon(this->paths_);
	}

	this->paths_.clear();
	this->vertices_.clear();

	// do not need to merge
	if (this->mergeable_count_)
	{
		this->parent_->north_west_->mergeable_count_--;
		this->parent_->north_east_->mergeable_count_--;
		this->parent_->south_west_->mergeable_count_--;
		this->parent_->south_east_->mergeable_count_--;
		auto current = this->parent_;
		while (current)
		{
			current->mergeable_count_--;
			current = current->parent_;
		}
	}
}

void DestructibleMapChunk::merge()
{
	assert(this->north_west_);

	if (this->north_west_->paths_.size() + this->north_east_->paths_.size() + this->south_west_->paths_.size() + this->south_east_->paths_.size() > 0) {
		ClipperLib::PolyTree result_poly_tree;
		ClipperLib::Clipper c;
		c.StrictlySimple(true);
		c.AddPaths(this->north_west_->paths_, ClipperLib::ptSubject, true);
		c.AddPaths(this->north_east_->paths_, ClipperLib::ptSubject, true);
		c.AddPaths(this->south_west_->paths_, ClipperLib::ptSubject, true);
		c.AddPaths(this->south_east_->paths_, ClipperLib::ptSubject, true);

		if (!c.Execute(ClipperLib::ctUnion, result_poly_tree, ClipperLib::pftNonZero))
		{
			std::cout << "Could not create Polygon Tree" << std::endl;
		}
		
		ClipperLib::Paths result_paths;
		ClipperLib::PolyTreeToPaths(result_poly_tree, result_paths);

		this->set_paths(result_paths, result_poly_tree, true);
	}

	// remove children
	delete this->north_west_;
	delete this->north_east_;
	delete this->south_west_;
	delete this->south_east_;

	this->north_west_ = nullptr;
	this->north_east_ = nullptr;
	this->south_west_ = nullptr;
	this->south_east_ = nullptr;

	// decrease merge counter
	auto current = this;
	while (current)
	{
		current->mergeable_count_--;
		current = current->parent_;
	}

	assert(this->mergeable_count_ == 0);
}


void DestructibleMapChunk::apply_polygon(const ClipperLib::Paths &input_paths)
{
	ClipperLib::PolyTree result_poly_tree;
	ClipperLib::Clipper c;
	c.StrictlySimple(true);
	c.AddPaths(input_paths, ClipperLib::ptSubject, true);
	c.AddPath(this->quad_, ClipperLib::ptClip, true);

	if (!c.Execute(ClipperLib::ctIntersection, result_poly_tree, ClipperLib::pftNonZero))
	{
		std::cout << "Could not create Polygon Tree" << std::endl;
	}

	if (result_poly_tree.Total() == 0)
	{
		return;
	}

	ClipperLib::Paths result_paths;
	ClipperLib::PolyTreeToPaths(result_poly_tree, result_paths);


	if (this->north_west_)
	{
		this->north_west_->apply_polygon(result_paths);
		this->north_east_->apply_polygon(result_paths);
		this->south_west_->apply_polygon(result_paths);
		this->south_east_->apply_polygon(result_paths);
	}
	else
	{
		this->set_paths(result_paths, result_poly_tree, false);
	}
}

void DestructibleMapChunk::query_range(const glm::vec2& query_begin, const glm::vec2& query_end, std::vector<DestructibleMapChunk*> &leaves)
{
	assert(query_begin.x < query_end.x && query_begin.y < query_end.y);
	assert(this->begin_.x < this->end_.x && this->begin_.y < this->end_.y);

	if (query_end.x < this->begin_.x || query_end.y < this->begin_.y || query_begin.x > this->end_.x || query_begin.y > this->end_.y)
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

void DestructibleMapChunk::set_paths(const ClipperLib::Paths &paths, const ClipperLib::PolyTree &poly_tree, bool fast)
{
	this->paths_ = paths;
	this->vertices_.clear();
#ifdef ENABLE_MERGING_SUBDIVIDING
	if (fast)
	{
		triangulate_fast(poly_tree, this->vertices_);
	} else
	{
		triangulate(poly_tree, this->vertices_);
	}
#else
	triangulate(poly_tree, this->vertices_);
#endif

	auto current = this;
	while (current && !current->mesh_dirty_)
	{
		current->mesh_dirty_ = true;
		current = current->parent_;
	}
}

void DestructibleMapChunk::query_dirty(std::vector<DestructibleMapChunk*>& dirty_chunks)
{
	if (!this->mesh_dirty_)
	{
		return;
	}
	this->mesh_dirty_ = false;

	if (this->north_west_)
	{
		this->north_west_->query_dirty(dirty_chunks);
		this->north_east_->query_dirty(dirty_chunks);
		this->south_west_->query_dirty(dirty_chunks);
		this->south_east_->query_dirty(dirty_chunks);
	}
	else 
	{
		dirty_chunks.push_back(this);
	}
}

void DestructibleMapChunk::update_batch(BatchInfo* info)
{
	this->batch_info_ = info;
}

DestructibleMapChunk* DestructibleMapChunk::get_best_mergeable() const
{
	if (this->mergeable_count_ == 0)
	{
		return nullptr;
	}

	if (this->north_west_)
	{
		DestructibleMapChunk *directions[] = {
			this->north_west_,
			this->north_east_,
			this->south_west_,
			this->south_east_
		};
		
		auto best_match = 0;
		DestructibleMapChunk *selected = nullptr;
		for (auto i = 0; i < 4; i++)
		{
			auto &dir = directions[i];
			if (best_match < dir->mergeable_count_)
			{
				best_match = dir->mergeable_count_;
				selected = dir;
			}
		}
		assert(selected != nullptr);

		return selected->get_best_mergeable();
	}
	else
	{
		return this->parent_;
	}
}

DestructibleMapChunk* DestructibleMapChunk::query_chunk(glm::vec2 point)
{
	if (point.x < this->begin_.x || point.y < this->begin_.y || point.x > this->end_.x || point.y > this->end_.y)
	{
		return nullptr;
	}

	if (this->north_west_)
	{
		DestructibleMapChunk *chunk;
		if ((chunk = this->north_west_->query_chunk(point)))
		{
			return chunk;
		}
		if ((chunk = this->north_east_->query_chunk(point)))
		{
			return chunk;
		}
		if ((chunk = this->south_west_->query_chunk(point)))
		{
			return chunk;
		}
		if ((chunk = this->south_east_->query_chunk(point)))
		{
			return chunk;
		}

		return nullptr;
	}
	else
	{
		return this;
	}
}
