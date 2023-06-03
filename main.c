#define GLFW_INCLUDE_NONE
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <stdio.h>
#include <stdlib.h>

#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

double fps_target = 60;

// keyboard event callback
static void keycb(GLFWwindow* window, int key, [[maybe_unused]] int scancode, int action, [[maybe_unused]] int mods)
{
    if((key == GLFW_KEY_ESCAPE || key == GLFW_KEY_Q) && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
}

// maps a file into memory
void mmap_file(char path[], const GLchar** content, off_t* contentl)
{
    int fd = open(path, O_RDONLY);
    if(fd == -1)
    {
        perror("open");
        *content = NULL;
        return;
    }

    *contentl = lseek(fd, 0, SEEK_END);
    if(*contentl == (off_t)-1)
    {
        perror("lseek");
        close(fd);
        *content = NULL;
        return;
    }

    *content = mmap(NULL, (size_t)(*contentl), PROT_READ, MAP_PRIVATE, fd, 0);
    if(*content == MAP_FAILED)
    {
        perror("mmap");
        close(fd);
        *content = NULL;
        return;
    }

    close(fd);
}

// creates and compiles a shader from a string
GLuint make_shader(GLuint type, const GLchar** ssrc)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, ssrc, NULL);
    glCompileShader(shader);

    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if(status == GL_FALSE)
    {
        GLint len = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);

        GLchar* log = malloc(sizeof(GLchar) * (size_t)len);
        glGetShaderInfoLog(shader, len, &len, log);

        printf("%s\n", log);
        free(log);

    }

    return shader;
}

// creates an opengl shader program with a fragment shader and a vertex shader
GLuint make_program(GLuint fs, GLuint vs)
{
    GLuint prog = glCreateProgram();
    glAttachShader(prog, fs);
    glAttachShader(prog, vs);
    glLinkProgram(prog);

    GLint status;
    glGetProgramiv(prog, GL_LINK_STATUS, &status);
    if(status == GL_FALSE)
    {
        GLint len = 0;
        glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &len);

        GLchar* log = malloc(sizeof(GLchar) * (size_t)len);
        glGetProgramInfoLog(prog, len, &len, log);

        printf("%s\n", log);
        free(log);

        return 0;
    }

    return prog;
}

GLint find_uniform(GLuint prog, char uni[])
{
    const GLint pos = glGetUniformLocation(prog, uni);
    if(pos == GL_INVALID_VALUE || pos == GL_INVALID_OPERATION)
    {
        printf("glGetUniformLocation: %s\n", pos == GL_INVALID_VALUE ? "GL_INVALID_VALUE" : "GL_INVALID_OPERATION");
        return 0;
    }
    return pos;
}

int main(void)
{
    if(!glfwInit())
        return 1;

    GLFWwindow* window = glfwCreateWindow(800, 600, "test", NULL, NULL);
    if(!window)
    {
        glfwTerminate();
        return 2;
    }

    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, keycb);

    glewExperimental = GL_TRUE;
    glewInit();

    const GLubyte* r = glGetString(GL_RENDERER);
    const GLubyte* v = glGetString(GL_VERSION);
    printf("%s %s\n", r, v);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // shape
    float triangle[] = {
        0.0f,  1.0f, 0.0f,
        1.0f, -1.0f, 0.0f,
       -1.0f, -1.0f, 0.0f
    };

    // vertex buffer object
    GLuint vbo = 0;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, 9 * sizeof(float), triangle, GL_STATIC_DRAW);

    // vertex array object
    GLuint vao = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);

    // load vertex shader source
    const GLchar* vsrc;
    off_t vsrcl;
    mmap_file("main.vert", &vsrc, &vsrcl);
    if(vsrc == NULL)
        goto end;
    
    printf("%s\n", vsrc);

    // create and compile vertex shader from source
    GLuint vs = make_shader(GL_VERTEX_SHADER, &vsrc);
    if(vs == 0)
        goto end;

    // unload vertex shader source
    if(munmap((void*)vsrc, (size_t)vsrcl) == -1)
    {
        perror("munmap");
        goto end;
    }

    // load fragment shader source
    const GLchar* fsrc;
    off_t fsrcl;
    mmap_file("main.frag", &fsrc, &fsrcl);
    if(vsrc == NULL)
        goto end;

    printf("%s\n", fsrc);

    // create and compile fragment shader from source
    GLuint fs = make_shader(GL_FRAGMENT_SHADER, &fsrc);
    if(vs == 0)
        goto end;

    // unload fragment shader source
    if(munmap((void*)fsrc, (size_t)fsrcl) == -1)
    {
        perror("munmap");
        goto end;
    }

    // make shader program
    GLuint sp = make_program(fs, vs);
    if(sp == 0)
        goto end;


    const GLint u_time_pos       = find_uniform(sp, "u_time"      );
    const GLint u_resolution_pos = find_uniform(sp, "u_resolution");

    int w, h;
    double t1, t2, ft, fd;

    while(!glfwWindowShouldClose(window))
    {
        t1 = glfwGetTime();

        glfwGetFramebufferSize(window, &w, &h);
        glViewport(0, 0, w, h);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glClearColor(0.16f, 0.16f, 0.16f, 1.0f);

        glUseProgram(sp);
        glBindVertexArray(vao);

        // set uniform values
        glUniform1f(u_time_pos      , (GLfloat)glfwGetTime());
        glUniform2f(u_resolution_pos, (GLfloat)w, (GLfloat)h);

        glDrawArrays(GL_TRIANGLES, 0, 3);

        glfwPollEvents();

        glfwSwapBuffers(window);
        t2 = glfwGetTime();

        // fps limiter
        // t1 = time before drawing
        // t2 = time affter drawing
        // ft = frame target in ms
        // fd = frame delta in ms
        ft = (t2 - t1) * 1000.0;
        fd = (1000.0 / fps_target) - ft;
        printf("%.4fs %.0ffps %.0fms %.0fms\n", glfwGetTime(), 1000.0 / (ft + fd), ft, fd);
        if(fd > 0)
        {
            struct timespec sleepl = {
                .tv_sec = 0,
                .tv_nsec = (long int)(fd * 1000000.0)
            };
            nanosleep(&sleepl, NULL);
        }
    }

    end:
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
