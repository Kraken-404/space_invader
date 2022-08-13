#include <gl/glew.h>
#include <GLFW/glfw3.h>
#include <array>
#include <fmt/format.h>
#include <memory>
#include <utility>
#include <vector>


// Globals:
bool game_running{false};
int move_dir{};
bool fire_pressed{};

struct Buffer {
  std::size_t width{}, height{};
  std::vector<uint32_t> m_data{};
};

// 8 bit character
struct Sprite {
  std::size_t width{}, height{};
  std::vector<uint8_t> m_data{};
};

struct Alien {
  std::size_t x{}, y{};
  uint8_t type{};
};

struct Player {
  std::size_t x{}, y{};
  std::size_t lives{};
};

struct Bullet {
  std::size_t x{}, y{};
  int dir{};
};

constexpr int game_max_bullets{128};

struct Game {
  std::size_t width{}, height{};
  std::size_t num_aliens{};
  std::vector<Alien> aliens{};
  Player player{};
  std::array<Bullet, game_max_bullets> bullets{};
};

struct Sprite_animation {
  bool loop{};
  std::size_t num_frames{};
  std::size_t frame_duration{};
  std::size_t time{};
  std::vector<Sprite *> frames{};
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

auto key_callback(GLFWwindow *window, int key, int scancode, int action,
                  int mods) -> void {
  switch (key) {
  case GLFW_KEY_ESCAPE:
    if (action == GLFW_PRESS) {
      game_running = false;
      break;
    }
  case GLFW_KEY_D: {
    if (action == GLFW_PRESS)
      move_dir += 1;
    else if (action == GLFW_RELEASE)
      move_dir -= 1;
    break;
  }
  case GLFW_KEY_A: {
    if (action == GLFW_PRESS)
      move_dir -= 1;
    else if (action == GLFW_RELEASE)
      move_dir += 1;
    break;
  }
  case GLFW_KEY_SPACE: {
    if (action == GLFW_PRESS)
      fire_pressed = true;
    break;
  }

  default:
    break;
  }
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
auto buffer_clear(Buffer &bfr, uint32_t color) -> void {
  for (auto i{0u}; i < bfr.width * bfr.height; ++i) {
    bfr.m_data[i] = color;
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
auto validate_shader(GLuint shader, const char *file = 0) -> void {
  static const unsigned int BUFFER_SIZE = 512;
  char buffer[BUFFER_SIZE];
  GLsizei length = 0;

  glGetShaderInfoLog(shader, BUFFER_SIZE, &length, buffer);

  if (length > 0) {
    fmt::print(" shader {}({}) compile error: {}\n", shader, (file ? file : ""),
               buffer);
  }
}

auto validate_program(GLuint program) -> bool {
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

auto buf_sprt_draw(Buffer &bfr, const Sprite &sprt, std::size_t x,
                   std::size_t y, uint32_t color) -> void {
  for (size_t xi = 0; xi < sprt.width; ++xi) {
    for (size_t yi = 0; yi < sprt.height; ++yi) {
      auto sy = sprt.height - 1 + y - yi;
      auto sx = x + xi;
      if (sprt.m_data[yi * sprt.width + xi] && sy < bfr.height &&
          sx < bfr.width) {
        bfr.m_data[sy * bfr.width + sx] = color;
      }
    }
  }
}

auto main(int argc, char *argv[]) -> int {
  glfwSetErrorCallback(error_callback);
  // init glfw
  if (!glfwInit()) {
    return -1;
  }

  constexpr uint32_t buffer_width{224};
  constexpr uint32_t buffer_height{256};

  // creating window
  auto window = glfwCreateWindow(buffer_width, buffer_height, "Space Invaders",
                                 nullptr, nullptr);
  if (!window) {
    glfwTerminate();
    return -1;
  }

  // using keys
  glfwSetKeyCallback(window, key_callback);

  // making that window appear
  glfwMakeContextCurrent(window);

  // init glew
  if (glewInit() != GLEW_OK) {
    fmt::print("Error initializing `glew`\n");
    return -1;
  }
  //
  const auto clear_color{rgb_uint32(32, 120, 100)};

  // glClearColor(1.0, 0.0, 0.0, 1.0);
  // Create graphics buffer
  Buffer bfr;
  bfr.height = buffer_height;
  bfr.width = buffer_width;
  bfr.m_data.resize(buffer_width * buffer_height);
  //
  buffer_clear(bfr, clear_color);
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
  GLuint full_screen_triangle_vao{};
  glGenVertexArrays(1, &full_screen_triangle_vao);
  glBindVertexArray(full_screen_triangle_vao); // bind obj with the name

  // transfer image data to GPU using OpenGL texture
  GLuint buffer_texture;
  glGenTextures(1, &buffer_texture);
  glBindTexture(GL_TEXTURE_2D, buffer_texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, bfr.width, bfr.height, 0, GL_RGBA,
               GL_UNSIGNED_INT_8_8_8_8, bfr.m_data.data());
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

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
    glDeleteVertexArrays(1, &full_screen_triangle_vao);
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

  GLint location = glGetUniformLocation(
      shader_id, "bfr"); // Returns the location of a uniform variable
  glUniform1i(location, 0);
  glDisable(GL_DEPTH_TEST); // disable server-side GL capabilities
  glActiveTexture(GL_TEXTURE0);

  // creating player
  Sprite player_sprite{};
  player_sprite.width = 11;
  player_sprite.height = 7;
  player_sprite.m_data.resize(player_sprite.height * player_sprite.width);
  player_sprite.m_data = {
      0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, // .....@.....
      0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, // ....@@@....
      0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, // ....@@@....
      0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, // .@@@@@@@@@.
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // @@@@@@@@@@@
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // @@@@@@@@@@@
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // @@@@@@@@@@@
  };

  // creating bullet sprite
  Sprite bullet_sprite;
  bullet_sprite.width = 1;
  bullet_sprite.height = 3;
  bullet_sprite.m_data.resize(bullet_sprite.width
                        * bullet_sprite.height);
  bullet_sprite.m_data = {
      1, //@
      1, //@
      1  //@
  };

  // init game struct
  Game game{};
  game.width = bfr.width;
  game.height = bfr.height;
  game.num_aliens = 55;
  game.aliens = std::vector<Alien>(game.num_aliens);
  game.player.x = 112 - 5;
  game.player.y = 32;
  game.player.lives = 3;

  // creating sprite alien
  Sprite alien_sprite0;
  alien_sprite0.width = 11;
  alien_sprite0.height = 8;
  alien_sprite0.m_data.resize(alien_sprite0.width * alien_sprite0.height);
  alien_sprite0.m_data = {
      0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, // ..@.....@..
      0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, // ...@...@...
      0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, // ..@@@@@@@..
      0, 1, 1, 0, 1, 1, 1, 0, 1, 1, 0, // .@@.@@@.@@.
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // @@@@@@@@@@@
      1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, // @.@@@@@@@.@
      1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 1, // @.@.....@.@
      0, 0, 0, 1, 1, 0, 1, 1, 0, 0, 0  // ...@@.@@...
  };
  //

  Sprite alien_sprite1{};
  alien_sprite1.width = 11;
  alien_sprite1.height = 8;
  alien_sprite1.m_data.resize(alien_sprite1.width * alien_sprite1.height);
  alien_sprite1.m_data = {
      0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, // ..@.....@..
      1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, // @..@...@..@
      1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, // @.@@@@@@@.@
      1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, // @@@.@@@.@@@
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // @@@@@@@@@@@
      0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, // .@@@@@@@@@.
      0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, // ..@.....@..
      0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0  // .@.......@.
  };

  // positioning aliens
  for (size_t i = 0; i < 5; ++i) {
    for (size_t j = 0; j < 11; ++j) {
      game.aliens[i * 11 + j].x = 17 * j + 22;
      game.aliens[i * 11 + j].y = 17 * i + 128;
    }
  }

  // creating animation for the aliens

  auto alien_animation = std::make_unique<Sprite_animation>();
  alien_animation->loop = true;
  alien_animation->num_frames = 2;
  alien_animation->frame_duration = 10;
  alien_animation->time = 0;

  // defining frames
  alien_animation->frames.resize(2);
  alien_animation->frames[0] = &alien_sprite0;
  alien_animation->frames[1] = &alien_sprite1;

  // V-Sync
  glfwSwapInterval(1);

  // control player direction movement
  int player_move = 0;
  // indicates the game is still running
  game_running = true;
  // creating the game loop
  while (!glfwWindowShouldClose(window) && game_running) {

    buffer_clear(bfr, clear_color);

    // drawing bullets
    for (const auto &bullets : game.bullets) {
      buf_sprt_draw(bfr, bullet_sprite, bullets.x, bullets.y, rgb_uint32(128, 0, 0));
    }
    // positioning bullets
    for (auto &bullets : game.bullets) {
      bullets.y += bullets.dir;
      //
      if (bullets.y >= game.height || bullets.y < bullet_sprite.height) {
        bullets.dir = 0;
      }
    }
    // drawing alien spites each animation
    for (const auto &alien : game.aliens) {
      std::size_t curr_frame =
          alien_animation->time / alien_animation->frame_duration;
      const Sprite &sprite = *alien_animation->frames[curr_frame];
      buf_sprt_draw(bfr, sprite, alien.x, alien.y, rgb_uint32(128, 0, 0));
    }

    // drawing player one time
    buf_sprt_draw(bfr, player_sprite, game.player.x, game.player.y,
                  rgb_uint32(128, 0, 0));

    // alien animation:
    ++alien_animation->time;
    if (alien_animation->time ==
        alien_animation->num_frames * alien_animation->frame_duration) {
      if (alien_animation->loop)
        alien_animation->time = 0;
      else {
        alien_animation.release();
        alien_animation.reset();
      }
    }

    // player movement
    player_move = 2 * move_dir;
    if (player_move != 0) {
      if (game.player.x + player_sprite.width + player_move >= game.width) {
        game.player.x = game.width - player_sprite.width;
      } else if (static_cast<int>(game.player.x) + player_move <= 0) {
        game.player.x = 0;
      } else {
        game.player.x += player_move;
      }
    }

    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, bfr.width, bfr.height, GL_RGBA,
                    GL_UNSIGNED_INT_8_8_8_8, bfr.m_data.data());
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 3);
    glfwSwapBuffers(window); // double buffering scheme, front to display image
    // back to drawing, the buffers swapped each itr using this func
    glfwPollEvents(); // terminates the loop if user intended to
  } // end game loop

  glfwDestroyWindow(window);
  glfwTerminate();
  glDeleteVertexArrays(1, &full_screen_triangle_vao);
}