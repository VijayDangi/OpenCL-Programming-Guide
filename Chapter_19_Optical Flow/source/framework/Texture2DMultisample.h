#pragma once

class Texture2DMultisample
{
    public:
        Texture2DMultisample(
            unsigned int width, unsigned int height,
            GLenum internal_format, GLuint num_samples,
            GLboolean fixed_sample_location = GL_FALSE
        );

        ~Texture2DMultisample();

        void Bind( const GLuint texture_unit = 0)
        {
            glActiveTexture( GL_TEXTURE0 + texture_unit);
            glBindTexture( GL_TEXTURE_2D_MULTISAMPLE, m_id);
        }

        void Unbind(const GLuint texture_unit = 0)
        {
            glActiveTexture( GL_TEXTURE0 + texture_unit);
            glBindTexture( GL_TEXTURE_2D_MULTISAMPLE, 0);
        }
        
        inline GLuint GetID() const { return m_id; };
        inline GLuint GetWidth() const { return m_width; };
        inline GLuint GetHeight() const { return m_height; };
        inline GLenum GetInternalFormat() const { return m_internal_format; };

        void Release();

    public:
        static const GLenum type = GL_TEXTURE_2D_MULTISAMPLE;

    private:
        GLuint m_id = 0;
        GLuint m_width = 0;
        GLuint m_height = 0;
        GLenum m_internal_format = GL_NONE;
        // GLenum m_wrap_s;
        // GLenum m_wrap_t;
        // GLenum m_min_filter;
        // GLenum m_mag_filter;
};
