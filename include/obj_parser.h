#pragma once

#include <iostream>
#include <map>
#include <vector>
#include <algorithm>
#include <stdio.h>
#include <windows.h>
#include <wingdi.h>


#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

using namespace std;
using namespace glm;

class vertex {
public:
	glm::vec3 pos;
	glm::vec4 col;
	glm::vec3 nor;
	glm::vec2 tex;
};

int obj_parse(const char* filename, std::vector<vertex>* io_vertices,
	float scale, const char* obj_folder, std::vector<tinyobj::shape_t>* io_shapes,
	std::vector<tinyobj::material_t>* io_materials, std::vector<int>* io_material_id) {
	printf("Parsing \"%s\"...\n", filename);

	tinyobj::attrib_t attrib;
	std::string warn, err;

	if (!tinyobj::LoadObj(&attrib, io_shapes, io_materials, &warn, &err, filename, obj_folder)) {
		throw std::runtime_error(warn + err);
	}

	bool has_mtl = !io_materials->empty();

	size_t tex_count = 0;
	for (const auto& mtl : *io_materials) {
		if (!mtl.diffuse_texname.empty()) {
			tex_count++;
		}
	}

	std::vector<uint32_t> indices;

	float max_vert = *std::max_element(attrib.vertices.begin(), attrib.vertices.end());

	for (const auto& shape : *io_shapes) {
		size_t i_offset = 0;

		for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
			const int fv = shape.mesh.num_face_vertices[f];
			const int mtl_id = shape.mesh.material_ids[f];

			// Check if obj has normals (because some may not for some reason)
			// If no normals, create them
			glm::vec3 new_nor(0.0f);
			if (attrib.normals.empty() && fv >= 3) {
				tinyobj::index_t index_0 = shape.mesh.indices[i_offset + 0];
				tinyobj::index_t index_1 = shape.mesh.indices[i_offset + 1];
				tinyobj::index_t index_2 = shape.mesh.indices[i_offset + 2];

				glm::vec3 pos_0 = {
					attrib.vertices[3 * index_0.vertex_index + 0],
					attrib.vertices[3 * index_0.vertex_index + 1],
					attrib.vertices[3 * index_0.vertex_index + 2]
				};
				glm::vec3 pos_1 = {
					attrib.vertices[3 * index_1.vertex_index + 0],
					attrib.vertices[3 * index_1.vertex_index + 1],
					attrib.vertices[3 * index_1.vertex_index + 2]
				};
				glm::vec3 pos_2 = {
					attrib.vertices[3 * index_2.vertex_index + 0],
					attrib.vertices[3 * index_2.vertex_index + 1],
					attrib.vertices[3 * index_2.vertex_index + 2]
				};

				// Calculate face normal
				glm::vec3 edge_1 = pos_1 - pos_0;
				glm::vec3 edge_2 = pos_2 - pos_0;
				new_nor = glm::normalize(glm::cross(edge_1, edge_2));
			}

			for (int v = 0; v < fv; v++) {
				vertex vert{};
				const auto& index = shape.mesh.indices[i_offset + v];
				int i = indices.size();

				vert.pos =
				{
					(attrib.vertices[3 * index.vertex_index + 0] / max_vert) * scale,
					(attrib.vertices[3 * index.vertex_index + 1] / max_vert) * scale,
					(attrib.vertices[3 * index.vertex_index + 2] / max_vert) * scale
				};

				if (!attrib.normals.empty()) {
					vert.nor =
					{
						attrib.normals[3 * index.normal_index + 0],
						attrib.normals[3 * index.normal_index + 1],
						attrib.normals[3 * index.normal_index + 2]
					};
				}
				else
					vert.nor = new_nor;

				if (index.texcoord_index >= 0) {
					vert.tex = {
						attrib.texcoords[2 * index.texcoord_index + 0],
						attrib.texcoords[2 * index.texcoord_index + 1]
					};
				}

				if (has_mtl && mtl_id >= 0) {
					const auto& mtl = (*io_materials)[mtl_id];

					if (!mtl.diffuse_texname.empty())
						vert.col = { 1, 1, 1, mtl.dissolve };
					else {
						vert.col = {
							mtl.diffuse[0],
							mtl.diffuse[1],
							mtl.diffuse[2],
							mtl.dissolve
						};
					}

				}
				else {
					vert.col = { 1, 1, 1, 1.f };
				}

				io_vertices->push_back(vert);
				indices.push_back(i);
			}

			io_material_id->push_back(mtl_id);
			i_offset += fv;
		}
	}

	printf("Successfully parsed \"%s\" and read %zu vertices\n", filename, io_vertices->size());

	return 0;
}