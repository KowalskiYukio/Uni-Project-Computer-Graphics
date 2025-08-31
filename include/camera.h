#pragma once

#include <stdio.h>
#include <glm/glm.hpp>

struct SCamera {
	enum Camera_Movement {
		FORWARD,
		BACKWARD,
		LEFT,
		RIGHT
	};

	glm::vec3 Position;
	glm::vec3 Front;
	glm::vec3 Up;
	glm::vec3 Right;

	glm::vec3 WorldUp;

	float Yaw;
	float Pitch;

	float MovementSpeed = 0.005f;
	float MouseSensitivity = 0.03f;
};


void InitCamera(SCamera& in) {
	in.Front = glm::vec3(0.0f, 0.0f, -1.0f);
	in.Position = glm::vec3(0.0f, 0.0f, 0.0f);
	in.Up = glm::vec3(0.0f, 1.0f, 0.0f);
	in.WorldUp = in.Up;
	in.Right = glm::normalize(glm::cross(in.Front, in.WorldUp));

	in.Yaw = -90.f;
	in.Pitch = 0.f;
}

float cam_dist = 2.f;

void MoveAndOrientCamera(SCamera& in, glm::vec3 target, float distance, float x_view, float y_view, float z_move, float x_move, float y_move) {
	in.Yaw += x_view * in.MouseSensitivity;
	in.Pitch += y_view * in.MouseSensitivity;

	// Limit Pitch to avoid flipping at 90 deg
	if (in.Pitch > 89.0f)
		in.Pitch = 89.0f;
	if (in.Pitch < -89.0f)
		in.Pitch = -89.0f;

	// Front replaces Position for FPS-style camera control
	glm::vec3 front = glm::vec3(
		cos(glm::radians(in.Yaw)) * cos(glm::radians(in.Pitch)),
		sin(glm::radians(in.Pitch)),
		sin(glm::radians(in.Yaw)) * cos(glm::radians(in.Pitch))
	);
	in.Front = glm::normalize(front);

	in.Right = glm::normalize(glm::cross(in.Front, in.WorldUp));
	in.Up = glm::normalize(glm::cross(in.Right, in.Front));

	glm::vec3 y_lock = glm::normalize(glm::vec3(in.Front.x, 0.0f, in.Front.z));
	in.Position += y_lock * z_move * in.MovementSpeed;
	in.Position += in.Right * x_move * in.MovementSpeed;
	in.Position += in.WorldUp * y_move * in.MovementSpeed;
}