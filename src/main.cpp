#include "util.hpp"

#include <vec3.hpp>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "imgui/imgui_internal.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "fmt/format.h"

#if _MSC_VER
	#define NO_MIN_MAX
	#define WIN_LEAN_AND_MEAN
	#include <Windows.h>
	#define DEBUG_BREAK() DebugBreak()
#else
	#define DEBUG_BREAK() std::abort()
#endif

namespace minecraftpp {
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
			int linking_status = 0;
			glGetProgramiv(id, GL_LINK_STATUS, &linking_status);
			if (linking_status == GL_FALSE) {
				std::string message = get_shader_linking_info(id);
				throw std::runtime_error(std::move(message));
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

	private:
		std::string get_shader_linking_info(u32 const program) {
			i32 log_length;
			glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_length);
			std::string log(log_length, '\0');
			glGetProgramInfoLog(program, log_length, &log_length, &log[0]);
			return log;
		}
	};
	struct camera {
		static constexpr float speed = 4.f;
		static constexpr float sensitivity = 10.0e-2f;
		double yaw;
		double pitch;
		glm::vec3 prec_pos = glm::vec3(-5.0f, 0.0f, 0.0f);
		glm::vec3 cam_pos = glm::vec3(-5.0f, 5.0f, 0.0f);
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
			std::string const path_as_string = path.generic_string();
			auto data = stbi_load(path_as_string.c_str(), &pwidth, &lheight, &nr_components, 0);
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

	struct Vertex {
		glm::vec3 position;
		glm::vec2 tex_coords;
	};

	enum class Block_Type {
		air, dirt,
	};

	bool is_opaque(Block_Type const block) {
		return block == Block_Type::dirt;
	}

	struct Chunk {
		vec3 position;
		std::array<Block_Type, 4096> blocks;

		Chunk() = default;
		explicit Chunk(std::array<Block_Type, 4096> blocks): blocks(blocks) {}

		Block_Type block_at(i32 const x, i32 const y, i32 const z) const {
			if(x < 0 || x > 15 || y < 0 || y > 15 || z < 0 || z > 15) {
				return Block_Type::air;
			} else {
				return blocks[z * 256 + y * 16 + x];
			}
		}
	};

	std::vector<vec3> trim(Chunk const& chunk) {
		std::vector<vec3> offsets;
		for(i32 x = 0; x < 16; ++x) {
			for(i32 y = 0; y < 16; ++y) {
				for(i32 z = 0; z < 16; ++z) {
					bool visible = is_opaque(chunk.block_at(x, y, z)) &&
								   (!is_opaque(chunk.block_at(x - 1, y, z)) || !is_opaque(chunk.block_at(x + 1, y, z)) || 
								   !is_opaque(chunk.block_at(x, y - 1, z)) || !is_opaque(chunk.block_at(x, y + 1, z)) || 
								   !is_opaque(chunk.block_at(x, y, z - 1)) || !is_opaque(chunk.block_at(x, y, z + 1)));
					if(visible) {
						offsets.push_back(chunk.position + vec3(x, y, z));
					}
				}
			}
		}

		return offsets;
	}

	std::array<Vertex, 36> generate_nonindexed_cube_geometry() {
		return {
			Vertex{{ -0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f }},
			Vertex{{ 0.5f, -0.5f, -0.5f }, { 1.0f, 0.0f }},
			Vertex{{ 0.5f, 0.5f, -0.5f }, { 1.0f, 1.0f }},
			Vertex{{ 0.5f, 0.5f, -0.5f }, { 1.0f, 1.0f }},
			Vertex{{ -0.5f, 0.5f, -0.5f }, { 0.0f, 1.0f }},
			Vertex{{ -0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f }},

			Vertex{{ -0.5f, -0.5f, 0.5f }, { 0.0f, 0.0f }},
			Vertex{{ 0.5f, -0.5f, 0.5f }, { 1.0f, 0.0f }},
			Vertex{{ 0.5f, 0.5f, 0.5f }, { 1.0f, 1.0f }},
			Vertex{{ 0.5f, 0.5f, 0.5f }, { 1.0f, 1.0f }},
			Vertex{{ -0.5f, 0.5f, 0.5f }, { 0.0f, 1.0f }},
			Vertex{{ -0.5f, -0.5f, 0.5f }, { 0.0f, 0.0f }},

			Vertex{{ -0.5f, 0.5f, 0.5f }, { 1.0f, 0.0f }},
			Vertex{{ -0.5f, 0.5f, -0.5f }, { 1.0f, 1.0f }},
			Vertex{{ -0.5f, -0.5f, -0.5f }, { 0.0f, 1.0f }},
			Vertex{{ -0.5f, -0.5f, -0.5f }, { 0.0f, 1.0f }},
			Vertex{{ -0.5f, -0.5f, 0.5f }, { 0.0f, 0.0f }},
			Vertex{{ -0.5f, 0.5f, 0.5f }, { 1.0f, 0.0f }},

			Vertex{{ 0.5f, 0.5f, 0.5f }, { 1.0f, 0.0f }},
			Vertex{{ 0.5f, 0.5f, -0.5f }, { 1.0f, 1.0f }},
			Vertex{{ 0.5f, -0.5f, -0.5f }, { 0.0f, 1.0f }},
			Vertex{{ 0.5f, -0.5f, -0.5f }, { 0.0f, 1.0f }},
			Vertex{{ 0.5f, -0.5f, 0.5f }, { 0.0f, 0.0f }},
			Vertex{{ 0.5f, 0.5f, 0.5f }, { 1.0f, 0.0f }},

			Vertex{{ -0.5f, -0.5f, -0.5f }, { 0.0f, 1.0f }},
			Vertex{{ 0.5f, -0.5f, -0.5f }, { 1.0f, 1.0f }},
			Vertex{{ 0.5f, -0.5f, 0.5f }, { 1.0f, 0.0f }},
			Vertex{{ 0.5f, -0.5f, 0.5f }, { 1.0f, 0.0f }},
			Vertex{{ -0.5f, -0.5f, 0.5f }, { 0.0f, 0.0f }},
			Vertex{{ -0.5f, -0.5f, -0.5f }, { 0.0f, 1.0f }},

			Vertex{{ -0.5f, 0.5f, -0.5f }, { 0.0f, 1.0f }},
			Vertex{{ 0.5f, 0.5f, -0.5f }, { 1.0f, 1.0f }},
			Vertex{{ 0.5f, 0.5f, 0.5f }, { 1.0f, 0.0f }},
			Vertex{{ 0.5f, 0.5f, 0.5f }, { 1.0f, 0.0f }},
			Vertex{{ -0.5f, 0.5f, 0.5f }, { 0.0f, 0.0f }},
			Vertex{{ -0.5f, 0.5f, -0.5f }, { 0.0f, 1.0f }}
		};
	}

	static void debug_callback(GLenum const source, GLenum const type, GLuint, GLenum const severity, GLsizei, GLchar const* const message, void const*) {
        auto stringify_source = [](GLenum const source) -> char const* {
            switch (source) {
                case GL_DEBUG_SOURCE_API:
                    return "API";
                case GL_DEBUG_SOURCE_APPLICATION:
                    return "Application";
                case GL_DEBUG_SOURCE_SHADER_COMPILER:
                    return "Shader Compiler";
                case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
                    return "Window System";
                case GL_DEBUG_SOURCE_THIRD_PARTY:
                    return "Third Party";
                case GL_DEBUG_SOURCE_OTHER:
                    return "Other";
                default:
                    MINECRAFTPP_UNREACHABLE();
            }
        };

        auto stringify_type = [](GLenum const type) -> char const* {
            switch (type) {
                case GL_DEBUG_TYPE_ERROR:
                    return "Error";
                case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
                    return "Deprecated Behavior";
                case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
                    return "Undefined Behavior";
                case GL_DEBUG_TYPE_PORTABILITY:
                    return "Portability";
                case GL_DEBUG_TYPE_PERFORMANCE:
                    return "Performance";
                case GL_DEBUG_TYPE_MARKER:
                    return "Marker";
                case GL_DEBUG_TYPE_PUSH_GROUP:
                    return "Push Group";
                case GL_DEBUG_TYPE_POP_GROUP:
                    return "Pop Group";
                case GL_DEBUG_TYPE_OTHER:
                    return "Other";
                default:
                    MINECRAFTPP_UNREACHABLE();
            }
        };

        auto stringify_severity = [](GLenum const severity) -> char const* {
            switch (severity) {
                case GL_DEBUG_SEVERITY_HIGH:
                    return "Fatal Error";
                case GL_DEBUG_SEVERITY_MEDIUM:
                    return "Error";
                case GL_DEBUG_SEVERITY_LOW:
                    return "Warning";
                case GL_DEBUG_SEVERITY_NOTIFICATION:
                    return "Note";
                default:
                    MINECRAFTPP_UNREACHABLE();
            }
        };

        std::cout << stringify_severity(severity) << " " << stringify_source(source) << " (" << stringify_type(type) << "): " << message << std::flush;
        if (severity == GL_DEBUG_SEVERITY_HIGH || severity == GL_DEBUG_SEVERITY_MEDIUM) {
            DEBUG_BREAK();
        }
    }

	void install_debug_callback() {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(debug_callback, nullptr);
	}

	class application {
		// Rendering
		u32 vao;
		u32 vbo;
		i64 block_data_offset = -1;

		// Windowing
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
			
			install_debug_callback();
			glEnable(GL_DEPTH_TEST);
			glEnable(GL_BLEND);
			glEnable(GL_STENCIL_TEST);


			glGenVertexArrays(1, &vao);
			glBindVertexArray(vao);
			glEnableVertexAttribArray(0);
			glVertexAttribFormat(0, 3, GL_FLOAT, false, offsetof(Vertex, position));
			glVertexAttribBinding(0, 0);
			glEnableVertexAttribArray(1);
			glVertexAttribFormat(1, 2, GL_FLOAT, false, offsetof(Vertex, tex_coords));
			glVertexAttribBinding(1, 0);
			glEnableVertexAttribArray(2);
			glVertexAttribFormat(2, 3, GL_FLOAT, false, 0);
			glVertexAttribBinding(2, 1);
			glVertexBindingDivisor(1, 1);

			glGenBuffers(1, &vbo);
			glBindBuffer(GL_ARRAY_BUFFER, vbo);
			i64 const buffer_size = 36 * sizeof(Vertex) + 4194304 * sizeof(vec3);
			glBufferStorage(GL_ARRAY_BUFFER, buffer_size,nullptr, GL_DYNAMIC_STORAGE_BIT);
			std::array<Vertex, 36> const cube_geom = generate_nonindexed_cube_geometry();
			glBufferSubData(GL_ARRAY_BUFFER, 0, 36 * sizeof(Vertex), cube_geom.data());
			block_data_offset = 36 * sizeof(Vertex);

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
				"shaders/v_block.glsl",
				"shaders/f_block.glsl"
			};
			auto proj = glm::perspective(glm::radians(60.f), float(width) / float(height), 0.1f, 100.f);
			s.use();
			s.set_mat4("model", glm::mat4(1.0f));
			std::vector<texture> tex_vec{
				{ "textures/dirt.jpg" }
			};

			Chunk chunk0;
			for(i32 i = 0; i < 4096; ++i) {
				chunk0.blocks[i] = Block_Type::dirt;
			}

			std::vector<Chunk> chunks{chunk0};
			static int nframes = 0;
			while (!glfwWindowShouldClose(window)) {
				double current_frame = glfwGetTime();
				delta_time = current_frame - last_frame;
				last_frame = current_frame;

				glfwPollEvents();
				process_input();

				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
				glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

				s.use();
				s.set_mat4("pv_mat", proj * cam.get_view_mat());

				{
					glBindVertexArray(vao);
					glBindBuffer(GL_ARRAY_BUFFER, vbo);
					glBindVertexBuffer(0, vbo, 0, sizeof(Vertex));
					glBindVertexBuffer(1, vbo, 36 * sizeof(Vertex), sizeof(vec3));

					i64 offset = 0;
					for (Chunk const& chunk : chunks) {
						std::vector<vec3> trimmed_blocks = trim(chunk);
						glBufferSubData(GL_ARRAY_BUFFER, offset + block_data_offset, trimmed_blocks.size() * sizeof(vec3), trimmed_blocks.data());
						offset += trimmed_blocks.size() * sizeof(vec3);
					}

					glActiveTexture(GL_TEXTURE0);
					glBindTexture(GL_TEXTURE_2D, tex_vec[0].id);
					glDrawArraysInstanced(GL_TRIANGLES, 0, 36, offset / sizeof(vec3));
				}

				ImGui_ImplOpenGL3_NewFrame();
				ImGui_ImplGlfw_NewFrame();
				ImGui::NewFrame();
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
				glfwSwapBuffers(window);
				++nframes;
			}

			return 0;
		}
	};
}

int main() {
	minecraftpp::application a{};
	if (int err; (err = a.init_gl())) {
		return err;
	}
	return a.run();
}
