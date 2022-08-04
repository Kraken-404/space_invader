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
// err callback func, we use it for `glfw_callback`
auto error_callback(int error, const char *description) -> void {
  fmt::print("Error: {} msg: {}\n", error, description);
}

auto main(int argc, char *argv[]) -> int {
  glfwSetErrorCallback(error_callback);
  // init glfw
  if (!glfwInit()) {
    return -1;
  }
  // We need to tell GLFW that we would like a context that is at least
  // version 3.3. This is done by giving GLFW the appropriate "hints" before
  // creating the window,
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  // creating window
  auto window = glfwCreateWindow(640, 480, "Space Invaders", nullptr, nullptr);
  if (!window) {
    glfwTerminate();
    return -1;
  }
  // making that window appear
  glfwMakeContextCurrent(window);

  // init glew
  GLenum err = glewInit();
  if (err != GLEW_OK) {
    fmt::print("Error initializing `glew`\n");
    return -1;
  }
  // using a little OpenGL functions
  int opgl_version[2]{-1, 1};
  glGetIntegerv(GL_MAJOR_VERSION, &opgl_version[0]);
  glGetIntegerv(GL_MAJOR_VERSION, &opgl_version[1]);
  fmt::print("using OPGL: {}, {}", opgl_version[0], opgl_version[1]);

  // creating the game loop
  glClearColor(1.0, 1.0, 0.0, 0.0);
  while (!glfwWindowShouldClose(window)) {
    glClear(GL_COLOR_BUFFER_BIT); //
    glfwSwapBuffers(window); // double buffering scheme, front to dispaly image
    // back to drawing, the buffers swappet each itr using this func
    glfwPollEvents(); // terminates the loop if user intented to
  }
  glfwDestroyWindow(window);
  glfwTerminate();
}