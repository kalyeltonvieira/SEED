#ifndef STDLIB_STUB_H
#define STDLIB_STUB_H

#include <string>

// Minimal stubs for stdlib modules used in render3d_demo.seed

struct Vec3 {
    double x = 0, y = 0, z = 0;
};

struct Color {
    double r = 0, g = 0, b = 0, a = 1;
};

struct Transform {};
struct Material {
    Color ambient; Color diffuse; Color specular; double shininess = 0;
};
struct Mesh {};
struct Scene {};
struct Light {};
struct FrameBuffer {};

struct Render3D {
    inline Vec3 vec3_new(double x, double y, double z) { return {x, y, z}; }
    inline Vec3 vec3_zero() { return {0.0, 0.0, 0.0}; }
    inline Mesh mesh_sphere(double radius, int segments) { return {}; }
    inline Mesh mesh_plane(double size) { return {}; }
    inline Transform transform_new() { return {}; }
    inline Material Material() { return {}; }
    inline Color color_new(double r, double g, double b, double a) { return {r, g, b, a}; }
    inline Color color_white() { return {1.0, 1.0, 1.0, 1.0}; }
    inline Scene scene_new() { return {}; }
    inline void scene_add_mesh(Scene&, Mesh, Transform, Material) {}
    inline void scene_add_light(Scene&, Light) {}
    inline void render(Scene&, FrameBuffer) {}
    inline FrameBuffer framebuffer_new(int, int) { return {}; }
    inline void framebuffer_to_terminal(FrameBuffer&, int, int) {}
};
inline Render3D render3d;

struct Terminal {
    inline void clear() {}
    inline std::string fg_color(int) { return ""; }
    inline std::string reset() { return ""; }
    inline void print(const std::string&) {}
};
inline Terminal terminal;

struct Time {
    inline void sleep(int) {}
};
inline Time time;

#endif // STDLIB_STUB_H
