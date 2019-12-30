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

auto width = 1280;
auto height = 720;
double delta_time = 0;
double last_frame = 0;

class shader {
	unsigned id = 0;
public:
	template <typename ...Args,
	    std::enable_if<sizeof...(Args) == 2 || sizeof...(Args) == 3>* = nullptr>
	explicit shader(Args&& ...args) {
		std::array<std::string, sizeof...(Args)> shaders{ args... };
		std::ifstream f(shaders[0]);
		auto temp = std::string{ std::istreambuf_iterator<char>{ f }, {}};
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
		temp = std::string{ std::istreambuf_iterator<char>{ f }, {}};
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
		if constexpr (shaders.size() == 3) {
			gshader = glCreateShader(GL_GEOMETRY_SHADER);
			f.open(shaders[2]);
			temp = std::string{ std::istreambuf_iterator<char>{ f }, {}};
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
	void set_mat4(const char* name, glm::mat4 mat) {
		use();
		glUniformMatrix4fv(glGetUniformLocation(id, name), 1, false, glm::value_ptr(mat));
	}
};
struct camera {
	static constexpr float speed = 4.f;
	static constexpr float sensitivity = 10.0e-2f;
	double yaw;
	double pitch;
	glm::vec3 prec_pos = glm::vec3(0.0f, 0.0f, 3.0f);
	glm::vec3 cam_pos = glm::vec3(0.0f, 0.0f, 3.0f);
	glm::vec3 cam_front = glm::vec3(0.0f, 0.0f, -1.0f);
	glm::vec3 cam_up = glm::vec3(0.0f, 1.0f, 0.0f);
	glm::vec3 cam_right = glm::vec3();
	glm::vec3 cam_wup = glm::vec3(0.0f, 1.0f, 0.0f);
	camera()
		: yaw(-90.0f),
		pitch(0.0f) {
		update();
	}
	bool has_moved() {
		return prec_pos == cam_pos ? false : (prec_pos = cam_pos, true);
	}
	void move(GLFWwindow* window) {
		float velocity = speed * delta_time;
		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
			cam_pos.x += cos(glm::radians(yaw)) * velocity;
			cam_pos.z += sin(glm::radians(yaw)) * velocity;
		}
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
			cam_pos.x -= cos(glm::radians(yaw)) * velocity;
			cam_pos.z -= sin(glm::radians(yaw)) * velocity;
		}
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
			cam_pos -= cam_right * velocity;
		}
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
			cam_pos += cam_right * velocity;
		}
		if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
			cam_pos += cam_wup * velocity;
		}
		if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
			cam_pos -= cam_wup * velocity;
		}
		update();
	}
	auto get_view_mat() const {
		return glm::lookAt(cam_pos, cam_pos + cam_front, cam_up);
	}
	void process(double xoffset, double yoffset, const GLboolean constrain_pitch = true) {
		xoffset *= sensitivity;
		yoffset *= sensitivity;
		yaw += xoffset;
		pitch += yoffset;
		if (constrain_pitch) {
			if (pitch > 89.0f)
				pitch = 89.0f;
			if (pitch < -89.0f)
				pitch = -89.0f;
		}
		update();
	}
	void update() {
		glm::vec3 front{
			cos(glm::radians(yaw)) * cos(glm::radians(pitch)),
			sin(glm::radians(pitch)),
			sin(glm::radians(yaw)) * cos(glm::radians(pitch))
		};
		cam_front = glm::normalize(front);
		cam_right = glm::normalize(glm::cross(cam_front, cam_wup));
		cam_up = glm::normalize(glm::cross(cam_right, cam_front));
	}
} static cam{};
struct vertex {
	glm::vec3 vtx;
	glm::vec2 tx_coords;
};
struct texture {
	unsigned id;
	texture() = default;
	texture(const std::filesystem::path& path) {
		glGenTextures(1, &id);
		int pwidth, lheight, nr_components;
		auto data = stbi_load(path.c_str(), &pwidth, &lheight, &nr_components, 0);
		if (data) {
			GLenum format = 0;
			if (nr_components == 1) {
				format = GL_RED;
			} else if (nr_components == 3) {
				format = GL_RGB;
			} else if (nr_components == 4) {
				format = GL_RGBA;
			}
			glBindTexture(GL_TEXTURE_2D, id);
			glTexImage2D(GL_TEXTURE_2D, 0, format, pwidth, lheight, 0, format, GL_UNSIGNED_BYTE, data);
			glGenerateMipmap(GL_TEXTURE_2D);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			stbi_image_free(data);
		} else {
			std::cout << fmt::format("Texture failed to load at path: {}", path.string());
			assert(false);
		}
	}
};

struct chunk {
	unsigned vao{}, vbo{};
	shader s;
	std::vector<glm::vec3> offsets;
	constexpr static std::array<vertex, 36> cube{{
		{{ -0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f }},
		{{ 0.5f, -0.5f, -0.5f }, { 1.0f, 0.0f }},
		{{ 0.5f, 0.5f, -0.5f }, { 1.0f, 1.0f }},
		{{ 0.5f, 0.5f, -0.5f }, { 1.0f, 1.0f }},
		{{ -0.5f, 0.5f, -0.5f }, { 0.0f, 1.0f }},
		{{ -0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f }},

		{{ -0.5f, -0.5f, 0.5f }, { 0.0f, 0.0f }},
		{{ 0.5f, -0.5f, 0.5f }, { 1.0f, 0.0f }},
		{{ 0.5f, 0.5f, 0.5f }, { 1.0f, 1.0f }},
		{{ 0.5f, 0.5f, 0.5f }, { 1.0f, 1.0f }},
		{{ -0.5f, 0.5f, 0.5f }, { 0.0f, 1.0f }},
		{{ -0.5f, -0.5f, 0.5f }, { 0.0f, 0.0f }},

		{{ -0.5f, 0.5f, 0.5f }, { 1.0f, 0.0f }},
		{{ -0.5f, 0.5f, -0.5f }, { 1.0f, 1.0f }},
		{{ -0.5f, -0.5f, -0.5f }, { 0.0f, 1.0f }},
		{{ -0.5f, -0.5f, -0.5f }, { 0.0f, 1.0f }},
		{{ -0.5f, -0.5f, 0.5f }, { 0.0f, 0.0f }},
		{{ -0.5f, 0.5f, 0.5f }, { 1.0f, 0.0f }},

		{{ 0.5f, 0.5f, 0.5f }, { 1.0f, 0.0f }},
		{{ 0.5f, 0.5f, -0.5f }, { 1.0f, 1.0f }},
		{{ 0.5f, -0.5f, -0.5f }, { 0.0f, 1.0f }},
		{{ 0.5f, -0.5f, -0.5f }, { 0.0f, 1.0f }},
		{{ 0.5f, -0.5f, 0.5f }, { 0.0f, 0.0f }},
		{{ 0.5f, 0.5f, 0.5f }, { 1.0f, 0.0f }},

		{{ -0.5f, -0.5f, -0.5f }, { 0.0f, 1.0f }},
		{{ 0.5f, -0.5f, -0.5f }, { 1.0f, 1.0f }},
		{{ 0.5f, -0.5f, 0.5f }, { 1.0f, 0.0f }},
		{{ 0.5f, -0.5f, 0.5f }, { 1.0f, 0.0f }},
		{{ -0.5f, -0.5f, 0.5f }, { 0.0f, 0.0f }},
		{{ -0.5f, -0.5f, -0.5f }, { 0.0f, 1.0f }},

		{{ -0.5f, 0.5f, -0.5f }, { 0.0f, 1.0f }},
		{{ 0.5f, 0.5f, -0.5f }, { 1.0f, 1.0f }},
		{{ 0.5f, 0.5f, 0.5f }, { 1.0f, 0.0f }},
		{{ 0.5f, 0.5f, 0.5f }, { 1.0f, 0.0f }},
		{{ -0.5f, 0.5f, 0.5f }, { 0.0f, 0.0f }},
		{{ -0.5f, 0.5f, -0.5f }, { 0.0f, 1.0f }}
	}};
	explicit chunk(const shader& s, std::vector<glm::vec3> offsets) :
	s(s), offsets(std::move(offsets)) {
		glGenVertexArrays(1, &vao);
		glGenBuffers(1, &vbo);
		glBindVertexArray(vao);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertex) * cube.size() + sizeof(glm::vec3) * this->offsets.size(), nullptr, GL_STATIC_DRAW);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertex) * cube.size(), cube.data());
		glBufferSubData(GL_ARRAY_BUFFER, sizeof(vertex) * cube.size(), sizeof(glm::vec3) * this->offsets.size(), this->offsets.data());
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(vertex), reinterpret_cast<void*>(offsetof(vertex, vtx)));
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, false, sizeof(vertex), reinterpret_cast<void*>(offsetof(vertex, tx_coords)));
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 3, GL_FLOAT, false, sizeof(glm::vec3), reinterpret_cast<void*>(sizeof(vertex) * cube.size()));
		glVertexAttribDivisor(2, 1);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
		this->s.use();
		this->s.set_mat4("projection", glm::perspective(glm::radians(60.f), float(width) / float(height), 0.1f, 100.f));
		this->s.set_mat4("view", glm::mat4(1.0f));
		this->s.set_mat4("model", glm::mat4(1.0f));
	}
	chunk() = default;
	void draw(const texture& t) const {
		glBindVertexArray(vao);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		s.use();
		glBindTexture(GL_TEXTURE_2D, t.id);
		glDrawArrays(GL_TRIANGLES, 0, cube.size());
		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
	void set_projection(const glm::mat4& mat) {
		s.use();
		s.set_mat4("projection", mat);
	}
	void set_view(const glm::mat4& mat) {
		s.use();
		s.set_mat4("view", mat);
	}
	void set_model(const glm::mat4& mat) {
		s.use();
		s.set_mat4("model", mat);
	}
};
class application {
	GLFWwindow* window;
	std::vector<texture> tx_vec;
	inline static auto framebuffer_callback = [](GLFWwindow*, const int fwidth, const int fheight) {
		glViewport(0, 0, width = fwidth, height = fheight);
	};
	inline static auto mouse_callback = [](GLFWwindow*, const double xpos, const double ypos) {
		static double lastX = height / 2.0, lastY = width / 2.0;
		static bool first = true;
		if (first) {
			lastX = xpos;
			lastY = ypos;
			first = false;
		}
		double xoffset = xpos - lastX;
		double yoffset = lastY - ypos;
		lastX = xpos;
		lastY = ypos;
		cam.process(xoffset, yoffset, true);
	};
	void init_imgui() const {
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		(void) io;
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
		window = glfwCreateWindow(width = 1280, height = 720, "MinecraftPP", nullptr, nullptr);
		if (!window) {
			return -2;
		}
		glfwMakeContextCurrent(window);
		if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
			return -3;
		}
		glViewport(0, 0, width, height);
		glfwSetFramebufferSizeCallback(window, framebuffer_callback);
		glfwSetCursorPosCallback(window, mouse_callback);
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		glEnable(GL_STENCIL_TEST);
		init_imgui();
		glfwSwapInterval(0);
		return 0;
	}
	void process_input() {
		cam.move(window);
		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
			glfwSetWindowShouldClose(window, true);
	}
	int run() {
		tx_vec.emplace_back("../resources/textures/dirt.jpg");
		chunk b{
			shader{
				"../resources/shaders/v_block.glsl",
				"../resources/shaders/f_block.glsl"
			},
			{ glm::vec3{ 0.f, 0.f, -50.0f } }
		};
		while (!glfwWindowShouldClose(window)) {
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
			double current_frame = glfwGetTime();
			delta_time = current_frame - last_frame;
			last_frame = current_frame;
			b.set_view(cam.get_view_mat());
			b.draw(tx_vec[0]);
			process_input();
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