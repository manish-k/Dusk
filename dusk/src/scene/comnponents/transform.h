#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

namespace dusk
{
	struct TransformComponent
	{
		glm::vec3 translation = glm::vec3{ 0.0f };
		glm::vec3 rotation = glm::vec3{ 0.0f };
		glm::vec3 scale = glm::vec3{ 1.0f };

		
		glm::mat4 Mat4()
		{
			return glm::mat4{};
		}

		glm::mat4 NormalMat4()
		{
			return glm::mat4{};
		}
	};
}
