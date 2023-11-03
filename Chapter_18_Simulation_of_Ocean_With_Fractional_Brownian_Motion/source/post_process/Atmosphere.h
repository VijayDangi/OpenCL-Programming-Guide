#pragma once

class Texture2D;
class TextureCubeMap;

namespace AtmospherePostProcess
{
    // variables
    extern vmath::vec3 u_sun_direction;
    extern vmath::vec3 u_sun_color;

    // function declaration
    bool Initialize();
    void RenderImGui();
    bool Process(
        Texture2D *color_texture, Texture2D *depth_texture,
        TextureCubeMap *p_env_map,
        float near_plane, float far_plane,
        vmath::vec3& camera_position,
        vmath::mat4 view_matrix = vmath::mat4::identity(),
        vmath::mat4 projection_matrix = vmath::mat4::identity()
    );
    void Update( float delta_time);
    void Uninitialize();
} // namespace AtmospherePostProcess

