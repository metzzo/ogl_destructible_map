#pragma once
#include "clipper.hpp"
#include "TransformationNode.h"
#include "IDrawable.h"
#include "MeshResource.h"

class Quadtree
{
	int max_points_;
	std::vector<glm::vec2> points_;
	glm::vec2 begin_;
	glm::vec2 end_;

	Quadtree *north_west_;
	Quadtree *north_east_;
	Quadtree *south_west_;
	Quadtree *south_east_;
public:
	explicit Quadtree(const int max_points, const glm::vec2 begin, const glm::vec2 end)
	{
		this->max_points_ = max_points;
		this->begin_ = begin;
		this->end_ = end;
		this->north_west_ = nullptr;
		this->north_east_ = nullptr;
		this->south_west_ = nullptr;
		this->south_east_ = nullptr;
	}

	void get_lines(std::vector<glm::vec2>& lines) const
	{
		if (this->north_west_)
		{
			this->north_west_->get_lines(lines);
			this->north_east_->get_lines(lines);
			this->south_west_->get_lines(lines);
			this->south_east_->get_lines(lines);
		} else
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

	bool insert(const glm::vec2& point)
	{
		if (point.x < this->begin_.x || point.y < this->begin_.y || point.x >= this->end_.x || point.y >= this->end_.y)
		{
			return false;
		}

		if (this->points_.size() < max_points_)
		{
			this->points_.push_back(point);
			return true;
		}

		if (this->north_west_ == nullptr)
		{
			const glm::vec2 size = (this->end_ - this->begin_) / float(2.0);
			const glm::vec2 size_x = glm::vec2(size.x, 0);
			const glm::vec2 size_y = glm::vec2(0, size.y);

			this->north_west_ = new Quadtree(this->max_points_, this->begin_, this->begin_ + size);
			this->north_east_ = new Quadtree(this->max_points_, this->begin_ + size_x, this->begin_ + size_x + size);
			this->south_west_ = new Quadtree(this->max_points_, this->begin_ + size_y, this->begin_ + size_y + size);
			this->south_east_ = new Quadtree(this->max_points_, this->begin_ + size, this->end_);

			for (auto &old_points : this->points_)
			{
				this->insert(old_points);
			}
			this->points_.clear();
		}

		if (this->north_west_->insert(point))
		{
			return true;
		}
		if (this->north_east_->insert(point))
		{
			return true;
		}
		if (this->south_west_->insert(point))
		{
			return true;
		}
		if (this->south_east_->insert(point))
		{
			return true;
		}
		return false;
	}
};

class MeshResource;
class DestructibleMapNode :
	public TransformationNode,
	public IDrawable
{
	glm::mat4 trafo_;
	glm::mat4 itrafo_;
	std::vector<glm::vec2> vertices_;
	std::vector<glm::vec2> points_;
	std::vector<glm::vec2> lines_;

	void load(ClipperLib::Paths poly_tree);
public:
	MeshResource* total_map_resource_;
	MeshResource* point_distribution_resource_;
	MeshResource* quadtree_resource_;

	explicit DestructibleMapNode(const std::string& name);
	~DestructibleMapNode();

	void load_from_svg(const std::string& path);
	void load_sample();

	void init(RenderingEngine* rendering_engine) override;

	std::vector<IDrawable*> get_drawables() override;
	
	void draw(ShaderResource* shader) const override;
};

