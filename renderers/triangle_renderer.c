#include <engine_tools/ogl_tools.h>
#include <renderers/triangle_renderer.h>


void
initRenderer_triangle(uGLRenderer* const triangle_renderer)
{
    //
    // fill out struct
    //
    const char* vshdr = GLSL
        (450 core,

         in vec2 vshdr_pos;

         void main()
         {
             gl_Position = vec4(vshdr_pos, 0.0f, 1.0f);
         });

    const char* fshdr = GLSL
        (450 core,

         uniform vec3 fshdr_color;

         out vec4 fshdr_final;

         void main()
         {
             fshdr_final = vec4(fshdr_color, 1.0f);
         });

    triangle_renderer->shader_program = 0;
    triangle_renderer->vshdr_position_location = -1;
    triangle_renderer->fshdr_color_location = -1;
    
    GLfloat triangle_vertex_data[] =
        {
            // top
            0.0f, 0.5f,

            // bottom left
            -0.5f, -0.5f,

            // bottom right
            0.5f, -0.5f
        };
    
    GLfloat test_fbo_data[100] = { 255 };
    
    //
    // create_program
    //
    glError;
    triangle_renderer->shader_program = uGLCreateShaderProgram_vf(&vshdr,
                                                                  &fshdr);
    assert(triangle_renderer->shader_program);    
    glUseProgram(triangle_renderer->shader_program);
    glError;

    //
    // discover_attribute_locations
    //
    glError;
    triangle_renderer->vshdr_position_location =
        glGetAttribLocation(triangle_renderer->shader_program, "vshdr_pos");
    triangle_renderer->fshdr_color_location =
        glGetUniformLocation(triangle_renderer->shader_program, "fshdr_color");
    glError;

    assert(triangle_renderer->vshdr_position_location != -1);
    assert(triangle_renderer->fshdr_color_location != -1);

    //
    // set_default_attribute_values
    //
    glError;
    glUniform3f(triangle_renderer->fshdr_color_location, 1.0f, 0.0f, 0.0f);
    glError;

    glError;
    glGenVertexArrays(1, &triangle_renderer->vertex_array_bufer_location);
    glBindVertexArray(triangle_renderer->vertex_array_bufer_location);
    glError;

    glGenBuffers(1, &triangle_renderer->vertex_buffer_location);
    glBindBuffer(GL_ARRAY_BUFFER, triangle_renderer->vertex_buffer_location);
    glBufferData(GL_ARRAY_BUFFER,
                 sizeof(triangle_vertex_data),
                 triangle_vertex_data,
                 GL_STATIC_DRAW);

    glVertexAttribPointer(triangle_renderer->vshdr_position_location,
                          2,
                          GL_FLOAT,
                          GL_FALSE,
                          0,
                          (void*) 0);
    glError;
    

    //
    //
    // framebuffer_test
    //
    //

    //
    // color_attachment
    //
    glError;
    glGenTextures(1, &triangle_renderer->texture_map);
    glBindTexture(GL_TEXTURE_2D, triangle_renderer->texture_map);
    glError;

    glError;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glError;

    glTexImage2D(GL_TEXTURE_2D,         // GLenum  target
                 0,                     // GLint   level
                 GL_RGBA,               // GLint   internalFormat
                 10,                    // GLsizei width
                 10,                    // GLsizei height
                 0,                     // GLint   border
                 GL_RGBA,               // GLenum  format
                 GL_UNSIGNED_BYTE,      // GLenum  type
                 test_fbo_data);        // const GLvoid* data
    
    glBindTexture(GL_TEXTURE_2D, 0);
    glError;

    //
    // depth_attachment
    //
    glError;
    glGenTextures(1, &triangle_renderer->depth_texture);
    glBindTexture(GL_TEXTURE_2D, triangle_renderer->depth_texture);
    glError;

    glError;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glError;

    glError;
    glTexImage2D(GL_TEXTURE_2D,         // GLenum  target
                 0,                     // GLint   level
                 GL_DEPTH_COMPONENT,    // GLint   internalFormat
                 10,                    // GLsizei width
                 10,                    // GLsizei height
                 0,                     // GLint   border
                 GL_DEPTH_COMPONENT,    // GLenum  format
                 GL_UNSIGNED_BYTE,      // GLenum  type
                 test_fbo_data);        // const GLvoid* data
    glError;
    
    glBindTexture(GL_TEXTURE_2D, 0);
    glError;

    //
    // build_fbo
    //
    glGenFramebuffers(1, &triangle_renderer->framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, triangle_renderer->framebuffer);
    glError;

    glFramebufferTexture2D(GL_FRAMEBUFFER,       // GLenum target
                           GL_COLOR_ATTACHMENT0, // GLenum attachment
                           GL_TEXTURE_2D,        // GLenum textureTarget
                           triangle_renderer->texture_map,           // GLuint texture
                           0);                   // Glint  mipmap level
    glError;

    glFramebufferTexture2D(GL_FRAMEBUFFER,       // GLenum target
                           GL_DEPTH_ATTACHMENT,  // GLenum attachment
                           GL_TEXTURE_2D,        // GLenum textureTarget
                           triangle_renderer->depth_texture,        // GLuint texture
                           0);                   // Glint  mipmap level
    glError;

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);;
    assert(status == GL_FRAMEBUFFER_COMPLETE);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void
render_triangle(uGLRenderer* const triangle_renderer)
{
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(triangle_renderer->shader_program);
    glBindVertexArray(triangle_renderer->vertex_array_bufer_location);
    glEnableVertexAttribArray(triangle_renderer->vshdr_position_location);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
    glEnableVertexAttribArray(0);
}