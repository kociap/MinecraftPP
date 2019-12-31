#include "util.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "imgui/imgui_internal.h"

#define GLEW_STATIC
#define STB_IMAGE_IMPLEMENTATION

#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "stb/stb_image.h"
#include "fmt/format.h"

auto width = 1280;
auto height = 720;
double delta_time = 0;
double last_frame = 0;

class shader {
	unsigned id = 0;
public:
	explicit shader(const std::filesystem::path& vshader, const std::filesystem::path& fshader, const std::filesystem::path& gshader = {}) {
		std::ifstream f(vshader);
		auto temp = std::string{ std::istreambuf_iterator<char>{ f }, {}};
		auto source = temp.c_str();
		auto vshaderi = glCreateShader(GL_VERTEX_SHADER);
		auto fshaderi = glCreateShader(GL_FRAGMENT_SHADER);
		auto gshaderi = 0u;
		int success;
		char ilog[512];
		glShaderSource(vshaderi, 1, &source, nullptr);
		glCompileShader(vshaderi);
		glGetShaderiv(vshaderi, GL_COMPILE_STATUS, &success);
		if (!success) {
			glGetShaderInfoLog(vshaderi, 512, nullptr, ilog);
			std::cout << "[Error] " << vshader << " shader compilation failed\n" << ilog;
			throw std::runtime_error("shader compilation failed");
		}
		f.close();
		f.open(fshader);
		temp = std::string{ std::istreambuf_iterator<char>{ f }, {}};
		source = temp.c_str();
		glShaderSource(fshaderi, 1, &source, nullptr);
		glCompileShader(fshaderi);
		glGetShaderiv(fshaderi, GL_COMPILE_STATUS, &success);
		if (!success) {
			glGetShaderInfoLog(fshaderi, 512, nullptr, ilog);
			std::cout << "[Error] " << fshader << " shader compilation failed\n" << ilog;
			throw std::runtime_error("shader compilation failed");
		}
		id = glCreateProgram();
		glAttachShader(id, vshaderi);
		glAttachShader(id, fshaderi);
		f.close();
		if (!gshader.empty()) {
			gshaderi = glCreateShader(GL_GEOMETRY_SHADER);
			f.open(gshader);
			temp = std::string{ std::istreambuf_iterator<char>{ f }, {}};
			source = temp.c_str();
			glShaderSource(gshaderi, 1, &source, nullptr);
			glCompileShader(gshaderi);
			glGetShaderiv(gshaderi, GL_COMPILE_STATUS, &success);
			if (!success) {
				glGetShaderInfoLog(gshaderi, 512, nullptr, ilog);
				std::cout << "[Error] " << gshader << " shader compilation failed\n" << ilog;
				throw std::runtime_error("shader compilation failed");
			}
			glAttachShader(id, gshaderi);
		}
		glLinkProgram(id);
		int g = 0;
		glGetProgramiv(id, GL_LINK_STATUS, &g);
		if (!g) {
			throw std::runtime_error(fmt::format("Failed generating shader program, id: {}", id));
		}
		glDeleteShader(vshaderi);
		glDeleteShader(fshaderi);
		if (!gshader.empty()) {
			glDeleteShader(gshaderi);
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
	glm::vec3 prec_pos = glm::vec3(-5.0f, 0.0f, 0.0f);
	glm::vec3 cam_pos = glm::vec3(-5.0f, 0.0f, 0.0f);
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
	void process(double xoffset, double yoffset) {
		xoffset *= sensitivity;
		yoffset *= sensitivity;
		yaw += xoffset;
		pitch += yoffset;
		if (pitch > 89.0f) {
			pitch = 89.0f;
		}
		if (pitch < -89.0f) {
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
struct vertex {
	glm::vec3 vtx;
	glm::vec2 tx_coords;
};
struct vec3 {
	int x, y, z;
};
struct block {
	vec3 pos;
	enum class type {
		dirt,
		air
	} type;
};
struct chunk {
	unsigned vao{}, vbo{}, fvbo{};
	std::vector<block> offsets{}, foffsets{};
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
	explicit chunk(std::vector<block> offsets)
	: offsets(std::move(offsets)) {
		glGenVertexArrays(1, &vao);
		glGenBuffers(1, &vbo);
		glGenBuffers(1, &fvbo);
		glBindVertexArray(vao);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertex) * cube.size(), cube.data(), GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(vertex), reinterpret_cast<void*>(offsetof(vertex, vtx)));
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, false, sizeof(vertex), reinterpret_cast<void*>(offsetof(vertex, tx_coords)));
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
		trim();
	}
	chunk() = default;
	block& cube_at(const int x, const int y, const int z) {
		return offsets[x * 256 + y * 16 + z];
	}
	bool is_cube_visible(const block& b) {
		auto cubes = std::vector{
			cube_at(b.pos.x + 1, b.pos.y, b.pos.z),
			cube_at(b.pos.x - 1, b.pos.y, b.pos.z),
			cube_at(b.pos.x, b.pos.y + 1, b.pos.z),
			cube_at(b.pos.x, b.pos.y - 1, b.pos.z),
			cube_at(b.pos.x, b.pos.y, b.pos.z + 1),
			cube_at(b.pos.x, b.pos.y, b.pos.z - 1)
		};
		return std::count_if(cubes.begin(), cubes.end(), [](const block& x) {
			return x.type == block::type::dirt;
		}) != 6;
	}
	void draw(const texture& t, const shader& s) const {
		glBindVertexArray(vao);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBindBuffer(GL_ARRAY_BUFFER, fvbo);
		glBindTexture(GL_TEXTURE_2D, t.id);
		s.use();
		glDrawArraysInstanced(GL_TRIANGLES, 0, cube.size(), foffsets.size());
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}
	void trim() {
		for (const auto& block : offsets) {
			if (is_cube_visible(block)) {
				foffsets.emplace_back(block);
			}
		}
		/*for (const auto& c : offsets) {
			if (std::find_if(offsets.begin(), offsets.end(), [&c](const block& v) {
				return c.pos.x + 1 == v.pos.x &&
					   c.pos.y == v.pos.y &&
					   c.pos.z == v.pos.z;
			}) == offsets.end()) {
				foffsets.emplace_back(c);
				continue;
			}
			if (std::find_if(offsets.begin(), offsets.end(), [&c](const block& v) {
				return c.pos.x - 1 == v.pos.x &&
					   c.pos.y == v.pos.y &&
					   c.pos.z == v.pos.z;
			}) == offsets.end()) {
				foffsets.emplace_back(c);
				continue;
			}
			if (std::find_if(offsets.begin(), offsets.end(), [&c](const block& v) {
				return c.pos.x == v.pos.x &&
					   c.pos.y + 1 == v.pos.y &&
					   c.pos.z == v.pos.z;
			}) == offsets.end()) {
				foffsets.emplace_back(c);
				continue;
			}
			if (std::find_if(offsets.begin(), offsets.end(), [&c](const block& v) {
				return c.pos.x == v.pos.x &&
					   c.pos.y - 1 == v.pos.y &&
					   c.pos.z == v.pos.z;
			}) == offsets.end()) {
				foffsets.emplace_back(c);
				continue;
			}
			if (std::find_if(offsets.begin(), offsets.end(), [&c](const block& v) {
				return c.pos.x == v.pos.x &&
					   c.pos.y == v.pos.y &&
					   c.pos.z + 1 == v.pos.z;
			}) == offsets.end()) {
				foffsets.emplace_back(c);
				continue;
			}
			if (std::find_if(offsets.begin(), offsets.end(), [&c](const block& v) {
				return c.pos.x == v.pos.x &&
					   c.pos.y == v.pos.y &&
					   c.pos.z - 1 == v.pos.z;
			}) == offsets.end()) {
				foffsets.emplace_back(c);
				continue;
			}
		}*/
		glBindVertexArray(vao);
		glBindBuffer(GL_ARRAY_BUFFER, fvbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(block) * foffsets.size(), foffsets.data(), GL_STATIC_DRAW);
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 3, GL_INT, false, sizeof(block), 0);
		glVertexAttribDivisor(2, 1);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}
};
class application {
	GLFWwindow* window;
	inline static bool cursor_captured = true;
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
		cam.process(xoffset, yoffset);
	};
	inline static auto key_callback = [](GLFWwindow* w, int key, int scancode, int action, int mods) {
		if (key == GLFW_KEY_P && action == GLFW_RELEASE) {
			if (cursor_captured) {
				glfwSetInputMode(w, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			} else {
				glfwSetInputMode(w, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			}
			cursor_captured = !cursor_captured;
		}
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
		window = glfwCreateWindow(width, height, "MinecraftPP", nullptr, nullptr);
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
		glfwSetKeyCallback(window, key_callback);
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
		shader s{
			"../resources/shaders/v_block.glsl",
			"../resources/shaders/f_block.glsl"
		};
		auto proj = glm::perspective(glm::radians(60.f), float(width) / float(height), 0.1f, 100.f);
		s.use();
		s.set_mat4("model", glm::mat4(1.0f));
		std::vector<texture> tx_vec{
			{ "../resources/textures/dirt.jpg" }
		};
		std::vector<block> offsets{}, offsets2{};
		for (int i = 0; i < 16; ++i) {
			for (int j = 0; j < 16; ++j) {
				for (int k = 0; k < 16; ++k) {
					offsets.emplace_back(block{ vec3{ i, j, k }, block::type::dirt });
				}
			}
		}
		for (int i = 16; i < 32; ++i) {
			for (int j = 0; j < 16; ++j) {
				for (int k = 0; k < 16; ++k) {
					offsets2.emplace_back(block{ vec3{ i, j, k }, block::type::dirt });
				}
			}
		}
		std::vector<chunk> chunks{
			chunk{ std::move(offsets) },
			chunk{ std::move(offsets2) }
		};
		static int nframes = 0;
		while (!glfwWindowShouldClose(window)) {
			nframes++;
			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
			double current_frame = glfwGetTime();
			delta_time = current_frame - last_frame;
			last_frame = current_frame;
			s.use();
			s.set_mat4("pv_mat", proj * cam.get_view_mat());
			for (const auto& chunk : chunks) {
				chunk.draw(tx_vec[0], s);
			}
			ImGui::SetNextWindowPos(ImVec2{ float(width) - 400.0f, 0 });
			ImGui::SetNextWindowSize({ 400.0f, 0.0f });
			ImGui::Begin("Debug info");
			ImGui::Text("fps: %.2f, delta_time: %f, frame: %d\n"
			   "x: %.2f, y: %.2f, z: %.2f, is_moving: %s",
				1 / delta_time, delta_time, nframes,
				cam.cam_pos.x, cam.cam_pos.y, cam.cam_pos.z,
				cam.has_moved() ? "true" : "false");
			ImGui::End();
			ImGui::Render();
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
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