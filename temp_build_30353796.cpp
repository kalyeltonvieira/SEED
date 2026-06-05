#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <chrono>
#include <thread>
#include <sstream>
#include <windows.h>
using namespace std::string_literals;

struct Vector2 { double x; double y; };
struct Vector3 { double x; double y; double z; };
struct Rect { double x; double y; double width; double height; };

inline std::string operator*(const std::string& s, int count) {
    std::string result;
    if (count > 0) {
        result.reserve(s.length() * count);
        for (int i = 0; i < count; ++i) result += s;
    }
    return result;
}
inline std::string operator*(int count, const std::string& s) { return s * count; }

inline double to_float(int n) { return (double)n; }
inline double to_float(double d) { return d; }
namespace stdlib {
    template<typename T>
    std::string to_string(const T& val) {
        std::ostringstream ss;
        ss << val;
        return ss.str();
    }
    inline std::string to_string(bool b) { return b ? "true"s : "false"s; }
}

namespace stdlib {
    namespace ansi {
        const std::string RESET = "\033[0m"s;
        const std::string BOLD = "\033[1m"s;
        const std::string DIM = "\033[2m"s;
        const std::string ITALIC = "\033[3m"s;
        const std::string UNDERLINE = "\033[4m"s;
        const std::string BLINK = "\033[5m"s;
        const std::string REVERSE = "\033[7m"s;
        const std::string HIDDEN = "\033[8m"s;
        const std::string BLACK = "\033[30m"s;
        const std::string RED = "\033[31m"s;
        const std::string GREEN = "\033[32m"s;
        const std::string YELLOW = "\033[33m"s;
        const std::string BLUE = "\033[34m"s;
        const std::string MAGENTA = "\033[35m"s;
        const std::string CYAN = "\033[36m"s;
        const std::string WHITE = "\033[37m"s;
        inline std::string style(const std::string& text, const std::string& style_code) { return style_code + text + RESET; }
        inline std::string clear_screen() { return "\033[2J\033[H"s; }
        inline std::string clear_line() { return "\033[2K"s; }
    }
    namespace io {
        inline void print(const std::string& s) { std::cout << s; std::cout.flush(); }
        inline void println(const std::string& s) { std::cout << s << "\n"; std::cout.flush(); }
        inline std::string read_line() { std::string line; std::getline(std::cin, line); return line; }
    }
    namespace geometry {
        inline Vector2 vec2(double x, double y) { return Vector2{x, y}; }
        inline Vector2 vec2_add(Vector2 a, Vector2 b) { return Vector2{a.x + b.x, a.y + b.y}; }
        inline Vector2 vec2_sub(Vector2 a, Vector2 b) { return Vector2{a.x - b.x, a.y - b.y}; }
        inline double vec2_magnitude(Vector2 v) { return std::sqrt(v.x * v.x + v.y * v.y); }
        inline double vec2_distance(Vector2 a, Vector2 b) { return vec2_magnitude(vec2_sub(a, b)); }
    }
    namespace time {
        inline void sleep(int ms) { std::this_thread::sleep_for(std::chrono::milliseconds(ms)); }
    }
    namespace terminal {
        inline void clear() { std::cout << "\033[2J\033[H"; std::cout.flush(); }
        inline std::string fg_color(int code) { return "\033[38;5;" + std::to_string(code) + "m"; }
        inline std::string bg_color(int code) { return "\033[48;5;" + std::to_string(code) + "m"; }
        inline std::string reset() { return "\033[0m"; }
    }
}



int main(int argc, char** argv) {
    // Criar cena;
    auto scene = scene_new();
    

    // Câmera giratória;
    scene.camera.position = Vec3 { x: 0.0, y: 3.0, z: 8.0 }
    scene.camera.target = vec3_zero();
    

    // Adicionar esfera;
    const auto sphere = mesh_sphere(1.0, 16);
    const auto sphere_transform = Transform {
        .position = vec3_zero(),
        .rotation = vec3_zero(),
        .scale = vec3_one(),
    };
    const auto sphere_material = Material {
        .ambient = color_new(0.2, 0.1, 0.1, 1.0),
        .diffuse = color_new(0.8, 0.2, 0.2, 1.0),
        .specular = color_white(),
        .shininess = 32.0,
    };
    scene_add_mesh(&mut scene, sphere, sphere_transform, sphere_material);
    

    // Adicionar plano (chão);
    const auto plane = mesh_plane(10.0);
    const auto plane_transform = Transform {
        .position = Vec3 { .x = 0.0, .y = -2.0, .z = 0.0 },
        .rotation = vec3_zero(),
        .scale = vec3_one(),
    };
    const auto plane_material = Material {
        .ambient = color_new(0.1, 0.1, 0.2, 1.0),
        .diffuse = color_new(0.3, 0.3, 0.8, 1.0),
        .specular = color_new(0.5, 0.5, 0.5, 1.0),
        .shininess = 8.0,
    };
    scene_add_mesh(&mut scene, plane, plane_transform, plane_material);
    

    // Adicionar luz;
    const auto light = Light {
        .position = Vec3 { .x = 5.0, .y = 8.0, .z = 5.0 },
        .color = color_white(),
        .intensity = 1.5,
    };
    scene_add_light(&mut scene, light);
    

    // Framebuffer;
    auto fb = framebuffer_new(80, 30);
    

    // Loop de animação (100 frames);
    auto angle = 0.0;
    for (int frame = 0; frame < 100; frame++) {
        // Girar câmera;
        angle = angle + 0.05;
        scene.camera.position.x = std::sin(angle) * 8.0;
        scene.camera.position.z = std::cos(angle) * 8.0;
        scene.camera.target = vec3_zero();
        

        // Renderizar;
        render(&scene, &mut fb);
        

        // Exibir no terminal;
        stdlib::terminal::clear();
        framebuffer_to_terminal(&fb, 80, 30);
        

        stdlib::time::sleep(50);
    }
}
