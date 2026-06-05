#include "seed_lib.hpp"



int main(int argc, char** argv) {
terminal . clear ( ) let mut scene = render3d . scene_new ( ) scene . camera . position = render3d . vec3_new ( 0.0 , 3.0 , 8.0 ) scene . camera . target = render3d . vec3_zero ( ) let sphere = render3d . mesh_sphere ( 1.0 , 16 ) let sphere_transform = render3d . transform_new ( ) let mut sphere_material = render3d . Material ( ) ;
sphere_material . ambient = render3d . color_new ( 0.2 , 0.1 , 0.1 , 1.0 ) sphere_material . diffuse = render3d . color_new ( 0.8 , 0.2 , 0.2 , 1.0 ) sphere_material . specular = render3d . color_white ( ) sphere_material . shininess = 32.0 render3d . scene_add_mesh ( scene , sphere , sphere_transform , sphere_material ) let plane = render3d . mesh_plane ( 10.0 ) let mut plane_transform = render3d . transform_new ( ) plane_transform . position = render3d . vec3_new ( 0.0 , - 2.0 , 0.0 ) let mut plane_material = render3d . Material ( ) ;
plane_material . ambient = render3d . color_new ( 0.1 , 0.1 , 0.2 , 1.0 ) plane_material . diffuse = render3d . color_new ( 0.3 , 0.3 , 0.8 , 1.0 ) plane_material . specular = render3d . color_new ( 0.5 , 0.5 , 0.5 , 1.0 ) plane_material . shininess = 8.0 render3d . scene_add_mesh ( scene , plane , plane_transform , plane_material ) let mut light = render3d . Light ( ) ;
light . position = render3d . vec3_new ( 5.0 , 8.0 , 5.0 ) light . color = render3d . color_white ( ) light . intensity = 1.5 render3d . scene_add_light ( scene , light ) let mut fb = render3d . framebuffer_new ( 80 , 30 ) let mut angle = 0.0 for frame in 0 .. 100 { angle = angle + 0.05 scene . camera . position . x = std::sin angle ) * 8.0 scene . camera . position . z = std::cos angle ) * 8.0 scene . camera . target = render3d . vec3_zero ( ) render3d . render ( scene , fb ) terminal . clear ( ) render3d . framebuffer_to_terminal ( fb , 80 , 30 ) time . sleep ( 50 ) } } ;
}

