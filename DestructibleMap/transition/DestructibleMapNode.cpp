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

	std::vector<glm::vec2> vertices;
	std::vector<glm::vec2> points;

	std::random_device seeder;
	std::mt19937 engine(seeder());
	std::uniform_real_distribution<float> u_dist(0.0, 1.0);

	const auto max_float = std::numeric_limits<float>::max();
	glm::vec2 boundary_begin = glm::vec2(max_float, max_float);
	glm::vec2 boundary_end = glm::vec2(-max_float, -max_float);

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

					include_point_in_begin_boundary(boundary_begin, v0);
					include_point_in_begin_boundary(boundary_begin, v1);
					include_point_in_begin_boundary(boundary_begin, v2);

					include_point_in_end_boundary(boundary_end, v0);
					include_point_in_end_boundary(boundary_end, v1);
					include_point_in_end_boundary(boundary_end, v2);


					vertices.push_back(v0);
					vertices.push_back(v1);
					vertices.push_back(v2);

					points.push_back(v0);
					points.push_back(v1);
					points.push_back(v2);
					const auto center = (v0 + v1 + v2) * float(1.0 / 3.0);
					points.push_back(center);


					// get density distribution
					const auto area = int(triangle_area(v0.x, v0.y, v1.x, v1.y, v2.x, v2.y)/10.0);
					std::cout << "Area: " << area << std::endl;


					for (auto distr = 0; distr < area; distr++) {
						const float u = u_dist(engine);
						const std::uniform_real_distribution<double> v_dist(0.0, 1.0 - u);
						const float v = v_dist(engine);

						const auto point = u*v0 + v*v1 + (1 - u - v)*v2;
						points.push_back(point);
					}
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


	std::cout << "Vertices: " << std::endl;
	for (auto &vertex : vertices)
	{
		std::cout << vertex.x << " " << vertex.y << std::endl;
	}

	// shuffle points to avoid degenerate quadtree
	std::random_shuffle(points.begin(), points.end());

	Quadtree quad_tree = Quadtree(10, boundary_begin, boundary_end);

	for (auto& point : points)
	{
		quad_tree.insert(point);
	}

	quad_tree.get_lines(this->lines_);
	this->vertices_ = vertices;
	this->points_ = points;

	const auto lines_vertices = new float[lines_.size() * 3];
	const auto lines_normal = new float[lines_.size() * 3];
	const auto lines_uv = new float[lines_.size() * 2];
	auto i = 0;
	for (auto& line : lines_)
	{
		lines_uv[i * 2] = 0;
		lines_uv[i * 2 + 1] = 0;

		lines_normal[i * 3] = 0;
		lines_normal[i * 3 + 1] = 0;
		lines_normal[i * 3 + 2] = 0;


		lines_vertices[i * 3] = line.x;
		lines_vertices[i * 3 + 1] = line.y;
		lines_vertices[i * 3 + 2] = 0;

		i++;
	}
	Material mat;
	mat.set_diffuse_color(glm::vec3(0.5, 0.5, 0.5));
	mat.set_ambient_color(glm::vec3(1.0, 0.1, 0.1));
	this->quadtree_resource_ = new MeshResource(lines_vertices, lines_normal, lines_uv, lines_.size(), nullptr, 0, mat);

	const auto points_vertices = new float[points.size() * 3];
	const auto points_normal = new float[points.size() * 3];
	const auto points_uv = new float[points.size() * 2];
	i = 0;
	for (auto& point : points)
	{
		points_uv[i * 2] = 0;
		points_uv[i * 2 + 1] = 0;

		points_normal[i * 3] = 0;
		points_normal[i * 3 + 1] = 0;
		points_normal[i * 3 + 2] = 0;


		points_vertices[i * 3] = point.x;
		points_vertices[i * 3 + 1] = point.y;
		points_vertices[i * 3 + 2] = 0;

		i++;
	}
	mat.set_diffuse_color(glm::vec3(0.5, 0.5, 0.5));
	mat.set_ambient_color(glm::vec3(1.0, 0.1, 0.1));
	this->point_distribution_resource_ = new MeshResource(points_vertices, points_normal, points_uv, points.size(), nullptr, 0, mat);

	const auto global_vertices = new float[vertices.size() * 3];
	const auto global_normal = new float[vertices.size() * 3];
	const auto global_uv = new float[vertices.size() * 2];
	i = 0;
	for (auto& vertex : vertices)
	{
		global_uv[i*2] = 0;
		global_uv[i*2 + 1] = 0;

		global_normal[i*3] = 0;
		global_normal[i*3 + 1] = 0;
		global_normal[i*3 + 2] = 0;


		global_vertices[i*3] = vertex.x;
		global_vertices[i*3 + 1] = vertex.y;
		global_vertices[i*3 + 2] = 0;

		i++;
	}
	mat.set_diffuse_color(glm::vec3(0.5, 0.5, 0.5));
	mat.set_ambient_color(glm::vec3(0.0, 1.0, 0.1));
	this->total_map_resource_ = new MeshResource(global_vertices, global_normal, global_uv, vertices.size(), nullptr, 0, mat);
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
	glDrawArrays(GL_POINTS, 0, this->points_.size());
	glBindVertexArray(0);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	point_display = false;
	shader->set_model_uniforms(this, &point_display);
	glBindVertexArray(this->total_map_resource_->get_resource_id());
	glDrawArrays(GL_TRIANGLES, 0, this->vertices_.size());
	glBindVertexArray(0);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}
