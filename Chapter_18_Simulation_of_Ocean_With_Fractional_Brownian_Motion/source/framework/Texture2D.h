#pragma once

class Texture2D
{
    public:
        static Texture2D *CreateFromFile( const char *file_name, bool flip_vertically = false);

        Texture2D(
            unsigned int width, unsigned int height,
            GLenum internal_format, GLenum format, GLenum type,
            GLenum min_filter = GL_NEAREST, GLenum mag_filter = GL_NEAREST,
            GLenum wrap_s = GL_REPEAT, GLenum wrap_t = GL_REPEAT,
            const GLvoid *data = nullptr
        );

        ~Texture2D();

        void SetWrapParameter( GLenum wrap_s, GLenum wrap_t);
        void SetFilterParameter( GLenum min_filter, GLenum mag_filter);

        void Bind( const GLuint texture_unit = 0)
        {
            glActiveTexture( GL_TEXTURE0 + texture_unit);
            glBindTexture( GL_TEXTURE_2D, m_id);
        }

        void Unbind(const GLuint texture_unit = 0)
        {
            glActiveTexture( GL_TEXTURE0 + texture_unit);
            glBindTexture( GL_TEXTURE_2D, 0);
        }

        void BindImage( GLuint texture_uint, GLenum access, GLenum internal_format);
        void GenerateMipmap();

        inline GLuint GetID() const { return m_id; };
        inline GLuint GetWidth() const { return m_width; };
        inline GLuint GetHeight() const { return m_height; };
        inline GLenum GetInternalFormat() const { return m_internal_format; };
        inline GLenum GetFormat() const { return m_format; };

        void Release();


    public:
        static const GLenum type = GL_TEXTURE_2D;

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
