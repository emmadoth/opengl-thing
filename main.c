#define GLFW_INCLUDE_NONE
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <stdio.h>
#include <stdlib.h>

#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

static void keycb(GLFWwindow* window, int key, [[maybe_unused]] int scancode, int action, [[maybe_unused]] int mods)
{
    if((key == GLFW_KEY_ESCAPE || key == GLFW_KEY_Q) && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
}

/*[[maybe_unused]] typedef struct {
    float x, y, z;
} vec3;*/

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

    int vsrcf = open("main.vert", O_RDONLY);
    if(vsrcf == -1)
    {
        perror("open");
        goto end;
    }
    off_t vsrcl = lseek(vsrcf, 0, SEEK_END);
    if(vsrcl == (off_t)-1)
    {
        close(vsrcf);
        perror("lseek");
        goto end;
    }
    const GLchar* vsrc  = mmap(NULL, (size_t)vsrcl, PROT_READ, MAP_PRIVATE, vsrcf, 0);
    if(vsrc == MAP_FAILED) {
        close(vsrcf);
        perror("mmap");
        goto end;
    }
    // use vsrc here

    printf("%s\n", vsrc);

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vsrc, NULL);
    glCompileShader(vs);
    GLint status = 0;
    glGetShaderiv(vs, GL_COMPILE_STATUS, &status);
    if(status == GL_FALSE)
    {
        GLint len = 0;
        glGetShaderiv(vs, GL_INFO_LOG_LENGTH, &len);

        GLchar* log = malloc(sizeof(GLchar) * (size_t)len);
        glGetShaderInfoLog(vs, len, &len, log);

        printf("%s\n", log);
        free(log);
        goto end;
    }

    // dont care about close failing because retrying is not advised and failure is not fatal
    close(vsrcf);
    if(munmap((void*)vsrc, (size_t)vsrcl) == -1)
    {
        perror("munmap");
        goto end;
    }

    int fsrcf = open("main.frag", O_RDONLY);
    if(fsrcf == -1)
    {
        perror("open");
        goto end;
    }
    off_t fsrcl = lseek(fsrcf, 0, SEEK_END);
    if(fsrcl == (off_t)-1)
    {
        close(fsrcf);
        perror("lseek");
        goto end;
    }
    const GLchar* fsrc  = mmap(NULL, (size_t)fsrcl, PROT_READ, MAP_PRIVATE, vsrcf, 0);
    if(fsrc == MAP_FAILED) {
        close(fsrcf);
        perror("mmap");
        goto end;
    }
    // use fsrc here

    printf("%s\n", fsrc);

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fsrc, NULL);
    glCompileShader(fs);
    glGetShaderiv(fs, GL_COMPILE_STATUS, &status);
    if(status == GL_FALSE)
    {
        GLint len = 0;
        glGetShaderiv(fs, GL_INFO_LOG_LENGTH, &len);

        GLchar* log = malloc(sizeof(GLchar) * (size_t)len);
        glGetShaderInfoLog(fs, len, &len, log);

        printf("%s\n", log);
        free(log);
        goto end;
    }


    // dont care about close failing because retrying is not advised and failure is not fatal
    close(fsrcf);
    if(munmap((void*)fsrc, (size_t)fsrcl) == -1)
    {
        perror("munmap");
        goto end;
    }

    GLuint sp = glCreateProgram();
    glAttachShader(sp, fs);
    glAttachShader(sp, vs);
    glLinkProgram(sp);
    glGetProgramiv(sp, GL_LINK_STATUS, &status);
    if(status == GL_FALSE)
    {
        GLint len = 0;
        glGetProgramiv(fs, GL_INFO_LOG_LENGTH, &len);

        GLchar* log = malloc(sizeof(GLchar) * (size_t)len);
        glGetProgramInfoLog(fs, len, &len, log);

        printf("%s\n", log);
        free(log);
        goto end;
    }


    const GLint u_time_pos = glGetUniformLocation(sp, "u_time");
    if(u_time_pos == GL_INVALID_VALUE || u_time_pos == GL_INVALID_OPERATION)
    {
        printf("glGetUniformLocation: %s\n", u_time_pos == GL_INVALID_VALUE ? "GL_INVALID_VALUE" : "GL_INVALID_OPERATION");
        goto end;
    }

    const GLint u_res_pos = glGetUniformLocation(sp, "u_resolution");
    if(u_res_pos == GL_INVALID_VALUE || u_time_pos == GL_INVALID_OPERATION)
    {
        printf("glGetUniformLocation: %s\n", u_res_pos == GL_INVALID_VALUE ? "GL_INVALID_VALUE" : "GL_INVALID_OPERATION");
        goto end;
    }


    while(!glfwWindowShouldClose(window))
    {
        double t1 = glfwGetTime();

        int w, h;
        glfwGetFramebufferSize(window, &w, &h);
        glViewport(0, 0, w, h);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glClearColor(0.16f, 0.16f, 0.16f, 1.0f);

        glUseProgram(sp);
        glBindVertexArray(vao);

        glUniform1f(u_time_pos, (GLfloat)glfwGetTime());
        glUniform2f(u_res_pos , (GLfloat)w, (GLfloat)h);

        glDrawArrays(GL_TRIANGLES, 0, 3);

        glfwPollEvents();

        glfwSwapBuffers(window);
        double t2 = glfwGetTime();

        // fps limiter
        // t1 = time before drawing
        // t2 = time affter drawing
        // ft = frame target in ms
        // fd = frame delta in ms
        double ft = (t2 - t1) * 1000.0;
        double fd = (1000.0 / 60.0) - ft;
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
