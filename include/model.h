#pragma once

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "obj_parser.h"
#define STB_IMAGE_IMPLEMENTATION
#include "texture.h"

class model {
private:
	GLuint VBO, VAO;
	std::vector<vertex> vertices;
	glm::mat4 modelMat = glm::mat4(1.f);
	bool has_textures = false;
	std::vector<tinyobj::shape_t> shapes;
	std::map<int, GLuint> textures;
	GLuint defaultTexture;
	std::vector<int> material_id;
	std::vector<tinyobj::material_t> materials;

public:
	// Constructor for parsed obj models
	model(const std::string obj_path, std::string obj_folder) {
		glGenTextures(1, &defaultTexture);
		glBindTexture(GL_TEXTURE_2D, defaultTexture);
		unsigned char white[] = { 255, 255, 255, 255 };
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, white);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glBindTexture(GL_TEXTURE_2D, 0);

		obj_parse(obj_path.c_str(), &vertices, 1.f, obj_folder.c_str(), &shapes, &materials, &material_id);

		for (size_t i = 0; i < materials.size(); i++) {
			const auto& mtl = materials[i];

			if (!mtl.diffuse_texname.empty()) {
				std::string tex_path = obj_folder + materials[i].diffuse_texname;
				std::cout << "Loading texture: " << tex_path << std::endl;
				GLuint texture = setup_texture(tex_path.c_str());
				if (texture != 0) {
					textures[i] = texture;
					has_textures = true;
				}
			}
		}

		glCreateBuffers(1, &VBO);
		glNamedBufferStorage(VBO, vertices.size() * sizeof(vertex), vertices.data(), 0);
		glGenVertexArrays(1, &VAO);
		glBindVertexArray(VAO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)offsetof(vertex, pos));
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)offsetof(vertex, col));
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)offsetof(vertex, nor));
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)offsetof(vertex, tex));
		glEnableVertexAttribArray(3);

		glBindVertexArray(0);
	}

	// Constructor for procedurally generated models
	model(const std::vector<vertex>& custom_vertices) {
		vertices = custom_vertices;

		glGenTextures(1, &defaultTexture);
		glBindTexture(GL_TEXTURE_2D, defaultTexture);
		unsigned char white[] = { 255, 255, 255, 255 };
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, white);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glBindTexture(GL_TEXTURE_2D, 0);

		glCreateBuffers(1, &VBO);
		glNamedBufferStorage(VBO, vertices.size() * sizeof(vertex), vertices.data(), 0);
		glGenVertexArrays(1, &VAO);
		glBindVertexArray(VAO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)offsetof(vertex, pos));
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)offsetof(vertex, col));
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)offsetof(vertex, nor));
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)offsetof(vertex, tex));
		glEnableVertexAttribArray(3);

		glBindVertexArray(0);
	}

	// Destructor
	~model() {
		glDeleteTextures(1, &defaultTexture);
		glDeleteBuffers(1, &VBO);
		glDeleteVertexArrays(1, &VAO);
	}

	// Draw function
	void draw(unsigned int shaderProgram) {
		glUseProgram(shaderProgram);
		glBindVertexArray(VAO);
		glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(modelMat));

		if (shapes.empty()) {
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, defaultTexture);
			glUniform1i(glGetUniformLocation(shaderProgram, "Texture"), 0);
			glDrawArrays(GL_TRIANGLES, 0, vertices.size());
			return;
		}

		// Cache location to reduce lag
		GLint texLocCache = glGetUniformLocation(shaderProgram, "Texture");

		// Render opaque, then transparent
		for (int layer = 0; layer < 2; layer++) {
			bool transparent_layer = (layer == 1);

			size_t f_index = 0;
			size_t i_offset = 0;
			for (const auto& shape : shapes) {
				int current_mtl = -1;
				size_t batch_count = 0;

				for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
					int mtl_id = material_id[f_index];
					int fv = shape.mesh.num_face_vertices[f];

					// If material is transparent but not rendering transparent layer yet, skip
					bool transparent_mtl = materials[mtl_id].dissolve < 1.0f;
					if (transparent_layer != transparent_mtl) {
						f_index++;
						i_offset += fv;
						continue;
					}

					// Call only if mtl_id changes to reduce lag
					if (mtl_id != current_mtl) {
						// Draw previous batch
						if (batch_count > 0)
							glDrawArrays(GL_TRIANGLES, i_offset - batch_count, batch_count);

						current_mtl = mtl_id;

						if (textures.find(mtl_id) != textures.end()) {
							glActiveTexture(GL_TEXTURE0);
							glBindTexture(GL_TEXTURE_2D, textures[mtl_id]);
						}
						else {
							glActiveTexture(GL_TEXTURE0);
							glBindTexture(GL_TEXTURE_2D, defaultTexture);
						}
						glUniform1i(texLocCache, 0);

						batch_count = 0;
					}

					batch_count += fv;
					i_offset += fv;
					f_index++;
				}

				// Draw leftover batch
				if (batch_count > 0)
					glDrawArrays(GL_TRIANGLES, i_offset - batch_count, batch_count);
			}
		}
	}

	// Transformations
	void translate(glm::vec3 translation) {
		modelMat = glm::translate(modelMat, translation);
	}

	void rotate(float angle_rad, glm::vec3 axis) {
		modelMat = glm::rotate(modelMat, angle_rad, axis);
	}

	// Non-uniform scaling
	void scale(glm::vec3 factors) {
		modelMat = glm::scale(modelMat, factors);
	}

	// Uniform scaling
	void scale(float factor) {
		scale(glm::vec3(factor));
	}
};