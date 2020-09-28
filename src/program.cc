/*
 * MIT License
 *
 * Copyright (c) 2020 Andy Young
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "program.h"

#define SHADER_STRING(text) #text

const std::string k_fb_fetch_ext_heander = "#extension GL_EXT_shader_framebuffer_fetch : enable\n";
const std::string k_fb_fetch_ext_color = "gl_LastFragData[0]";
const std::string k_fb_fetch_arm_heander = "#extension GL_ARM_shader_framebuffer_fetch : enable\n";
const std::string k_fb_fetch_arm_color = "gl_LastFragColorARM";
const std::string k_empty_string = "";
static int s_fb_fetch_type = 0; // 0 for none, 1 for ext, 2 for arm
static bool s_is_fb_fetch_checked = false;

GLProgram::GLProgram() {

}

GLProgram::~GLProgram() {
    glDeleteProgram(program_);
    program_ = -1;
}

GLProgram* GLProgram::create_by_shader_string(const std::string& vertex_shader_source,
                                              const std::string& fragment_shader_source) {
    GLProgram* ret = new (std::nothrow) GLProgram();
    if (ret != nullptr) {
        if (!ret->init_with_shader_string(vertex_shader_source, fragment_shader_source)) {
            delete ret;
            ret = nullptr;
        }
    }
    return ret;
}

bool GLProgram::init_with_shader_string(const std::string& vertex_shader_source,
                                         const std::string& fragment_shader_source) {
    if (program_ != -1) {
        glDeleteProgram(program_);
        program_ = -1;
    }
    program_ = glCreateProgram();
    
    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    const char* vertex_str = vertex_shader_source.c_str();
//    fprintf(stderr, "vertex shader:\n%s\n", vertex_str);
    glShaderSource(vertex_shader, 1, &vertex_str, NULL);
    glCompileShader(vertex_shader);
    int error = glGetError();
//    fprintf(stderr, "GLError after glCompileShader error_code[0x%x]\n", error);
    GLint res;
    glGetShaderiv(GL_VERTEX_SHADER, GL_COMPILE_STATUS, &res);
    char error_message[1024];
    int error_len = 0;
    if (GL_FALSE == res) {
        glGetShaderInfoLog(GL_FRAGMENT_SHADER, 1024, &error_len, error_message);
//        fprintf(stderr, "GLError after create vertex shader %s, error_code[0x%x]\n", error_message, error);
    }
    glGetError();
    
    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    const char* fragment_str = fragment_shader_source.c_str();
//    fprintf(stderr, "fragment shader:\n%s\n", fragment_str);
    glShaderSource(fragment_shader, 1, &fragment_str, NULL);
    glCompileShader(fragment_shader);
    error = glGetError();
//    fprintf(stderr, "GLError after glCompileShader error_code[0x%x]\n", error);
    glGetShaderiv(GL_FRAGMENT_SHADER, GL_COMPILE_STATUS, &res);
    if (GL_FALSE == res) {
        glGetShaderInfoLog(GL_FRAGMENT_SHADER, 1024, &error_len, error_message);
//        fprintf(stderr, "GLError after create fragment shader %s, error_code[0x%x]\n", error_message, error);
    }
    glGetError();

    glAttachShader(program_, vertex_shader);
    glAttachShader(program_, fragment_shader);
    
    glLinkProgram(program_);
    error = glGetError();
//    fprintf(stderr, "GLError after glLinkProgram error_code[0x%x]\n", error);
    glGetProgramiv(program_, GL_LINK_STATUS, &res);
    if (GL_FALSE == res) {
        glGetProgramInfoLog(program_, 1024, &error_len, error_message);
//        fprintf(stderr, "GLError after link program %s, error_code[0x%x]\n", error_message, error);
    }
    glGetError();
    
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    
    return true;
}

void GLProgram::use() {
    glUseProgram(program_);
}

static bool test_fragment_shader(GLuint program, const std::string fs) {
    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    const char *fragment_str = fs.c_str();
    glShaderSource(fragment_shader, 1, &fragment_str, NULL);
    glCompileShader(fragment_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);
    glUseProgram(program);
    glDetachShader(program, fragment_shader);
    glDeleteShader(fragment_shader);
    return !glGetError();
}

void GLProgram::check_fb_fetch() {
    if (s_is_fb_fetch_checked) { // already checked
        return;
    }
    
    static const std::string normal_vs = SHADER_STRING(
            attribute vec4 position;
            attribute vec4 tex_coord;
            varying vec2 v_tex_coord;
            void main() {
                gl_Position = position;
                v_tex_coord = tex_coord.xy;
            }
    );
    static const std::string ext_fs = k_fb_fetch_ext_heander + SHADER_STRING(
            precision lowp float;
            varying highp vec2 v_tex_coord;
            uniform sampler2D color_map;
            void main() {
                gl_FragColor = gl_LastFragData[0];
            }
    );
    static const std::string arm_fs = k_fb_fetch_arm_heander + SHADER_STRING(
            precision lowp float;
            varying highp vec2 v_tex_coord;
            uniform sampler2D color_map;
            void main() {
                gl_FragColor = gl_LastFragColorARM;
            }
    );

    GLuint program = glCreateProgram();
    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    const char* vertex_str = normal_vs.c_str();
    glShaderSource(vertex_shader, 1, &vertex_str, NULL);
    glCompileShader(vertex_shader);
    glAttachShader(program, vertex_shader);

    if (test_fragment_shader(program, ext_fs)) {
        s_fb_fetch_type = 1;
        //ALOGI("GLProgram", "using ext_fb_fetch_header.");
    } else if (test_fragment_shader(program, arm_fs)) {
        s_fb_fetch_type = 2;
        //ALOGI("GLProgram", "using arm_fb_fetch_header.");
    } else {
        s_fb_fetch_type = 0;
        //ALOGI("GLProgram", "using no fb_fetch_header.");
    }

    glDetachShader(program, vertex_shader);
    glDeleteShader(vertex_shader);
    glUseProgram(0);
    glDeleteProgram(program);
    s_is_fb_fetch_checked = true;
}

const std::string& GLProgram::get_fb_fetch_header() {
    switch (s_fb_fetch_type) {
        case 1 :
            return k_fb_fetch_ext_heander;
        case 2:
            return k_fb_fetch_arm_heander;
        default:
            return k_empty_string;
    }
}

const std::string& GLProgram::get_fb_fetch_color() {
    switch (s_fb_fetch_type) {
        case 1 :
            return k_fb_fetch_ext_color;
        case 2:
            return k_fb_fetch_arm_color;
        default:
            return k_empty_string;
    }
}

GLuint GLProgram::get_attrib_location(const std::string& attribute_name) {
    return glGetAttribLocation(program_, attribute_name.c_str());
}

GLuint GLProgram::get_uniform_location(const std::string& uniform_name) {
    return glGetUniformLocation(program_, uniform_name.c_str());
}

