#include <chrono>
#include <fstream>
#include <string>

#include <GL/glew.h>
#include <SDL2/SDL.h>
#if defined(__APPLE__)
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/euler_angles.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <iostream>
using namespace std;

#include "utils.cpp"

int main()
{
	int original_window_width = 256;
	int original_window_height = 256;
	bool fullscreen = false;

	// create the state of the machine
	State state = State{};
	state.window.x = original_window_width;
	state.window.y = original_window_height;

	// initialize the window
	glContextWindow context_window =
		init_window(state.window.x, state.window.y);
	SDL_GLContext glContext = context_window.glContext;
	SDL_Window *window = context_window.window;

	// set window to fullscreen
	if (fullscreen)
		SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);

	// create shaders
	std::string VertexShaderSource = load_file("shaders/vertex_shader.glsl");
	std::string PixelShaderSource = load_file("shaders/pixel_shader.glsl");
	std::string ConwayeShaderSource = load_file("shaders/conway_shader.glsl");

	// load vertex and pixel shaders program
	GLuint renderShader = create_render_shader(VertexShaderSource.c_str(),
											   PixelShaderSource.c_str());

	// load compute shader program
	GLuint conwayShader = create_compute_shader(ConwayeShaderSource.c_str());

	// crash if shaders were not created
	if (renderShader == 0 or conwayShader == 0) {
		return 1;
	}

	// create textures
	// GLuint current_generation =
	// load_texture("textures/first_generation.png");
	GLuint current_generation =
		create_empty_texture(state.window.x, state.window.y);
	GLuint next_generation =
		create_empty_texture(state.window.x, state.window.y);

	// create geometry
	GLuint Quad_VAO, Quad_VBO;
	Quad_VAO = create_screen_quad_ndc(Quad_VBO);

	// begin timer
	chrono::time_point<chrono::system_clock> timerStart =
		chrono::system_clock::now();

	// create fps counter
	float fps_counter = 0.;

	// begin the frame loop
	SDL_Event event;
	bool run = true;
	while (run) {
		// update datas with events
		while (SDL_PollEvent(&event)) {
			run = state.update_with_event(event);
		}

		// stop the loop if needed
		if (!run)
			break;

		// conway shader pass
		{
			// use compute shader
			glUseProgram(conwayShader);

			// bind images
			glBindImageTexture(0, current_generation, 0, GL_FALSE, 0,
							   GL_READ_WRITE, GL_R8);
			glBindImageTexture(1, next_generation, 0, GL_FALSE, 0,
							   GL_READ_WRITE, GL_R8);

			// set uniforms
			glUniform4f(glGetUniformLocation(conwayShader, "mouse"),
						state.mouse.x, state.mouse.y, state.mouse.z,
						state.mouse.w);
			glUniform1f(glGetUniformLocation(conwayShader, "radius_mouse"), 5.);
			glUniform1f(glGetUniformLocation(conwayShader, "time"), state.time);

			// launch computation with compute group to (32,32,1)
			glDispatchCompute(original_window_width / 8,
							  original_window_height / 8, 1);

			// pause CPU execution until the image is closed
			glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		}

		// render shader pass
		{
			// update framebuffer and viewport
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT |
					GL_STENCIL_BUFFER_BIT);
			if (fullscreen)
				glViewport(0, 0, state.window.x, state.window.y);
			else
				glViewport(0, 0, state.window.x * 2, state.window.y * 2);

			// use render shader
			glUseProgram(renderShader);

			// bind framebuffer
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glActiveTexture(GL_TEXTURE0);

			// bind texture to render
			glBindTexture(GL_TEXTURE_2D, current_generation);
			glUniform1i(
				glGetUniformLocation(renderShader, "current_generation"), 0);

			// bind to and draw geometry
			glBindVertexArray(Quad_VAO);
			glDrawArrays(GL_TRIANGLES, 0, 6);
		}

		// copy textures
		{
			next_generation = current_generation;
		}

		// update time
		float curTime = chrono::duration_cast<chrono::microseconds>(
							chrono::system_clock::now() - timerStart)
							.count() /
						1000000.0f;
		state.timeDelta = curTime - state.time;
		state.time = curTime;
		state.frameIndex++;
		fps_counter += 1 / state.timeDelta;

		// show frame, time and fps every 30 frames
		if (state.frameIndex % 30 == 0) {
			float fps_mean = fps_counter / 30;
			cout << "frame number " << state.frameIndex << ", time "
				 << state.time << ", " << fps_mean << " fps" << endl;
			fps_counter = 0.;
		}

		// update the window content
		SDL_GL_SwapWindow(window);
	}

	// stop using shader program
	glUseProgram(0);

	// delete the window
	SDL_GL_DeleteContext(glContext);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}
