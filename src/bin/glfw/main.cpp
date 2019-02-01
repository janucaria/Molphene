#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif
#include <sstream>
#include <string>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <molphene/Camera.hpp>
#include <molphene/GlRenderer.hpp>
#include <molphene/Scene.hpp>
#include <molphene/MoleculeRepresentation.hpp>

static molphene::Scene scene;
static molphene::GlRenderer renderer;
static molphene::Scene::Camera camera;
static GLFWwindow* window;
static bool mouse_press = false;
static int mouse_button = -1;
static double delt_x = 0;
static double delt_y = 0;
static double last_x = 0;
static double last_y = 0;

static void error_callback(int error, const char* description)
{
  fputs(description, stderr);
}

static void
key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
  if(action == GLFW_PRESS) {
    switch(key) {
    case GLFW_KEY_ESCAPE:
      glfwSetWindowShouldClose(window, GL_TRUE);
      break;
    case GLFW_KEY_P:
      camera.projection_mode(true);
      break;
    case GLFW_KEY_O:
      camera.projection_mode(false);
      break;
    case GLFW_KEY_K:
      scene.representation = molphene::MoleculeRepresentation::spacefill;
      break;
    case GLFW_KEY_L:
      scene.representation = molphene::MoleculeRepresentation::ball_and_stick;
      break;
    }
  }
}

static void window_size_callback(GLFWwindow* window, int width, int height)
{
  scene.change_dimension(width, height);
  camera.aspect_ratio(width, height);
  camera.update_view_matrix();
}

static void
mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
  delt_x = 0;
  delt_y = 0;
  glfwGetCursorPos(window, &last_x, &last_y);

  mouse_press = action == GLFW_PRESS;
  mouse_button = button;
}

static void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos)
{
  if(mouse_press) {
    delt_x = xpos - last_x;
    delt_y = ypos - last_y;

    last_x = xpos;
    last_y = ypos;

    switch(mouse_button) {
    case GLFW_MOUSE_BUTTON_LEFT:
      scene.rotate({delt_y / 80, delt_x / 80, 0});
      break;
    case GLFW_MOUSE_BUTTON_RIGHT:

      break;
    case GLFW_MOUSE_BUTTON_MIDDLE:

      break;
    default:
      break;
    }
  }
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
  yoffset > 0 ? camera.zoom_in() : camera.zoom_out();
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
  scene.setup_graphics();
  scene.change_dimension(width, height);
  camera.aspect_ratio(width, height);
  camera.update_view_matrix();
}

auto init_window(int width, int height) -> bool
{
  glfwSetErrorCallback(error_callback);
  if(!glfwInit()) {
    return false;
  }

  window = glfwCreateWindow(width, height, "Simple example", nullptr, nullptr);
  if(!window) {
    glfwTerminate();
    return false;
  }

  glfwMakeContextCurrent(window);

  glfwSetKeyCallback(window, key_callback);
  glfwSetWindowSizeCallback(window, window_size_callback);
  glfwSetMouseButtonCallback(window, mouse_button_callback);
  glfwSetCursorPosCallback(window, cursor_pos_callback);
  glfwSetScrollCallback(window, scroll_callback);
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

  return true;
}

void render()
{
  renderer.render(scene, camera);

  glfwSwapBuffers(window);
  glfwPollEvents();
}

void main_loop()
{
#ifdef __EMSCRIPTEN__
  emscripten_set_main_loop(render, 0, 1);
#else
  while(!glfwWindowShouldClose(window)) {
    render();
  }
#endif
}

auto main(int argc, char* argv[]) -> int
{
  if(!init_window(640, 480)) {
    return EXIT_FAILURE;
  }

  int width = 0, height = 0;

  glfwGetFramebufferSize(window, &width, &height);
  scene.setup_graphics();
  scene.change_dimension(width, height);
  camera.aspect_ratio(width, height);
  camera.projection_mode(true);

  renderer.init();

  if(argc > 1) {
    std::ifstream pdbfile(argv[1]);
    if(pdbfile.is_open()) {
      std::cout << "openfile success!" << std::endl;
      struct close_guard {
        void operator()(std::ifstream* ptr) const
        {
          ptr->close();
        }
      };
      const auto close = std::unique_ptr<std::ifstream, close_guard>{&pdbfile};

      scene.open_chemdoodle_json_stream(pdbfile);
      scene.reset_mesh();
      camera.top = scene.bounding_sphere().radius() + 2;
      camera.update_view_matrix();
      main_loop();
    } else {
      std::cout << "openfile failure!" << std::endl;
    }
  } else {
    auto pdbstm = std::stringstream{};

    scene.open_chemdoodle_json_stream(pdbstm);
    scene.reset_mesh();
    camera.update_view_matrix();
    main_loop();
  }

  glfwDestroyWindow(window);
  glfwTerminate();

  return EXIT_SUCCESS;
}
