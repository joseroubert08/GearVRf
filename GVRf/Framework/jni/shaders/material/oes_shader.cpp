/* Copyright 2015 Samsung Electronics Co., LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


/***************************************************************************
 * Renders a GL_TEXTURE_EXTERNAL_OES texture.
 ***************************************************************************/

#include "oes_shader.h"

#include "gl/gl_program.h"
#include "objects/material.h"
#include "objects/mesh.h"
#include "objects/components/render_data.h"
#include "util/gvr_gl.h"

namespace gvr {
static const char VERTEX_SHADER[] = "attribute vec4 a_position;\n"
        "attribute vec4 a_tex_coord;\n"
        "uniform mat4 u_mvp;\n"
        "varying vec2 v_tex_coord;\n"
        "void main() {\n"
        "  v_tex_coord = a_tex_coord.xy;\n"
        "  gl_Position = u_mvp * a_position;\n"
        "}\n";

static const char FRAGMENT_SHADER[] =
        "#extension GL_OES_EGL_image_external : require\n"
                "precision highp float;\n"
                "uniform samplerExternalOES u_texture;\n"
                "uniform vec3 u_color;\n"
                "uniform float u_opacity;\n"
                "varying vec2 v_tex_coord;\n"
                "void main()\n"
                "{\n"
                "  vec4 color = texture2D(u_texture, v_tex_coord);"
                "  gl_FragColor = vec4(color.r * u_color.r * u_opacity, color.g * u_color.g * u_opacity, color.b * u_color.b * u_opacity, color.a * u_opacity);\n"
                "}\n";

OESShader::OESShader() :
        program_(0), a_position_(0), a_tex_coord_(0), u_mvp_(0), u_texture_(0), u_color_(
                0), u_opacity_(0) {
    program_ = new GLProgram(VERTEX_SHADER, FRAGMENT_SHADER);
    a_position_ = glGetAttribLocation(program_->id(), "a_position");
    a_tex_coord_ = glGetAttribLocation(program_->id(), "a_tex_coord");
    u_mvp_ = glGetUniformLocation(program_->id(), "u_mvp");
    u_texture_ = glGetUniformLocation(program_->id(), "u_texture");
    u_color_ = glGetUniformLocation(program_->id(), "u_color");
    u_opacity_ = glGetUniformLocation(program_->id(), "u_opacity");
}

OESShader::~OESShader() {
    if (program_ != 0) {
        recycle();
    }
}

void OESShader::recycle() {
    delete program_;
    program_ = 0;
}

void OESShader::render(const glm::mat4& mvp_matrix,
        std::shared_ptr<RenderData> render_data) {
    std::shared_ptr<Mesh> mesh = render_data->mesh();
    std::shared_ptr<Texture> texture = render_data->material()->getTexture(
            "main_texture");
    glm::vec3 color = render_data->material()->getVec3("color");
    float opacity = render_data->material()->getFloat("opacity");

    if (texture->getTarget() != GL_TEXTURE_EXTERNAL_OES) {
        std::string error = "OESShader::render : texture with wrong target";
        throw error;
    }

#if _GVRF_USE_GLES3_
    mesh->setVertexLoc(a_position_);
    mesh->setTexCoordLoc(a_tex_coord_);
    mesh->generateVAO();

    glUseProgram(program_->id());

    glUniformMatrix4fv(u_mvp_, 1, GL_FALSE, glm::value_ptr(mvp_matrix));
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(texture->getTarget(), texture->getId());
    glUniform1i(u_texture_, 0);
    glUniform3f(u_color_, color.r, color.g, color.b);
    glUniform1f(u_opacity_, opacity);

    glBindVertexArray(mesh->getVAOId());
    glDrawElements(GL_TRIANGLES, mesh->triangles().size(), GL_UNSIGNED_SHORT, 0);
    glBindVertexArray(0);
#else

    glUseProgram(program_->id());

    glVertexAttribPointer(a_position_, 3, GL_FLOAT, GL_FALSE, 0,
            mesh->vertices().data());
    glEnableVertexAttribArray(a_position_);

    glVertexAttribPointer(a_tex_coord_, 2, GL_FLOAT, GL_FALSE, 0,
            mesh->tex_coords().data());
    glEnableVertexAttribArray(a_tex_coord_);

    glUniformMatrix4fv(u_mvp_, 1, GL_FALSE, glm::value_ptr(mvp_matrix));

    glActiveTexture (GL_TEXTURE0);
    glBindTexture(texture->getTarget(), texture->getId());
    glUniform1i(u_texture_, 0);

    glUniform3f(u_color_, color.r, color.g, color.b);

    glUniform1f(u_opacity_, opacity);

    glDrawElements(GL_TRIANGLES, mesh->triangles().size(), GL_UNSIGNED_SHORT,
            mesh->triangles().data());
#endif
    checkGlError("OESShader::render");
}

}
;