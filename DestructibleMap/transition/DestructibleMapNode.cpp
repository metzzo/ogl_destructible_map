#include "DestructibleMapNode.h"
#include "clipper.hpp"
#include <iostream>
#include "poly2tri/sweep/cdt.h"

// TODO: scaling to avoid clipping issues

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
					for (auto i = 0; i < 3; i++)
					{
						const auto p = triangle->GetPoint(i);
						vertices.push_back(glm::vec2(float(p->x), float(p->y)));
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
}

DestructibleMapNode::DestructibleMapNode(const std::string& name) : Node(name)
{
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
		<< make_rect(0, 0, 100, 20)
		<< make_rect(20, 20, 100, 20)
		<< make_rect(200, 200, 0, 20)
		<< make_rect(0, 0, 100, 20)
		<< make_rect(0, 400, 100, 200);

	this->load(paths);
}


void DestructibleMapNode::init(RenderingEngine* rendering_engine)
{
}

void DestructibleMapNode::apply_transformation(const glm::mat4& transformation, const glm::mat4& inverse_transformation)
{
	this->trafo_ = transformation;
	this->itrafo_ = inverse_transformation;
}
