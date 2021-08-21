#include "Camera.h"


void Camera::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
	{
	auto app = reinterpret_cast<Camera*>(glfwGetWindowUserPointer(window));
	if(action == GLFW_RELEASE) return; //only handle press events
	if(key == GLFW_KEY_L)
		{
		app->update_render_list = !app->update_render_list;
		printf("update_render_list == %s\n", app->update_render_list?"true":"false");
		app->render_dirty = true;
		}
	else if(key == GLFW_KEY_R)
		{
		app->cameraDist = 50.f;
		app->cameraRot = glm::vec2();
		app->cameraTarget = glm::vec3();
		app->render_dirty = true;
		}
	else if(key == GLFW_KEY_SPACE)
		{
		app->debug_render_layer = !app->debug_render_layer;
		printf( "debug_render_layer == %s\n", app->debug_render_layer ? "true" : "false" );
		app->render_dirty = true;
		}
	else if(key == GLFW_KEY_ENTER)
		{
		app->debug_take_screenshot = true;
		printf( "screenshot taken\n" );
		app->render_dirty = true;
		}
	else if(key == GLFW_KEY_ESCAPE)
		{
		glfwSetWindowShouldClose(window,1);
		}
	else if(key == GLFW_KEY_P)
		{
		app->pyramid_cull = !app->pyramid_cull;
		printf( "pyramid_cull == %s\n", app->pyramid_cull ? "true" : "false" );
		app->render_dirty = true;
		}
	else if(key == GLFW_KEY_S)
		{
		app->slow_down_frames = !app->slow_down_frames;
		printf( "slow_down_frames == %s\n", app->slow_down_frames ? "true" : "false" );
		app->render_dirty = true;
		}
	}

void Camera::framebufferResizeCallback( GLFWwindow* window, int width, int height )
	{
	auto cam = reinterpret_cast<Camera*>(glfwGetWindowUserPointer( window ));
	cam->framebufferResized = true;
	cam->ScreenW = width;
	cam->ScreenH = height;
	cam->aspectRatio = (float)width / (float)height;
	cam->render_dirty = true;
	}

void Camera::Setup(GLFWwindow* window)
	{
	glfwSetWindowUserPointer( window, this );
	glfwSetKeyCallback(window, key_callback);
	glfwSetFramebufferSizeCallback( window, framebufferResizeCallback );
	}

void Camera::UpdateInput(GLFWwindow* window)
	{
	double xpos;
	double ypos;
	glfwGetCursorPos(window, &xpos, &ypos);

	double relativexpos = xpos - (double)ScreenW / 2;
	double relativeypos = ypos - (double)ScreenH / 2;

	if (glfwGetKey(window, GLFW_KEY_LEFT_ALT))
		{
		if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1))
			{
			cameraRot.x += ((float)(xpos - lastxpos)) / 100.f;
			cameraRot.y += ((float)(ypos - lastypos)) / 100.f;
			if (cameraRot.y < -1.5f)
				cameraRot.y = -1.5f;
			if (cameraRot.y > 1.5f)
				cameraRot.y = 1.5f;
			render_dirty = true;
			}
		else if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_2))
			{
			cameraDist *= 1.f + (((float)(ypos - lastypos)) / 1000.f);
			render_dirty = true;
			}
		else if(glfwGetMouseButton( window, GLFW_MOUSE_BUTTON_3 ))
			{
			glm::mat4x4 inv = glm::transpose( this->view );

			glm::vec3 xaxis = glm::vec3( inv[0] );
			glm::vec3 yaxis = glm::vec3( inv[1] );

			float speed = cameraDist;

			cameraTarget += xaxis * -((((float)(xpos - lastxpos)) / 1000.f) * speed);
			cameraTarget += yaxis * ((((float)(ypos - lastypos)) / 1000.f) * speed);

			render_dirty = true;
			}
		}
	else if(glfwGetKey( window, GLFW_KEY_LEFT_CONTROL ))
		{
		if(glfwGetMouseButton( window, GLFW_MOUSE_BUTTON_1 ))
			{
			debug_float_value1 -= ((float)(ypos - lastypos)) / 100.f;
			render_dirty = true;
			}
		if(glfwGetMouseButton( window, GLFW_MOUSE_BUTTON_2 ))
			{
			debug_float_value2 -= ((float)(ypos - lastypos)) / 100.f;
			render_dirty = true;
			}
		if(glfwGetMouseButton( window, GLFW_MOUSE_BUTTON_3 ))
			{
			debug_float_value3 -= ((float)(ypos - lastypos)) / 100.f;
			render_dirty = true;
			}
		}


	lastxpos = xpos;
	lastypos = ypos;

	if (render_dirty)
		{
		if (lastxpos < 5) { lastxpos += ((double)ScreenW - 10.0); }
		else if (lastxpos > ((double)ScreenW - 5.0)) { lastxpos -= ((double)ScreenW - 10.0); }

		if (lastypos < 5) { lastypos += ((double)ScreenH - 10.0); }
		else if (lastypos > ((double)ScreenH - 5.0)) { lastypos -= ((double)ScreenH - 10.0); }

		if (lastxpos != xpos || lastypos != ypos) { glfwSetCursorPos(window, lastxpos, lastypos); }
		}
	}


glm::mat4 perspectiveProjection( float fovY, float aspect, float zNear, float zFar )
	{
	const float tanHalfFovy = std::tan( fovY / 2.f );

	glm::mat4 mtx(0);
	mtx[0][0] = 1.f / (aspect * tanHalfFovy);
	mtx[1][1] = -1.f / (tanHalfFovy);
	mtx[2][2] = zFar / (zNear - zFar);
	mtx[2][3] = -1.f;
	mtx[3][2] = -(zFar * zNear) / (zFar - zNear);

	return mtx;
	}

void Camera::UpdateFrame()
	{
	// calculate new settings for the rendering
	float camR = cosf(cameraRot.y) * cameraDist;
	
	this->cameraPosition.y = sinf(cameraRot.y) * cameraDist;
	this->cameraPosition.x = cosf(cameraRot.x) * camR;
	this->cameraPosition.z = sinf(cameraRot.x) * camR;

	this->cameraPosition += this->cameraTarget;

	this->view = glm::lookAt(this->cameraPosition, this->cameraTarget, glm::vec3(0.0f, 1.0f, 0.0f));
	this->proj = perspectiveProjection(glm::radians(45.0f), aspectRatio, nearZ, farZ);
	this->viewI = glm::inverse(this->view);
	this->projI = glm::inverse(this->proj);
	}