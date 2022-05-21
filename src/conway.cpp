#include "utils.cpp"

int main()
{
	int window_width = 1000;
	int window_height = 1000;
	bool fullscreen = false;

	// initial sed and sys (to store the system variables)
	sedData sed = sedData{};
	sysData sys = sysData{};
	initSysData(sys, sed, window_width, window_height);

	// initialize the window
	glContextWindow context_window = InitWindow(sed.windowWidth, sed.windowHeight);
	SDL_GLContext glContext = context_window.glContext;
	SDL_Window *window = context_window.window;

	// set window to fullscreen
	if (fullscreen)
		SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);

	// create shaders
	std::string VertexShaderSource = LoadFile("shaders/vertex_shader.glsl");
	std::string PixelShaderSource = LoadFile("shaders/pixel_shader.glsl");
	std::string ConwayeShaderSource = LoadFile("shaders/conway_shader.glsl");

	// load vertex and pixel shaders program
	GLuint renderShader = CreateRenderShader(VertexShaderSource.c_str(), PixelShaderSource.c_str());

	// load compute shader program
	GLuint conwayShader = CreateComputeShader(ConwayeShaderSource.c_str());

	// crash if shaders were not created
	if (renderShader == 0 or conwayShader == 0)
	{
		return 1;
	}

	// create textures
	// GLuint current_generation = LoadTexture("textures/first_generation.png");
	GLuint current_generation = CreateEmptyTexture(sed.windowWidth, sed.windowHeight);
	GLuint next_generation = CreateEmptyTexture(sed.windowWidth, sed.windowHeight);

	// create geometry
	GLuint Quad_VAO, Quad_VBO;
	Quad_VAO = CreateScreenQuadNDC(Quad_VBO);

	// begin timer
	chrono::time_point<chrono::system_clock> timerStart = chrono::system_clock::now();

	// create fps counter
	float fps_counter = 0.;

	// begin the frame loop
	SDL_Event event;
	bool run = true;
	while (run)
	{
		// update datas with events
		while (SDL_PollEvent(&event))
		{
			run = updateWithEvent(event, sed, sys);
		}

		// stop the loop if needed
		if (!run)
			break;

		// conway shader pass
		{
			// use compute shader
			glUseProgram(conwayShader);

			// bind images
			glBindImageTexture(0, current_generation, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R8);
			glBindImageTexture(1, next_generation, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R8);

			// set uniforms
			glUniform4f(glGetUniformLocation(conwayShader, "mouse"), sys.mouse.x, sys.mouse.y, sys.mouse.z, sys.mouse.w);
			glUniform1f(glGetUniformLocation(conwayShader, "radius_mouse"), 2.);

			// launch computation with compute group to (32,32,1)
			glDispatchCompute(window_width / 8, window_width / 8, 1);

			// pause CPU execution until the image is closed
			glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		}

		// render shader pass
		{
			// update framebuffer and viewport
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
			if (fullscreen)
				glViewport(0, 0, sed.windowWidth, sed.windowHeight);
			else
				glViewport(0, 0, sed.windowWidth * 2, sed.windowHeight * 2);

			// use render shader
			glUseProgram(renderShader);

			// bind framebuffer
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glActiveTexture(GL_TEXTURE0);

			// bind texture to render
			glBindTexture(GL_TEXTURE_2D, current_generation);
			glUniform1i(glGetUniformLocation(renderShader, "current_generation"), 0);

			// bind to and draw geometry
			glBindVertexArray(Quad_VAO);
			glDrawArrays(GL_TRIANGLES, 0, 6);
		}

		// copy textures
		{
			next_generation = current_generation;
		}

		// update time
		float curTime = chrono::duration_cast<chrono::microseconds>(chrono::system_clock::now() - timerStart).count() / 1000000.0f;
		sys.timeDelta = curTime - sys.time;
		sys.time = curTime;
		sys.frameIndex++;
		fps_counter += 1 / sys.timeDelta;

		// show frame, time and fps every 30 frames
		if (sys.frameIndex % 30 == 0)
		{
			float fps_mean = fps_counter / 30;
			cout << "frame number " << sys.frameIndex
				 << ", time " << sys.time
				 << ", " << fps_mean << " fps"
				 << endl;
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
