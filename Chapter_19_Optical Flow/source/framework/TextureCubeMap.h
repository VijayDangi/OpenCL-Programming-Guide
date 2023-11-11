#pragma once


class TextureCubeMap
{
    public:
        static TextureCubeMap *CreateFromFiles(
            const char *pos_x_file_name, const char *neg_x_file_name,
            const char *pos_y_file_name, const char *neg_y_file_name,
            const char *pos_z_file_name, const char *neg_z_file_name,
            bool flip_vertically = false);

        TextureCubeMap();

        ~TextureCubeMap();

        /**
         * @param { tex_target} =>
         *      GL_TEXTURE_CUBE_MAP_POSITIVE_X, GL_TEXTURE_CUBE_MAP_NEGATIVE_X
         *      GL_TEXTURE_CUBE_MAP_POSITIVE_Y, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y
         *      GL_TEXTURE_CUBE_MAP_POSITIVE_Z, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
         */
        bool AddTextureData(
            GLenum tex_target, GLenum internal_format, GLenum format, GLenum type,
            int width, int height,
            // GLenum min_filter = GL_NEAREST, GLenum mag_filter = GL_NEAREST,
            // GLenum wrap_s = GL_REPEAT, GLenum wrap_t = GL_REPEAT,
            const GLvoid *data = nullptr
        );

        void SetWrapParameter( GLenum wrap_s, GLenum wrap_t);
        void SetFilterParameter( GLenum min_filter, GLenum mag_filter);

        void Bind( const GLuint texture_unit = 0)
        {
            glActiveTexture( GL_TEXTURE0 + texture_unit);
            glBindTexture( GL_TEXTURE_CUBE_MAP, m_id);
        }

        void Unbind(const GLuint texture_unit = 0)
        {
            glActiveTexture( GL_TEXTURE0 + texture_unit);
            glBindTexture( GL_TEXTURE_CUBE_MAP, 0);
        }

        //void BindImage( GLuint texture_uint, GLenum access, GLenum internal_format);
        void GenerateMipmap();

        inline GLuint GetID() const { return m_id; };
        inline GLuint GetWidth() const { return m_width; };
        inline GLuint GetHeight() const { return m_height; };
        inline GLenum GetInternalFormat() const { return m_internal_format; };
        inline GLenum GetFormat() const { return m_format; };

    public:
        static const GLenum type = GL_TEXTURE_CUBE_MAP;

    private:
        GLuint m_id = 0;
        GLuint m_width = 0;
        GLuint m_height = 0;
        GLenum m_internal_format = GL_NONE;
        GLenum m_format = GL_NONE;
        // GLenum m_wrap_s;
        // GLenum m_wrap_t;
        // GLenum m_min_filter;
        // GLenum m_mag_filter;
};
