#include <gl/glew.h>
#include <GLFW/glfw3.h>
#include <fmt/format.h>

struct buffer {
  std::size_t width{}, height{};
  uint32_t *m_data{};
};

// to get error events, events in glfw reported through callbacks
// this thing is a function pointer...
// GLFWerrorfun glfwSetErrorCallback(GLFWerrorfun cbfun);
/**  err callback func, we use it for `glfwSetErrorCallback`
 * @brief
 *
 * @param error
 * @param description
 */
auto error_callback(int error, const char *description) -> void {
  fmt::print("Error: {} msg: {}\n", error, description);
}

/**
 * @brief  sets the left-most 24 bits to the r, g, and b values
 *
 * @param red
 * @param green
 * @param blue
 * @return uint32_t
 */
inline auto rgb_uint32(uint8_t red, uint8_t green, uint8_t blue) -> uint32_t {
  return (red << 24) | (green << 16) | (blue << 8) | 255;
}

/**
 * @brief clears the buffer to a certain color
 * The function iterates over all the pixels and
 * sets each of the pixels to the given color.
 * @param bfr
 * @param color
 */
auto buffer_clear(buffer *bfr, uint32_t color) -> void {
  for (std::size_t i{}; i < bfr->width * bfr->height; ++i) {
    bfr->m_data[i] = color;
  }
}

/**
 * @brief
 OpenGL outputs various information during the compilation process, like e.g. a
C++ compiler, but we need to intercept this information. For this I created two
simple functions, validate_shader and validate_program
}
 *
 * @param shader
 * @param file
 */
void validate_shader(GLuint shader, const char *file = 0) {
  static const unsigned int BUFFER_SIZE = 512;
  char buffer[BUFFER_SIZE];
  GLsizei length = 0;

  glGetShaderInfoLog(shader, BUFFER_SIZE, &length, buffer);

  if (length > 0) {
    fmt::print(" shader {}({}) compile error: {}\n", shader, (file ? file : ""),
               buffer);
  }
}

bool validate_program(GLuint program) {
  static const GLsizei BUFFER_SIZE = 512;
  GLcharARB buffer[BUFFER_SIZE];
  GLsizei length = 0;

  glGetProgramInfoLog(program, BUFFER_SIZE, &length, buffer);

  if (length > 0) {
    fmt::print("program {} link error: {}\n", program, buffer);
    return false;
  }

  return true;
}

auto main(int argc, char *argv[]) -> int {
  glfwSetErrorCallback(error_callback);
  // init glfw
  if (!glfwInit()) {
    return -1;
  }

  const uint32_t buffer_width{224};
  const uint32_t buffer_height{256};

  // creating window
  auto window = glfwCreateWindow(buffer_width, buffer_height, "Space Invaders",
                                 nullptr, nullptr);
  if (!window) {
    glfwTerminate();
    return -1;
  }

  // making that window appear
  glfwMakeContextCurrent(window);

  // init glew
  if (glewInit() != GLEW_OK) {
    fmt::print("Error initializing `glew`\n");
    return -1;
  }
  //
  const auto clear_color{rgb_uint32(0, 128, 0)};

  //glClearColor(1.0, 0.0, 0.0, 1.0);

  buffer bfr;
  bfr.height = buffer_height;
  bfr.width = buffer_width;
  bfr.m_data = new uint32_t[buffer_width * buffer_height];
  //
  buffer_clear(&bfr, clear_color);

  // transfer image data to GPU using OpenGL texture
  GLuint buffer_texture;
  glGenTextures(1, &buffer_texture);
  glBindTexture(GL_TEXTURE_2D, buffer_texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, bfr.width, bfr.height, 0, GL_RGBA,
               GL_UNSIGNED_INT_8_8_8_8, bfr.m_data);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  //
  const char *vertex_shader =
      "\n"
      "#version 330\n"
      "\n"
      "noperspective out vec2 TexCoord;\n"
      "\n"
      "void main(void){\n"
      "\n"
      "    TexCoord.x = (gl_VertexID == 2)? 2.0: 0.0;\n"
      "    TexCoord.y = (gl_VertexID == 1)? 2.0: 0.0;\n"
      "    \n"
      "    gl_Position = vec4(2.0 * TexCoord - 1.0, 0.0, 1.0);\n"
      "}\n";

  const char *fragment_shader =
      "\n"
      "#version 330\n"
      "\n"
      "uniform sampler2D buffer;\n"
      "noperspective in vec2 TexCoord;\n"
      "\n"
      "out vec3 outColor;\n"
      "\n"
      "void main(void){\n"
      "    outColor = texture(buffer, TexCoord).rgb;\n"
      "}\n";

  // VAO
  GLuint full_screen_traingle_vao{};
  glGenVertexArrays(1, &full_screen_traingle_vao);

  // creating shaders
  GLuint shader_id = glCreateProgram();
  // vertex shader
  {
    GLuint shader_vp = glCreateShader(GL_VERTEX_SHADER);

    glShaderSource(shader_vp, 1, &vertex_shader, 0);
    glCompileShader(shader_vp);
    validate_shader(shader_vp, vertex_shader);
    glAttachShader(shader_id, shader_vp);

    glDeleteShader(shader_vp);
  }

  // fragment shader
  {
    GLuint shader_fp = glCreateShader(GL_FRAGMENT_SHADER);

    glShaderSource(shader_fp, 1, &fragment_shader, 0);
    glCompileShader(shader_fp);
    validate_shader(shader_fp, fragment_shader);
    glAttachShader(shader_id, shader_fp);

    glDeleteShader(shader_fp);
  }

  glLinkProgram(shader_id);

  if (!validate_program(shader_id)) {
    fmt::print(" error while validating shader.\n");
    glfwTerminate();
    glDeleteVertexArrays(1, &full_screen_traingle_vao);
    delete[] bfr.m_data;
    return -1;
  }
  glUseProgram(shader_id);

  /**
   * @brief We now need to attach the texture to the uniform sampler2D variable
   * in the fragment shader. OpenGL has a number of texture units to which a
   * uniform can be attached. We get the location of the uniform in the shader
   * (the uniform location can be seen as a kind of "pointer") using
   * glGetUniformLocation, and set the uniform to texture unit '0' using
   * glUniform1i
   *
   */

  GLint location = glGetUniformLocation(shader_id, "buffer");
  glUniform1i(location, 0);

  glDisable(GL_DEPTH_TEST);
  glActiveTexture(GL_TEXTURE0);
  glBindVertexArray(full_screen_traingle_vao);
  // creating the game loop
  while (!glfwWindowShouldClose(window)) {

    buffer_clear(&bfr, clear_color);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, bfr.width, bfr.height, GL_RGBA,
                    GL_UNSIGNED_INT_8_8_8_8, bfr.m_data);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 3);
    glfwSwapBuffers(window); // double buffering scheme, front to dispaly image
    // back to drawing, the buffers swappet each itr using this func
    glfwPollEvents(); // terminates the loop if user intented to
  }

  glfwDestroyWindow(window);
  glfwTerminate();

  glDeleteVertexArrays(1, &full_screen_traingle_vao);
  delete[] bfr.m_data;
  return 0;
}