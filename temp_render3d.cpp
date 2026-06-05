#include <iostream>
#include <string>
#include <vector>
#include <array>
#include <tuple>
#include <cmath>
#include <chrono>
#include <thread>
#include <sstream>
#include <algorithm>
#include <windows.h>
#ifdef near
#undef near
#endif
#ifdef far
#undef far
#endif
using namespace std::string_literals;

struct Vec2 { double x; double y; };
struct Vec3 { double x; double y; double z; };
struct Vec4 { double x; double y; double z; double w; };
struct Mat4 { std::array<double, 16> data; };
struct Color { double r; double g; double b; double a; };
struct Vertex { Vec3 position; Vec3 normal; Vec2 uv; Color color; };
struct Triangle { Vertex v0; Vertex v1; Vertex v2; };
struct Mesh { std::vector<Triangle> triangles; std::string name; };
struct Transform { Vec3 position; Vec3 rotation; Vec3 scale; };
struct Camera { Vec3 position; Vec3 target; Vec3 up; double fov; double near; double far; };
struct Light { Vec3 position; Color color; double intensity; };
struct Material { Color ambient; Color diffuse; Color specular; double shininess; };
struct Framebuffer { int width; int height; std::vector<Color> pixels; std::vector<double> depth; };
struct Scene { std::vector<std::tuple<Mesh, Transform, Material>> meshes; std::vector<Light> lights; Camera camera; Color ambient_color; };

template<size_t N, typename T>
inline std::array<T, N> make_array(const T& val) {
    std::array<T, N> arr;
    arr.fill(val);
    return arr;
}

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
    namespace io {
        inline void println(const std::string& s) {
            std::cout << s << std::endl;
        }
        inline void print(const std::string& s) {
            std::cout << s;
        }
    }
    namespace time {
        inline void sleep(int ms) {
            std::this_thread::sleep_for(std::chrono::milliseconds(ms));
        }
    }
}

namespace math {
    const double PI = 3.14159265358979323846;
    inline double cos(double x) { return std::cos(x); }
    inline double sin(double x) { return std::sin(x); }
    inline double tan(double x) { return std::tan(x); }
    inline double sqrt(double x) { return std::sqrt(x); }
    inline double abs(double x) { return std::abs(x); }
    inline double max(double a, double b) { return std::max(a, b); }
    inline double min(double a, double b) { return std::min(a, b); }
    inline double clamp(double v, double lo, double hi) { return std::clamp(v, lo, hi); }
    inline double pow(double base, double exp) { return std::pow(base, exp); }
}

namespace terminal {
    inline std::string fg_color(int code) {
        return "\033[38;5;"s + stdlib::to_string(code) + "m"s;
    }
    inline std::string reset() {
        return "\033[0m"s;
    }
    inline void clear() {
        std::cout << "\033[2J\033[H"s;
        std::cout.flush();
    }
}

// RENDER3D.SEED - Motor de Renderizacao 3D;
// Simplificado para o transpilador rudimentar.;


// ============================================================;
// TIPOS BASICOS;
// ============================================================;


struct Vec2 {
    .x = float,
    .y = float,
};


struct Vec3 {
    .x = float,
    .y = float,
    .z = float,
};


struct Mat4 {
    .data = [float],
};


struct Color {
    .r = float,
    .g = float,
    .b = float,
    .a = float,
};


struct Vertex {
    .position = Vec3,
    .normal = Vec3,
    .uv = Vec2,
    .color = Color,
};


struct Triangle {
    .v0 = Vertex,
    .v1 = Vertex,
    .v2 = Vertex,
};


struct Mesh {
    .triangles = [Triangle],
    .name = string,
};


struct Transform {
    .position = Vec3,
    .rotation = Vec3,
    .scale = Vec3,
};


struct Camera {
    .position = Vec3,
    .target = Vec3,
    .up = Vec3,
    .fov = float,
    .z_near = float,
    .z_far = float,
};


struct Light {
    .position = Vec3,
    .color = Color,
    .intensity = float,
};


struct Material {
    .ambient = Color,
    .diffuse = Color,
    .specular = Color,
    .shininess = float,
};


struct SceneObject {
    .mesh = Mesh,
    .transform = Transform,
    .material = Material,
};


struct Framebuffer {
    .width = int,
    .height = int,
    .pixels = [Color],
    .depth = [float],
};


struct Scene {
    .meshes = [SceneObject],
    .lights = [Light],
    .camera = Camera,
    .ambient_color = Color,
};


// ============================================================;
// OPERACOES COM VETORES;
// ============================================================;


auto vec3_new(double x, double y, double z) -> Vec3 {
    auto v = Vec3();
    v.x = x;
    v.y = y;
    v.z = z;
    return v;
}


auto vec3_zero() -> Vec3 {
    return vec3_new(0.0, 0.0, 0.0);
}


auto vec3_one() -> Vec3 {
    return vec3_new(1.0, 1.0, 1.0);
}


auto vec3_add(Vec3 a, Vec3 b) -> Vec3 {
    return vec3_new(a.x + b.x, a.y + b.y, a.z + b.z);
}


auto vec3_sub(Vec3 a, Vec3 b) -> Vec3 {
    return vec3_new(a.x - b.x, a.y - b.y, a.z - b.z);
}


auto vec3_mul(Vec3 v, double scalar) -> Vec3 {
    return vec3_new(v.x * scalar, v.y * scalar, v.z * scalar);
}


auto vec3_div(Vec3 v, double scalar) -> Vec3 {
    return vec3_new(v.x / scalar, v.y / scalar, v.z / scalar);
}


auto vec3_dot(Vec3 a, Vec3 b) -> double {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}


auto vec3_cross(Vec3 a, Vec3 b) -> Vec3 {
    return vec3_new(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}


auto vec3_length(Vec3 v) -> double {
    return math::sqrt(vec3_dot(v, v));
}


auto vec3_normalize(Vec3 v) -> Vec3 {
    const auto len = vec3_length(v);
    if ((len > 0.0001)) {
        return vec3_div(v, len);
    }
    return vec3_zero();
}


auto vec3_lerp(Vec3 a, Vec3 b, double t) -> Vec3 {
    return vec3_add(vec3_mul(a, 1.0 - t), vec3_mul(b, t));
}


auto vec3_reflect(Vec3 v, Vec3 normal) -> Vec3 {
    return vec3_sub(v, vec3_mul(normal, 2.0 * vec3_dot(v, normal)));
}


auto vec3_distance(Vec3 a, Vec3 b) -> double {
    return vec3_length(vec3_sub(a, b));
}


// ============================================================;
// OPERACOES COM MATRIZES 4X4;
// ============================================================;


auto mat4_identity() -> Mat4 {
    auto m = Mat4();
    for (int i = 0; i < 16; i++) {
        m.data.push_back(0.0);
    }
    m.data[0] = 1.0;
    m.data[5] = 1.0;
    m.data[10] = 1.0;
    m.data[15] = 1.0;
    return m;
}


auto mat4_perspective(double fov, double aspect, double z_near, double z_far) -> Mat4 {
    const auto f = 1.0 / math::tan(fov * 0.5);
    const auto range_inv = 1.0 / (z_near - z_far);
    

    auto m = Mat4();
    for (int i = 0; i < 16; i++) {
        m.data.push_back(0.0);
    }
    m.data[0] = f / aspect;
    m.data[5] = f;
    m.data[10] = (z_near + z_far) * range_inv;
    m.data[11] = -1.0;
    m.data[14] = z_near * z_far * range_inv * 2.0;
    return m;
}


auto mat4_look_at(Vec3 eye, Vec3 target, Vec3 up) -> Mat4 {
    const auto f = vec3_normalize(vec3_sub(target, eye));
    const auto s = vec3_normalize(vec3_cross(f, up));
    const auto u = vec3_cross(s, f);
    

    auto m = Mat4();
    for (int i = 0; i < 16; i++) {
        m.data.push_back(0.0);
    }
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
    return m;
}


auto mat4_translation(Vec3 v) -> Mat4 {
    auto m = mat4_identity();
    m.data[12] = v.x;
    m.data[13] = v.y;
    m.data[14] = v.z;
    return m;
}


auto mat4_rotation_x(double angle) -> Mat4 {
    const auto c = math::cos(angle);
    const auto s = math::sin(angle);
    auto m = mat4_identity();
    m.data[5] = c;
    m.data[6] = s;
    m.data[9] = -s;
    m.data[10] = c;
    return m;
}


auto mat4_rotation_y(double angle) -> Mat4 {
    const auto c = math::cos(angle);
    const auto s = math::sin(angle);
    auto m = mat4_identity();
    m.data[0] = c;
    m.data[2] = -s;
    m.data[8] = s;
    m.data[10] = c;
    return m;
}


auto mat4_rotation_z(double angle) -> Mat4 {
    const auto c = math::cos(angle);
    const auto s = math::sin(angle);
    auto m = mat4_identity();
    m.data[0] = c;
    m.data[1] = s;
    m.data[4] = -s;
    m.data[5] = c;
    return m;
}


auto mat4_scale(Vec3 v) -> Mat4 {
    auto m = mat4_identity();
    m.data[0] = v.x;
    m.data[5] = v.y;
    m.data[10] = v.z;
    return m;
}


auto mat4_mul(Mat4 a, Mat4 b) -> Mat4 {
    auto m = Mat4();
    for (int x = 0; x < 16; x++) {
        m.data.push_back(0.0);
    }
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            auto sum = 0.0;
            for (int k = 0; k < 4; k++) {
                sum = sum + a.data[k * 4 + j] * b.data[i * 4 + k];
            }
            m.data[i * 4 + j] = sum;
        }
    }
    return m;
}


auto mat4_transform_point(Mat4 m, Vec3 v) -> Vec3 {
    const auto x = m.data[0] * v.x + m.data[4] * v.y + m.data[8] * v.z + m.data[12];
    const auto y = m.data[1] * v.x + m.data[5] * v.y + m.data[9] * v.z + m.data[13];
    const auto z = m.data[2] * v.x + m.data[6] * v.y + m.data[10] * v.z + m.data[14];
    const auto w = m.data[3] * v.x + m.data[7] * v.y + m.data[11] * v.z + m.data[15];
    

    if ((math::abs(w) > 0.0001)) {
        return vec3_new(x / w, y / w, z / w);
    }
    return vec3_new(x, y, z);
}


// ============================================================;
// TRANSFORM;
// ============================================================;


auto transform_matrix(Transform t) -> Mat4 {
    const auto translation = mat4_translation(t.position);
    const auto rotation_x = mat4_rotation_x(t.rotation.x);
    const auto rotation_y = mat4_rotation_y(t.rotation.y);
    const auto rotation_z = mat4_rotation_z(t.rotation.z);
    const auto scale = mat4_scale(t.scale);
    

    return mat4_mul(mat4_mul(mat4_mul(translation, rotation_z), rotation_y), mat4_mul(rotation_x, scale));
}


auto transform_new() -> Transform {
    auto t = Transform();
    t.position = vec3_zero();
    t.rotation = vec3_zero();
    t.scale = vec3_one();
    return t;
}


// ============================================================;
// CORES;
// ============================================================;


auto color_new(double r, double g, double b, double a) -> Color {
    auto c = Color();
    c.r = r;
    c.g = g;
    c.b = b;
    c.a = a;
    return c;
}


auto color_white() -> Color {
    return color_new(1.0, 1.0, 1.0, 1.0);
}


auto color_black() -> Color {
    return color_new(0.0, 0.0, 0.0, 1.0);
}


auto color_mul(Color c, double scalar) -> Color {
    return color_new(c.r * scalar, c.g * scalar, c.b * scalar, c.a);
}


auto color_mul_color(Color a, Color b) -> Color {
    return color_new(a.r * b.r, a.g * b.g, a.b * b.b, a.a * b.a);
}


auto color_add(Color a, Color b) -> Color {
    return color_new(a.r + b.r, a.g + b.g, a.b + b.b, a.a + b.a);
}


auto color_clamp(Color c) -> Color {
    return color_new(math::max(0.0, math::min(1.0, c.r)), math::max(0.0, math::min(1.0, c.g)), math::max(0.0, math::min(1.0, c.b)), math::max(0.0, math::min(1.0, c.a)));
}


// ============================================================;
// FRAMEBUFFER;
// ============================================================;


auto framebuffer_new(int width, int height) -> Framebuffer {
    auto fb = Framebuffer();
    fb.width = width;
    fb.height = height;
    

    const auto total = width * height;
    for (int i = 0; i < total; i++) {
        fb.pixels.push_back(color_black());
        fb.depth.push_back(999999.0);
    }
    

    return fb;
}


auto framebuffer_clear(Framebuffer& fb, Color color) -> void {
    const auto len = fb.pixels.size();
    for (int i = 0; i < len; i++) {
        fb.pixels[i] = color;
        fb.depth[i] = 999999.0;
    }
}


auto framebuffer_set_pixel(Framebuffer& fb, int x, int y, Color c) -> void {
    if (x >= 0 { if x < fb.width { if y >= 0 { if y < fb.height) {
        fb.pixels[y * fb.width + x] = c;
    } }}};
}


auto framebuffer_set_depth(Framebuffer& fb, int x, int y, double depth) -> bool {
    if (x >= 0 { if x < fb.width { if y >= 0 { if y < fb.height) {
        if (depth < fb.depth[y * fb.width + x]) {
            fb.depth[y * fb.width + x] = depth;
            return true;
        }
    } }}};
    return false;
}


// ============================================================;
// RASTERIZACAO;
// ============================================================;


auto edge_function(Vec2 a, Vec2 b, Vec2 c) -> double {
    return (c.x - a.x) * (b.y - a.y) - (c.y - a.y) * (b.x - a.x);
}


auto draw_triangle(Framebuffer& fb, Vec3 v0, Vec3 v1, Vec3 v2, Color c0, Color c1, Color c2) -> void {
    const auto half_w = fb.width * 0.5;
    const auto half_h = fb.height * 0.5;
    

    auto p0 = Vec2();
    p0.x = (v0.x + 1.0) * half_w; p0.y = (1.0 - v0.y) * half_h;
    auto p1 = Vec2();
    p1.x = (v1.x + 1.0) * half_w; p1.y = (1.0 - v1.y) * half_h;
    auto p2 = Vec2();
    p2.x = (v2.x + 1.0) * half_w; p2.y = (1.0 - v2.y) * half_h;
    

    auto min_x = math::min(p0.x, math::min(p1.x, p2.x));
    auto max_x = math::max(p0.x, math::max(p1.x, p2.x));
    auto min_y = math::min(p0.y, math::min(p1.y, p2.y));
    auto max_y = math::max(p0.y, math::max(p1.y, p2.y));
    

    if (min_x < 0.0) { min_x = 0.0; };
    if (max_x > fb.width - 1.0) { max_x = fb.width - 1.0; };
    if (min_y < 0.0) { min_y = 0.0; };
    if (max_y > fb.height - 1.0) { max_y = fb.height - 1.0; };
    

    const auto area = edge_function(p0, p1, p2);
    if ((math::abs(area) < 0.0001)) {
        return;
    }
    

    const auto imin_y = min_y;
    const auto imax_y = max_y;
    const auto imin_x = min_x;
    const auto imax_x = max_x;
    

    for (int y = imin_y; y < imax_y; y++) {
        for (int x = imin_x; x < imax_x; x++) {
            auto p = Vec2();
            p.x = x + 0.5; p.y = y + 0.5;
            

            const auto w0 = edge_function(p1, p2, p) / area;
            const auto w1 = edge_function(p2, p0, p) / area;
            const auto w2 = edge_function(p0, p1, p) / area;
            

            if (w0 >= 0.0 { if w1 >= 0.0 { if w2 >= 0.0) {
                const auto z = v0.z * w0 + v1.z * w1 + v2.z * w2;
                if (framebuffer_set_depth(fb, x, y, z)) {
                    auto px_c = Color();
                    px_c.r = c0.r * w0 + c1.r * w1 + c2.r * w2;
                    px_c.g = c0.g * w0 + c1.g * w1 + c2.g * w2;
                    px_c.b = c0.b * w0 + c1.b * w1 + c2.b * w2;
                    px_c.a = c0.a * w0 + c1.a * w1 + c2.a * w2;
                    framebuffer_set_pixel(fb, x, y, px_c);
                }
            } }};
        }
    }
}


// ============================================================;
// MALHAS 3D PRE-DEFINIDAS;
// ============================================================;


auto triangle_new(Vec3 v0, Vec3 v1, Vec3 v2, Vec3 normal) -> Triangle {
    auto t = Triangle();
    auto uv = Vec2(); uv.x = 0.0; uv.y = 0.0;
    

    auto vert0 = Vertex(); vert0.position = v0; vert0.normal = normal; vert0.uv = uv; vert0.color = color_white();
    auto vert1 = Vertex(); vert1.position = v1; vert1.normal = normal; vert1.uv = uv; vert1.color = color_white();
    auto vert2 = Vertex(); vert2.position = v2; vert2.normal = normal; vert2.uv = uv; vert2.color = color_white();
    

    t.v0 = vert0;
    t.v1 = vert1;
    t.v2 = vert2;
    return t;
}


auto mesh_cube(double size) -> Mesh {
    const auto s = size * 0.5;
    auto m = Mesh();
    m.name = "cube"s;
    

    const auto normal_z = vec3_new(0.0, 0.0, 1.0);
    const auto normal_z_neg = vec3_new(0.0, 0.0, -1.0);
    const auto normal_y = vec3_new(0.0, 1.0, 0.0);
    const auto normal_y_neg = vec3_new(0.0, -1.0, 0.0);
    const auto normal_x = vec3_new(1.0, 0.0, 0.0);
    const auto normal_x_neg = vec3_new(-1.0, 0.0, 0.0);
    

    m.triangles.push_back(triangle_new(vec3_new(-s, -s, s), vec3_new(s, -s, s), vec3_new(s, s, s), normal_z));
    m.triangles.push_back(triangle_new(vec3_new(-s, -s, s), vec3_new(s, s, s), vec3_new(-s, s, s), normal_z));
    m.triangles.push_back(triangle_new(vec3_new(s, -s, -s), vec3_new(-s, -s, -s), vec3_new(-s, s, -s), normal_z_neg));
    m.triangles.push_back(triangle_new(vec3_new(s, -s, -s), vec3_new(-s, s, -s), vec3_new(s, s, -s), normal_z_neg));
    m.triangles.push_back(triangle_new(vec3_new(-s, s, s), vec3_new(s, s, s), vec3_new(s, s, -s), normal_y));
    m.triangles.push_back(triangle_new(vec3_new(-s, s, s), vec3_new(s, s, -s), vec3_new(-s, s, -s), normal_y));
    m.triangles.push_back(triangle_new(vec3_new(-s, -s, -s), vec3_new(s, -s, -s), vec3_new(s, -s, s), normal_y_neg));
    m.triangles.push_back(triangle_new(vec3_new(-s, -s, -s), vec3_new(s, -s, s), vec3_new(-s, -s, s), normal_y_neg));
    m.triangles.push_back(triangle_new(vec3_new(s, -s, s), vec3_new(s, -s, -s), vec3_new(s, s, -s), normal_x));
    m.triangles.push_back(triangle_new(vec3_new(s, -s, s), vec3_new(s, s, -s), vec3_new(s, s, s), normal_x));
    m.triangles.push_back(triangle_new(vec3_new(-s, -s, -s), vec3_new(-s, -s, s), vec3_new(-s, s, s), normal_x_neg));
    m.triangles.push_back(triangle_new(vec3_new(-s, -s, -s), vec3_new(-s, s, s), vec3_new(-s, s, -s), normal_x_neg));
    

    return m;
}


auto mesh_sphere(double radius, int segments) -> Mesh {
    auto m = Mesh();
    m.name = "sphere"s;
    

    for (int lat = 0; lat < segments; lat++) {
        const auto theta1 = 3.14159265 * lat / segments;
        const auto theta2 = 3.14159265 * (lat + 1) / segments;
        

        for (int lon = 0; lon < segments; lon++) {
            const auto phi1 = 2.0 * 3.14159265 * lon / segments;
            const auto phi2 = 2.0 * 3.14159265 * (lon + 1) / segments;
            

            const auto p1 = vec3_new(radius * math::sin(theta1) * math::cos(phi1), radius * math::cos(theta1), radius * math::sin(theta1) * math::sin(phi1));
            const auto p2 = vec3_new(radius * math::sin(theta2) * math::cos(phi1), radius * math::cos(theta2), radius * math::sin(theta2) * math::sin(phi1));
            const auto p3 = vec3_new(radius * math::sin(theta2) * math::cos(phi2), radius * math::cos(theta2), radius * math::sin(theta2) * math::sin(phi2));
            const auto p4 = vec3_new(radius * math::sin(theta1) * math::cos(phi2), radius * math::cos(theta1), radius * math::sin(theta1) * math::sin(phi2));
            

            m.triangles.push_back(triangle_new(p1, p2, p3, vec3_normalize(p1)));
            m.triangles.push_back(triangle_new(p1, p3, p4, vec3_normalize(p1)));
        }
    }
    

    return m;
}


auto mesh_plane(double size) -> Mesh {
    const auto s = size * 0.5;
    const auto normal = vec3_new(0.0, 1.0, 0.0);
    

    auto m = Mesh();
    m.name = "plane"s;
    m.triangles.push_back(triangle_new(vec3_new(-s, 0.0, -s), vec3_new(s, 0.0, -s), vec3_new(s, 0.0, s), normal));
    m.triangles.push_back(triangle_new(vec3_new(-s, 0.0, -s), vec3_new(s, 0.0, s), vec3_new(-s, 0.0, s), normal));
    return m;
}


// ============================================================;
// ILUMINACAO PHONG;
// ============================================================;


auto phong_shade(Vec3 normal, Vec3 position, Vec3 camera_pos, Light light, Material material) -> Color {
    const auto ambient = color_mul(material.ambient, light.intensity * 0.1);
    

    const auto light_dir = vec3_normalize(vec3_sub(light.position, position));
    const auto diff = math::max(0.0, vec3_dot(normal, light_dir));
    const auto diffuse = color_mul(color_mul_color(material.diffuse, light.color), diff * light.intensity);
    

    const auto view_dir = vec3_normalize(vec3_sub(camera_pos, position));
    const auto reflect_dir = vec3_reflect(vec3_mul(light_dir, -1.0), normal);
    const auto spec = math::pow(math::max(0.0, vec3_dot(view_dir, reflect_dir)), material.shininess);
    const auto specular = color_mul(color_mul_color(material.specular, light.color), spec * light.intensity);
    

    return color_clamp(color_add(color_add(ambient, diffuse), specular));
}


// ============================================================;
// SCENE E RENDERIZACAO;
// ============================================================;


auto scene_new() -> Scene {
    auto s = Scene();
    s.ambient_color = color_new(0.1, 0.1, 0.1, 1.0);
    s.camera.position = vec3_new(0.0, 0.0, 5.0);
    s.camera.target = vec3_zero();
    s.camera.up = vec3_new(0.0, 1.0, 0.0);
    s.camera.fov = 1.047;
    s.camera.z_near = 0.1;
    s.camera.z_far = 100.0;
    return s;
}


auto scene_add_mesh(Scene& scene, Mesh mesh, Transform transform, Material material) -> void {
    auto obj = SceneObject();
    obj.mesh = mesh;
    obj.transform = transform;
    obj.material = material;
    scene.meshes.push_back(obj);
}


auto scene_add_light(Scene& scene, Light light) -> void {
    scene.lights.push_back(light);
}


auto render(const Scene& scene, Framebuffer& fb) -> void {
    framebuffer_clear(fb, scene.ambient_color);
    

    const auto aspect = fb.width / (fb.height * 2.0);
    const auto view_matrix = mat4_look_at(scene.camera.position, scene.camera.target, scene.camera.up);
    const auto proj_matrix = mat4_perspective(scene.camera.fov, aspect, scene.camera.z_near, scene.camera.z_far);
    const auto vp_matrix = mat4_mul(proj_matrix, view_matrix);
    

    const auto mlen = scene.meshes.size();
    for (int i = 0; i < mlen; i++) {
        const auto obj = scene.meshes[i];
        const auto model_matrix = transform_matrix(obj.transform);
        const auto mvp_matrix = mat4_mul(vp_matrix, model_matrix);
        

        const auto tlen = obj.mesh.triangles.size();
        for (int j = 0; j < tlen; j++) {
            const auto tri = obj.mesh.triangles[j];
            

            const auto v0_world = mat4_transform_point(model_matrix, tri.v0.position);
            const auto v1_world = mat4_transform_point(model_matrix, tri.v1.position);
            const auto v2_world = mat4_transform_point(model_matrix, tri.v2.position);
            

            const auto normal = vec3_normalize(vec3_cross(vec3_sub(v1_world, v0_world), vec3_sub(v2_world, v0_world)));
            

            const auto view_dir = vec3_normalize(vec3_sub(scene.camera.position, v0_world));
            if (vec3_dot(normal, view_dir) > 0.0) {
                const auto v0_clip = mat4_transform_point(mvp_matrix, tri.v0.position);
                const auto v1_clip = mat4_transform_point(mvp_matrix, tri.v1.position);
                const auto v2_clip = mat4_transform_point(mvp_matrix, tri.v2.position);
                

                auto c0 = obj.material.ambient;
                auto c1 = obj.material.ambient;
                auto c2 = obj.material.ambient;
                

                if (scene.lights.size() > 0) {
                    const auto light = scene.lights[0];
                    c0 = phong_shade(normal, v0_world, scene.camera.position, light, obj.material);
                    c1 = phong_shade(normal, v1_world, scene.camera.position, light, obj.material);
                    c2 = phong_shade(normal, v2_world, scene.camera.position, light, obj.material);
                }
                

                draw_triangle(fb, v0_clip, v1_clip, v2_clip, c0, c1, c2);
            }
        }
    }
}


auto framebuffer_to_terminal(const Framebuffer& fb, int width, int height) -> void {
    const auto chars_len = 10;
    

    for (int y = 0; y < height; y++) {
        auto row_str = ""s;
        for (int x = 0; x < width; x++) {
            const auto px_color = fb.pixels[y * fb.width + x];
            const auto luminance = px_color.r * 0.299 + px_color.g * 0.587 + px_color.b * 0.114;
            

            auto idx = luminance * chars_len;
            if idx >= chars_len { idx = chars_len - 1; };
            if idx < 0 { idx = 0; };
            

            auto c = " "s;
            if idx == 0 { c = " "s; };
            if idx == 1 { c = "."s; };
            if idx == 2 { c = ":"s; };
            if idx == 3 { c = "-"s; };
            if idx == 4 { c = "="s; };
            if idx == 5 { c = "+"s; };
            if idx == 6 { c = "*"s; };
            if idx == 7 { c = "#"s; };
            if idx == 8 { c = "%"s; };
            if idx == 9 { c = "@"s; };
            

            const auto cr = px_color.r * 255.0;
            const auto cg = px_color.g * 255.0;
            const auto cb = px_color.b * 255.0;
            

            row_str = row_str + terminal::fg_color(16 + (cr / 51.0)*36 + (cg / 51.0)*6 + (cb / 51.0)) + c;
        }
        print(row_str + terminal::reset() + "\n"s);
    }
}


/// Módulo de controle do terminal;
/// Manipulação de cores ANSI, cursor e tela;


auto clear() -> void {
    syscall.terminal_clear();
}


auto fg_color(int code) -> std::string {
    return "\033[38;5;"s + stdlib::to_string(code) + "m"s;
}


auto bg_color(int code) -> std::string {
    return "\033[48;5;"s + stdlib::to_string(code) + "m"s;
}


auto reset() -> std::string {
    return "\033[0m"s;
}


auto bold() -> std::string {
     return "\033[1m"s;
}
auto dim() -> std::string {
     return "\033[2m"s;
}
auto italic() -> std::string {
     return "\033[3m"s;
}
auto underline() -> std::string {
     return "\033[4m"s;
}
auto blink() -> std::string {
     return "\033[5m"s;
}
auto reverse() -> std::string {
     return "\033[7m"s;
}
auto hidden() -> std::string {
     return "\033[8m"s;
}


auto rgb_fg(int r, int g, int b) -> std::string {
    return "\033[38;2;"s + stdlib::to_string(r) + ";"s + stdlib::to_string(g) + ";"s + stdlib::to_string(b) + "m"s;
}


auto rgb_bg(int r, int g, int b) -> std::string {
    return "\033[48;2;"s + stdlib::to_string(r) + ";"s + stdlib::to_string(g) + ";"s + stdlib::to_string(b) + "m"s;
}


auto cursor_up(int n) -> std::string {
    return "\033["s + stdlib::to_string(n) + "A"s;
}


auto cursor_down(int n) -> std::string {
    return "\033["s + stdlib::to_string(n) + "B"s;
}


auto cursor_forward(int n) -> std::string {
    return "\033["s + stdlib::to_string(n) + "C"s;
}


auto cursor_back(int n) -> std::string {
    return "\033["s + stdlib::to_string(n) + "D"s;
}


auto cursor_pos(int row, int col) -> std::string {
    return "\033["s + stdlib::to_string(row) + ";"s + stdlib::to_string(col) + "H"s;
}


auto clear_line() -> std::string {
    return "\033[2K"s;
}


int main(int argc, char** argv) {
    terminal::clear();
    

    // Criar cena;
    auto scene = scene_new();
    

    // Câmera giratória;
    scene.camera.position = vec3_new(0.0, 3.0, 8.0);
    scene.camera.target = vec3_zero();
    

    // Adicionar esfera;
    const auto sphere = mesh_sphere(1.0, 16);
    const auto sphere_transform = transform_new();
    auto sphere_material = Material();
    sphere_material.ambient = color_new(0.2, 0.1, 0.1, 1.0);
    sphere_material.diffuse = color_new(0.8, 0.2, 0.2, 1.0);
    sphere_material.specular = color_white();
    sphere_material.shininess = 32.0;
    scene_add_mesh(scene, sphere, sphere_transform, sphere_material);
    

    // Adicionar plano (chão);
    const auto plane = mesh_plane(10.0);
    auto plane_transform = transform_new();
    plane_transform.position = vec3_new(0.0, -2.0, 0.0);
    auto plane_material = Material();
    plane_material.ambient = color_new(0.1, 0.1, 0.2, 1.0);
    plane_material.diffuse = color_new(0.3, 0.3, 0.8, 1.0);
    plane_material.specular = color_new(0.5, 0.5, 0.5, 1.0);
    plane_material.shininess = 8.0;
    scene_add_mesh(scene, plane, plane_transform, plane_material);
    

    // Adicionar luz;
    auto light = Light();
    light.position = vec3_new(5.0, 8.0, 5.0);
    light.color = color_white();
    light.intensity = 1.5;
    scene_add_light(scene, light);
    

    // Framebuffer;
    auto fb = framebuffer_new(80, 30);
    

    // Loop de animação;
    auto angle = 0.0;
    for (int frame = 0; frame < 100; frame++) {
        // Girar câmera;
        angle = angle + 0.05;
        scene.camera.position.x = math::sin(angle) * 8.0;
        scene.camera.position.z = math::cos(angle) * 8.0;
        scene.camera.target = vec3_zero();
        

        // Renderizar;
        render(scene, fb);
        

        // Exibir no terminal;
        terminal::clear();
        framebuffer_to_terminal(fb, 80, 30);
        

        stdlib::time::sleep(50);
    }
}
