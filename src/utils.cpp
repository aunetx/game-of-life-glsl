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

/// Stores window-specific datas.
struct sedData
{
    float windowWidth = 512,
          windowHeight = 512;
    float mouseX = 0.0f,
          mouseY = 0.0f;
};

/// Stores datas that can be given to the shaders.
struct sysData
{
    float time = 0.0f,
          timeDelta = 0.0f;
    uint frameIndex = 0;
    glm::vec2 mousePosition;
    glm::vec4 mouse;
    glm::vec2 viewportSize;
    glm::mat4 view;
    glm::mat4 projection;
    glm::mat4 orthographic;
    glm::mat4 geometryTransform;
    glm::mat4 viewProjection = projection * view;
    glm::mat4 viewOrthographic = orthographic * view;
};

/// Create the initial system datas thanks to the windows datas.
void initSysData(sysData &sys, sedData &sed, int window_width, int window_height)
{
    sed.windowWidth = window_width;
    sed.windowHeight = window_height;

    sys.time = 0.0f;
    sys.timeDelta = 0.0f;
    sys.frameIndex = 0;
    sys.mousePosition.x = sed.mouseX;
    sys.mousePosition.y = sed.mouseY;
    sys.mouse.x = sed.mouseX;
    sys.mouse.y = sed.mouseY;
    sys.mouse.z = 0.f;
    sys.mouse.w = 0.f;
    sys.viewportSize.x = sed.windowWidth;
    sys.viewportSize.y = sed.windowHeight;
    sys.view = glm::mat4(-0.754709f, -0.277263f, 0.594591f, 0.000000f, -0.000000f, 0.906308f, 0.422618f, 0.000000f, -0.656059f, 0.318954f, -0.683999f, 0.000000f, -0.000001f, -0.000001f, -34.000000f, 1.000000f);
    sys.projection = glm::perspective(glm::radians(45.0f), sed.windowWidth / sed.windowHeight, 0.1f, 1000.0f);
    sys.orthographic = glm::ortho(0.0f, sed.windowWidth, sed.windowHeight, 0.0f, 0.1f, 1000.0f);
    sys.geometryTransform = glm::mat4(1.0f);
    sys.viewProjection = sys.projection * sys.view;
    sys.viewOrthographic = sys.orthographic * sys.view;
}

/// Update the datas with the given event.
bool updateWithEvent(SDL_Event event, sedData &sed, sysData &sys)
{
    if (event.type == SDL_QUIT)
    {
        return false;
    }
    else if (event.type == SDL_MOUSEMOTION)
    {
        sed.mouseX = event.motion.x;
        sed.mouseY = event.motion.y;
        sys.mousePosition = glm::vec2(sed.mouseX / sed.windowWidth, 1.f - (sed.mouseY / sed.windowHeight));
        sys.mouse.x = event.motion.x / sed.windowWidth;
        sys.mouse.y = 1 - (event.motion.y / sed.windowHeight);

        /*
        cout << "mouse motion" << endl;
        cout << sys.mouse.x << endl;
        cout << sys.mouse.y << endl;
        */
        return true;
    }
    else if (event.type == SDL_MOUSEBUTTONUP || event.type == SDL_MOUSEBUTTONDOWN)
    {
        if (event.button.button == SDL_BUTTON_LEFT)
            sys.mouse.z = 0. + 1. * (event.button.state == SDL_PRESSED);
        else if (event.button.button == SDL_BUTTON_RIGHT)
            sys.mouse.w = 0. + 1. * (event.button.state == SDL_PRESSED);

        /*
        cout << "click" << endl;
        cout << sys.mouse.x << endl;
        cout << sys.mouse.y << endl;
        cout << sys.mouse.z << endl;
        cout << sys.mouse.w << endl;
        */
        return true;
    }
    else if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_RESIZED)
    {
        sed.windowWidth = event.window.data1;
        sed.windowHeight = event.window.data2;

        sys.viewportSize = glm::vec2(sed.windowWidth, sed.windowHeight);
        sys.projection = glm::perspective(glm::radians(45.0f), sed.windowWidth / sed.windowHeight, 0.1f, 1000.0f);
        sys.orthographic = glm::ortho(0.0f, sed.windowWidth, sed.windowHeight, 0.0f, 0.1f, 1000.0f);
        sys.geometryTransform = glm::mat4(1.0f);
        sys.viewProjection = sys.projection * sys.view;
        sys.viewOrthographic = sys.orthographic * sys.view;
        return true;
    }
    else
    {
        return true;
    }
};

// for the function InitWindow
struct glContextWindow
{
    SDL_GLContext glContext;
    SDL_Window *window;
};

// functions defined after
glContextWindow InitWindow(float width, float height);
GLuint CreateScreenQuadNDC(GLuint &vbo);
std::string LoadFile(const std::string &filename);
GLuint CreateShader(const char *vsCode, const char *psCode);
GLuint LoadTexture(const std::string &filename);

glContextWindow InitWindow(float width, float height)
{
    stbi_set_flip_vertically_on_load(1);

    // init sdl2
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO) < 0)
    {
        printf("Failed to initialize SDL2\n");
        exit(1);
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1); // double buffering

    // open window
    SDL_Window *window = SDL_CreateWindow(
        "Conway",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        width, height,
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
    if (glewInit_res != GLEW_OK)
    {
        std::cout << "Failed to initialize GLEW: "
                  << glewGetErrorString(glewInit_res)
                  << std::endl;
        // exit(1);
    }

    return glContextWindow{glContext, window};
}

GLuint CreateScreenQuadNDC(GLuint &vbo)
{
    GLfloat sqData[] = {
        -1,
        -1,
        0.0f,
        0.0f,
        1,
        -1,
        1.0f,
        0.0f,
        1,
        1,
        1.0f,
        1.0f,
        -1,
        -1,
        0.0f,
        0.0f,
        1,
        1,
        1.0f,
        1.0f,
        -1,
        1,
        0.0f,
        1.0f,
    };

    GLuint vao;

    // create vao
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // create vbo
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    // vbo data
    glBufferData(GL_ARRAY_BUFFER, 6 * 4 * sizeof(GLfloat), sqData, GL_STATIC_DRAW);

    // vertex positions
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void *)0);
    glEnableVertexAttribArray(0);

    // vertex texture coords
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void *)(2 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    return vao;
}

std::string LoadFile(const std::string &filename)
{
    std::ifstream file(filename);
    std::string src((std::istreambuf_iterator<char>(file)),
                    std::istreambuf_iterator<char>());
    file.close();
    return src;
}

GLuint CreateRenderShader(const char *vsCode, const char *psCode)
{
    GLint success = 0;
    char infoLog[512];

    // create vertex shader
    unsigned int vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vsCode, nullptr);
    glCompileShader(vs);
    glGetShaderiv(vs, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vs, 512, NULL, infoLog);
        printf("Failed to compile the shader.\n");
        return 0;
    }

    // create pixel shader
    unsigned int ps = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(ps, 1, &psCode, nullptr);
    glCompileShader(ps);
    glGetShaderiv(ps, GL_COMPILE_STATUS, &success);
    if (!success)
    {
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
    if (!success)
    {
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

GLuint CreateComputeShader(const char *csCode)
{
    GLint success = 0;
    char infoLog[512];

    // create compute shader
    unsigned int cs = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(cs, 1, &csCode, nullptr);
    glCompileShader(cs);
    glGetShaderiv(cs, GL_COMPILE_STATUS, &success);
    if (!success)
    {
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
    if (!success)
    {
        glGetProgramInfoLog(computeShader, 512, NULL, infoLog);
        printf("Failed to create the shader.\n");
        printf(infoLog);
        return 0;
    }

    // delete compute shader
    glDeleteShader(cs);

    return computeShader;
}

GLuint LoadTexture(const std::string &file)
{
    int width, height, nrChannels;
    unsigned char *data = stbi_load(file.c_str(), &width, &height, &nrChannels, 0);

    if (data == nullptr)
        return 0;

    GLuint ret = 0;
    glGenTextures(1, &ret);
    glBindTexture(GL_TEXTURE_2D, ret);

    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_R8, // internal format
                 width,
                 height,
                 0,
                 GL_RGBA,          // read format
                 GL_UNSIGNED_BYTE, // type of values in read format
                 data              // source
    );

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glBindTexture(GL_TEXTURE_2D, 0);

    stbi_image_free(data);

    return ret;
}

GLuint CreateEmptyTexture(float width, float height)
{
    GLuint ret = 0;
    glGenTextures(1, &ret);
    glBindTexture(GL_TEXTURE_2D, ret);

    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_R8, // internal format
                 width,
                 height,
                 0,
                 GL_RGBA,          // read format
                 GL_UNSIGNED_BYTE, // type of values in read format
                 NULL              // no data
    );

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glBindTexture(GL_TEXTURE_2D, 0);

    return ret;
}
