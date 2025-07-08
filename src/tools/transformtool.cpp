#include "../camera.h"
#include "transformtool.h"
#include "selectiontool.h"
#include "../viewport.h"
#include "../mesh.h"
#include "../transform3d.h"
#include "../application.h"

#include <QRect>

static const QVector3D arrowPoints[5] = {
    QVector3D(-1, 0, -1),
    QVector3D(-1, 0, 1),
    QVector3D(1, 0, -1),
    QVector3D(1, 0, 1),
    QVector3D(0, 1, 0)
};

static const QVector3D cubePoints[8] = {
    QVector3D(-1, -1, -1),
    QVector3D(-1, 1, -1),
    QVector3D(1, 1, -1),
    QVector3D(1, -1, -1),
    QVector3D(1, -1, 1),
    QVector3D(1, 1, 1),
    QVector3D(-1, 1, 1),
    QVector3D(-1, -1, 1)
};

static const GLushort arrowIndex[18] = {
    0, 1, 2,
    1, 2, 3,
    0, 1, 4,
    0, 4, 2,
    1, 3, 4,
    3, 2, 4
};

static const GLushort cubeIndex[36] = {
    6, 7, 4, 4, 5, 6, 0, 1, 2, 2, 3, 0, 5, 2, 1, 1, 6, 5, 7, 0, 3, 3, 4, 7, 3, 2, 5, 5, 4, 3, 6, 1, 0, 0, 7, 6
};


static const float selectionIDs[8] = {
    0,
    1/255.0f,
    2/255.0f,
    3/255.0f,
    4/255.0f,
    5/255.0f,
    6/255.0f,
    7/255.0f
};

TransformTool::TransformTool(): Tool()
{
    selectedColor = QVector3D(1, 1, 0);
}


// Build vertex buffers TODO: static const vertex buffers
void TransformTool::initialize()
{
    QMatrix4x4 m;
    QVector3D arrowScale = QVector3D(0.025f, 0.18f, 0.025f);
    QVector3D cubeScale = QVector3D(0.025f, 0.025f, 0.025f);

    m.setToIdentity();
    m.translate(1, 0, 0);
    m.rotate(-90, QVector3D(0, 0, 1));
    m.scale(arrowScale);
    for (int i = 0; i < 5; i++) {
        vertices.push_back(m * arrowPoints[i]);
    }
    for (int i = 0; i < 18; i++) {
        indices.push_back(arrowIndex[i]);
    }

    m.setToIdentity();
    m.translate(0, 1, 0);
    m.scale(arrowScale);
    for (int i = 0; i < 5; i++) {
        vertices.push_back(m * arrowPoints[i]);
    }
    for (int i = 0; i < 18; i++) {
        indices.push_back(arrowIndex[i] + 5);
    }


    m.setToIdentity();
    m.translate(0, 0, 1);
    m.rotate(90, QVector3D(1, 0, 0));
    m.scale(arrowScale);
    for (int i = 0; i < 5; i++) {
        vertices.push_back(m * arrowPoints[i]);
    }
    for (int i = 0; i < 18; i++) {
        indices.push_back(arrowIndex[i] + 10);
    }

    //make lines
    int k = vertices.size();
    vertices.push_back(QVector3D(0, 0, 0));
    vertices.push_back(QVector3D(1, 0, 0));
    vertices.push_back(QVector3D(0, 1, 0));
    vertices.push_back(QVector3D(0, 0, 1));

    indices.push_back(k);  indices.push_back(k+1);
    indices.push_back(k);  indices.push_back(k+2);
    indices.push_back(k);  indices.push_back(k+3);

    //in between triangles
    indices.push_back(k); indices.push_back(k + 1); indices.push_back(k + 2);
    indices.push_back(k); indices.push_back(k + 2); indices.push_back(k + 3);
    indices.push_back(k); indices.push_back(k + 1); indices.push_back(k + 3);

    // add cube X
    m.setToIdentity();
    m.translate(1, 0, 0);
    m.scale(cubeScale);
    k = vertices.size();
    for (int i = 0; i < 8; i++) {
        vertices.push_back(m * cubePoints[i]);
    }
    for (int i = 0; i < 36; i++) {
        indices.push_back(cubeIndex[i] + k);
    }

    // add cube Y
    m.setToIdentity();
    m.translate(0, 1, 0);
    m.scale(cubeScale);
    k = vertices.size();
    for (int i = 0; i < 8; i++) {
        vertices.push_back(m * cubePoints[i]);
    }
    for (int i = 0; i < 36; i++) {
        indices.push_back(cubeIndex[i] + k);
    }

    // add cube Z
    m.setToIdentity();
    m.translate(0, 0, 1);
    m.scale(cubeScale);
    k = vertices.size();
    for (int i = 0; i < 8; i++) {
        vertices.push_back(m * cubePoints[i]);
    }
    for (int i = 0; i < 36; i++) {
        indices.push_back(cubeIndex[i] + k);
    }


    //TODO refactor boilerplate
    shaderProgram = new QOpenGLShaderProgram();
    shaderProgram->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/position.vert");
    shaderProgram->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/only_color.frag");
    shaderProgram->link();
    shaderProgram->bind();
    u_modelViewProj = shaderProgram->uniformLocation("modelViewProj");
    u_color = shaderProgram->uniformLocation("color");
    u_point_size = shaderProgram->uniformLocation("point_size");

    vertexArrayObject.create();
    vertexArrayObject.bind();

    vertexBuffer.create();
    vertexBuffer.bind();
    vertexBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
    vertexBuffer.allocate(&vertices[0], vertices.size()*sizeof(QVector3D));
    shaderProgram->enableAttributeArray(0);
    shaderProgram->setAttributeBuffer(0, GL_FLOAT, 0, 3, 12);
    vertexBuffer.release();
    vertexArrayObject.release();




    selectTool = new SelectionTool(); // TODO prednere da tool reg
}


//TODO refactor: abbiamo una parte dove comincia la manipolazione e salva i dati, una che aggiorna, una che finisce.
//1 dovrebbe salvare oggetti e transform. Per i vertex potrebbe modificare direttamente il buffer, e dopo salvare?
void TransformTool::update(Viewport *viewport) {
    Camera camera = viewport->camera;

     if (translating) {
        QVector3D cameraPlanePoint = currentTransform.toMatrix().column(3).toVector3D(); // gizmo poition
        QVector3D cameraPlaneNormal = (camera.transform.column(3).toVector3D() - camera.translation).normalized();  // camera direction

        QVector3D camera_ray_start = viewport->getMouseWorldPosition();

        //camera ray
        QVector3D cameraPosition =  camera.transform.column(3).toVector3D();
        QVector3D camera_ray = (cameraPosition - camera_ray_start).normalized();

        //ray plane intersection
        float distance = QVector3D::dotProduct(cameraPlaneNormal, (cameraPlanePoint - camera_ray_start)) / QVector3D::dotProduct(camera_ray, cameraPlaneNormal);
        QVector3D rayPlanePoint = camera_ray_start + distance * camera_ray;

        QVector3D global_translation = rayPlanePoint - start_global_position; //global translation
        currentTransform.setTranslation(global_translation);

        local_translation = rayPlanePoint - start_local_position;

        if (!currentObjects.empty()) {
            // object mode
            for(int i = 0; i < currentObjects.size(); i ++) {
                currentObjects[i]->local_transform.setTranslation(currentTransforms[i].translation() + local_translation);
            }

        } else {
            // component mode
            Mesh* m = Viewport::getActiveViewport()->edit_mode_object;

            if (m != nullptr) {
                for(auto &v: currentVertices) { //from selection
                    m->geometry->vertices[v.header.id]->position = v.position + local_translation;
                }
                m->geometry_dirty = true;
            }
        }

    } else {
        if (!has_selection) return;

        startTranslation();

        QVector3D cameraPlanePoint = currentTransform.toMatrix().column(3).toVector3D(); //TODO refactor
        QVector3D cameraPlaneNormal = (camera.transform.column(3).toVector3D() - camera.translation).normalized();//TODO refactor

        QVector3D camera_ray_start = viewport->getMouseWorldPosition();

        //camera ray
        QVector3D cameraPosition =  camera.transform.column(3).toVector3D();
        QVector3D camera_ray = (cameraPosition - camera_ray_start).normalized();

        //ray plane intersection
        float distance = QVector3D::dotProduct(cameraPlaneNormal, (cameraPlanePoint - camera_ray_start)) / QVector3D::dotProduct(camera_ray, cameraPlaneNormal);
        QVector3D rayPlanePoint = camera_ray_start + distance * camera_ray;

        //start position in global and object local space
        start_global_position = rayPlanePoint - currentTransform.translation(); //global_space

        if (Viewport::getActiveViewport()->edit_mode_object == nullptr) {
            currentObjects.clear();
            currentTransforms.clear();

            std::vector<QVariant> &selection = Application::instance().selection;
            for(QVariant &variant: selection) { //from selection
                Object3D* o = qvariant_cast<Object3D*>(variant);
                Q_ASSERT(o != nullptr);
                if (o->parent()->selected) continue; // modifica solo il piu alto della gerarchia
                currentObjects.push_back(o);
                currentTransforms.push_back(o->localTransform());
            }
            start_local_position = rayPlanePoint;
        } else {
            currentVertices.clear();

            Mesh* m = Viewport::getActiveViewport()->edit_mode_object;
            for(auto &i: m->selected_component_array) { //from selection
                currentVertices.push_back(*m->geometry->vertices[i]); // copy
            }
            start_local_position = rayPlanePoint;
        }

    }
}

void TransformTool::updateSize(QMatrix4x4 cameraView)
{
    QVector4D cp = cameraView.column(3); //cameraView.column(3).toVector3D();
   // float distance = (currentTransform.translation() - QVector3D(cp.x(), cp.y(), cp.z())).length();
    float distance = (currentTransform.translation() - cameraView.column(3).toVector3D()).length();
    currentTransform.setScale(distance*0.13);
}


void TransformTool::showGizmo()
{

}


void TransformTool::startTranslation()
{
    translating = true;
    //startTransform = currentTransform;
}


void TransformTool::updateTranslation(QPoint mouseDelta)
{

}


void TransformTool::endTranslation()
{
    translating = false;

    if (!currentObjects.empty()) {
        // object mode
        for(int i = 0; i < currentObjects.size(); i ++) {
            currentObjects[i]->local_transform.setTranslation(currentTransforms[i].translation());
        }

        std::vector<QVariant> args;
        //args.push_back(QVariant::fromValue(currentObjects));
        args.push_back(QVariant::fromValue(local_translation));

        Application::instance().executeCommand("Translate", args);

    } else {
        // component mode

//        Mesh* m = Viewport::getActiveViewport()->edit_mode_object;

//        for(auto &v: currentVertices) { //from selection
//            m->geometry->vertices[v.header.id]->position = v.position;
//        }
//        m->geometry_dirty = true;
    }
}


void static drawRing() {
    float interval = 3.14159265359 / 24;
    float angle = 0.0f;
    for(int i = 0; i < 49; i++) {
        float x = sin(angle) * 1;
        float y = cos(angle) * 1;
        glVertex3f(x, y, -interval/4);
        glVertex3f(x, y, interval/4);

        angle += interval;
    }
}

void static drawCircle() {
    float interval = 3.14159265359 / 24;
    float angle = 0.0f;
    for(int i = 0; i < 49; i++) {
        float x = sin(angle) * 1;
        float y = cos(angle) * 1;
        glVertex3d(x, y, 0);
        angle += interval;
    }
}


void TransformTool::draw(QOpenGLFunctions *f, Camera &camera)
{
    if (!has_selection) return;

    float distance = (currentTransform.translation() - camera.transform.column(3).toVector3D()).length();
    currentTransform.setScale(distance * 0.18f);

    if (transform_mode == TRANSLATION) {

        shaderProgram->bind();
        shaderProgram->setUniformValue(u_modelViewProj, camera.viewProjection * currentTransform.toMatrix());
        vertexArrayObject.bind();
        //X
        if (this->componentSelected & X_AXIS_SELECTED) { //manage highlight
            shaderProgram->setUniformValue(u_color, selectedColor);
        } else {
            shaderProgram->setUniformValue(u_color, QVector3D(1, 0, 0));
        }
        f->glDrawElements(GL_LINES, 2, GL_UNSIGNED_SHORT, &indices[54]);
        f->glDrawElements(GL_TRIANGLES, 18, GL_UNSIGNED_SHORT, &indices[0]);

        //Y
        if (this->componentSelected & Y_AXIS_SELECTED) {
            shaderProgram->setUniformValue(u_color, selectedColor);
        } else {
            shaderProgram->setUniformValue(u_color, QVector3D(0, 1, 0));
        }
        f->glDrawElements(GL_LINES, 2, GL_UNSIGNED_SHORT, &indices[56]);
        f->glDrawElements(GL_TRIANGLES, 18, GL_UNSIGNED_SHORT, &indices[18]);

        //Z
        if (this->componentSelected & Z_AXIS_SELECTED) {
            shaderProgram->setUniformValue(u_color, selectedColor);
        } else {
            shaderProgram->setUniformValue(u_color, QVector3D(0, 0, 1));
        }
        f->glDrawElements(GL_LINES, 2, GL_UNSIGNED_SHORT, &indices[58]);
        f->glDrawElements(GL_TRIANGLES, 18, GL_UNSIGNED_SHORT, &indices[36]);

        vertexArrayObject.release();
        shaderProgram->release();

    } else if (transform_mode == ROTATION) {
        float interval = 3.14159265359 / 12;
        float angle = 0.0f;
        QMatrix4x4 m;
        m.setToIdentity();

        glMatrixMode(GL_MODELVIEW);

        glPushMatrix();
        glMultMatrixf((camera.viewProjection * currentTransform.toMatrix()).data());

        //glLineWidth(2.0f);
        glEnable(GL_CULL_FACE);

        glPushMatrix();

        glBegin(GL_TRIANGLE_STRIP);
        if (this->componentSelected & Y_AXIS_SELECTED) {
            glColor3f(1.0, 1.0, 0.0);
        } else {
            glColor3f(0.0, 1.0, 0.0);
        }
        drawRing();
        glEnd();

        m.rotate(90, QVector3D(1, 0, 0));
        glMultMatrixf(m.data());

        glBegin(GL_TRIANGLE_STRIP);
        if (this->componentSelected & X_AXIS_SELECTED) {
            glColor3f(1.0, 1.0, 0.0);
        } else {
            glColor3f(1.0, 0.0, 0.0);
        }
        drawRing();
        glEnd();

        m.rotate(90, QVector3D(0, 1, 0));
        glMultMatrixf(m.data());

        glBegin(GL_TRIANGLE_STRIP);
        if (this->componentSelected & Z_AXIS_SELECTED) {
            glColor3f(1.0, 1.0, 0.0);
        } else {
            glColor3f(0.0, 0.0, 1.0);
        }
        drawRing();
        glEnd();

        glPopMatrix();

        m.setToIdentity();
        m.rotate(camera.rotation); // <- view plane billboard, no distortion
        // TODO sphere, outline, XYZ lines
        glMultMatrixf(m.data());

        glBegin(GL_LINE_STRIP);
        if (this->componentSelected & CIRCLE_SELECTED) {
            glColor3f(1.0, 1.0, 0.0);
        } else {
            glColor3f(0.2, 0.2, 0.2);
        }
        drawCircle();
        glEnd();
        glPopMatrix();


        glDisable(GL_CULL_FACE);
    } else {

        shaderProgram->bind();
        shaderProgram->setUniformValue(u_modelViewProj, camera.viewProjection * currentTransform.toMatrix());
        vertexArrayObject.bind();
        //X
        if (this->componentSelected & X_AXIS_SELECTED) { //manage highlight
            shaderProgram->setUniformValue(u_color, selectedColor);
        } else {
            shaderProgram->setUniformValue(u_color, QVector3D(1, 0, 0));
        }
        f->glDrawElements(GL_LINES, 2, GL_UNSIGNED_SHORT, &indices[54]);
        f->glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, &indices[69]);

        //Y
        if (this->componentSelected & Y_AXIS_SELECTED) {
            shaderProgram->setUniformValue(u_color, selectedColor);
        } else {
            shaderProgram->setUniformValue(u_color, QVector3D(0, 1, 0));
        }
        f->glDrawElements(GL_LINES, 2, GL_UNSIGNED_SHORT, &indices[56]);
        f->glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, &indices[105]);

        //Z
        if (this->componentSelected & Z_AXIS_SELECTED) {
            shaderProgram->setUniformValue(u_color, selectedColor);
        } else {
            shaderProgram->setUniformValue(u_color, QVector3D(0, 0, 1));
        }
        f->glDrawElements(GL_LINES, 2, GL_UNSIGNED_SHORT, &indices[58]);
        f->glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, &indices[141]);

        vertexArrayObject.release();
        shaderProgram->release();
    }
}


void TransformTool::drawSelection(QOpenGLFunctions *f, Camera &camera)
{
    if (!has_selection) return;

    float distance = (currentTransform.translation() - camera.transform.column(3).toVector3D()).length();
    currentTransform.setScale(distance * 0.18f);

    if (transform_mode == TRANSLATION) {

    shaderProgram->bind();
    shaderProgram->setUniformValue(u_modelViewProj, camera.viewProjection * currentTransform.toMatrix());
    vertexArrayObject.bind();

//AXIS TRIANGLES
    shaderProgram->setUniformValue(u_color, QVector3D(selectionIDs[4], 0, 0)); //xy
    f->glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_SHORT, &indices[60]);

    shaderProgram->setUniformValue(u_color, QVector3D(selectionIDs[5], 0, 0)); //yz
    f->glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_SHORT, &indices[63]);

    shaderProgram->setUniformValue(u_color, QVector3D(selectionIDs[6], 0, 0)); //zx
    f->glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_SHORT, &indices[66]);

    glClear(GL_DEPTH_BUFFER_BIT);

//AXIS
    glLineWidth(35.0f);

    shaderProgram->setUniformValue(u_color, QVector3D(selectionIDs[0], 0, 0));
    f->glDrawElements(GL_LINES, 2, GL_UNSIGNED_SHORT, &indices[54]);
    f->glDrawElements(GL_TRIANGLES, 18, GL_UNSIGNED_SHORT, &indices[0]);

    shaderProgram->setUniformValue(u_color, QVector3D(selectionIDs[1], 0, 0));
    f->glDrawElements(GL_LINES, 2, GL_UNSIGNED_SHORT, &indices[56]);
    f->glDrawElements(GL_TRIANGLES, 18, GL_UNSIGNED_SHORT, &indices[18]);

    shaderProgram->setUniformValue(u_color, QVector3D(selectionIDs[2], 0, 0));
    f->glDrawElements(GL_LINES, 2, GL_UNSIGNED_SHORT, &indices[58]);
    f->glDrawElements(GL_TRIANGLES, 18, GL_UNSIGNED_SHORT, &indices[36]);

    glLineWidth(1.0f);


//CENTER
    f->glDisable(GL_DEPTH_TEST);
    shaderProgram->setUniformValue(u_color, QVector3D(selectionIDs[3], 0, 0));
    shaderProgram->setUniformValue(u_point_size, 20.0f);
    f->glDrawElements(GL_POINTS, 1, GL_UNSIGNED_SHORT, &indices[58]);
    f->glEnable(GL_DEPTH_TEST);


    vertexArrayObject.release();
    shaderProgram->release();

    } else if (transform_mode == ROTATION) {
        float interval = 3.14159265359 / 12;
        float angle = 0.0f;
        QMatrix4x4 m;
        m.setToIdentity();

        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glMultMatrixf((camera.viewProjection * currentTransform.toMatrix()).data());

        glEnable(GL_CULL_FACE);
// Y
        glBegin(GL_TRIANGLE_STRIP);
        glColor3f(selectionIDs[1], 0.0, 0.0);
        drawRing();
        glEnd();
// X
        m.rotate(90, QVector3D(1, 0, 0));
        glMultMatrixf(m.data());

        glBegin(GL_TRIANGLE_STRIP);
        glColor3f(selectionIDs[0], 0.0, 0.0);
        drawRing();
        glEnd();
// Z
        m.rotate(90, QVector3D(0, 1, 0));
        glMultMatrixf(m.data());

        glBegin(GL_TRIANGLE_STRIP);
        glColor3f(selectionIDs[2], 0.0, 0.0);
        drawRing();
        glEnd();
        glPopMatrix();

        glDisable(GL_CULL_FACE);

// circle
        glPushMatrix();
        QMatrix4x4 bb_view;
        bb_view.translate(currentTransform.translation());
        bb_view.rotate(camera.rotation);
        bb_view.scale(distance * 0.18f);
        glMultMatrixf((camera.viewProjection * bb_view).data());

        glLineWidth(15.0f);
        glBegin(GL_LINE_STRIP);
        glColor3f(selectionIDs[7], 0.0, 0.0);
        drawCircle();
        glEnd();
        glPopMatrix();
        glLineWidth(1.0f);


    } else {

        shaderProgram->bind();
        shaderProgram->setUniformValue(u_modelViewProj, camera.viewProjection * currentTransform.toMatrix());
        vertexArrayObject.bind();

    //AXIS TRIANGLES
        shaderProgram->setUniformValue(u_color, QVector3D(selectionIDs[4], 0, 0)); //xy
        f->glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_SHORT, &indices[60]);

        shaderProgram->setUniformValue(u_color, QVector3D(selectionIDs[5], 0, 0)); //yz
        f->glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_SHORT, &indices[63]);

        shaderProgram->setUniformValue(u_color, QVector3D(selectionIDs[6], 0, 0)); //zx
        f->glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_SHORT, &indices[66]);

        glClear(GL_DEPTH_BUFFER_BIT);

    //AXIS
        glLineWidth(35.0f);

        shaderProgram->setUniformValue(u_color, QVector3D(selectionIDs[0], 0, 0));
        f->glDrawElements(GL_LINES, 2, GL_UNSIGNED_SHORT, &indices[54]);
        f->glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, &indices[69]);

        shaderProgram->setUniformValue(u_color, QVector3D(selectionIDs[1], 0, 0));
        f->glDrawElements(GL_LINES, 2, GL_UNSIGNED_SHORT, &indices[56]);
        f->glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, &indices[105]);

        shaderProgram->setUniformValue(u_color, QVector3D(selectionIDs[2], 0, 0));
        f->glDrawElements(GL_LINES, 2, GL_UNSIGNED_SHORT, &indices[58]);
        f->glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, &indices[141]);

        glLineWidth(1.0f);


    //CENTER
        f->glDisable(GL_DEPTH_TEST);
        shaderProgram->setUniformValue(u_color, QVector3D(selectionIDs[3], 0, 0));
        shaderProgram->setUniformValue(u_point_size, 20.0f);
        f->glDrawElements(GL_POINTS, 1, GL_UNSIGNED_SHORT, &indices[58]);
        f->glEnable(GL_DEPTH_TEST);


        vertexArrayObject.release();
        shaderProgram->release();
    }

}

//set highlight
void TransformTool::setSelection(int id) {

}


void TransformTool::activate() {
    active = true;
    has_selection = false;
    selecting = false;
    currentObjects.clear();

    if (Viewport::getActiveViewport()->edit_mode_object != nullptr) { //TODO selection vertex array
        Mesh* m = Viewport::getActiveViewport()->edit_mode_object;
        if (m->selected_component_array.size() == 0) return;

        QVector3D center; //Transform
        for(auto &i: m->selected_component_array) { //from selection
            center += m->geometry->vertices[i]->position;
        }
        center /= m->selected_component_array.size();
        //currentObject = nullptr;
        currentTransform = (m->global_transform);
        currentTransform.translate(center);

        has_selection = true;
    } else {
        std::vector<QVariant> &selection = Application::instance().selection;
        if(!selection.empty()) {
            QVector3D center;
            for(QVariant &variant: selection) { //from selection
                Object3D* o = qvariant_cast<Object3D*>(variant);
                if (o) {
                    center += o->global_transform.translation();
                }
            }
            center /= selection.size();
            currentTransform = Transform3D();
            currentTransform.setTranslation(center);
           // Object3D* o = qvariant_cast<Object3D*>(selection->at(0));
            //if (o) {
           //     currentObject = o;
             //   currentTransform = o->global_transform;
            has_selection = true;
          //  }
        }
    }
}

//TODO setCurrentObject

void TransformTool::deactivate() {
    active = false;
    has_selection = false;
}




void TransformTool::keyPressEvent(QKeyEvent *event)
{
    //get selections
    if (event->key() == Qt::Key_V) {
        transform_mode = TRANSLATION;
    } else if (event->key() == Qt::Key_C) {
        transform_mode = ROTATION;
    } else if (event->key() == Qt::Key_X) {
        transform_mode = SCALE;
    } else if (event->key() == Qt::Key_B) {
        //transform_mode = TRANSLATION_TWEAK;
    }
}

void TransformTool::keyReleaseEvent(QKeyEvent *event)
{
    //TODO if selection engaged reset
    //TODO snaps
}


static void mouseOnBall(const QPoint &mouse, const QVector2D &center, QVector3D &ball_position) {
    float ball_radius = 200;
    ball_position.setX((mouse.x() - center.x()) / ball_radius);
    ball_position.setY((mouse.y() - center.y()) / ball_radius);

    qDebug() << "-------------------- v" << center;
    qDebug() << "mouse" << mouse;
    qDebug() << "ball_position" << ball_position;

  // double mag = ball_position.lengthSquared();
    double mag = ball_position.x() * ball_position.x() + ball_position.y() * ball_position.y();

    if (mag > 1.0) {
        double scale = 1.0/sqrtf(mag);
        ball_position.setX(ball_position.x()*scale);
        ball_position.setY(ball_position.y()*scale);
        ball_position.setZ(0.0);
    } else {
        ball_position.setZ(sqrt(1 - mag));
    }
    qDebug() << "mag" << mag;
}


static QVector3D ConstrainToAxis(QVector3D loose, QVector3D axis)
{
    QVector3D onPlane;
    float norm;
    onPlane = loose - axis * QVector3D::dotProduct(axis, loose);
    norm = onPlane.length();
    if (norm > 0.f) {
    if (onPlane.z() < 0.f) onPlane = onPlane * -1;
    return ( onPlane * 1/sqrtf(norm) );
    } /* else drop through */
    if (axis.z() == 1) {
    onPlane = QVector3D(1.f, 0.f, 0.f);
    } else {
    //onPlane = QVector3D(-axis.y(), axis.x(), 0.f);
    onPlane.normalize();
    }
    return (onPlane);
}


void TransformTool::mousePressEvent(QMouseEvent *event)
{


    //    if (this->componentSelected == NO_SELECTION || !has_selection) {
    //        selecting = true;
    //        selectTool->mousePressEvent(event);
    //        return;
    //    }

    // gizmo center to windows space

    if (transform_mode == ROTATION) {

    std::vector<QVariant> &selection = Application::instance().selection;
    if (!selection.empty()) {
        Object3D* o = qvariant_cast<Object3D*>(selection[0]);

        Camera &c = Viewport::getActiveViewport()->camera;
        //QMatrix4x4 vp =  c.view * c.projection;
        QMatrix4x4 mv = o->global_transform.toMatrix() * c.view;
     //   QMatrix4x4 mvp = o->global_transform.toMatrix() * vp;

       // float distance = (currentTransform.translation() - c.transform.column(3).toVector3D()).length();

        // sphere center in screen space

        float distance = (currentTransform.translation() - c.transform.column(3).toVector3D()).length();
        currentTransform.setScale(distance * 0.18f);

        QVector2D v = o->global_transform.translation().project(c.view, c.projection, QRect(0, 0, Viewport::getActiveViewport()->width, Viewport::getActiveViewport()->height)).toVector2D();

        QMatrix4x4 m;
        m.setToIdentity();
//        QVector3D test = QVector3D(1,0,0);
//        qDebug() << "test" << test;
//        test.setX(test.x() * distance * 0.18f );
//        qDebug() << "test" << test;
       // qDebug() << "distance * 0.18f" << distance * 0.18f;

        QVector2D circle_size = QVector3D(1,0,0).project(c.view, c.projection, QRect(0, 0, Viewport::getActiveViewport()->width, Viewport::getActiveViewport()->height)).toVector2D();
        float zero = circle_size.x() - (Viewport::getActiveViewport()->width / 2.0f);
        qDebug() << "circle_size" << zero;
       // v.setY(  Viewport::getActiveViewport()->height - v.y()); // Y opengl to window coord
       // v.setY(  Viewport::getActiveViewport()->height - v.y()); // Y opengl to window coord

        // sphere radius in screen space
       // float v_radius = o->global_transform.translation();

       // QVector3D sphere_edge = o->global_transform.translation().
     //   QMatrix4x4 fixmatrix = QMatrix4x4();

       // fixmatrix.scale(distance*0.13);
       // float pixel_sphere_center = QVector3D{0,0,0}.project(fixmatrix, c.projection, QRect(0, 0, Viewport::getActiveViewport()->width, Viewport::getActiveViewport()->height)).x();

//        QVector3D pixel_radius = QVector3D{0,0,0}.project(mv, c.projection, QRect(0, 0, Viewport::getActiveViewport()->width, Viewport::getActiveViewport()->height));
//        qDebug() << "pixel_radius" << pixel_radius;

      //  QVector4D test =  mv * c.projection * QVector4D(0, 0, 0, 1);
      //  qDebug() <<  "test" << test;

        //ball_position.setW(0.0);

        QPoint pos = event->pos();
        pos.setY(Viewport::getActiveViewport()->height - pos.y());


        mouseOnBall(pos, v, start_ball_rotation);

        // if constrain view Z ConstrainToAxis(start_ball_rotation, QVector3D(0, 0, 1)); // poi c.rotation * ...
        start_ball_rotation = c.rotation * start_ball_rotation;
        start_ball_rotation = ConstrainToAxis(start_ball_rotation, QVector3D());

        dragging = true;

        ball_center = v;
        start_rotation = o->local_transform.rotation();

       }
    } else {
        update(Viewport::getActiveViewport());
    }
    //startTranslation()
}

void TransformTool::mouseReleaseEvent(QMouseEvent *event)
{
    if (translating) {
        endTranslation();
    } else if (dragging) {
        dragging = false;
    } else if (selecting) {
        selecting = false;
        selectTool->mouseReleaseEvent(event);
    }
}

void TransformTool::mouseMoveEvent(QMouseEvent *event)
{
    if (translating) {
        update(Viewport::getActiveViewport());
    } else if (dragging) {
        std::vector<QVariant> &selection = Application::instance().selection;
        Object3D* o = qvariant_cast<Object3D*>(selection[0]);
        Camera &c = Viewport::getActiveViewport()->camera;
       // QMatrix4x4 mv = o->global_transform.toMatrix() * c.view;
      //  QVector2D v = o->global_transform.translation().project(mv, c.projection, QRect(0, 0, Viewport::getActiveViewport()->width, Viewport::getActiveViewport()->height)).toVector2D();
      //   v.setY( v.y() - Viewport::getActiveViewport()->height); // Y opengl to window coord

        QPoint pos = event->pos();
        pos.setY(  Viewport::getActiveViewport()->height - pos.y());

        QVector3D now_ball_position;
        mouseOnBall(pos, ball_center, now_ball_position);

        now_ball_position = c.rotation * now_ball_position;

        if (this->componentSelected != NO_SELECTION) {
            QVector3D axis;
            switch (this->componentSelected) {
                case(X_AXIS_SELECTED):
                axis = QVector3D(1, 0, 0);
                break;
                case(Y_AXIS_SELECTED):
                axis = QVector3D(0, 1, 0);
                break;
                case(Z_AXIS_SELECTED):
                axis = QVector3D(0, 1, 0);
                break;
            }
            now_ball_position = ConstrainToAxis(now_ball_position, axis);
        }

        QQuaternion rot =  QQuaternion::rotationTo(start_ball_rotation, now_ball_position);

        o->local_transform.setRotation(rot * start_rotation);
        //currentTransform.setRotation(o->local_transform.rotation());
    } else {
        int id = Viewport::getActiveViewport()->getToolPickBufferID();
        switch (id) {
         case 0:
            this->componentSelected = X_AXIS_SELECTED;
            break;
         case 1:
            this->componentSelected = Y_AXIS_SELECTED;
            break;
         case 2:
            this->componentSelected = Z_AXIS_SELECTED;
            break;
        case 3:
           this->componentSelected = ALL_AXIS_SELECTED;
           break;
        case 4:
           this->componentSelected = XY_AXIS_SELECTED;
           break;
        case 5:
           this->componentSelected = YZ_AXIS_SELECTED;
           break;
        case 6:
           this->componentSelected = ZX_AXIS_SELECTED;
           break;
        case 7:
           this->componentSelected = CIRCLE_SELECTED;
           break;
        default:
            this->componentSelected = NO_SELECTION;
        }
    }
}
