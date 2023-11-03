#pragma once

class TextureCubeMap;

namespace SceneWaves
{
    bool Initialize();
    void Update( float delta_time);
    void RenderImGUI();
    void Render(
        vmath::mat4 view_matrix, vmath::mat4 projection_matrix, vmath::vec3& camera_position,
        vmath::vec3& sun_direction, vmath::vec3& sun_color,
        TextureCubeMap *p_env_map
        );
    void Uninitialize();
} // namespace Ocean

