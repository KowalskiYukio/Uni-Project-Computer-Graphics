#include <GL/gl3w.h>

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <unordered_map>

#include "camera.h"
#include "error.h"
#include "file.h"
#include "shader.h"
#include "obj_parser.h"
#include "model.h"
#include "shadow.h"

#define STB_IMAGE_IMPLEMENTATION
#include "texture.h"


// Initial light properties
glm::vec3 lightDirection = glm::vec3(0.1f, -.81f, -.61f);
glm::vec3 lightPos = glm::vec3(2.f, 6.f, 7.f);

SCamera Camera;

#define WIDTH 1920
#define HEIGHT 1080
#define SH_MAP_WIDTH 20480
#define SH_MAP_HEIGHT 20480

std::vector<vertex> loadFloor() {
	std::vector<vertex> vertices;
	const float size = 0.5f;
	vertex v;
	v.col = glm::vec4(1.f, 1.f, 1.f, 1.0f);
	v.nor = glm::vec3(0.0f, 1.0f, 0.0f);
	v.tex = glm::vec2(0.0f, 0.0f);

	// Triangle 1
	v.pos = glm::vec3(-size, -size, -size); vertices.push_back(v);
	v.pos = glm::vec3(size, -size, size); vertices.push_back(v);
	v.pos = glm::vec3(size, -size, -size); vertices.push_back(v);

	// Triangle 2
	v.pos = glm::vec3(-size, -size, -size); vertices.push_back(v);
	v.pos = glm::vec3(-size, -size, size); vertices.push_back(v);
	v.pos = glm::vec3(size, -size, size); vertices.push_back(v);

	return vertices;
}

void processKeyboard(GLFWwindow* window) {
	// Quit
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	// Light repositioning
	if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
		lightDirection = Camera.Front;
		lightPos = Camera.Position;
	}

	float z_move = 0.f;
	float x_move = 0.f;
	float y_move = 0.f;
	bool cam_changed = false;

	// "Player" Direction
	// Forwards
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
		z_move = 1.f;
		cam_changed = true;
	}
	// Backwards
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
		z_move = -1.f;
		cam_changed = true;
	}
	// Right
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
		x_move = 1.f;
		cam_changed = true;
	}
	// Left
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
		x_move = -1.f;
		cam_changed = true;
	}
	// Up
	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
		y_move = 1.f;
		cam_changed = true;
	}
	// Down
	if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) {
		y_move = -1.f;
		cam_changed = true;
	}

	// Speed
	if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
		Camera.MovementSpeed = 0.05f;
	}

	if (cam_changed) {
		MoveAndOrientCamera(Camera, glm::vec3(0.f, 0.f, 0.f), cam_dist, 0, 0, z_move, x_move, y_move);
		Camera.MovementSpeed = 0.005f;
	}
}

void MouseCallback(GLFWwindow* window, double xpos, double ypos) {
	static double lastX = WIDTH / 2;
	static double lastY = HEIGHT / 2;
	static bool firstMouse = true;

	if (firstMouse) {
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos;
	lastX = xpos;
	lastY = ypos;

	MoveAndOrientCamera(Camera, glm::vec3(0.f, 0.f, 0.f), cam_dist, xoffset, yoffset, 0, 0, 0);
}

void SizeCallback(GLFWwindow* window, int w, int h) {
	glViewport(0, 0, w, h);
}

void generateDepthMap(unsigned int shadowShaderProgram, ShadowStruct shadow,
	glm::mat4 projectedLightSpaceMatrix, std::unordered_map<std::string, model>* models) {
	glViewport(0, 0, SH_MAP_WIDTH, SH_MAP_HEIGHT);
	glBindFramebuffer(GL_FRAMEBUFFER, shadow.FBO);
	glClear(GL_DEPTH_BUFFER_BIT);
	glUseProgram(shadowShaderProgram);
	glUniformMatrix4fv(glGetUniformLocation(shadowShaderProgram, "projectedLightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(projectedLightSpaceMatrix));

	// Model drawing
	glDisable(GL_BLEND);

	// Opaque models
	(*models).at("floor").draw(shadowShaderProgram);

	(*models).at("sonic").draw(shadowShaderProgram);
	(*models).at("sonic").rotate(glm::radians(-0.5f), glm::vec3(0.f, 1.f, 0.f));

	(*models).at("desk").draw(shadowShaderProgram);

	(*models).at("lamp").draw(shadowShaderProgram);

	(*models).at("chair").draw(shadowShaderProgram);

	// Models with transparency
	(*models).at("warhawk").draw(shadowShaderProgram);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glEnable(GL_BLEND);
}

void renderWithShadow(unsigned int renderShaderProgram, ShadowStruct shadow,
	glm::mat4 projectedLightSpaceMatrix, std::unordered_map<std::string, model>* models) {
	glViewport(0, 0, WIDTH, HEIGHT);

	static const GLfloat bgd[] = { .9f, .9f, .9f, 1.f };
	glClearBufferfv(GL_COLOR, 0, bgd);
	glClear(GL_DEPTH_BUFFER_BIT);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	glUseProgram(renderShaderProgram);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, shadow.Texture);
	glUniform1i(glGetUniformLocation(renderShaderProgram, "shadowMap"), 1);

	glUniformMatrix4fv(glGetUniformLocation(renderShaderProgram, "projectedLightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(projectedLightSpaceMatrix));
	glUniform3f(glGetUniformLocation(renderShaderProgram, "lightDirection"), lightDirection.x, lightDirection.y, lightDirection.z);
	glUniform3f(glGetUniformLocation(renderShaderProgram, "lightColour"), 1.f, 1.f, 1.f);
	glUniform3f(glGetUniformLocation(renderShaderProgram, "lightPos"), lightPos.x, lightPos.y, lightPos.z);
	glUniform3f(glGetUniformLocation(renderShaderProgram, "camPos"), Camera.Position.x, Camera.Position.y, Camera.Position.z);

	glm::mat4 view = glm::mat4(1.f);
	view = glm::lookAt(Camera.Position, Camera.Position + Camera.Front, Camera.Up);
	glUniformMatrix4fv(glGetUniformLocation(renderShaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));

	glm::mat4 projection = glm::mat4(1.f);
	projection = glm::perspective(glm::radians(45.f), (float)WIDTH / (float)HEIGHT, .01f, 100.f);
	glUniformMatrix4fv(glGetUniformLocation(renderShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

	// Model drawing

	// Opaque models
	(*models).at("floor").draw(renderShaderProgram);

	(*models).at("sonic").draw(renderShaderProgram);
	(*models).at("sonic").rotate(glm::radians(-0.5f), glm::vec3(0.f, 1.f, 0.f));

	(*models).at("desk").draw(renderShaderProgram);

	(*models).at("lamp").draw(renderShaderProgram);

	(*models).at("chair").draw(renderShaderProgram);

	// Models with transparency
	(*models).at("warhawk").draw(renderShaderProgram);
}

int main(int argc, char** argv) {
	glfwInit();

	glfwWindowHint(GLFW_SAMPLES, 8);
	GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Assessment 2", NULL, NULL);
	glfwMakeContextCurrent(window);
	glfwSetWindowSizeCallback(window, SizeCallback);

	gl3wInit();

	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(DebugCallback, 0);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// For transparent parts with inside and outside faces
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	ShadowStruct shadow = setup_shadowmap(SH_MAP_WIDTH, SH_MAP_HEIGHT);

	GLuint program = CompileShader("phong.vert", "phong.frag");
	GLuint shadow_program = CompileShader("shadow.vert", "shadow.frag");

	InitCamera(Camera);
	glfwSetCursorPosCallback(window, MouseCallback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// Models
	std::unordered_map<std::string, model> models;

	model warhawk("objs/warhawk/p40.obj", "objs/warhawk/");
	warhawk.scale(0.2f);
	warhawk.translate(vec3(3.f, -2.2f, -11.f));
	warhawk.rotate(glm::radians(-13.f), vec3(0.f, 0.f, 1.f));
	models.emplace("warhawk", warhawk);

	model sonic("objs/sonic/Sonic.obj", "objs/sonic/");
	sonic.scale(0.2f);
	sonic.translate(vec3(3.f, -2.5f, -9.f));
	models.emplace("sonic", sonic);

	model desk("objs/desk/desk.obj", "objs/desk/");
	desk.translate(vec3(0.f, -0.5f, -2.f));
	models.emplace("desk", desk);

	model lamp("objs/lamp/desk-lamp.obj", "objs/lamp/");
	lamp.scale(0.5f);
	lamp.translate(vec3(-1.5f, -0.99f, -4.6f));
	models.emplace("lamp", lamp);

	model chair("objs/chair/office-chair.obj", "objs/chair/");
	chair.rotate(glm::radians(180.f), glm::vec3(0.f, 1.f, 0.f));
	chair.translate(glm::vec3(0.3f, -1.3f, 1.f));
	models.emplace("chair", chair);

	std::vector<vertex> floor_verts = loadFloor();
	model floor(floor_verts);
	floor.translate(glm::vec3(0.f, -1.2f, 0.f));
	floor.scale(glm::vec3(100.f, 0.1f, 100.f));
	models.emplace("floor", floor);

	while (!glfwWindowShouldClose(window)) {
		float near_plane = 1.0f, far_plane = 70.5f;
		glm::mat4 lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, near_plane, far_plane);
		glm::mat4 lightView = glm::lookAt(lightPos, lightPos + lightDirection, glm::vec3(0.0f, 1.0f, 0.0f));
		glm::mat4 projectedLightSpaceMatrix = lightProjection * lightView;

		generateDepthMap(shadow_program, shadow, projectedLightSpaceMatrix, &models);
		renderWithShadow(program, shadow, projectedLightSpaceMatrix, &models);

		glfwSwapBuffers(window);
		glfwPollEvents();
		processKeyboard(window);
	}

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}