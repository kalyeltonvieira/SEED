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

// --- BEGIN IMPORT: stdlib\render3d ---
// ============================================================;
// RENDER3D.SEED — Motor de Renderização 3D;
// Matrizes, vetores, câmera, iluminação, rasterização;
// ============================================================;


// ============================================================;
// TIPOS BÁSICOS;
// ============================================================;


struct Vec2 {
    double x;
    double y;
};


struct Vec3 {
    double x;
    double y;
    double z;
};


struct Vec4 {
    double x;
    double y;
    double z;
    double w;
};


struct Mat4 {
    [float; 16]  // matriz 4x4 column-major data;
};


struct Color {
    double r;
    double g;
    double b;
    double a;
};


struct Vertex {
    Vec3 position;
    Vec3 normal;
    Vec2 uv;
    Color color;
};


struct Triangle {
    Vertex v0;
    Vertex v1;
    Vertex v2;
};


struct Mesh {
    [Triangle] triangles;
    std::string name;
};


struct Transform {
    Vec3 position;
    Vec3 rotation;
    Vec3 scale;
};


struct Camera {
    Vec3 position;
    Vec3 target;
    Vec3 up;
    double fov;
    double near;
    double far;
};


struct Light {
    Vec3 position;
    Color color;
    double intensity;
};


struct Material {
    Color ambient;
    Color diffuse;
    Color specular;
    double shininess;
};


struct Framebuffer {
    int width;
    int height;
    [Color] pixels;
    [float] depth;
};


// ============================================================;
// OPERAÇÕES COM VETORES;
// ============================================================;


pub fn vec3_new(x: float, y: float, z: float) -> Vec3 {
    Vec3 { x: x, y: y, z: z }
}


pub fn vec3_zero() -> Vec3 {
    Vec3 { x: 0.0, y: 0.0, z: 0.0 }
}


pub fn vec3_one() -> Vec3 {
    Vec3 { x: 1.0, y: 1.0, z: 1.0 }
}


pub fn vec3_add(a: Vec3, b: Vec3) -> Vec3 {
    Vec3 { x: a.x + b.x, y: a.y + b.y, z: a.z + b.z }
}


pub fn vec3_sub(a: Vec3, b: Vec3) -> Vec3 {
    Vec3 { x: a.x - b.x, y: a.y - b.y, z: a.z - b.z }
}


pub fn vec3_mul(v: Vec3, scalar: float) -> Vec3 {
    Vec3 { x: v.x * scalar, y: v.y * scalar, z: v.z * scalar }
}


pub fn vec3_div(v: Vec3, scalar: float) -> Vec3 {
    Vec3 { x: v.x / scalar, y: v.y / scalar, z: v.z / scalar }
}


pub fn vec3_dot(a: Vec3, b: Vec3) -> float {
    a.x * b.x + a.y * b.y + a.z * b.z;
}


pub fn vec3_cross(a: Vec3, b: Vec3) -> Vec3 {
    Vec3 {
        x: a.y * b.z - a.z * b.y,
        y: a.z * b.x - a.x * b.z,
        z: a.x * b.y - a.y * b.x;
    }
}


pub fn vec3_length(v: Vec3) -> float {
    std::sqrt(vec3_dot(v, v));
}


pub fn vec3_normalize(v: Vec3) -> Vec3 {
    const auto len = vec3_length(v);
    if (len > 0.0001) {
        vec3_div(v, len);
    }
        vec3_zero();
    }
}


pub fn vec3_lerp(a: Vec3, b: Vec3, t: float) -> Vec3 {
    vec3_add(vec3_mul(a, 1.0 - t), vec3_mul(b, t));
}


pub fn vec3_reflect(v: Vec3, normal: Vec3) -> Vec3 {
    vec3_sub(v, vec3_mul(normal, 2.0 * vec3_dot(v, normal)));
}


pub fn vec3_distance(a: Vec3, b: Vec3) -> float {
    vec3_length(vec3_sub(a, b));
}


// ============================================================;
// OPERAÇÕES COM MATRIZES 4X4;
// ============================================================;


pub fn mat4_identity() -> Mat4 {
    auto m = Mat4 { data: [0.0; 16] }
    m.data[0] = 1.0;
    m.data[5] = 1.0;
    m.data[10] = 1.0;
    m.data[15] = 1.0;
    m;
}


pub fn mat4_perspective(fov: float, aspect: float, near: float, far: float) -> Mat4 {
    const auto f = 1.0 / math.tan(fov * 0.5);
    const auto range_inv = 1.0 / (near - far);
    

    auto m = Mat4 { data: [0.0; 16] }
    m.data[0] = f / aspect;
    m.data[5] = f;
    m.data[10] = (near + far) * range_inv;
    m.data[11] = -1.0;
    m.data[14] = near * far * range_inv * 2.0;
    m;
}


pub fn mat4_look_at(eye: Vec3, target: Vec3, up: Vec3) -> Mat4 {
    const auto f = vec3_normalize(vec3_sub(target, eye));
    const auto s = vec3_normalize(vec3_cross(f, up));
    const auto u = vec3_cross(s, f);
    

    auto m = Mat4 { data: [0.0; 16] }
    m.data[0] = s.x;
    m.data[4] = s.y;
    m.data[8] = s.z;
    m.data[1] = u.x;
    m.data[5] = u.y;
    m.data[9] = u.z;
    m.data[2] = -f.x;
    m.data[6] = -f.y;
    m.data[10] = -f.z;
    m.data[12] = -vec3_dot(s, eye);
    m.data[13] = -vec3_dot(u, eye);
    m.data[14] = vec3_dot(f, eye);
    m.data[15] = 1.0;
    m;
}


pub fn mat4_translation(v: Vec3) -> Mat4 {
    auto m = mat4_identity();
    m.data[12] = v.x;
    m.data[13] = v.y;
    m.data[14] = v.z;
    m;
}


pub fn mat4_rotation_x(angle: float) -> Mat4 {
    const auto c = std::cos(angle);
    const auto s = std::sin(angle);
    auto m = mat4_identity();
    m.data[5] = c;
    m.data[6] = s;
    m.data[9] = -s;
    m.data[10] = c;
    m;
}


pub fn mat4_rotation_y(angle: float) -> Mat4 {
    const auto c = std::cos(angle);
    const auto s = std::sin(angle);
    auto m = mat4_identity();
    m.data[0] = c;
    m.data[2] = -s;
    m.data[8] = s;
    m.data[10] = c;
    m;
}


pub fn mat4_rotation_z(angle: float) -> Mat4 {
    const auto c = std::cos(angle);
    const auto s = std::sin(angle);
    auto m = mat4_identity();
    m.data[0] = c;
    m.data[1] = s;
    m.data[4] = -s;
    m.data[5] = c;
    m;
}


pub fn mat4_scale(v: Vec3) -> Mat4 {
    auto m = mat4_identity();
    m.data[0] = v.x;
    m.data[5] = v.y;
    m.data[10] = v.z;
    m;
}


pub fn mat4_mul(a: Mat4, b: Mat4) -> Mat4 {
    auto m = Mat4 { data: [0.0; 16] }
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            auto sum = 0.0;
            for (int k = 0; k < 4; k++) {
                sum = sum + a.data[k * 4 + j] * b.data[i * 4 + k];
            }
            m.data[i * 4 + j] = sum;
        }
    }
    m;
}


pub fn mat4_transform_point(m: Mat4, v: Vec3) -> Vec3 {
    const auto x = m.data[0] * v.x + m.data[4] * v.y + m.data[8] * v.z + m.data[12];
    const auto y = m.data[1] * v.x + m.data[5] * v.y + m.data[9] * v.z + m.data[13];
    const auto z = m.data[2] * v.x + m.data[6] * v.y + m.data[10] * v.z + m.data[14];
    const auto w = m.data[3] * v.x + m.data[7] * v.y + m.data[11] * v.z + m.data[15];
    

    if (std::abs(w) > 0.0001) {
        Vec3 { x: x / w, y: y / w, z: z / w }
    }
        Vec3 { x: x, y: y, z: z }
    }
}


pub fn mat4_transform_direction(m: Mat4, v: Vec3) -> Vec3 {
    Vec3 {
        x: m.data[0] * v.x + m.data[4] * v.y + m.data[8] * v.z,
        y: m.data[1] * v.x + m.data[5] * v.y + m.data[9] * v.z,
        z: m.data[2] * v.x + m.data[6] * v.y + m.data[10] * v.z;
    }
}


// ============================================================;
// TRANSFORM;
// ============================================================;


pub fn transform_matrix(t: Transform) -> Mat4 {
    const auto translation = mat4_translation(t.position);
    const auto rotation_x = mat4_rotation_x(t.rotation.x);
    const auto rotation_y = mat4_rotation_y(t.rotation.y);
    const auto rotation_z = mat4_rotation_z(t.rotation.z);
    const auto scale = mat4_scale(t.scale);
    

    mat4_mul(mat4_mul(mat4_mul(translation, rotation_z), rotation_y), mat4_mul(rotation_x, scale));
}


pub fn transform_new() -> Transform {
    Transform {
        position: vec3_zero(),
        rotation: vec3_zero(),
        scale: vec3_one();
    }
}


// ============================================================;
// CORES;
// ============================================================;


pub fn color_new(r: float, g: float, b: float, a: float) -> Color {
    Color { r: r, g: g, b: b, a: a }
}


pub fn color_white() -> Color { color_new(1.0, 1.0, 1.0, 1.0) }
pub fn color_black() -> Color { color_new(0.0, 0.0, 0.0, 1.0) }
pub fn color_red() -> Color { color_new(1.0, 0.0, 0.0, 1.0) }
pub fn color_green() -> Color { color_new(0.0, 1.0, 0.0, 1.0) }
pub fn color_blue() -> Color { color_new(0.0, 0.0, 1.0, 1.0) }
pub fn color_yellow() -> Color { color_new(1.0, 1.0, 0.0, 1.0) }
pub fn color_cyan() -> Color { color_new(0.0, 1.0, 1.0, 1.0) }
pub fn color_magenta() -> Color { color_new(1.0, 0.0, 1.0, 1.0) }


pub fn color_mul(c: Color, scalar: float) -> Color {
    Color { r: c.r * scalar, g: c.g * scalar, b: c.b * scalar, a: c.a }
}


pub fn color_add(a: Color, b: Color) -> Color {
    Color { r: a.r + b.r, g: a.g + b.g, b: a.b + b.b, a: a.a + b.a }
}


pub fn color_clamp(c: Color) -> Color {
    Color {
        r: math.clamp(c.r, 0.0, 1.0),
        g: math.clamp(c.g, 0.0, 1.0),
        b: math.clamp(c.b, 0.0, 1.0),
        a: math.clamp(c.a, 0.0, 1.0);
    }
}


// ============================================================;
// FRAMEBUFFER;
// ============================================================;


pub fn framebuffer_new(width: int, height: int) -> Framebuffer {
    Framebuffer {
        width: width,
        height: height,
        pixels: [color_black(); width * height],
        depth: [999999.0; width * height];
    }
}


pub fn framebuffer_clear(fb: &mut Framebuffer, color: Color) {
    for (int i = 0; i < fb.pixels.len; i++) {
        fb.pixels[i] = color;
        fb.depth[i] = 999999.0;
    }
}


pub fn framebuffer_set_pixel(fb: &mut Framebuffer, x: int, y: int, color: Color) {
    if (x >= 0 && x < fb.width && y >= 0 && y < fb.height) {
        fb.pixels[y * fb.width + x] = color;
    }
}


pub fn framebuffer_get_pixel(fb: &Framebuffer, x: int, y: int) -> Color {
    if (x >= 0 && x < fb.width && y >= 0 && y < fb.height) {
        fb.pixels[y * fb.width + x];
    }
        color_black();
    }
}


pub fn framebuffer_set_depth(fb: &mut Framebuffer, x: int, y: int, depth: float) -> bool {
    if (x >= 0 && x < fb.width && y >= 0 && y < fb.height) {
        if (depth < fb.depth[y * fb.width + x]) {
            fb.depth[y * fb.width + x] = depth;
            return true;
        }
    }
    false;
}


// ============================================================;
// RASTERIZAÇÃO DE TRIÂNGULOS;
// ============================================================;


auto edge_function(Vec2 a, Vec2 b, Vec2 c) -> double {
    (c.x - a.x) * (b.y - a.y) - (c.y - a.y) * (b.x - a.x);
}


pub fn draw_triangle(;
    fb: &mut Framebuffer,
    v0: Vec3, v1: Vec3, v2: Vec3,
    c0: Color, c1: Color, c2: Color;
) {
    // Converter para coordenadas de tela;
    const auto half_w = fb.width as float * 0.5;
    const auto half_h = fb.height as float * 0.5;
    

    const auto p0 = Vec2 { x: (v0.x + 1.0) * half_w, y: (1.0 - v0.y) * half_h }
    const auto p1 = Vec2 { x: (v1.x + 1.0) * half_w, y: (1.0 - v1.y) * half_h }
    const auto p2 = Vec2 { x: (v2.x + 1.0) * half_w, y: (1.0 - v2.y) * half_h }
    

    // Bounding box;
    const auto min_x = math.max(0.0, math.min(math.min(p0.x, p1.x), p2.x)) as int;
    const auto max_x = math.min(fb.width as float - 1.0, math.max(math.max(p0.x, p1.x), p2.x)) as int;
    const auto min_y = math.max(0.0, math.min(math.min(p0.y, p1.y), p2.y)) as int;
    const auto max_y = math.min(fb.height as float - 1.0, math.max(math.max(p0.y, p1.y), p2.y)) as int;
    

    const auto area = edge_function(p0, p1, p2);
    if (std::abs(area) < 0.0001) {
        return;
    }
    

    for (int y = min_y; y < ; y++) {
        for (int x = min_x; x < ; x++) {
            const auto p = Vec2 { x: x as float + 0.5, y: y as float + 0.5 }
            

            const auto w0 = edge_function(p1, p2, p) / area;
            const auto w1 = edge_function(p2, p0, p) / area;
            const auto w2 = edge_function(p0, p1, p) / area;
            

            if (w0 >= 0.0 && w1 >= 0.0 && w2 >= 0.0) {
                const auto z = v0.z * w0 + v1.z * w1 + v2.z * w2;
                

                if (framebuffer_set_depth(fb, x, y, z)) {
                    const auto color = Color {
                        .r = c0.r * w0 + c1.r * w1 + c2.r * w2,
                        .g = c0.g * w0 + c1.g * w1 + c2.g * w2,
                        .b = c0.b * w0 + c1.b * w1 + c2.b * w2,
                        .a = c0.a * w0 + c1.a * w1 + c2.a * w2,
                    };
                    framebuffer_set_pixel(fb, x, y, color);
                }
            }
        }
    }
}


// ============================================================;
// MALHAS 3D PRÉ-DEFINIDAS;
// ============================================================;


pub fn mesh_cube(size: float) -> Mesh {
    const auto s = size * 0.5;
    auto tris = [];
    

    // 6 faces, 2 triângulos cada = 12 triângulos;
    // Frente (z+);
    tris.push(triangle_new(;
        Vec3 { x: -s, y: -s, z: s }, Vec3 { x: s, y: -s, z: s }, Vec3 { x: s, y: s, z: s },
        Vec3 { x: 0.0, y: 0.0, z: 1.0 }
    ));
    tris.push(triangle_new(;
        Vec3 { x: -s, y: -s, z: s }, Vec3 { x: s, y: s, z: s }, Vec3 { x: -s, y: s, z: s },
        Vec3 { x: 0.0, y: 0.0, z: 1.0 }
    ));
    // Trás (z-);
    tris.push(triangle_new(;
        Vec3 { x: s, y: -s, z: -s }, Vec3 { x: -s, y: -s, z: -s }, Vec3 { x: -s, y: s, z: -s },
        Vec3 { x: 0.0, y: 0.0, z: -1.0 }
    ));
    tris.push(triangle_new(;
        Vec3 { x: s, y: -s, z: -s }, Vec3 { x: -s, y: s, z: -s }, Vec3 { x: s, y: s, z: -s },
        Vec3 { x: 0.0, y: 0.0, z: -1.0 }
    ));
    // Cima (y+);
    tris.push(triangle_new(;
        Vec3 { x: -s, y: s, z: s }, Vec3 { x: s, y: s, z: s }, Vec3 { x: s, y: s, z: -s },
        Vec3 { x: 0.0, y: 1.0, z: 0.0 }
    ));
    tris.push(triangle_new(;
        Vec3 { x: -s, y: s, z: s }, Vec3 { x: s, y: s, z: -s }, Vec3 { x: -s, y: s, z: -s },
        Vec3 { x: 0.0, y: 1.0, z: 0.0 }
    ));
    // Baixo (y-);
    tris.push(triangle_new(;
        Vec3 { x: -s, y: -s, z: -s }, Vec3 { x: s, y: -s, z: -s }, Vec3 { x: s, y: -s, z: s },
        Vec3 { x: 0.0, y: -1.0, z: 0.0 }
    ));
    tris.push(triangle_new(;
        Vec3 { x: -s, y: -s, z: -s }, Vec3 { x: s, y: -s, z: s }, Vec3 { x: -s, y: -s, z: s },
        Vec3 { x: 0.0, y: -1.0, z: 0.0 }
    ));
    // Direita (x+);
    tris.push(triangle_new(;
        Vec3 { x: s, y: -s, z: s }, Vec3 { x: s, y: -s, z: -s }, Vec3 { x: s, y: s, z: -s },
        Vec3 { x: 1.0, y: 0.0, z: 0.0 }
    ));
    tris.push(triangle_new(;
        Vec3 { x: s, y: -s, z: s }, Vec3 { x: s, y: s, z: -s }, Vec3 { x: s, y: s, z: s },
        Vec3 { x: 1.0, y: 0.0, z: 0.0 }
    ));
    // Esquerda (x-);
    tris.push(triangle_new(;
        Vec3 { x: -s, y: -s, z: -s }, Vec3 { x: -s, y: -s, z: s }, Vec3 { x: -s, y: s, z: s },
        Vec3 { x: -1.0, y: 0.0, z: 0.0 }
    ));
    tris.push(triangle_new(;
        Vec3 { x: -s, y: -s, z: -s }, Vec3 { x: -s, y: s, z: s }, Vec3 { x: -s, y: s, z: -s },
        Vec3 { x: -1.0, y: 0.0, z: 0.0 }
    ));
    

    Mesh { triangles: tris, name: "cube"s }
}


pub fn mesh_sphere(radius: float, segments: int) -> Mesh {
    auto tris = [];
    

    for (int lat = 0; lat < segments; lat++) {
        const auto theta1 = 3.141592653589793 * lat as float / segments as float;
        const auto theta2 = 3.141592653589793 * (lat + 1) as float / segments as float;
        

        for (int lon = 0; lon < segments; lon++) {
            const auto phi1 = 2.0 * 3.141592653589793 * lon as float / segments as float;
            const auto phi2 = 2.0 * 3.141592653589793 * (lon + 1) as float / segments as float;
            

            const auto p1 = vec3_new(;
                radius * std::sin(theta1) * std::cos(phi1),
                radius * std::cos(theta1),
                radius * std::sin(theta1) * std::sin(phi1);
            );
            const auto p2 = vec3_new(;
                radius * std::sin(theta2) * std::cos(phi1),
                radius * std::cos(theta2),
                radius * std::sin(theta2) * std::sin(phi1);
            );
            const auto p3 = vec3_new(;
                radius * std::sin(theta2) * std::cos(phi2),
                radius * std::cos(theta2),
                radius * std::sin(theta2) * std::sin(phi2);
            );
            const auto p4 = vec3_new(;
                radius * std::sin(theta1) * std::cos(phi2),
                radius * std::cos(theta1),
                radius * std::sin(theta1) * std::sin(phi2);
            );
            

            tris.push(triangle_new(p1, p2, p3, vec3_normalize(p1)));
            tris.push(triangle_new(p1, p3, p4, vec3_normalize(p1)));
        }
    }
    

    Mesh { triangles: tris, name: "sphere"s }
}


pub fn mesh_plane(size: float) -> Mesh {
    const auto s = size * 0.5;
    const auto normal = Vec3 { x: 0.0, y: 1.0, z: 0.0 }
    auto tris = [];
    

    tris.push(triangle_new(;
        Vec3 { x: -s, y: 0.0, z: -s },
        Vec3 { x: s, y: 0.0, z: -s },
        Vec3 { x: s, y: 0.0, z: s },
        normal;
    ));
    tris.push(triangle_new(;
        Vec3 { x: -s, y: 0.0, z: -s },
        Vec3 { x: s, y: 0.0, z: s },
        Vec3 { x: -s, y: 0.0, z: s },
        normal;
    ));
    

    Mesh { triangles: tris, name: "plane"s }
}


auto triangle_new(Vec3 v0, Vec3 v1, Vec3 v2, Vec3 normal) -> Triangle {
    Triangle {
        v0: Vertex { position: v0, normal: normal, uv: Vec2 { x: 0.0, y: 0.0 }, color: color_white() },
        v1: Vertex { position: v1, normal: normal, uv: Vec2 { x: 1.0, y: 0.0 }, color: color_white() },
        v2: Vertex { position: v2, normal: normal, uv: Vec2 { x: 0.0, y: 1.0 }, color: color_white() }
    }
}


// ============================================================;
// ILUMINAÇÃO PHONG;
// ============================================================;


pub fn phong_shade(;
    normal: Vec3,
    position: Vec3,
    camera_pos: Vec3,
    light: Light,
    material: Material;
) -> Color {
    // Ambient;
    const auto ambient = color_mul(material.ambient, light.intensity * 0.1);
    

    // Diffuse;
    const auto light_dir = vec3_normalize(vec3_sub(light.position, position));
    const auto diff = math.max(0.0, vec3_dot(normal, light_dir));
    const auto diffuse = color_mul(color_mul(material.diffuse, light.color), diff * light.intensity);
    

    // Specular;
    const auto view_dir = vec3_normalize(vec3_sub(camera_pos, position));
    const auto reflect_dir = vec3_reflect(vec3_mul(light_dir, -1.0), normal);
    const auto spec = math.pow(math.max(0.0, vec3_dot(view_dir, reflect_dir)), material.shininess);
    const auto specular = color_mul(color_mul(material.specular, light.color), spec * light.intensity);
    

    color_clamp(color_add(color_add(ambient, diffuse), specular));
}


// ============================================================;
// RENDERIZAÇÃO PRINCIPAL;
// ============================================================;


struct Scene {
    [(Mesh, Transform, Material)] meshes;
    [Light] lights;
    Camera camera;
    Color ambient_color;
};


pub fn scene_new() -> Scene {
    Scene {
        meshes: [],
        lights: [],
        camera: Camera {
            position: Vec3 { x: 0.0, y: 5.0, z: 10.0 },
            target: vec3_zero(),
            up: Vec3 { x: 0.0, y: 1.0, z: 0.0 },
            fov: 60.0 * 3.141592653589793 / 180.0,
            near: 0.1,
            far: 100.0;
        }
        ambient_color: Color { r: 0.1, g: 0.1, b: 0.15, a: 1.0 }
    }
}


pub fn scene_add_mesh(scene: &mut Scene, mesh: Mesh, transform: Transform, material: Material) {
    scene.meshes.push((mesh, transform, material));
}


pub fn scene_add_light(scene: &mut Scene, light: Light) {
    scene.lights.push(light);
}


pub fn render(scene: &Scene, fb: &mut Framebuffer) {
    framebuffer_clear(fb, scene.ambient_color);
    

    const auto view = mat4_look_at(scene.camera.position, scene.camera.target, scene.camera.up);
    const auto aspect = fb.width as float / fb.height as float;
    const auto projection = mat4_perspective(scene.camera.fov, aspect, scene.camera.near, scene.camera.far);
    const auto vp = mat4_mul(projection, view);
    

    for (mesh, transform, material) in scene.meshes {
        const auto model = transform_matrix(transform);
        const auto mvp = mat4_mul(vp, model);
        

        for tri in mesh.triangles {
            // Transformar vértices;
            const auto tv0 = mat4_transform_point(mvp, tri.v0.position);
            const auto tv1 = mat4_transform_point(mvp, tri.v1.position);
            const auto tv2 = mat4_transform_point(mvp, tri.v2.position);
            

            // Transformar normais;
            const auto n0 = vec3_normalize(mat4_transform_direction(model, tri.v0.normal));
            const auto n1 = vec3_normalize(mat4_transform_direction(model, tri.v1.normal));
            const auto n2 = vec3_normalize(mat4_transform_direction(model, tri.v2.normal));
            

            // Posições world para iluminação;
            const auto w0 = mat4_transform_point(model, tri.v0.position);
            const auto w1 = mat4_transform_point(model, tri.v1.position);
            const auto w2 = mat4_transform_point(model, tri.v2.position);
            

            // Calcular cores com iluminação;
            auto colors = [];
            for light in scene.lights {
                const auto c0 = phong_shade(n0, w0, scene.camera.position, light, material);
                const auto c1 = phong_shade(n1, w1, scene.camera.position, light, material);
                const auto c2 = phong_shade(n2, w2, scene.camera.position, light, material);
                colors.push((c0, c1, c2));
            }
            

            // Combinar cores de todas as luzes;
            const auto (mut final_c0, mut final_c1, mut final_c2) = colors[0];
            for (int i = 1; i < colors.len; i++) {
                const auto (c0, c1, c2) = colors[i];
                final_c0 = color_add(final_c0, c0);
                final_c1 = color_add(final_c1, c1);
                final_c2 = color_add(final_c2, c2);
            }
            

            draw_triangle(fb, tv0, tv1, tv2,
                color_clamp(final_c0),
                color_clamp(final_c1),
                color_clamp(final_c2);
            );
        }
    }
}


// ============================================================;
// SAÍDA PARA TERMINAL (ASCII ART);
// ============================================================;


pub fn framebuffer_to_ascii(fb: &Framebuffer, out_width: int, out_height: int) -> string {
    const auto chars = " .:-=+*#%@"s;
    auto result = ""s;
    

    for (int y = 0; y < out_height; y++) {
        for (int x = 0; x < out_width; x++) {
            const auto src_x = (x as float / out_width as float * fb.width as float) as int;
            const auto src_y = (y as float / out_height as float * fb.height as float) as int;
            const auto pixel = framebuffer_get_pixel(fb, src_x, src_y);
            

            const auto brightness = (pixel.r + pixel.g + pixel.b) / 3.0;
            const auto char_index = math.clamp((brightness * (chars.len() - 1) as float) as int, 0, chars.len() - 1);
            result = result + stdlib::to_string(chars[char_index]);
        }
        result = result + "\n"s;
    }
    

    result;
}


pub fn framebuffer_to_terminal(fb: &Framebuffer, out_width: int, out_height: int) {
    const auto chars = " .:-=+*#%@"s;
    

    for (int y = 0; y < out_height; y++) {
        auto line = ""s;
        for (int x = 0; x < out_width; x++) {
            const auto src_x = (x as float / out_width as float * fb.width as float) as int;
            const auto src_y = (y as float / out_height as float * fb.height as float) as int;
            const auto pixel = framebuffer_get_pixel(fb, src_x, src_y);
            

            const auto brightness = (pixel.r + pixel.g + pixel.b) / 3.0;
            const auto char_index = math.clamp((brightness * (chars.len() - 1) as float) as int, 0, chars.len() - 1);
            const auto ch = chars[char_index];
            

            // Cor ANSI;
            const auto r = (pixel.r * 5.0) as int;
            const auto g = (pixel.g * 5.0) as int;
            const auto b = (pixel.b * 5.0) as int;
            const auto ansi_code = 16 + 36 * r + 6 * g + b;
            

            line = line + stdlib::terminal::fg_color(ansi_code) + stdlib::to_string(ch);
        }
        stdlib::io::stdlib::io::println(line + stdlib::terminal::reset());
    }
}
// --- END IMPORT: stdlib\render3d ---


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
