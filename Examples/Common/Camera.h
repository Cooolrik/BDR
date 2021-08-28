#pragma once

#include "Common.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "Vlk_Renderer.h"

class Camera
    {
    public:
		float cameraDist = 50.f; // distance from target
		glm::vec2 cameraRot{}; // rotation from target
		glm::vec3 cameraTarget{}; // well, target

		// per-frame calculated values
		glm::vec3 cameraPosition{};
		glm::mat4 view{};
		glm::mat4 proj{};
		glm::mat4 viewI{};
		glm::mat4 projI{};

		float nearZ = 0.1f;
		float farZ = 10000.f;

		float fovY = 45.f;

		double lastxpos = 0;
		double lastypos = 0;

		bool framebufferResized = false;
		uint ScreenW = 640;
		uint ScreenH = 512;
		float aspectRatio = (float)640 / (float)512;

		bool render_dirty = true;
		bool update_render_list = true;
		bool pyramid_cull = false;
		bool slow_down_frames = false;

		bool debug_render_layer = false;
		bool debug_take_screenshot = false;

		float debug_float_value1 = 0.f;
		float debug_float_value2 = 0.f;
		float debug_float_value3 = 0.f;

		glm::vec3 debug_selection_target = {};
		float debug_selection_dist = 50.f;

		static void framebufferResizeCallback( GLFWwindow* window, int width, int height );
		static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
		void Setup(GLFWwindow* window);
		void UpdateInput(GLFWwindow* window);
		void UpdateFrame();
	};

