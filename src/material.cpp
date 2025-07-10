#include "material.h"

#include <QOpenGLShaderProgram>
#include <QOpenGLFunctions>
#include <QOpenGLTexture>


Material::Material()
{

    shaderProgram = new QOpenGLShaderProgram();
    //shaderProgram->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/position.vert");
    //shaderProgram->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/only_color.frag");
    shaderProgram->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/color_normal.vert");
    shaderProgram->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/color_normal.frag");
    shaderProgram->link();
    shaderProgram->bind();
    u_modelViewProj = shaderProgram->uniformLocation("modelViewProj");
    u_modelInverseTranspose = shaderProgram->uniformLocation("modelInverseTranspose");
    u_color = shaderProgram->uniformLocation("color");
    u_light = shaderProgram->uniformLocation("light");

    texture = new QOpenGLTexture(QImage("g:/Workspace/PS/checkerboard.jpg").mirrored());
    texture->setWrapMode(QOpenGLTexture::WrapMode::Repeat);
    texture->setMinificationFilter(QOpenGLTexture::NearestMipMapLinear);
    texture->setMagnificationFilter(QOpenGLTexture::NearestMipMapLinear);

    shaderProgram->release();
}


void Material::setModelViewProj(const QMatrix4x4& MVP)
{
    shaderProgram->setUniformValue(u_modelViewProj, MVP);
}

void Material::setModelInverseTranspose(const QMatrix4x4& MVP)
{
    shaderProgram->setUniformValue(u_modelInverseTranspose, MVP);
}

void Material::setColor(const QVector3D& color)
{
    shaderProgram->setUniformValue(u_color, color);
}

void Material::setLight(const QVector3D& vector)
{
    shaderProgram->setUniformValue(u_light, vector);
}



void Material::testVariables()
{
    QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();

    GLint i;
    GLint count;

    GLint size; // size of the variable
    GLenum type; // type of the variable (float, vec3 or mat4, etc)

    const GLsizei bufSize = 16; // maximum name length
    GLchar name[bufSize]; // variable name in GLSL
    GLsizei length; // name length

    f->glGetProgramiv(shaderProgram->programId(), GL_ACTIVE_ATTRIBUTES, &count);
    qDebug("Active Attributes: %d\n", count);

    for (i = 0; i < count; i++)
    {
        f->glGetActiveAttrib(shaderProgram->programId(), (GLuint)i, bufSize, &length, &size, &type, name);

        qDebug("Attribute #%d Type: %u Name: %s\n", i, type, name);
    }

    f->glGetProgramiv(shaderProgram->programId(), GL_ACTIVE_UNIFORMS, &count);
    qDebug("Active Uniforms: %d\n", count);

    for (i = 0; i < count; i++)
    {
        f->glGetActiveUniform(shaderProgram->programId(), (GLuint)i, bufSize, &length, &size, &type, name);

        qDebug("Uniform #%d Type: %u Name: %s\n", i, type, name);
    }

}
