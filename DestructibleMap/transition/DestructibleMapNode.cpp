#include "DestructibleMapNode.h"
#include "clipper.hpp"
#include <iostream>
#include "poly2tri/sweep/cdt.h"
#include <glad/glad.h>
#include "ShaderResource.h"
#include "MeshResource.h"
#include <random>
#include <limits>

#define SCALE_FACTOR (1000.0)
#define SCALE_FACTOR_INV (1.0/SCALE_FACTOR)

// TODO: scaling to avoid clipping issues

float triangle_area(const float d_x0, const float d_y0, const float d_x1, const float d_y1, const float d_x2, const float d_y2)
{
	return abs(((d_x1 - d_x0)*(d_y2 - d_y0) - (d_x2 - d_x0)*(d_y1 - d_y0)) / 2.0);
}


ClipperLib::Path make_rect(const int x, const int y, const int w, const int h)
{
	ClipperLib::Path rect;
	rect << ClipperLib::IntPoint(x, y) << ClipperLib::IntPoint(x + w, y) << ClipperLib::IntPoint(x + w, y + h) << ClipperLib::IntPoint(x, y + h);
	return rect;
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

void generate_point_cloud(const std::vector<glm::vec2> &vertices, std::vector<glm::vec2> &points)
{
	std::random_device seeder;
	std::mt19937 engine(seeder());
	std::uniform_real_distribution<float> u_dist(0.0, 1.0);

	points.clear();
	for (auto i = 0; i < vertices.size(); i += 3)
	{
		const auto& v0 = vertices[i];
		const auto& v1 = vertices[i + 1];
		const auto& v2 = vertices[i + 2];

		points.push_back(v0);
		points.push_back(v1);
		points.push_back(v2);
		const auto center = (v0 + v1 + v2) * float(1.0 / 3.0);
		points.push_back(center);


		// get density distribution
		const auto area = int(triangle_area(v0.x, v0.y, v1.x, v1.y, v2.x, v2.y) / 10.0);
		std::cout << "Area: " << area << std::endl;


		for (auto distr = 0; distr < area; distr++) {
			const float u = u_dist(engine);
			const std::uniform_real_distribution<double> v_dist(0.0, 1.0 - u);
			const float v = v_dist(engine);

			const auto point = u*v0 + v*v1 + (1 - u - v)*v2;
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
		if (!current_node->IsHole())
		{
			if (current_node->Contour.size() >= 3)
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
					if (!child_node->Childs.size())
					{
						std::cout << "Expected only Empty Nodes." << std::endl;
						continue;
					}

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

					const auto v0 = glm::vec2(float(p0->x), float(p0->y));
					const auto v1 = glm::vec2(float(p1->x), float(p1->y));
					const auto v2 = glm::vec2(float(p2->x), float(p2->y));

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

void DestructibleMapNode::load(ClipperLib::Paths paths)
{
	ClipperLib::PolyTree poly_tree;
	ClipperLib::Clipper c;
	c.StrictlySimple(true);
	c.AddPaths(paths, ClipperLib::ptSubject, true);
	if (!c.Execute(ClipperLib::ctUnion, poly_tree, ClipperLib::pftNonZero))
	{
		std::cout << "Could not create Polygon Tree" << std::endl;
	}

	if (poly_tree.Total() == 0)
	{
		return;
	}

	const auto max_float = std::numeric_limits<float>::max();
	glm::vec2 boundary_begin = glm::vec2(max_float, max_float);
	glm::vec2 boundary_end = glm::vec2(-max_float, -max_float);

	triangulate(poly_tree, this->vertices_);
	generate_point_cloud(this->vertices_, this->points_);
	generate_aabb(this->vertices_, boundary_begin, boundary_end);

	std::cout << "Vertices: " << std::endl;
	for (auto &vertex : this->vertices_)
	{
		std::cout << vertex.x << " " << vertex.y << std::endl;
	}

	this->quad_tree_ = Quadtree(nullptr, 5, boundary_begin, boundary_end);

	for (auto& point : this->points_)
	{
		this->quad_tree_.insert(point);
	}

	this->quad_tree_.apply_polygon(paths);

	Material mat;
	mat.set_diffuse_color(glm::vec3(0.5, 0.5, 0.5));
	mat.set_ambient_color(glm::vec3(1.0, 0.1, 0.1));
	this->point_distribution_resource_ = new MeshResource(this->points_, mat);

	mat.set_diffuse_color(glm::vec3(0.5, 0.5, 0.5));
	mat.set_ambient_color(glm::vec3(0.0, 1.0, 0.1));
	this->total_map_resource_ = new MeshResource(this->vertices_, mat);
}

std::vector<IDrawable*> DestructibleMapNode::get_drawables()
{
	return{ this };
}

DestructibleMapNode::DestructibleMapNode(const std::string& name) : TransformationNode(name)
{
	this->total_map_resource_ = nullptr;
	this->point_distribution_resource_ = nullptr;
	this->quadtree_resource_ = nullptr;
}


DestructibleMapNode::~DestructibleMapNode()
{
}

void DestructibleMapNode::load_from_svg(const std::string& path)
{
	ClipperLib::Paths paths;

	// TODO

	load(paths);
}

void DestructibleMapNode::load_sample()
{
	ClipperLib::Paths paths(5);

	paths
		<< make_rect(0, 0, 20, 20)
		<< make_rect(10, 10, 20, 20)
		<< make_rect(80, 90, 10, 20)
		<< make_rect(50, 0, 10, 20)
		<< make_rect(0, 40, 10, 20);

	this->load(paths);
}


void DestructibleMapNode::init(RenderingEngine* rendering_engine)
{
	this->total_map_resource_->init();
	this->point_distribution_resource_->init();
	this->quadtree_resource_->init();
	this->quad_tree_.init(rendering_engine);
}

void DestructibleMapNode::draw(ShaderResource* shader) const
{
	bool point_display;

	const auto trafo = this->get_transformation();

	point_display = true;
	shader->set_model_uniforms(this, &point_display);

	glBindVertexArray(this->quadtree_resource_->get_resource_id());
	glDrawArrays(GL_LINES, 0, this->lines_.size());
	glBindVertexArray(0);

	glPointSize(8);
	glBindVertexArray(this->point_distribution_resource_->get_resource_id());
	//glDrawArrays(GL_POINTS, 0, this->points_.size());
	glBindVertexArray(0);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	point_display = false;
	shader->set_model_uniforms(this, &point_display);
	glBindVertexArray(this->total_map_resource_->get_resource_id());
	//glDrawArrays(GL_TRIANGLES, 0, this->vertices_.size());
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	this->quad_tree_.draw();
	glBindVertexArray(0);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void DestructibleMapNode::remove_rect(const glm::vec2& begin, const glm::vec2& end)
{
	assert(begin.x < end.x && begin.y < end.y);
	std::vector<Quadtree*> affected_leaves;
	this->quad_tree_.query_range(begin, end, affected_leaves);

	const auto size = end - begin;
	const auto rect_path = make_rect(int(begin.x), int(begin.y), int(size.x), int(size.y));

	for (auto &leave : affected_leaves)
	{
		if (leave->begin_.x >= begin.x && leave->begin_.y >= begin.y && leave->end_.x <= end.x && leave->end_.y <= end.y)
		{
			// removing this quadtree entirely is sufficient
			leave->remove();
		}
		else {
			ClipperLib::PolyTree result_poly_tree;
			ClipperLib::Clipper c;
			c.StrictlySimple(true);
			c.AddPaths(leave->paths_, ClipperLib::ptSubject, true);
			c.AddPath(rect_path, ClipperLib::ptClip, true);
			if (!c.Execute(ClipperLib::ctDifference, result_poly_tree, ClipperLib::pftNonZero))
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
	this->update_quadtree_representation();
}

void DestructibleMapNode::update_quadtree_representation()
{
	this->lines_.clear();
	this->quad_tree_.get_lines(this->lines_);

	Material mat;
	mat.set_diffuse_color(glm::vec3(0.5, 0.5, 0.5));
	mat.set_ambient_color(glm::vec3(1.0, 0.1, 0.1));
	// TODO: update or delete old quadtree resource
	this->quadtree_resource_ = new MeshResource(this->lines_, mat);
}


Quadtree::Quadtree(Quadtree *parent, const int max_points, const glm::vec2 begin, const glm::vec2 end)
{
	assert(begin.x < end.x && begin.y < end.y);
	this->max_points_ = max_points;
	this->begin_ = begin;
	this->end_ = end;
	this->north_west_ = nullptr;
	this->north_east_ = nullptr;
	this->south_west_ = nullptr;
	this->south_east_ = nullptr;
	this->mesh_ = nullptr;
	this->parent_ = parent;
}

Quadtree::Quadtree()
{
	this->max_points_ = 0;
	this->north_west_ = nullptr;
	this->north_east_ = nullptr;
	this->south_west_ = nullptr;
	this->south_east_ = nullptr;
	this->mesh_ = nullptr;
	this->parent_ = nullptr;
}

Quadtree::~Quadtree()
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

void Quadtree::get_lines(std::vector<glm::vec2>& lines) const
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

bool Quadtree::insert(const glm::vec2& point)
{
	if (point.x < this->begin_.x || point.y < this->begin_.y || point.x > this->end_.x || point.y > this->end_.y)
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

		this->north_west_ = new Quadtree(this, this->max_points_, this->begin_, this->begin_ + size);
		this->north_east_ = new Quadtree(this, this->max_points_, this->begin_ + size_x, this->begin_ + size_x + size);
		this->south_west_ = new Quadtree(this, this->max_points_, this->begin_ + size_y, this->begin_ + size_y + size);
		this->south_east_ = new Quadtree(this, this->max_points_, this->begin_ + size, this->end_);

		for (auto& old_points : this->points_)
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

void Quadtree::apply_polygon(const ClipperLib::Paths &input_paths)
{
	const glm::ivec2 quad_pos = glm::ivec2(this->begin_);
	const glm::ivec2 quad_size = glm::ivec2(this->end_ - this->begin_);
	const ClipperLib::Path quad = make_rect(quad_pos.x, quad_pos.y, quad_size.x + 1, quad_size.y + 1);
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

	ClipperLib::Paths result_paths;
	ClipperLib::PolyTreeToPaths(result_poly_tree, result_paths);

	if (this->north_west_)
	{
		this->north_west_->apply_polygon(result_paths);
		this->north_east_->apply_polygon(result_paths);
		this->south_west_->apply_polygon(result_paths);
		this->south_east_->apply_polygon(result_paths);
	} else
	{
		this->set_paths(result_paths, result_poly_tree);
	}
}

void Quadtree::query_range(const glm::vec2& query_begin, const glm::vec2& query_end, std::vector<Quadtree*> &leaves)
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
	} else
	{
		leaves.push_back(this);
	}
}

void Quadtree::set_paths(const ClipperLib::Paths &paths, const ClipperLib::PolyTree &poly_tree)
{
	this->paths_ = paths;
	this->vertices_.clear();
	triangulate(poly_tree, this->vertices_);

	Material mat;
	mat.set_diffuse_color(glm::vec3(0.5, 0.5, 0.5));
	mat.set_ambient_color(glm::vec3(0.0, 1.0, 0.1));
	// TODO: reuse old mesh
	this->mesh_ = new MeshResource(this->vertices_, mat);
}

void Quadtree::simplify()
{
	// TODO
}

void Quadtree::draw() const
{
	if (this->north_west_)
	{
		this->north_west_->draw();
		this->north_east_->draw();
		this->south_west_->draw();
		this->south_east_->draw();
	}
	else if (this->mesh_ != nullptr)
	{
		glBindVertexArray(this->mesh_->get_resource_id());
		glDrawArrays(GL_TRIANGLES, 0, this->vertices_.size());
	}
}

void Quadtree::init(RenderingEngine* engine)
{
	if (this->north_west_)
	{
		this->north_west_->init(engine);
		this->north_east_->init(engine);
		this->south_west_->init(engine);
		this->south_east_->init(engine);
	} else if (this->mesh_ != nullptr)
	{
		this->mesh_->init();
	}
}

void Quadtree::remove()
{
	// TODO: proper removing
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
