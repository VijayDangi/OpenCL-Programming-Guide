#include "../Common.h"
#include "Atmosphere.h"

#include "../framework/ShaderProgram.h"
#include "../framework/Buffer.h"
#include "../framework/Texture2D.h"
#include "../framework/TextureCubeMap.h"

#include "../system/FileIO.h"
#include "../system/Keyboard.h"

#include <string>
#include <sstream>

#define DATA_FILE_PATH "resource/data/atmosphere.ini"
#define TO_STRING(x)    #x

namespace AtmospherePostProcess
{
    static ShaderProgram *program = nullptr;
    static VertexArray *empty_vertex_array = nullptr;

    static float u_fog_density = 0.002800f;
    static float u_fog_offset = 200.0f;
    static float u_fog_color[3] = { 1.0f, 1.0f, 1.0f};

    static float u_fog_height = 500.0f;
    static float u_fog_attenuation = 1.2f;

    vmath::vec3 u_sun_direction;
    vmath::vec3 u_sun_color;
    static float u_sun_exponetial = 3500.0f;

    static float u_sky_box_direction[3] = { 0.0, -1.0, 0.0};
    static float u_sky_box_speed = 0.1;

    static float u_time = 0.0;

    /**
     * @brief LoadData()
     */
    static void LoadData()
    {
        std::string data;
        if( FileIO::ReadFile( DATA_FILE_PATH, data))
        {
            std::string line;
            std::string token_key;
            std::string token_value;

            std::stringstream sd(data);

#define READ_TOKEN_FLOAT( x, token)    \
        {   \
            getline( ss, token_value, token); \
            x = (float)atof( token_value.c_str()); \
        }

#define READ_TOKEN_FLOAT2( x, token)    \
        {   \
            getline( ss, token_value, token); \
            x[0] = (float)atof( token_value.c_str()); \
            \
            getline( ss, token_value, token); \
            x[1] = (float)atof( token_value.c_str()); \
        }

#define READ_TOKEN_FLOAT3( x, token)    \
        {   \
            getline( ss, token_value, token); \
            x[0] = (float)atof( token_value.c_str()); \
            \
            getline( ss, token_value, token); \
            x[1] = (float)atof( token_value.c_str()); \
            \
            getline( ss, token_value, token); \
            x[2] = (float)atof( token_value.c_str()); \
        }

#define READ_TOKEN_INT( x, token)    \
        {   \
            getline( ss, token_value, token); \
            x = atoi( token_value.c_str()); \
        }

            while( getline( sd, line, '\n'))
            {
                // get token
                std::stringstream ss(line);
                getline( ss, token_key, '=');

                if( 0 == line.size())
                {
                    continue;
                }

                    // Fog
                if( TO_STRING( u_fog_density) == token_key)
                {
                    READ_TOKEN_FLOAT(u_fog_density, ',');
                }
                else if( TO_STRING( u_fog_offset) == token_key)
                {
                    READ_TOKEN_FLOAT(u_fog_offset, ',');
                }
                else if( TO_STRING( u_fog_color) == token_key)
                {
                    READ_TOKEN_FLOAT3(u_fog_color, ',');
                }
                else if( TO_STRING( u_fog_height) == token_key)
                {
                    READ_TOKEN_FLOAT(u_fog_height, ',');
                }
                else if( TO_STRING( u_fog_attenuation) == token_key)
                {
                    READ_TOKEN_FLOAT(u_fog_attenuation, ',');
                }

                    // sun
                else if( TO_STRING( u_sun_direction) == token_key)
                {
                    READ_TOKEN_FLOAT3(u_sun_direction, ',');
                }
                else if( TO_STRING( u_sun_color) == token_key)
                {
                    READ_TOKEN_FLOAT3(u_sun_color, ',');
                }
                else if( TO_STRING( u_sun_exponetial) == token_key)
                {
                    READ_TOKEN_FLOAT(u_sun_exponetial, ',');
                }

                    // sky
                else if( TO_STRING( u_sky_box_direction) == token_key)
                {
                    READ_TOKEN_FLOAT3(u_sky_box_direction, ',');
                }
                else if( TO_STRING( u_sky_box_speed) == token_key)
                {
                    READ_TOKEN_FLOAT(u_sky_box_speed, ',');
                }

            }

#undef READ_TOKEN_FLOAT
#undef READ_TOKEN_FLOAT2
#undef READ_TOKEN_FLOAT3
#undef READ_TOKEN_INT

        }
    }

    /**
     * @brief Initialize()
     * @return 
     */
    bool Initialize()
    {
        // code
        program = new ShaderProgram;
        if( program == nullptr)
        {
            Log("Error: Memory Allocation Failed.");
            return false;
        }

        if( program->AddShaderFromFile( "resource/shaders/atmosphere/vertex_shader.glsl", GL_VERTEX_SHADER) == false)
        {
            Log("Error: Vertex Shader Failed.");
            return false;
        }

        if( program->AddShaderFromFile( "resource/shaders/atmosphere/fragment_shader.glsl", GL_FRAGMENT_SHADER) == false)
        {
            Log("Error: Fragment Shader Failed.");
            return false;
        }

        if( program->Build() == false)
        {
            Log("Error: Program Build Failed.");
            return false;
        }

            // buffer
        empty_vertex_array = new VertexArray;

            // initialize variables
        LoadData();

        return true;
    }
    
    /**
     * @brief Save()
     */
    void Save()
    {
        // code
        std::string data;

        data.append( "[FOG]");
        data.append( "\n" TO_STRING( u_fog_density)     "=" + std::to_string(u_fog_density));
        data.append( "\n" TO_STRING( u_fog_offset)      "=" + std::to_string(u_fog_offset));
        data.append( "\n" TO_STRING( u_fog_color)       "=" + std::to_string(u_fog_color[0]) + ", " + std::to_string(u_fog_color[1]) + ", " + std::to_string(u_fog_color[2]));
        data.append( "\n" TO_STRING( u_fog_height)      "=" + std::to_string(u_fog_height));
        data.append( "\n" TO_STRING( u_fog_attenuation) "=" + std::to_string(u_fog_attenuation));
        data.append( "\n\n");

        data.append( "[SUN]");
        data.append( "\n" TO_STRING( u_sun_direction)   "=" + std::to_string(u_sun_direction[0]) + ", " + std::to_string(u_sun_direction[1]) + ", " + std::to_string(u_sun_direction[2]));
        data.append( "\n" TO_STRING( u_sun_color)       "=" + std::to_string(u_sun_color[0])     + ", " + std::to_string(u_sun_color[1])     + ", " + std::to_string(u_sun_color[2]));
        data.append( "\n" TO_STRING( u_sun_exponetial)  "=" + std::to_string(u_sun_exponetial));
        data.append( "\n\n");

        data.append( "[SKY]");
        data.append( "\n" TO_STRING( u_sky_box_direction)   "=" + std::to_string(u_sky_box_direction[0]) + ", " + std::to_string(u_sky_box_direction[1]) + ", " + std::to_string(u_sky_box_direction[2]));
        data.append( "\n" TO_STRING( u_sky_box_speed)  "=" + std::to_string(u_sky_box_speed));
        data.append( "\n\n");

        FileIO::Write( DATA_FILE_PATH, data);
    }

    /**
     * @brief RenderImGui()
     */
    void RenderImGui()
    {
        // code
        if( ImGui::TreeNode("Atmosphere"))
        {
            if( ImGui::TreeNode("Fog Parameter"))
            {
                ImGui::DragFloat( "Fog Density", &u_fog_density, 0.0001f, 0.0f, 3.0f, "%.6f");
                ImGui::DragFloat( "Fog Offset", &u_fog_offset, 0.1f, 0.0f, 1000.0f, "%.6f");
                ImGui::ColorEdit3( "Fog Color", &u_fog_color[0], ImGuiColorEditFlags_Float);

                ImGui::DragFloat( "Fog Height", &u_fog_height, 0.1f, 0.0f, 1000.0f, "%.6f");
                ImGui::DragFloat( "Fog Attenuation", &u_fog_attenuation, 0.001f, 0.0f, 5.0f, "%.6f");

                ImGui::TreePop();
            }

            if( ImGui::TreeNode("Sun Parameter"))
            {
                ImGui::ColorEdit3( "Sun Color", &u_sun_color[0], ImGuiColorEditFlags_Float);
                ImGui::DragFloat3( "Sun Direction", &u_sun_direction[0], 0.001f, -1.0f, 1.0f, "%.6f");
                ImGui::DragFloat( "Sun Exponetial", &u_sun_exponetial, 0.1f, -5000.0f, 5000.0f, "%.6f");

                ImGui::TreePop();
            }

            if( ImGui::TreeNode("Sky Parameter"))
            {
                ImGui::DragFloat3( "SkyBox Direction", u_sky_box_direction, 0.001f, -1.0f, 1.0f, "%.6f");
                ImGui::DragFloat( "SkyBox Speed", &u_sky_box_speed, 0.001f, 0.0f, 2.0f, "%.6f");

                ImGui::TreePop();
            }

            if( ImGui::Button("Save"))
            {
                Save();
            }

            ImGui::TreePop();
        }
    }
    
    /**
     * @brief Process()
     * @param color_texture 
     * @param depth_texture 
     * @return 
     */
    bool Process(
        Texture2D *color_texture, Texture2D *depth_texture,
        TextureCubeMap *p_env_map,
        float near_plane, float far_plane,
        vmath::vec3& camera_position,
        vmath::mat4 view_matrix, vmath::mat4 projection_matrix)
    {
        // code
        if( !color_texture || !depth_texture || !program || !p_env_map)
        {
            Log("Error: Invalide Parameter.");
            return false;
        }

        program->Bind();
            program->SetUniformInt( "u_color_buffer", 0);
            program->SetUniformInt( "u_depth_buffer", 1);
            program->SetUniformInt( "u_sky_map", 2);

            program->SetUniformFloat( "u_time", u_time);

            program->SetUniformFloat( "u_near_plane", near_plane);
            program->SetUniformFloat( "u_far_plane", far_plane);

            program->SetUniformFloat( "u_fog_density", u_fog_density);
            program->SetUniformFloat( "u_fog_offset", u_fog_offset);
            program->SetUniformFloat3( "u_fog_color", u_fog_color[0], u_fog_color[1], u_fog_color[2]);

            program->SetUniformFloat( "u_fog_height", u_fog_height);
            program->SetUniformFloat( "u_fog_attenuation", u_fog_attenuation);

            program->SetMatrix4x4( "u_view", GL_FALSE, view_matrix);
            program->SetMatrix4x4( "u_projection", GL_FALSE, projection_matrix);

            program->SetUniformFloat3( "u_camera_position", camera_position[0], camera_position[1], camera_position[2]);

            program->SetUniformFloat3( "u_sun_direction", u_sun_direction[0], u_sun_direction[1], u_sun_direction[2]);
            program->SetUniformFloat3( "u_sun_color", u_sun_color[0], u_sun_color[1], u_sun_color[2]);
            program->SetUniformFloat( "u_sun_exponetial", u_sun_exponetial);

            program->SetUniformFloat3( "u_sky_box_direction", u_sky_box_direction[0], u_sky_box_direction[1], u_sky_box_direction[2]);
            program->SetUniformFloat( "u_sky_box_speed", u_sky_box_speed);

            color_texture->Bind(0);
            depth_texture->Bind(1);
            p_env_map->Bind(2);

            empty_vertex_array->Bind();
            glDrawArrays( GL_TRIANGLES, 0, 6);
            empty_vertex_array->Unbind();

            color_texture->Unbind(0);
            depth_texture->Unbind(1);
            p_env_map->Unbind(2);
        program->Unbind();

        return true;
    }

    /**
     * @brief Update()
     * @param delta_time 
     */
    void Update( float delta_time)
    {
        // code
        u_time += delta_time;
    }
    
    /**
     * @brief Uninitialize()
     */
    void Uninitialize()
    {
        // code
        if( program)
        {
            program->Release();
            delete program;
            program = nullptr;
        }

        if( empty_vertex_array)
        {
            delete empty_vertex_array;
            empty_vertex_array = nullptr;
        }
    }
    
} // namespace AtmospherePostProcess

