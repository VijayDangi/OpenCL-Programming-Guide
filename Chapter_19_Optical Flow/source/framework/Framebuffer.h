#pragma once

class Texture2D;
class Texture2DMultisample;

class Framebuffer
{
    public:
        Framebuffer();
        ~Framebuffer();

        void Bind( GLenum target);
        void Unbind( GLenum target);

        void AttachTexture(
            GLenum framebuffer_target, GLenum attachment_target,
            Texture2D *texture, int level
        );

        void AttachTexture(
            GLenum framebuffer_target, GLenum attachment_target,
            Texture2DMultisample *texture, int level
        );

        void SetDrawBuffers( int buffer_count, GLenum *buffer_target_attchments);

        void Release();

        GLuint GetID()
        {
            return m_id;
        }

    private:
        bool IsValidTarget( GLenum target);

        GLuint m_id;
};
