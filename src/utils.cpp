/// Stores the state of the program, that can be given to the shaders.
struct State
{
	uint frameIndex = 0;
	float time = 0.f;
	float timeDelta = 0.f;
	glm::vec2 window = glm::vec2(512, 512);
	glm::vec4 mouse = glm::vec4(0.f, 0.f, 0.f, 0.f);

	/// Update the state with the given event.
	bool update_with_event(SDL_Event event)
	{
		if (event.type == SDL_QUIT) {
			return false;
		}
		else if (event.type == SDL_MOUSEMOTION) {
			mouse.x = event.motion.x / window.x;
			mouse.y = 1 - (event.motion.y / window.y);
		}
		else if (event.type == SDL_MOUSEBUTTONUP ||
				 event.type == SDL_MOUSEBUTTONDOWN) {
			if (event.button.button == SDL_BUTTON_LEFT)
				mouse.z = 0. + 1. * (event.button.state == SDL_PRESSED);
			else if (event.button.button == SDL_BUTTON_RIGHT)
				mouse.w = 0. + 1. * (event.button.state == SDL_PRESSED);
		}
		else if (event.type == SDL_WINDOWEVENT &&
				 event.window.event == SDL_WINDOWEVENT_RESIZED) {
			window.x = event.window.data1;
			window.y = event.window.data2;
		}
		return true;
	};
};

// for the function InitWindow
struct glContextWindow
{
	SDL_GLContext glContext;
	SDL_Window *window;
};

// functions defined after
glContextWindow init_window(float width, float height);
GLuint create_screen_quad_ndc(GLuint &vbo);
std::string load_file(const std::string &filename);
GLuint create_shader(const char *vsCode, const char *psCode);
GLuint load_texture(const std::string &filename);

glContextWindow init_window(float width, float height)
{
	stbi_set_flip_vertically_on_load(1);

	// init sdl2
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO) < 0) {
		printf("Failed to initialize SDL2\n");
		exit(1);
	}

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
						SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1); // double buffering

	// open window
	SDL_Window *window = SDL_CreateWindow(
		"Conway", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height,
		SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
	SDL_SetWindowMinimumSize(window, 200, 200);

	// get GL context
	SDL_GLContext glContext = SDL_GL_CreateContext(window);
	SDL_GL_MakeCurrent(window, glContext);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_STENCIL_TEST);

	// init glew
	glewExperimental = true;
	GLenum glewInit_res = glewInit();
	if (glewInit_res != GLEW_OK) {
		std::cout
			<< "Failed to initialize GLEW: " << glewGetErrorString(glewInit_res)
			<< std::endl;
		// exit(1);
	}

	return glContextWindow{ glContext, window };
}

GLuint create_screen_quad_ndc(GLuint &vbo)
{
	GLfloat sqData[] = {
		-1, -1, 0.0f, 0.0f, 1, -1, 1.0f, 0.0f, 1,  1, 1.0f, 1.0f,
		-1, -1, 0.0f, 0.0f, 1, 1,  1.0f, 1.0f, -1, 1, 0.0f, 1.0f,
	};

	GLuint vao;

	// create vao
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	// create vbo
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	// vbo data
	glBufferData(GL_ARRAY_BUFFER, 6 * 4 * sizeof(GLfloat), sqData,
				 GL_STATIC_DRAW);

	// vertex positions
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat),
						  (void *)0);
	glEnableVertexAttribArray(0);

	// vertex texture coords
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat),
						  (void *)(2 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	return vao;
}

std::string load_file(const std::string &filename)
{
	std::ifstream file(filename);
	std::string src((std::istreambuf_iterator<char>(file)),
					std::istreambuf_iterator<char>());
	file.close();
	return src;
}

GLuint create_render_shader(const char *vsCode, const char *psCode)
{
	GLint success = 0;
	char infoLog[512];

	// create vertex shader
	unsigned int vs = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vs, 1, &vsCode, nullptr);
	glCompileShader(vs);
	glGetShaderiv(vs, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(vs, 512, NULL, infoLog);
		printf("Failed to compile the shader.\n");
		return 0;
	}

	// create pixel shader
	unsigned int ps = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(ps, 1, &psCode, nullptr);
	glCompileShader(ps);
	glGetShaderiv(ps, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(ps, 512, NULL, infoLog);
		printf("Failed to compile the shader.\n");
		printf(infoLog);
		return 0;
	}

	// create render shader program
	GLuint renderShader = glCreateProgram();
	glAttachShader(renderShader, vs);
	glAttachShader(renderShader, ps);
	glLinkProgram(renderShader);
	glGetProgramiv(renderShader, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(renderShader, 512, NULL, infoLog);
		printf("Failed to create the shader.\n");
		printf(infoLog);
		return 0;
	}

	// delete vertex and pixel shaders
	glDeleteShader(vs);
	glDeleteShader(ps);

	return renderShader;
}

GLuint create_compute_shader(const char *csCode)
{
	GLint success = 0;
	char infoLog[512];

	// create compute shader
	unsigned int cs = glCreateShader(GL_COMPUTE_SHADER);
	glShaderSource(cs, 1, &csCode, nullptr);
	glCompileShader(cs);
	glGetShaderiv(cs, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(cs, 512, NULL, infoLog);
		printf("Failed to compile the shader.\n");
		printf(infoLog);
		return 0;
	}

	// create compute shader program
	GLuint computeShader = glCreateProgram();
	glAttachShader(computeShader, cs);
	glLinkProgram(computeShader);
	glGetProgramiv(computeShader, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(computeShader, 512, NULL, infoLog);
		printf("Failed to create the shader.\n");
		printf(infoLog);
		return 0;
	}

	// delete compute shader
	glDeleteShader(cs);

	return computeShader;
}

GLuint load_texture(const std::string &file, uint format)
{
	int width, height, nrChannels;
	unsigned char *data =
		stbi_load(file.c_str(), &width, &height, &nrChannels, 0);

	if (data == nullptr)
		return 0;

	GLuint ret = 0;
	glGenTextures(1, &ret);
	glBindTexture(GL_TEXTURE_2D, ret);

	glTexImage2D(GL_TEXTURE_2D, 0,
				 format, // internal format
				 width, height, 0,
				 GL_RGB, // read format
				 GL_UNSIGNED_BYTE, // type of values in read format
				 data // source
	);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glBindTexture(GL_TEXTURE_2D, 0);

	stbi_image_free(data);

	return ret;
}

GLuint create_empty_texture(float width, float height, uint format)
{
	GLuint ret = 0;
	glGenTextures(1, &ret);
	glBindTexture(GL_TEXTURE_2D, ret);

	glTexImage2D(GL_TEXTURE_2D, 0,
				 format, // internal format
				 width, height, 0,
				 GL_RGBA, // read format
				 GL_UNSIGNED_BYTE, // type of values in read format
				 NULL // no data
	);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glBindTexture(GL_TEXTURE_2D, 0);

	return ret;
}
