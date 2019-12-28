#include "util.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "imgui/imgui_internal.h"

#define GLEW_STATIC
#define STB_IMAGE_IMPLEMENTATION

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stb/stb_image.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <fmt/format.h>
#include <freetype2/ft2build.h>
#include <freetype2/freetype/freetype.h>

using namespace std::literals;

class shader {
	unsigned id = 0;
public:
	template <typename ...Args, std::enable_if<sizeof...(Args) == 2 || sizeof...(Args) == 3>* = nullptr>
	explicit shader(Args&& ...args) {
		std::array<std::string, sizeof...(Args)> shaders{ args... };
		std::ifstream f(shaders[0]);
		auto temp = std::string{ std::istreambuf_iterator<char>{ f }, {} };
		auto source = temp.c_str();
		auto vshader = glCreateShader(GL_VERTEX_SHADER);
		auto fshader = glCreateShader(GL_FRAGMENT_SHADER);
		auto gshader = 0u;
		int success;
		char ilog[512];
		glShaderSource(vshader, 1, &source, nullptr);
		glCompileShader(vshader);
		glGetShaderiv(vshader, GL_COMPILE_STATUS, &success);
		if (!success) {
			glGetShaderInfoLog(vshader, 512, nullptr, ilog);
			std::cout << "[Error] " << shaders[0] << " shader compilation failed\n" << ilog;
			throw std::runtime_error("shader compilation failed");
		}
		f.close();
		f.open(shaders[1]);
		temp = std::string{ std::istreambuf_iterator<char>{ f }, {} };
		source = temp.c_str();
		glShaderSource(fshader, 1, &source, nullptr);
		glCompileShader(fshader);
		glGetShaderiv(fshader, GL_COMPILE_STATUS, &success);
		if (!success) {
			glGetShaderInfoLog(fshader, 512, nullptr, ilog);
			std::cout << "[Error] " << shaders[1] << " shader compilation failed\n" << ilog;
			throw std::runtime_error("shader compilation failed");
		}
		id = glCreateProgram();
		glAttachShader(id, vshader);
		glAttachShader(id, fshader);
		f.close();
		if constexpr (sizeof...(Args) == 3) {
			gshader = glCreateShader(GL_GEOMETRY_SHADER);
			f.open(shaders[2]);
			temp = std::string{ std::istreambuf_iterator<char>{ f }, {} };
			source = temp.c_str();
			glShaderSource(gshader, 1, &source, nullptr);
			glCompileShader(gshader);
			glGetShaderiv(gshader, GL_COMPILE_STATUS, &success);
			if (!success) {
				glGetShaderInfoLog(gshader, 512, nullptr, ilog);
				std::cout << "[Error] " << shaders[2] << " shader compilation failed\n" << ilog;
				throw std::runtime_error("shader compilation failed");
			}
			glAttachShader(id, gshader);
		}
		glLinkProgram(id);
		int g = 0;
		glGetProgramiv(id, GL_LINK_STATUS, &g);
		if (!g) {
			throw std::runtime_error(fmt::format("Failed generating shader program, id: {}", id));
		}
		glDeleteShader(vshader);
		glDeleteShader(fshader);
		if constexpr (sizeof...(Args) == 3) {
			glDeleteShader(gshader);
		}
	}
	shader() = default;
	shader(const shader& other) {
		id = other.id;
	}
	void use() const {
		glUseProgram(id);
	}
};

struct block {
	unsigned vao{}, vbo{};
	shader s;
	constexpr static std::array<glm::vec3, 3> cube{
		glm::vec3{ 0.0f, 0.5f, 0.0f },
		{ 0.5f, -0.5f, 0.0f },
		{ -0.5f, -0.5f, 0.0f }
	};
	explicit block(const shader& s) : s(s) {
		glGenVertexArrays(1, &vao);
		glGenBuffers(1, &vbo);
		glBindVertexArray(vao);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * cube.size(), cube.data(), GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, cube.size(), GL_FLOAT, GL_FALSE, sizeof(glm::vec3), 0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}
	block() = default;
	void draw() const {
		glBindVertexArray(vao);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		s.use();
		glDrawArrays(GL_TRIANGLES, 0, cube.size());
		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
};

class application {
	GLFWwindow* window;
	void init_imgui() const {
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		(void)io;
		ImGui::StyleColorsDark();
		ImGui_ImplGlfw_InitForOpenGL(window, true);
		ImGui_ImplOpenGL3_Init("#version 330");
	}
public:
	int init_gl() {
		if (!glfwInit()) {
			return -1;
		}
		stbi_set_flip_vertically_on_load(false);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		window = glfwCreateWindow(1280, 720, "MinecraftPP", nullptr, nullptr);
		if (!window) {
			return -2;
		}
		glfwMakeContextCurrent(window);
		if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
			return -3;
		}
		glViewport(0, 0, 1280, 720);
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		glEnable(GL_STENCIL_TEST);
		init_imgui();
		glfwSwapInterval(0);
		return 0;
	}
	int run() {
		block b{ shader {
				"../resources/shaders/v_block.glsl",
				"../resources/shaders/f_block.glsl"
			}
		};
		while (!glfwWindowShouldClose(window)) {
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
			b.draw();
			glfwPollEvents();
			glfwSwapBuffers(window);
		}
		return 0;
	}
};

int main() {
	application a{};
	if (int err; (err = a.init_gl())) {
		return err;
	}
	return a.run();
}