#include "viewport.h"
#include "object3d.h"
#include "material.h"
#include "application.h"
#include "locator.h"

#include "tools/tool.h"

#include <QMenu>
#include <QPainter>
#include <QPainterPath>
#include <QTime>
#include <chrono>
#include <QElapsedTimer>
#include <QOpenGLExtraFunctions>
#include <QTimer>
#include <QOpenGLFunctions_4_2_Core>


static QOpenGLShaderProgram *selectShader; //render element id color TODO mettere in shader manager?

static Viewport* viewport_tofix; // global pointer da togliere

// FIXME
Viewport* Viewport::getActiveViewport() {
    return viewport_tofix;
}


// TODO in selection maager
QOpenGLShaderProgram * Viewport::getSelectShader()
{
    return selectShader;
}



Viewport::Viewport(QWidget *parent) : QOpenGLWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);

    //TODO in Application global selection
    selectBuffer = nullptr;
    selectedObject = 16777215;

    hotkey_bindings = &Application::instance().hotkey_bindings;

    tool_timer = new QElapsedTimer();

    //test
    viewport_tofix = this; // REFACTOR remove
}

Viewport::~Viewport()
{

}


void Viewport::initializeGL()
{      
  //  QTimer *pTimer = new QTimer(this);
  //  connect(pTimer, SIGNAL(timeout()), this, SLOT(update()));
   // pTimer->start(1000 / 60.0);
   connect(this, SIGNAL(frameSwapped()), this, SLOT(update()));

   initializeOpenGLFunctions();

   glClearColor(0.5f, 0.5f, 0.5f, 1.0f);


   //selection shader per le componenti
   //TODO refactor - move to helper?
   selectShader = new QOpenGLShaderProgram(this); //TODO in Application or selection tool
   selectShader->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/selection.vert");
   selectShader->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/selection.frag");
   selectShader->link();


// TEST init selection_texture
   int width = 10000;

   glGenTextures(1, &select_texture);
   glBindTexture(GL_TEXTURE_1D, select_texture);

 //  QOpenGLFunctions_4_2_Core* funcs = context()->versionFunctions<QOpenGLFunctions_4_2_Core>();
 //  funcs->glTexStorage1D(GL_TEXTURE_1D, 1, GL_R8, width);

   glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glTexImage1D(GL_TEXTURE_1D, 0, GL_R8, width, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);

   unsigned char *zeros = new unsigned char[width]();
   zeros[0] = 1;

   glTexSubImage1D(GL_TEXTURE_1D, 0, 0, width, GL_RED, GL_UNSIGNED_BYTE, zeros);

   qDebug() << glGetError();
   qDebug() << zeros[0];

    delete[] zeros;

    unsigned int *zeros2 = new unsigned int[width];

    glGetTexImage(GL_TEXTURE_1D, 0, GL_RED, GL_UNSIGNED_BYTE, zeros2);

    qDebug() << zeros2[0];

    delete[] zeros2;

    glBindTexture(GL_TEXTURE_1D, 0);
// end TEST


    grid.init();
    axis.init();
    material = new Material();
}


void Viewport::AddObject3D(int type) {
    Object3D* new_object;
    if (type == 0) {
       makeCurrent(); // TODO refactor opengl shared context
       Mesh* m = new Mesh();
       m->makeCube(3);
       m->init();
       m->setMaterial(material);
       new_object = (Object3D*)m;
       new_object->setObjectName("Polygon Mesh");
    } else {
       new_object = new Locator();
       new_object->setObjectName("Locator");
    }

    std::vector<QVariant> &selection = Application::instance().selection;
    if (selection.size() > 0) {
        Object3D* selected =  qvariant_cast<Object3D*>(selection.at(0));
        if (selected) {
            new_object->setParent(selected);
            selected->addObject3D(new_object);
            //set at center
            QVector3D vec = selected->global_transform.translation();
            new_object->local_transform.setTranslation(-vec.x(), -vec.y(), -vec.z()); //TODO transform inverse
            //select new and deselect selected
            selected->selected = false;
            selection.clear();
            selection.push_back(QVariant::fromValue(new_object));
            new_object->selected = true;
        }
    } else {
        new_object->setParent(scene.root());
        scene.addObject3D(new_object);
    }
    scene.updateObjectList();
}


void Viewport::resizeGL(int w, int h)
{
    width = w;
    height = h;
    camera.updateProjection(w, h);

    //TODO da sposare in QOpenGLWidget::resizeEven?  o collegare al signal QOpenGLWidget::resized

    delete selectBuffer; // FIXME perche va senza check?
    selectBuffer = new QOpenGLFramebufferObject(w, h, QOpenGLFramebufferObject::Depth);

    //Per adesso, un framebuffer per la selezione dei manipolatori tool
    if (tool_selectBuffer != nullptr) {
      delete tool_selectBuffer;
    }
    tool_selectBuffer = new QOpenGLFramebufferObject(w, h, QOpenGLFramebufferObject::Depth);
}


void Viewport::paintGL()
{
    int64_t nanosec;
    auto start = std::chrono::high_resolution_clock::now();

    QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();
    std::vector<Object3D*>* sceneObjects = scene.getObjectList();

//Flags
    glEnable(GL_PROGRAM_POINT_SIZE);


//Tool manipulation framebuffer for seletion // REFACTOR into method
    tool_selectBuffer->bind();
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    if (active_tool != nullptr) {
        active_tool->drawSelection(f, camera);
    }
    glDisable(GL_DEPTH_TEST);
    tool_selectBuffer->bindDefault(); //restore default framebuffer


//Drawing scene
    glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
   //    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);


//draw grid
    grid.draw(f, &camera.viewProjection);


//draw objects

    // if wireframe ...
   // glEnable(GL_CULL_FACE);
 //   glShadeModel(GL_FLAT); // disable interpolation
    glEnable(GL_POLYGON_OFFSET_FILL); //avoid zfight polygon-edge
    glPolygonOffset(0.6, 1); //TODO distance based
    for (const auto& sceneObject : *sceneObjects) {
        sceneObject->draw(f, &camera.viewProjection, this);
    }
  //  glShadeModel(GL_SMOOTH);
   // glDisable(GL_CULL_FACE);
    //glDisable(GL_POLYGON_OFFSET_FILL);


//draw tool
    glClear(GL_DEPTH_BUFFER_BIT); //disegna tool sopra a tutto il resto
    if (active_tool != nullptr) {
        active_tool->draw(f, camera);

    }


//draw axis gizmo
    glViewport(10, 10, 80, 80);
    axis.draw(f, camera.rotation);
    glViewport(0, 0, width, height);

    glDisable(GL_DEPTH_TEST);




//    glMatrixMode(GL_MODELVIEW);
//    glPushMatrix();

    //glMultMatrixf(camera.viewProjection.data());

//    QMatrix4x4 m;
//    m.setToIdentity();
//    m.rotate(90, QVector3D(1, 0, 0));
//   // glMultMatrixf(m.data());

//    glMultMatrixf((camera.viewProjection * m).data());

//    glBegin(GL_TRIANGLE_STRIP);
//    glColor3f(0.0, 1.0, 0.0);

//    float interval = 3.14159265359 / 24;
//    float angle = 0.0f;
//    for(int i = 0; i < 49; i++) {
//        float x = sin(angle) * 1.5;
//        float y = cos(angle) * 1.5;
//        glVertex3f(x, y, interval/2);
//        glVertex3f(x, y, -interval/2);
//        angle += interval;
//    }

//    glEnd();


//    glBegin(GL_POINTS);
//    glColor3f(1.0, 0.0, 0.0);
//    float x = sin(interval/2) * 1.5;
//    float y = cos(interval/2) * 1.5;
//    glVertex3f(x, y, interval/2);
//    x = sin(-interval/2) * 1.5;
//    y = cos(-interval/2) * 1.5;
//    glVertex3f(x, y, -interval/2);
//    glEnd();

//    glPopMatrix();






//TESTS
//    glMatrixMode(GL_MODELVIEW);
//    glPushMatrix();
//    glMultMatrixf(viewProjection.data());

//    glBegin(GL_TRIANGLES);
//    glColor3f(0.0, 1.0, 0.0);
//    glVertex3f(0.0, 0.0, 0.0);
//    glVertex3f(-4.0, 0.0, -4.0);
//    glVertex3f(-1.0, -5.0, -1.0);
//    glEnd();

    //glEnable(GL_POINT_SMOOTH);
//    glPointSize(10);
//    glBegin(GL_POINTS);
//    glColor3f(0.0, 1.0, 0.0);
//    glVertex3f(0.0, 0.0, 0.0);
//    glEnd();
 //   glPopMatrix();


//FPS
    static int frames = 0;
    static int frame_time = 0;
    static double frameRate = 0;

    auto end = std::chrono::high_resolution_clock::now();
    auto delta_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    frame_time += delta_time.count();
    frames++;
    if (frame_time > 10000 ) {
        frameRate = frames;
        frame_time = 0;
        frames = 0;
    }
    QPainter painter(this);
    const QRect rectangle = QRect(10, 10, 30, 30);
    painter.drawText(rectangle, Qt::AlignCenter, QString::number(frameRate));
    painter.end();
}


void Viewport::contextMenuEvent(QContextMenuEvent *event)
{
//    QMenu *menu = new QMenu(this);
//    menu->addAction(new QAction("TEST", this));
    //    menu->exec(event->globalPos());
}


bool Viewport::event(QEvent *e) //move in Application?
{
    if (e->type() == QEvent::KeyPress)
    {
      keyPressEvent(static_cast<QKeyEvent*>(e));
      return true;
    }
    else if (e->type() == QEvent::KeyRelease)
    {
      keyReleaseEvent(static_cast<QKeyEvent*>(e));
      return true;
    }
    return QOpenGLWidget::event(e);
}



void Viewport::update()
{
    QOpenGLWidget::update();
}

void Viewport::setDefaultTool()
{
    active_tool = default_tool;
}


void Viewport::keyPressEvent(QKeyEvent *event)
{
    if (event->isAutoRepeat()) {
      event->ignore();
    } else {
        //find and activate commands or tool; or operator?

        auto it = hotkey_bindings->find((Qt::Key)event->key());
        if (it != hotkey_bindings->end()) {
            //if (it->second.userType() == qMetaTypeId<Tool*>()) { //NOTE qMetaTypeId ritorna la sottoclasse del tool, serve sapere se Tool o Command
            Tool* tool = qvariant_cast<Tool*>(it->second); //TODO trovare alternativa che fare qvariant_cast per tool, commands e operators
            if (tool) {
                //activate new tool and set old tool as prev tool
                if (active_tool == tool) {
                    // already activated
                    tool->keyPressEvent(event);
                    return;
                }
                if (active_tool != nullptr) {
                    previousTool = active_tool;
                }

                if(!tool->initialized) { // TODO da spostare: inizializza tool per la prima volta che si usa.
                    makeCurrent(); // nel caso si dovesse inizializzare vertex buffers
                    tool->initialize();
                }

                tool->activate();
                tool->keyPressEvent(event);
                active_tool = tool;
                tool_timer->start(); //start timer for hold mode         
                qDebug() << "activated new Tool"; 
            } else {
                //TODO commands
            }
            return;
       }

// Tools not found

       std::vector<QVariant> &selection = Application::instance().selection;
       if (event->key() == Qt::Key_A) { // TEST

           if (!selection.empty()) {
               Object3D* o = qvariant_cast<Object3D*>(selection.at(0));
               Transform3D *t = qvariant_cast<Transform3D*>(o->property("local_transform"));
               qDebug() << "Transform: " << t->translation();
           }

          makeCurrent();

          //testGLSelect();
          testOcclusionQuery();

          //QPoint mp = mapFromGlobal(InputManager::mousePosition());

          unsigned char data[4];

          //glBindFramebuffer(GL_READ_FRAMEBUFFER, selectBuffer->handle());
          selectBuffer->bind();

          //glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        //  glReadPixels(mp.x(), height - mp.y(), 1, 1, GL_RGBA, GL_UNSIGNED_BYTE , data);

           qDebug() << "keypress A";
           qDebug() << data[0] << data[1] << data[2] << data[3];
           qDebug() << data[0] + data[1]*256 + data[2]*256*256;

        } else if (event->key() == Qt::Key_F) { // Frame functions
           if (event->modifiers() == Qt::SHIFT) {
               camera.reset();
           }

           if (!selection.empty()) {
               //TODO points, forloop
               Object3D* o = qvariant_cast<Object3D*>(selection.at(0));
               if (o) {
                   camera.translation = o->global_transform.translation();
                   camera.orbitDistance = o->bb_distance * 3.0f;
                   camera.updateView();
               }
           }
       } else if (event->key() == Qt::Key_Plus) {
           if (!selection.empty()) {
               Mesh* m = qvariant_cast<Mesh*>(selection.at(0));
               if (m) m->increaseSubdivisionLevel();
           }
       } else if (event->key() == Qt::Key_Minus) {
           if (!selection.empty()) {
               Mesh* m = qvariant_cast<Mesh*>(selection.at(0));
               if(m) m->decreaseSubdivisionLevel();
           }
       } else if (event->key() == Qt::Key_2) {
           if (!selection.empty()) {
               Mesh* m = qvariant_cast<Mesh*>(selection.at(0));
               m->geometry->splitEdge(0, 0.5);
               m->geometry_dirty = true;
           }
       } else if (event->key() == Qt::Key_Z) {
           if (event->modifiers() == Qt::CTRL) {
               Application::instance().undo();
           }
       } else if (event->key() == 96) {
         //  if (!selection.empty()) {
               if (edit_mode_object == nullptr || edit_mode_object->selection_type != POLYGON) {
                    return;
               }
               if (edit_mode_object->selected_component_array.empty()) return;

               std::vector<int> indices(edit_mode_object->selected_component_array.begin(), edit_mode_object->selected_component_array.end());
               qDebug() << "start deleting faces: " << indices;
               edit_mode_object->geometry->deleteFace(indices);
               edit_mode_object->deselectComponents();
               edit_mode_object->geometry_dirty = true;
         //  }
       } else if (event->key() == Qt::Key::Key_Right) {
             qDebug() << "print star";
             if (edit_mode_object == nullptr || edit_mode_object->selection_type != VERTEX) {
                  return;
             }
             if (edit_mode_object->selected_component_array.empty()) return;
             Vertex *v = edit_mode_object->geometry->vertices[edit_mode_object->selected_component_array[0]];
             HalfEdge* start = v->half_edge;
             HalfEdge* it = v->half_edge;
             do  {
                  qDebug() << "v" << it->vertex->header.id << "e" << it->header.id/2;
                  it = it->star_next;
             } while (start != it);

       } else if (event->key() == Qt::Key::Key_Up) {
           qDebug() << "print loop";
           if (edit_mode_object == nullptr || edit_mode_object->selection_type != VERTEX) {
                return;
           }
           if (edit_mode_object->selected_component_array.empty()) return;
           Vertex *v = edit_mode_object->geometry->vertices[edit_mode_object->selected_component_array[0]];
           HalfEdge* start = v->half_edge;
           HalfEdge* it = v->half_edge;
           do  {
                qDebug() << "v" << it->vertex->header.id << "e" << it->header.id/2;
                it = it->loop_next;
           } while (start != it);

     }
    }
    //qDebug() << "key " << event->key();
    update();
}


void Viewport::keyReleaseEvent(QKeyEvent *event)
{
    if (event->isAutoRepeat()) {
      event->ignore();
    } else {
//--------------------------------TOOL TEMP MODE----------------------------------
        if (tool_timer->isValid()) {

            //Get hotkey commands or tool or operator
            auto it = hotkey_bindings->find((Qt::Key)event->key());
            if (it == hotkey_bindings->end()) {
                return;
            }

           Tool* tool = qvariant_cast<Tool*>(it->second);
           if ((tool != nullptr) & (tool == active_tool)) {
               // same hotkey as active tool
               if (tool_timer->elapsed() > 200) {
                   //key pressed too long, swap back old tool
                   active_tool->deactivate();
                   active_tool = previousTool;
               } else if (previousTool != nullptr) {
                   previousTool->deactivate();
               }
               tool_timer->invalidate();
           }

        }
    }
    update();
}


void Viewport::mousePressEvent(QMouseEvent *event)
{
    if (active_tool != nullptr) {
       active_tool->mousePressEvent(event);
    }
//    qDebug() << "event";
//    qDebug() << cursor().pos();
//    qDebug() << event->pos();
update();
}


void Viewport::mouseReleaseEvent(QMouseEvent *event)
{
    if (active_tool != nullptr) {
       active_tool->mouseReleaseEvent(event);
    }

    update();
}


void Viewport::wheelEvent(QWheelEvent *event)
{
//TODO if activeTool return false --------------

    //    if (activeTool != nullptr) {
    //       if (activeTool->wheelEvent(event)) return;
    //    }
//TODO -----------------------------------------

// default event zoom to cursor
    Camera* camera = &Viewport::getActiveViewport()->camera;
    QVector3D screen_point = QVector3D(event->position().rx(), height - event->position().ry() , 0.0f);
    QVector3D camera_ray_start = screen_point.unproject(camera->view, camera->projection, QRect(0, 0, width, height)); //camera near plane projection

    QVector3D camera_ray = (camera->transform.column(3).toVector3D() - camera_ray_start).normalized();
    QVector3D cameraPlaneNormal = (camera->transform.column(3).toVector3D() - camera->translation).normalized();

    float distance = QVector3D::dotProduct(cameraPlaneNormal, (camera->translation - camera_ray_start)) / QVector3D::dotProduct(camera_ray, cameraPlaneNormal);

    QVector3D rayPlanePoint = camera_ray_start + distance * camera_ray; // TODO fix nan
    QVector3D target = camera->translation - rayPlanePoint;

    if (event->delta() > 0) {
       camera->zoomToCursor(-1, target);
    } else {
       camera->zoomToCursor(1, target);
    }


    update();

}


void Viewport::mouseMoveEvent(QMouseEvent *event)
{
    if (active_tool != nullptr) {
       active_tool->mouseMoveEvent(event);
    }

    prevPos = event->pos();

    update();
}

void Viewport::mouseDoubleClickEvent(QMouseEvent *event)
{

}

void Viewport::enterEvent(QEvent *event)
{
    setFocus(); //TODO focus keyboard on widget over cursor
}

void Viewport::leaveEvent(QEvent *event)
{
}

void Viewport::dragMoveEvent(QDragMoveEvent *event)
{

}





// mouse position in world space, camera near plane
QVector3D Viewport::getMouseWorldPosition()
{
    //QPoint click_point = mapFromGlobal(InputManager::mousePosition());
    QPoint click_point = this->cursor().pos();
    QVector3D point = QVector3D(click_point.rx(), height - click_point.ry() , 0.0f);
    makeCurrent();
    //GLint view[4];
    //glGetIntegerv(GL_VIEWPORT, &view[0]);
    //return point.unproject(camera.view.inverted(), camera.projection, QRect(view[0], view[1], view[2], view[3]));
    return point.unproject(camera.view, camera.projection, QRect(0, 0, width, height));
}

QVector3D Viewport::unprojectClick(QPoint click_point)
{
    QVector3D point = QVector3D(click_point.rx(), height - click_point.ry() , 0.0f);
    //makeCurrent();
    point = point.unproject(camera.view, camera.projection, QRect(0, 0, width, height));
    QVector3D cameraPosition =  camera.transform.column(3).toVector3D();
    return (cameraPosition - point).normalized();
}




//TODO 2 metodi, passare object e component e niente.
//TODO cache se view non cambia
void Viewport::drawSelectionBuffer(Mesh* object, SelectionType selection_type)
{
    QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions(); //TODO default camera per viewport e metodo: this->getCamera()->getViewProjectionMatrix();
    makeCurrent();
    selectBuffer->bind(); //glBindFramebuffer write?
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_CULL_FACE);

    if (object != nullptr) {
         object->drawSelection(f, &camera.viewProjection, selection_type, 0);
    } else {
        std::vector<Object3D*> *sceneObjects = scene.getObjectList();
        for(int i = 0; i < sceneObjects->size(); i++) {
            sceneObjects->at(i)->drawSelection(f, &camera.viewProjection, OBJECT, i);
        }
    }

    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);

    selectBuffer->bindDefault(); //restore default framebuffer

    QOpenGLExtraFunctions *ef = QOpenGLContext::currentContext()->extraFunctions();
//   ef->glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
  // glFinish();

    return;
    //test --------------
    glBindTexture(GL_TEXTURE_1D, select_texture);
   // float* pixels = new float[1024]();
    unsigned char* pixels = new unsigned char[10000];

    glGetTexImage(GL_TEXTURE_1D, 0, GL_RED,  GL_UNSIGNED_BYTE, pixels);
    std::vector<int> ids;
    for (int i = 0; i < 8; i++) {
        if(pixels[i] != 0) {
            ids.push_back(i);
        }
    }
    qDebug() << "picked ids " << ids;
   // int id = pixels[0] + pixels[1]*256 + pixels[2]*256*256;
    qDebug() << "color2: " << pixels[0] << pixels[1] << pixels[2] << pixels[3] << pixels[4];
    qDebug() << "error: " << glGetError();
    delete[] pixels;
    glBindTexture(GL_TEXTURE_1D, 0);
}



// TODO 10x10
int Viewport::getToolPickBufferID()
{
    makeCurrent();
    QPoint mp = mapFromGlobal(this->cursor().pos());
    unsigned char data[4];
    glBindFramebuffer(GL_READ_FRAMEBUFFER, tool_selectBuffer->handle());
    glReadPixels(mp.x(), height - mp.y(), 1, 1, GL_RGBA, GL_UNSIGNED_BYTE , data);
    return data[0] + data[1]*256 + data[2]*256*256;
}

//std::vector<int> Viewport::getPickBufferIDs(SelectionType selection_type, QPoint click_point, Mesh* object3D)
//{
//   std::vector<int> ids;
//   return ids;
//}

std::vector<int> Viewport::getPickBufferIDs(SelectionType selection_type, bool single_selection, QPoint top_left, QPoint bottom_right, Mesh* object3D)
{
    //swap if top_right is bottom_left
    if ((top_left.x() - bottom_right.x()) > 0) {
        int temp = top_left.x();
        top_left.setX(bottom_right.x());
        bottom_right.setX(temp);
    }
    if ((top_left.y() - bottom_right.y()) > 0) {
        int temp = top_left.y();
        top_left.setY(bottom_right.y());
        bottom_right.setY(temp);
    }

    // convert y to opengl y
    top_left.setY(height - top_left.y());
    bottom_right.setY(height - bottom_right.y());

    // clamp rectangle to viewport
    if (top_left.x() < 0) top_left.setX(0);
    if (top_left.y() > height) top_left.setY(height);

    if (bottom_right.x() > width) bottom_right.setX(width);
    if (bottom_right.y() < 0) bottom_right.setY(0);

    // calc rectangle size
    int rectangle_height = top_left.y() - bottom_right.y();
    int rectangle_width = bottom_right.x() - top_left.x();

    //qDebug() << "ogl top_left" << top_left << "ogl bottom_right" << bottom_right;
    //qDebug() << "rectangle_height" << rectangle_height << "rectangle_width" << rectangle_width;

    if (object3D == nullptr && edit_mode_object != nullptr) {
        object3D = edit_mode_object;
    }

    makeCurrent();
    drawSelectionBuffer(object3D, selection_type);  //TODO draw xy

    //unsigned char *data = new unsigned char[rectangle_height * rectangle_width * 4](); // TODO fisso per mouse move
    float *data = new float[rectangle_height * rectangle_width * 4]();
    glBindFramebuffer(GL_READ_FRAMEBUFFER, selectBuffer->handle());
    glReadPixels(top_left.x(), bottom_right.y(), rectangle_width, rectangle_height, GL_RGBA, GL_FLOAT , data); //left corner is at location (x, y)

    int rectangles_bytes = rectangle_height * rectangle_width * 4;
    std::vector<int> ids;
    if (single_selection) {

        // TODO spiral loop
        for(int i = 0; i < rectangles_bytes; i += 4) {
            int id = data[i]*255 + data[i+1]*255*256 + data[i+2]*255*256*256;
            if (id != MAX_SELECTION_ID) {
                ids.push_back(id);
                break;
            }
        }

    } else {

        int element_count;
        if(selection_type & OBJECT) {
            element_count = scene.objectList.size();
        } else if (selection_type & VERTEX) {
             element_count = object3D->geometry->vertices.size();
        } else if (selection_type & POLYGON) {
            element_count = object3D->geometry->triangle_to_polygon.size();
        } else {
            element_count = object3D->geometry->half_edges.size() / 2;
        }
        bool *visited = new bool[element_count]();

        for(int i = 0; i < rectangles_bytes; i += 4) {
            int id = data[i]*255 + data[i+1]*255*256 + data[i+2]*255*256*256;

            if (id != MAX_SELECTION_ID && !visited[id]) {
                visited[id] = true;
                ids.push_back(id);
            }
        }
        delete[] visited;
    }

    delete[] data;
    return ids;
}

void Viewport::getRectangleSelectionBufferPixelID(QPoint rectangle_sel_start, QPoint rectangle_sel_end)
{
    makeCurrent();
    int h = abs((rectangle_sel_start.x() - rectangle_sel_end.x()));
    int w = abs((rectangle_sel_start.y() - rectangle_sel_end.y()));
    int bufferSize = h * w;
    //qDebug() << "size: "  << bufferSize << h << w;

    unsigned char *data = new unsigned char[bufferSize * 4]();

    glBindFramebuffer(GL_READ_FRAMEBUFFER, selectBuffer->handle());

    //selectBuffer->bind();
    //glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    //qDebug() << "mouse start: "  << rectangle_sel_start;

    //  qDebug() << "read: "  << rectangle_sel_start.x() << height - rectangle_sel_start.y() << rectangle_sel_end.x() << height - rectangle_sel_end.y();
    //glReadPixels(rectangle_sel_start.x(), height - rectangle_sel_start.y(), rectangle_sel_end.x(), height - rectangle_sel_end.y(), GL_RGBA, GL_UNSIGNED_BYTE , data);

    //swap x,y if negative for glReadPixels
    int x = rectangle_sel_start.x();
    int y = rectangle_sel_start.y();

    if ((x - rectangle_sel_end.x()) > 0) {
       x = rectangle_sel_end.x();
    }
    if (y - (rectangle_sel_end.y()) < 0) {
       y =  rectangle_sel_end.y();
    }

    //qDebug() << "read: "  << x << height - y << x + w << y + h;
    glReadPixels(x, height - y, h, w, GL_RGBA, GL_UNSIGNED_BYTE , data);
   // glReadPixels(0, 0, 100, 100, GL_RGBA, GL_UNSIGNED_BYTE , data);


    //test vertex rectangle selection
    std::vector<QVariant> &selection = Application::instance().selection;
    if (!selection.empty()) {
       Mesh* m = qvariant_cast<Mesh*>(selection.at(0));
       if (!m) return; //TODO refactor
       bool *visited = new bool[m->geometry->vertices.size()]();
       for(int i = 0; i < bufferSize * 4 ; i+=4) {
           //qDebug() << data[0] << data[1] << data[2] << data[3];
           if (data[i] != 255 && !visited[data[i]]) {
               m->selectComponent(data[i]);
           }
       }
       delete[] visited;
    }
    delete[] data;
}


void Viewport::testGLSelect() {

    //mouse coord
    QPoint mp = mapFromGlobal(this->cursor().pos());
    double x = mp.x();
    double y = height - mp.y();

    //setup pickMatrix
    QMatrix4x4 viewProjection;
    viewProjection.setToIdentity();

    int viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);

    double select_width = 5.0;
    double select_height = 5.0;
    viewProjection.translate((viewport[2] - 2 * (x - viewport[0])) / select_width, (viewport[3] - 2 * (y - viewport[1])) / select_height, 0);
    viewProjection.scale(viewport[2] / select_width, viewport[3] / select_height, 1.0);

    viewProjection *= camera.viewProjection;

    //init selection buffer
    GLuint selectBuf[128]; //BUFSIZE 128
    GLint hits;
    glSelectBuffer(128, selectBuf);
    glRenderMode(GL_SELECT);
    glInitNames();
    glPushName(0); //init name stack

    QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();
    std::vector<Object3D*>* sceneObjects = scene.getObjectList();

    for(int i = 0; i < sceneObjects->size(); i++) {
        glLoadName(i);
        ((Mesh*)sceneObjects->at(i))->draw(f, &viewProjection, this);
    }

    //glFlush(); serve?
    hits = glRenderMode(GL_RENDER);
    //TODO parse buffer: [i] number of pushed name [i+1] min depht, [i+2] max depth, [i+3] name (contenuto del name stack)
    qDebug() << "hits:" << hits;
    qDebug() << selectBuf;
}


void Viewport::testOcclusionQuery() {
    QOpenGLExtraFunctions *f = QOpenGLContext::currentContext()->extraFunctions();
    std::vector<Object3D*>* sceneObjects = scene.getObjectList();
    QMatrix4x4 viewProjection;

    viewProjection *= camera.projection * camera.view.inverted();

    GLuint queries[128];
    std::vector<int> result;
    f->glGenQueries(128, queries);

    for(int i = 0; i < sceneObjects->size(); i++) {
        f->glBeginQuery(GL_ANY_SAMPLES_PASSED, queries[i]); //OpenGL 3.3
        ((Mesh*)sceneObjects->at(i))->draw(f, &viewProjection, this);
        f->glEndQuery(GL_ANY_SAMPLES_PASSED);
        GLuint passed = 0;
        f->glGetQueryObjectuiv(queries[i], GL_QUERY_RESULT, &passed);
        if (passed) {
            result.push_back(i);
        }
    }

    f->glDeleteQueries(128, queries);

    qDebug() << "hits:" << result.size();
    qDebug() << result;
}


//define a picking region
//void gluPickMatrix(double x, double y, double width, double height, int viewport[4])
//{
//    glTranslated((viewport[2] - 2 * (x - viewport[0])) / width, (viewport[3] - 2 * (y - viewport[1])) / height, 0);
//    glScaled(viewport[2] / width, viewport[3] / height, 1.0);
//}

//Mesa implementation
//void gluPickMatrix( GLdouble x, GLdouble y, GLdouble width, GLdouble height, const GLint viewport[4] )
//{
//    GLfloat m[16];
//    GLfloat sx, sy;
//    GLfloat tx, ty;
//    sx = viewport[2] / width;
//    sy = viewport[3] / height;
//    tx = (viewport[2] + 2.0 * (viewport[0] - x)) / width;
//    ty = (viewport[3] + 2.0 * (viewport[1] - y)) / height;
//#define M(row,col) m[col*4+row]
//    M(0,0) = sx; M(0,1) = 0.0; M(0,2) = 0.0;  M(0,3) = tx;
//    M(1,0) = 0.0; M(1,1) = sy;  M(1,2) = 0.0; M(1,3) = ty;
//    M(2,0) = 0.0; M(2,1) = 0.0; M(2,2) = 1.0; M(2,3) = 0.0;
//    M(3,0) = 0.0; M(3,1) = 0.0; M(3,2) = 0.0; M(3,3) = 1.0;
//#undef M
//    glMultMatrixf( m );
//}




//void static begin_immediate(GL_PRIMITIVE) {
//    if (GL_PRIMITIVE == POINT) {
//        static vertexPointer = vertexPoint.map();
//    } else {
//        static vertexPointer = vertexTRiangle.map();
//    }

//}
//void static immediate_vertex(pos, color) {
//    case...
//       v = imm_pointer[vertexCount];
//       vertexPointer << vertex(pos, color);
//   vertexCount++;
//}
//void static end_immediate() {
//    vertexBuffer.unmap();
//}

//void static draw_immediate() {
//    if (vertexPoint > 0) {
//        glDrawArrays(GL_POINTS, 0, GL_UNSIGNED_SHORT);
//    }
//    if (line > 0) ...
//}




