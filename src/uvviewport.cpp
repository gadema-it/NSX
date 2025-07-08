#include "camera.h"
#include "uvviewport.h"
#include "mesh.h"
#include "application.h"
#include "viewport.h"

#include <QMatrix4x4>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLBuffer>

UVViewport::UVViewport(QWidget *parent): QOpenGLWidget(parent)
{
    camera = new Camera(true, true);
}


void UVViewport::initializeGL()
{

    makeCurrent();
    initializeOpenGLFunctions();

    shaderProgram = new QOpenGLShaderProgram(this);
    shaderProgram->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/uv.vert");
    shaderProgram->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/uv.frag");
    shaderProgram->link();

    texture = new QOpenGLTexture(QImage("g:/Workspace/PS/checkerboard.jpg").mirrored());
    texture->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
    texture->setMagnificationFilter(QOpenGLTexture::Linear);


//    vertex_buffer.create();
//    vertex_buffer.bind();
//    vertex_buffer.setUsagePattern(QOpenGLBuffer::DynamicDraw);
//    vertex_buffer.allocate(sizeof(float) * 4);

//    GLfloat* verticesArray = (GLfloat*)vertex_buffer.map(QOpenGLBuffer::WriteOnly);
//    verticesArray[0] = 1.0f;
//    verticesArray[0] = 0.0f;
//    verticesArray[0] = 0.5f;
//    verticesArray[0] = 0.3f;
//    vertex_buffer.unmap();
//    vertex_buffer.release();
}

void UVViewport::resizeGL(int w, int h)
{
    camera->updateProjection(w, h);
}

void UVViewport::paintGL()
{
    glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_PROGRAM_POINT_SIZE);
    glDisable(GL_DEPTH_TEST);

    std::vector<QVariant> &selection = Application::instance().selection;
    //std::vector<QVariant> selection = Viewport::getActiveViewport()->selection;
    if (selection.empty()) return;

    current_mesh = qvariant_cast<Mesh*>(selection.at(0));

    if (!current_mesh) return;
  //  if (current_mesh == nullptr) return;

    QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();
   // QMatrix4x4 viewProjection = camera->viewp;

   // current_mesh->draw(f, &viewProjection, this);


    glEnable(GL_TEXTURE_2D);
    texture->bind();

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glMultMatrixf(camera->viewProjection.data());
            glBegin(GL_TRIANGLES);
            glColor3f(1.0, 1.0, 1.0);
            glTexCoord2f(0, 0);
            glVertex2f(0, 0);
            glTexCoord2f(1, 1);
            glVertex2f(1, 1);
            glTexCoord2f(0, 1);
            glVertex2f(0, 1);

            glTexCoord2f(1, 1);
            glVertex2f(1, 1);
            glTexCoord2f(0, 0);
            glVertex2f(0, 0);
            glTexCoord2f(1, 0);
            glVertex2f(1, 0);
    glEnd();
    glPopMatrix();
    texture->release();
    glDisable(GL_TEXTURE_2D);

 //Draw uv grid
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glMultMatrixf(camera->viewProjection.data());

        glBegin(GL_LINES);
        glColor3f(0.0, 0.0, 0.0);
        glVertex2f(0.0, -10000.0);
        glVertex2f(0.0, 10000.0);
        glVertex2f(10000.0, 0.0);
        glVertex2f(-10000.0, 0.0);

        glColor3f(0.33, 0.33, 0.33);
        glVertex2f(0.25, 0);
        glVertex2f(0.25, 1);
        glVertex2f(0.5, 0);
        glVertex2f(0.5, 1);
        glVertex2f(0.75, 0);
        glVertex2f(0.75, 1);
        glVertex2f(1.0, 0);
        glVertex2f(1.0, 1);

        glVertex2f(0, 0.25);
        glVertex2f(1, 0.25);
        glVertex2f(0, 0.5);
        glVertex2f(1, 0.5);
        glVertex2f(0 , 0.75);
        glVertex2f(1, 0.75);
        glVertex2f(1, 1.0);
        glVertex2f(0, 1.0);

        glEnd();
   glPopMatrix();


   shaderProgram->bind();
   shaderProgram->setUniformValue("modelViewProj", camera->viewProjection);
   shaderProgram->setUniformValue("color", QVector3D(0, 0, 1));

   current_mesh->uv_buffer.bind();
   shaderProgram->enableAttributeArray(0);
   shaderProgram->setAttributeBuffer(0, GL_FLOAT, 0, 2, 8);

   glDrawArrays(GL_POINTS, 0, current_mesh->geometry->half_edges.size());

   shaderProgram->setUniformValue("color", QVector3D(1, 0.82f, 0));
   glDrawElements(GL_LINES, current_mesh->edge_indices.size(), GL_UNSIGNED_SHORT, current_mesh->edge_indices.data());

   current_mesh->uv_buffer.release();
  // vertex_buffer.release();
   shaderProgram->release();

   //qDebug() << "UV gl errors" << glGetError();
}
