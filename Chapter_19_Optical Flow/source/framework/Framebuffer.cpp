#include "../Common.h"
#include "Texture2D.h"
#include "Texture2DMultisample.h"

#include "Framebuffer.h"

/**
 * @brief IsValidTarget()
 * @param target 
 * @return 
 */
bool Framebuffer::IsValidTarget( GLenum target)
{
    // code
    switch( target)
    {
        case GL_DRAW_FRAMEBUFFER:
        case GL_READ_FRAMEBUFFER:
        case GL_FRAMEBUFFER:
            return true;
    }

    return false;
}

/**
 * @brief Framebuffer()
 */
Framebuffer::Framebuffer()
{
    // code
    glGenFramebuffers( 1, &m_id);
}

/**
 * @brief ~Framebuffer()
 */
Framebuffer::~Framebuffer()
{
    // code
    Release();
}

/**
 * @brief Bind()
 * @param target 
 */
void Framebuffer::Bind( GLenum target)
{
    // code
    if(!IsValidTarget(target))
    {
        Log("Framebuffer Error:: Invalid target parameter provided to Framebuffer Object.");
        return;
    }
    glBindFramebuffer( target, m_id);
}

/**
 * @brief Unbind()
 * @param target 
 */
void Framebuffer::Unbind( GLenum target)
{
    // code
    if(!IsValidTarget(target))
    {
        Log("Framebuffer Error:: Invalid target parameter provided to Framebuffer Object.");
        return;
    }
    glBindFramebuffer( target, 0);
}

/**
 * @brief AttachTexture()
 * @param framebuffer_target 
 * @param attachment_target 
 * @param texture 
 * @param level 
 */
void Framebuffer::AttachTexture(
    GLenum framebuffer_target, GLenum attachment_target,
    Texture2D *texture, int level
)
{
    // code
    if( !texture)
    {
        Log("Framebuffer Error:: Null pointer");
        return;
    }

    if(!IsValidTarget(framebuffer_target))
    {
        Log("Framebuffer Error:: Invalid framebuffer_target parameter provided to Framebuffer Object.");
        return;
    }

    GLint max_color_attachments = 0;
    glGetIntegerv( GL_MAX_COLOR_ATTACHMENTS, &max_color_attachments);

    if(
        !((attachment_target >= GL_COLOR_ATTACHMENT0) && (attachment_target <= GL_COLOR_ATTACHMENT0 + max_color_attachments)) &&
         (attachment_target != GL_DEPTH_ATTACHMENT) &&
         (attachment_target != GL_STENCIL_ATTACHMENT)
    )
    {
        Log("Framebuffer Error:: Invalid attachment_target parameter provided to Framebuffer Object.");
        return;
    }

    // glBindFramebuffer( framebuffer_target, m_id);
    glFramebufferTexture2D( framebuffer_target, attachment_target, GL_TEXTURE_2D, texture->GetID(), level);
    // glBindFramebuffer( framebuffer_target, 0);
}

/**
 * @brief AttachTexture()
 * @param framebuffer_target 
 * @param attachment_target 
 * @param texture 
 * @param level 
 */
void Framebuffer::AttachTexture(
    GLenum framebuffer_target, GLenum attachment_target,
    Texture2DMultisample *texture, int level
)
{
    // code
    if( !texture)
    {
        Log("Framebuffer Error:: Null pointer");
        return;
    }

    if(!IsValidTarget(framebuffer_target))
    {
        Log("Framebuffer Error:: Invalid framebuffer_target parameter provided to Framebuffer Object.");
        return;
    }

    GLint max_color_attachments = 0;
    glGetIntegerv( GL_MAX_COLOR_ATTACHMENTS, &max_color_attachments);

    if(
        !((attachment_target >= GL_COLOR_ATTACHMENT0) && (attachment_target <= GL_COLOR_ATTACHMENT0 + max_color_attachments)) &&
         (attachment_target != GL_DEPTH_ATTACHMENT) &&
         (attachment_target != GL_STENCIL_ATTACHMENT)
    )
    {
        Log("Framebuffer Error:: Invalid attachment_target parameter provided to Framebuffer Object.");
        return;
    }

    //glBindFramebuffer( framebuffer_target, m_id);
    glFramebufferTexture2D( framebuffer_target, attachment_target, GL_TEXTURE_2D_MULTISAMPLE, texture->GetID(), level);
    //glBindFramebuffer( framebuffer_target, 0);
}

/**
 * @brief SetDrawBuffers()
 * @param buffer_count 
 * @param buffer_target_attchments 
 */
void Framebuffer::SetDrawBuffers(int buffer_count, GLenum *buffer_target_attchments)
{
    // code
    glDrawBuffers( buffer_count, buffer_target_attchments);
}

/**
 * @brief Release()
 */
void Framebuffer::Release()
{
    // code
    glDeleteFramebuffers( 1, &m_id);
    m_id = 0;
}
