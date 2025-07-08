#ifndef MESH_H
#define MESH_H

#include "object3d.h"
#include "vertex.h"
#include "geometry.h"
#include "selection.h"

class QOpenGLShader;
class QOpenGLShaderProgram;
class Material;
class Viewport;


class Mesh : public Object3D
{
    Q_OBJECT

public:
    Mesh();
    Mesh(std::vector<Vertex> vertices, std::vector<int> indices);
    ~Mesh() override;

    void makeCube(float size);

    SelectionType selection_type = OBJECT;

    //indices
    std::vector<std::vector<int>> polygons;
   // std::vector<QVector3D> polygon_normals;

    std::vector<GLuint> vertex_indices; // map triangle vertex -> vertices
    std::vector<GLuint> edge_indices;
    std::vector<GLuint> edges_selection; //TEST
    std::vector<GLuint> polygon_triangles_index; // polygon -> triangles :: index in triangles arrays corresponding to the first triangle of the polygon
    std::vector<GLuint> triangles_polygon; // triangles -> polygon
    std::vector<GLuint> triangles; // array of vertex indices
    std::vector<GLuint> triangles_index_buffer; // array of vertex indices
  //  std::vector<GLushort> subdivision_triangles_index_buffer; // array of vertex indices

    //selections
    void selectComponent(uint index); //TODO selectComponent(std::vector<uint>)
    void deselectComponents(); //deselectComponents
    void restoreSelection(SelectionType selection_type);


    //TODO enum selection type
    std::vector<GLuint> selected_component_array;
    std::vector<GLuint> selected_component_indices_array;

    void updateTopology();
    void updateBuffers(); //updateVertexBuffer
    void updateVertexPosition();
    void updateVertexNormal();

    QOpenGLVertexArrayObject vertexArrayObject;
    //QOpenGLBuffer vertexBuffer;
    QOpenGLBuffer subdivision_vertex_buffer;
    QOpenGLBuffer triangleVertexBuffer;

    int vertex_buffer_count;

    QOpenGLBuffer indexBuffer; //triangles
    QOpenGLBuffer vertexIndicesBuffer; //selections
    QOpenGLBuffer edgeIndicesBuffer; //wireframe

    QOpenGLBuffer normalBuffer; // normal display

    QOpenGLBuffer uv_buffer; // normal display

    Material* material; //vector<Material, polygons> polygroups_materials;
    QOpenGLShaderProgram* shaderProgram; //TODO refator use only material
    //typedef std::vector<Polygon> polygonsGroup;
    //map<polygonGroups, Material> polygonGroups;
    //setMaterial(polygonsGroup, Material);

    void setMaterial(Material *material);
    void draw(QOpenGLFunctions *f, QMatrix4x4 *viewProjection, Viewport *viewport) override;
    void drawSelection(QOpenGLFunctions *f, QMatrix4x4 *viewProjection, SelectionType selection_type, int id) override;
    void init();

    QVector3D color = QVector3D(0.66f, 0.66f, 0.66f);
    QVector3D selColor = QVector3D(0.0f, 0.0f, 0.66f);

    void updateBBox() override;
    bool geometry_dirty = false;

    //int polygon_edges_count; //foreach polygon, same as vertex_buffer_count
    int triangle_count;

    void increaseSubdivisionLevel();
    void decreaseSubdivisionLevel();

  //  Surface_mesh* geometry;
    Geometry *geometry;


};


#endif // MESH_H
