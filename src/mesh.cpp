#include "mesh.h"
#include "viewport.h"
#include "transform3d.h"
#include "material.h"
#include "geometry.h"

#include <QOpenGLFunctions_4_2_Core>
#include <QOpenGLShaderProgram>
#include <QVector2D>
#include <QOpenGLTexture>


Mesh::Mesh(): Object3D()
{

}

Mesh::~Mesh()
{
    // TODO vertexBuffer.destroy();
}

void Mesh::init()
{
    normalBuffer.create();
    normalBuffer.bind();
    normalBuffer.setUsagePattern(QOpenGLBuffer::DynamicDraw);
    normalBuffer.release();

    triangleVertexBuffer.create();
    triangleVertexBuffer.bind();
    triangleVertexBuffer.setUsagePattern(QOpenGLBuffer::DynamicDraw);
    triangleVertexBuffer.release();

    uv_buffer.create();
    uv_buffer.bind();
    uv_buffer.setUsagePattern(QOpenGLBuffer::DynamicDraw);
    uv_buffer.release();

    geometry_dirty = true;

    updateBBox();
}

void Mesh::updateBBox()
{
    // max point from center
    bb_distance = 0;
    for(auto &v: geometry->vertices) {
         bb_distance = fmax(bb_distance, v->position.length());
    }
}

void Mesh::increaseSubdivisionLevel()
{
    if (geometry->subdivision_level > 2) return;

    geometry->subdivision_level++;
    geometry->createSubdivision();
    geometry->updateSubdivision();

    if (!subdivision_vertex_buffer.isCreated()) subdivision_vertex_buffer.create();
    subdivision_vertex_buffer.bind();
    subdivision_vertex_buffer.setUsagePattern(QOpenGLBuffer::DynamicDraw);
    int vertex_count = geometry->subdivision_vertices.size();
    subdivision_vertex_buffer.allocate(sizeof(float) * 6 * vertex_count);
    subdivision_vertex_buffer.release();

    updateBuffers();
}

void Mesh::decreaseSubdivisionLevel()
{
    if (geometry->subdivision_level < 1) return;
    geometry->subdivision_level--;
    geometry->createSubdivision();
    geometry->updateSubdivision();

    if (!subdivision_vertex_buffer.isCreated()) subdivision_vertex_buffer.create();
    subdivision_vertex_buffer.bind();
    subdivision_vertex_buffer.setUsagePattern(QOpenGLBuffer::DynamicDraw);
    int vertex_count = geometry->subdivision_vertices.size();
    subdivision_vertex_buffer.allocate(sizeof(float) * 6 * vertex_count);
    subdivision_vertex_buffer.release();

    updateBuffers();
}


void Mesh::setMaterial(Material* material)
{
    //TODO ?
  triangleVertexBuffer.bind();
  vertexArrayObject.bind();

  material->shaderProgram->bind();
  material->shaderProgram->enableAttributeArray(0);
  material->shaderProgram->setAttributeBuffer(0, GL_FLOAT, 3, 6 * sizeof(float), 24);
  material->shaderProgram->enableAttributeArray(1);
  material->shaderProgram->setAttributeBuffer(1, GL_FLOAT, 12, 0, 24);
  material->shaderProgram->release();

  vertexArrayObject.release();
  triangleVertexBuffer.release();

  //TODO refactor
  this->material = material;
  shaderProgram = material->shaderProgram;
}


void Mesh::makeCube(float size) //TOOD operator
{
    std::vector<Vertex> vertices;

    float fromCenter = size/2;
    vertices.push_back(Vertex(-fromCenter, -fromCenter, -fromCenter)); //back
    vertices.push_back(Vertex(-fromCenter, fromCenter, -fromCenter));
    vertices.push_back(Vertex(fromCenter, fromCenter, -fromCenter));
    vertices.push_back(Vertex(fromCenter, -fromCenter, -fromCenter));

    vertices.push_back(Vertex(fromCenter, -fromCenter, fromCenter));
    vertices.push_back(Vertex(fromCenter, fromCenter, fromCenter));
    vertices.push_back(Vertex(-fromCenter, fromCenter, fromCenter));
    vertices.push_back(Vertex(-fromCenter, -fromCenter, fromCenter));

    polygons.push_back(std::vector<int>{6,7,4,5}); //front
    polygons.push_back(std::vector<int>{0,1,2,3}); //back
    polygons.push_back(std::vector<int>{5,2,1,6}); //top
    polygons.push_back(std::vector<int>{7,0,3,4}); //bottom
    polygons.push_back(std::vector<int>{3,2,5,4}); //left
    polygons.push_back(std::vector<int>{6,1,0,7}); //right

    geometry = new Geometry;
    geometry->fromVertexFaceArray(vertices, polygons);
    geometry->updateFaceNormals();
    geometry->updateHalfEdgeNormals();
    geometry->triangulate();

    //test UVS property
//    geometry->half_edge_uvs.resize(geometry->half_edges.size());
//    std::vector<QVector2D> *half_edge_uvs = &geometry->half_edge_uvs;
//    for (auto &face : geometry->faces) {
//        HalfEdge *he = face->half_edge;
//        half_edge_uvs->at(he->header.id) = QVector2D(0.0f, 0.0f);
//        he = he->loop_next;
//        half_edge_uvs->at(he->header.id) = QVector2D(1.0f, 0.0f);
//        he = he->loop_next;
//        half_edge_uvs->at(he->header.id) = QVector2D(1.0f, 1.0f);
//        he = he->loop_next;
//        half_edge_uvs->at(he->header.id) = QVector2D(0.0f, 1.0f);
//    }

    for (auto &face : geometry->faces) {
        HalfEdge *he = face->half_edge;
        he->uv = QVector2D(0.0f, 0.0f);
        he = he->loop_next;
        he->uv = QVector2D(1.0f, 0.0f);
        he = he->loop_next;
        he->uv = QVector2D(1.0f, 1.0f);
        he = he->loop_next;
        he->uv = QVector2D(0.0f, 1.0f);
    }
}


void Mesh::updateTopology() {

    vertex_indices.resize(geometry->vertices.size());
  //  vertex_indices[geometry->vertices.size() - 1];

    vertex_buffer_count = 0;
    triangle_count = 0;

    for (auto &face : geometry->faces) {
        polygon_triangles_index.push_back(triangle_count);
        triangle_count += face->vertex_count - 2;
        vertex_buffer_count += face->vertex_count;
    }


    normalBuffer.bind();
    normalBuffer.allocate(sizeof(float) * 3 * geometry->half_edges.size() * 2);
    normalBuffer.release();

    triangleVertexBuffer.bind();
    triangleVertexBuffer.allocate(sizeof(float) * 6 * vertex_buffer_count);
    triangleVertexBuffer.release();


    uv_buffer.bind();
    uv_buffer.allocate(sizeof(float) * 2 * geometry->half_edges.size() * 2);
    uv_buffer.release();
}


// TODO ogni faccia ha la sua lista di vertex, per aggiorne solo i modificati servirebbe una tabella vertex -> vertices_in_buffer
// si puo ottimizzare aggiornando dal minimo index modificato
void Mesh::updateBuffers() {

  //  if () {
        //vertex_indices = new GLushort[geometry->vertices.size()];
         // TODO SOLO QUANDO CAMBIANO VERTICI
   // }

    //updateBBox();

    triangles_index_buffer.clear();
    triangles_index_buffer.reserve(geometry->triangles_indices.size());

    triangleVertexBuffer.bind();
    GLfloat* verticesArray = (GLfloat*)triangleVertexBuffer.map(QOpenGLBuffer::WriteOnly);

    int vertex_buffer_count = 0;
    int triangle_index_count = 0;
    for (const auto &face : geometry->faces) {

        HalfEdge* start = face->half_edge;
        HalfEdge* it = face->half_edge;
        do {
            Vertex *v = it->vertex;
            vertex_indices[v->header.id] = vertex_buffer_count++; // map from triangle vertex indices to generated vertex buffer index

            *verticesArray = v->position[0]; verticesArray++;
            *verticesArray = v->position[1]; verticesArray++;
            *verticesArray = v->position[2]; verticesArray++;

            //QVector3D he_normal = geometry->half_edge_normals[it->normal_index];
            QVector3D he_normal = *it->normal;
            *verticesArray = he_normal[0]; verticesArray++;
            *verticesArray = he_normal[1]; verticesArray++;
            *verticesArray = he_normal[2]; verticesArray++;

            it = it->loop_next;

        } while (start != it);

        int index_count = triangle_index_count + (face->vertex_count - 2) * 3;
        while (triangle_index_count < index_count) {
            triangles_index_buffer.push_back(vertex_indices[geometry->triangles_indices[triangle_index_count++]]);
        }
    }

    triangleVertexBuffer.unmap();
    triangleVertexBuffer.release();

// update UVS
    uv_buffer.bind();
    GLfloat* uv_buffer_map = (GLfloat*)uv_buffer.map(QOpenGLBuffer::WriteOnly);
    for (const auto &face : geometry->faces) {
        HalfEdge* start = face->half_edge;
        HalfEdge* it = face->half_edge;
        do {
            //QVector2D uv = geometry->half_edge_uvs[it->header.id];
            QVector2D uv = it->uv;
            *uv_buffer_map = uv[0]; uv_buffer_map++;
            *uv_buffer_map = uv[1]; uv_buffer_map++;
            it = it->loop_next;
        } while (start != it);
    }
    uv_buffer.unmap();
    uv_buffer.release();

// IF see normals
    normalBuffer.bind();
    verticesArray = (GLfloat*)normalBuffer.map(QOpenGLBuffer::WriteOnly);
    for (const auto &face : geometry->faces) {
        HalfEdge* start = face->half_edge;
        HalfEdge* it = face->half_edge;
        do {
            Vertex *v = it->vertex;
            *verticesArray = v->position[0]; verticesArray++;
            *verticesArray = v->position[1]; verticesArray++;
            *verticesArray = v->position[2]; verticesArray++;

            //QVector3D he_normal = geometry->half_edge_normals[it->normal_index];
            QVector3D he_normal = *it->normal;
            *verticesArray = v->position[0] + he_normal[0]*0.3; verticesArray++;
            *verticesArray = v->position[1] + he_normal[1]*0.3; verticesArray++;
            *verticesArray = v->position[2] + he_normal[2]*0.3; verticesArray++;
            it = it->loop_next;
        }  while (start != it);
    }
    normalBuffer.unmap();
    normalBuffer.release();



    if (geometry->subdivision_level > 0) {
        geometry->updateSubdivision();

        subdivision_vertex_buffer.bind();
        int vertex_count = geometry->subdivision_vertices.size();
        verticesArray = (GLfloat*)subdivision_vertex_buffer.map(QOpenGLBuffer::WriteOnly);
        for (int i = 0; i < vertex_count; i++) {
            Vertex *v = &geometry->subdivision_vertices[i];
            *verticesArray = v->position[0]; verticesArray++;
            *verticesArray = v->position[1]; verticesArray++;
            *verticesArray = v->position[2]; verticesArray++;

            *verticesArray = v->normal[0]; verticesArray++;
            *verticesArray = v->normal[1]; verticesArray++;
            *verticesArray = v->normal[2]; verticesArray++;

        }
        subdivision_vertex_buffer.unmap();
        subdivision_vertex_buffer.release();
    }

    edge_indices.clear();
    for (std::vector<HalfEdge*>::iterator it = geometry->half_edges.begin(); it != geometry->half_edges.end(); it += 2) {
        edge_indices.push_back(vertex_indices[(*it)->vertex->header.id]);
        edge_indices.push_back(vertex_indices[(*it)->twin->vertex->header.id]);
    }

}

void Mesh::updateVertexPosition() {

}


//TODO poly normals
void Mesh::updateVertexNormal() {
   // geometry->updateFaceNormals();
}


// draw pick buffers
void Mesh::drawSelection(QOpenGLFunctions *f, QMatrix4x4 *viewProjection, SelectionType selection_type, int id)
{
    if (geometry_dirty) { //TODO in update
        geometry->updateFaceNormals();
        geometry->triangulate();
        geometry->updateHalfEdgeNormals();
        updateTopology();
        updateBuffers();
        geometry_dirty = false;
    }

    Viewport *viewport = Viewport::getActiveViewport();
    QOpenGLShaderProgram *selectShader = Viewport::getSelectShader();
    selectShader->bind();
    triangleVertexBuffer.bind();
    selectShader->enableAttributeArray(0);
    selectShader->setAttributeBuffer(0, GL_FLOAT, 0, 3, 24);
    selectShader->setUniformValue("modelViewProj", *viewProjection * global_transform.toMatrix());

// WORKING
//    QOpenGLFunctions_4_2_Core* funcs = nullptr;
//    funcs = viewport->context()->versionFunctions<QOpenGLFunctions_4_2_Core>();
//    funcs->glBindImageTexture(0, viewport->select_texture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R8);
//    qDebug() << "errrrrrr" << glGetError();


    //qDebug() << "selection_type" << selection_type;

    if (selection_type == OBJECT) {
        selectShader->setUniformValue("selection_mode", 0); // color
        selectShader->setUniformValue("color", QVector3D(id/255.0f, 0, 0));
        f->glDrawElements(GL_TRIANGLES, triangles_index_buffer.size(), GL_UNSIGNED_INT, triangles_index_buffer.data()); //TODO oppure renderer->draw(material, mesh)? material->draw(mesh)
    } else {
        //TODO if wireframe
        int offset_id = 0;
        selectShader->setUniformValue("selection_mode", 2); // primitive id
        if(selection_type & POLYGON) {
            selectShader->setUniformValue("start_index", offset_id);
            f->glDrawElements(GL_TRIANGLES, triangles_index_buffer.size(), GL_UNSIGNED_INT, triangles_index_buffer.data());
            offset_id += triangle_count;
        } else { // if (selection_type & front) {
            selectShader->setUniformValue("selection_mode", 3); // clear
            f->glDrawElements(GL_TRIANGLES, triangles_index_buffer.size(), GL_UNSIGNED_INT, triangles_index_buffer.data());
            selectShader->setUniformValue("selection_mode", 2); // primitive id
        }


        if (selection_type & VERTEX) {
            selectShader->setUniformValue("start_index", offset_id);
            f->glDrawElements(GL_POINTS, geometry->vertices.size(), GL_UNSIGNED_INT, vertex_indices.data());
            offset_id += geometry->vertices.size();
        }
        if(selection_type & EDGE) {
            selectShader->setUniformValue("start_index", offset_id);
            glLineWidth(12.0f);
            f->glDrawElements(GL_LINES, edge_indices.size(), GL_UNSIGNED_INT, edge_indices.data());
            glLineWidth(1.0f);
            //offset_id += edge_indices.size();
        }

//        if(selection_type & POLYGON) {
//            QOpenGLShaderProgram *selectShader = Viewport::getSelectShader();
//            selectShader->bind(); //TODO index color shader
//            selectShader->setUniformValue("modelViewProj", viewProjection * global_transform.toMatrix());
//            selectShader->setUniformValue("selection_mode", 1);

//            triangleVertexBuffer.bind();
//            shaderProgram->enableAttributeArray(0);
//            shaderProgram->setAttributeBuffer(0, GL_FLOAT, 0, 3, 28);
//            shaderProgram->enableAttributeArray(1);
//            shaderProgram->setAttributeBuffer(1, GL_FLOAT, 24, 1, 28);
//            f->glDrawElements(GL_TRIANGLES, triangle_count*3, GL_UNSIGNED_SHORT, &triangles[0]);
//        }
    }
    triangleVertexBuffer.release();
    selectShader->release();
}



void Mesh::draw(QOpenGLFunctions *f, QMatrix4x4 *viewProjection, Viewport *viewport)
{
// Update object ---------------------------------------------------------------------------
 //    local_transform.rotate(1, QVector3D(1,0,0)); //test
    if (geometry_dirty) { //TODO in update
        geometry->updateFaceNormals();
        geometry->triangulate();
        geometry->updateHalfEdgeNormals();
        updateTopology();
        updateBuffers();
        geometry_dirty = false;
    }

    global_transform = parent()->global_transform * local_transform;

    QMatrix4x4 global = parent()->global_transform.toMatrix() * local_transform.toMatrix();


// Setup shader --------------------------------------------------------------------------------TODO dynamic shader
    shaderProgram->bind();
    material->setModelViewProj(*viewProjection * global_transform.toMatrix());
    material->setModelInverseTranspose(global_transform.toMatrix().inverted().transposed());
//    material->setModelViewProj(*viewProjection * global);
//    material->setModelInverseTranspose(global.inverted().transposed());
    shaderProgram->setUniformValue("viewport_size", QPoint{viewport->width, viewport->height});

    triangleVertexBuffer.bind();
    shaderProgram->enableAttributeArray(0);
    shaderProgram->setAttributeBuffer(0, GL_FLOAT, 0, 3, 24);

    shaderProgram->enableAttributeArray(1);
    shaderProgram->setAttributeBuffer(1, GL_FLOAT, 12, 3, 24);

    uv_buffer.bind();
    shaderProgram->enableAttributeArray(3);
    shaderProgram->setAttributeBuffer(3, GL_FLOAT, 0, 2, 8);

 // Draw selection ------------------------------------------------------------------------------ //TODO
    if (selected) {
        shaderProgram->setUniformValue("unlit", true);
        if (selection_type == VERTEX) {

            if (selected_component_array.size() > 0) {
                material->setColor(QVector3D(0.9, 0, 0));
                f->glDrawElements(GL_POINTS, selected_component_indices_array.size(), GL_UNSIGNED_INT, &selected_component_indices_array[0]);
            }

            material->setColor(QVector3D(0, 0, 1));
            f->glDrawElements(GL_POINTS, geometry->vertices.size(), GL_UNSIGNED_INT, vertex_indices.data());

        } else if (selection_type == EDGE) {

            if (selected_component_array.size() > 0) {
                material->setColor(QVector3D(0.9, 0, 0));
                f->glDrawElements(GL_LINES, selected_component_indices_array.size(), GL_UNSIGNED_INT, &selected_component_indices_array[0]);
            }

        } else if (selection_type == POLYGON && selected_component_array.size() > 0) {
            material->setColor(QVector3D(0.9, 0, 0));
            shaderProgram->setUniformValue("stripple", true);
            f->glDrawElements(GL_TRIANGLES, selected_component_indices_array.size(), GL_UNSIGNED_INT, &selected_component_indices_array[0]);
            shaderProgram->setUniformValue("stripple", false);
        }

        if (selection_type == OBJECT) {
            material->setColor(QVector3D(1, 1, 1));
        } else {
            material->setColor(QVector3D(1, 0.82f, 0));
        }

        //shaderProgram->setUniformValue("stipple_line", true);
        //shaderProgram->setUniformValue("stipple_line", false);
        //glPushAttrib(GL_ENABLE_BIT);
        //glLineStipple(1, 0x0F0F);
        //glEnable(GL_LINE_STIPPLE);
        //f->glDrawElements(GL_LINE_LOOP, internal_line_buffer.size(), GL_UNSIGNED_SHORT, internal_line_buffer.data());
        //glDisable(GL_LINE_STIPPLE);
        //glPopAttrib();

        f->glDrawElements(GL_LINES, edge_indices.size(), GL_UNSIGNED_INT, edge_indices.data());
        shaderProgram->setUniformValue("unlit", false);
    }

// Draw mesh -----------------------------
    material->setLight(Viewport::getActiveViewport()->camera.rotation.rotatedVector({0.0f, 0.33f, 1.0f})); // headlight TODO directional light TODO from material
    material->setColor(color);

    material->texture->bind();

    if (geometry->subdivision_level > 0) {
        triangleVertexBuffer.release();
        subdivision_vertex_buffer.bind();
        shaderProgram->enableAttributeArray(0);
        shaderProgram->setAttributeBuffer(0, GL_FLOAT, 0, 3, 24);
        shaderProgram->enableAttributeArray(1);
        shaderProgram->setAttributeBuffer(1, GL_FLOAT, 12, 3, 24);
        f->glDrawElements(GL_TRIANGLES, geometry->subdivision_triangle_indices.size(), GL_UNSIGNED_INT, geometry->subdivision_triangle_indices.data());
        subdivision_vertex_buffer.release();
    } else {
        glDrawElements(GL_TRIANGLES, triangles_index_buffer.size(), GL_UNSIGNED_INT, triangles_index_buffer.data());
        triangleVertexBuffer.release();
    }

     material->texture->release();
     uv_buffer.release();
     shaderProgram->disableAttributeArray(3);

// Draw normals --------------------------
    if (0) {
        normalBuffer.bind();
        shaderProgram->setAttributeBuffer(0, GL_FLOAT, 0, 3, 12);
        shaderProgram->disableAttributeArray(1);
        material->setColor(QVector3D(1, 1, 0));
        shaderProgram->setUniformValue("unlit", true);
        glDrawArrays(GL_LINES, 0, geometry->half_edges.size() * 2);
        shaderProgram->setUniformValue("unlit", false);
        normalBuffer.release();
    }

    shaderProgram->release();
}


void Mesh::selectComponent(uint index) {

    switch (selection_type) {
        case(VERTEX): {
            Vertex *v = geometry->vertices.at(index);
            if (!v->header.selected) {
               v->header.selected = true;
               selected_component_array.push_back(index);
               selected_component_indices_array.push_back(vertex_indices[index]);
            }
            break;
        }
        case(EDGE): {
            HalfEdge *he = geometry->half_edges.at(index * 2);
            if (!he->header.selected) {
               he->header.selected = true;
               selected_component_array.push_back(index);
               selected_component_indices_array.push_back(vertex_indices[he->vertex->header.id]);
               selected_component_indices_array.push_back(vertex_indices[he->twin->vertex->header.id]);
            }
        break;
        }
        case(POLYGON): {
            int face_index = geometry->triangle_to_polygon[index];
            Face *f = geometry->faces.at(face_index);
            if (!f->header.selected) { 
               int tri_index_start = geometry->polygon_to_triangles[face_index] * 3;
               selected_component_array.push_back(face_index);
               f->header.selected = true;
               for (int i = 0; i < (f->vertex_count - 2)*3; i++) {
                    selected_component_indices_array.push_back(vertex_indices[geometry->triangles_indices[tri_index_start + i]]);
               }
            }
        break;
        }
    }
}

void Mesh::restoreSelection(SelectionType selection_type) {
      selected_component_array.clear();
//    for (auto& verts: vertices) {
//        verts.selected = false;
//    }
}


void Mesh::deselectComponents() {
    selected_component_array.clear();
    selected_component_indices_array.clear();
    switch (selection_type) {
        case(VERTEX): {
            for (auto& verts: geometry->vertices) {
                verts->header.selected = false;
            }
            break;
        }
        case(EDGE): {
            for (auto& he: geometry->half_edges) {
                he->header.selected = false;
            }
        break;
        }
        case(POLYGON): {
            for (auto &f : geometry->faces) {
                f->header.selected = false;
            }
        break;
        }
    }
}



