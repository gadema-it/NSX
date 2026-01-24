#ifndef GEOMETRY_H
#define GEOMETRY_H

#include <QVector3D>
#include <QVector2D>
#include <cmath>
#include <vector>
#include <qdebug.h>
#include "memorypool.h"


#ifdef WITH_OPENSUBDIV
#include <far/stencilTable.h>
#include <far/topologyRefiner.h>
#endif

struct HalfEdge;

struct Header {
    uint id;
    bool selected; // TODO bit flags
    bool visited;
    //int mask
    //uint state_flag;
    //uint tool_flag;
    //ComponentProperties *properties
};

struct Vertex
{
    Vertex() { }
    Vertex(float x, float y, float z): position{x,y,z} {}
    Vertex(Vertex const & src) {
        position[0] = src.position[0];
        position[1] = src.position[1];
        position[2] = src.position[2];

        half_edge = src.half_edge;

        header = src.header;
    }

    void Clear( void * = nullptr ) {
        position[0]=position[1]=position[2]=0.0f;
    }

    void AddWithWeight(Vertex const & src, float weight) {
        position[0]+=weight*src.position[0];
        position[1]+=weight*src.position[1];
        position[2]+=weight*src.position[2];
    }

    void SetPosition(float x, float y, float z) {
        position[0]=x;
        position[1]=y;
        position[2]=z;
    }

    const float * GetPosition() const {
        return (float*)&position;
    }

    Header header; //12
    QVector3D position; //24
    QVector3D normal; //32
    HalfEdge* half_edge; //40
};


struct Face
{
    Header header;
    QVector3D normal;
    int vertex_count;
    HalfEdge* half_edge;
};


struct HalfEdge
{
    Header header;
    Vertex* vertex;
    Face* face;
    HalfEdge* loop_next;
    HalfEdge* loop_prev;
    HalfEdge* star_next;
    HalfEdge* star_prev;
    HalfEdge* twin;

    //HalfEdgeProperties* properties;
    QVector3D* normal;
    QVector3D stored_normal;
    QVector2D uv;
};


struct Triangle
{
    int vertex_indices[3];
    int face_index;
};


struct HalfEdgeNormal
{
    QVector3D normal;
};

class Properties
{


};


struct EarPoint // ear cutting
{
    QVector2D point;
    int index;
    bool convex;
    EarPoint *next;
    EarPoint *prev;

    bool containPoint(QVector2D p) {
        float abp = cross(p - point, prev->point - point);
        float pbc = cross(next->point - point, p - point);

        bool positive = abp > 0 && pbc > 0;
        bool negative = abp < 0 && pbc < 0;

        if (positive || negative) {
            float acp = cross(p - next->point, next->point - prev->point);
            return (negative && acp < 0) || (positive && acp > 0);
        } else {
            return false;
        }
    }

    bool containPoints(const std::list<EarPoint*> &points) {
        for(EarPoint *p: points) {
            if (this->containPoint(p->point)) {
                return false;
            }
        }
        return true;
    }

    bool isConvex(bool polygon_positive) {
        if (polygon_positive) {
            return cross(next->point - point, prev->point - point) > 0;
        } else {
            return cross(next->point - point, prev->point - point) < 0;
        }
    }

    bool positive() {
        return cross(next->point - point, next->next->point - next->point) > 0;
    }

    float areaX2() {
        return cross(next->point - point, prev->point - point);
    }

//    float isConcave(bool direction) {
//        return cross(next->point - point, prev->point - point);
//    }

    float sign() {
        return (point.x() - next->point.x()) * (prev->point.y() -  next->point.y()) - (prev->point.x() -  next->point.x()) * (point.y() -  next->point.y());
    }


//perp dot product
    float cross(const QVector2D &a, const QVector2D &b) {
        return a.x() * b.y()  -  a.y() * b.x();
    }
};



class Geometry
{
public:
    Geometry();

    std::vector<Vertex*> vertices;
    std::vector<Face*> faces;
    std::vector<HalfEdge*> half_edges;

    MemoryPool<Vertex> vertex_pool;
    MemoryPool<Face> face_pool;
    MemoryPool<HalfEdge> half_edge_pool;

//Properties TODO
    //std::vector<QVector3D> half_edge_normals;
    //std::vector<QVector2D> half_edge_uvs;
    //Properties half_edge_propetries;


    float angle_treshold = cos( 60 * 0.0174533 );

    void fromVertexFaceArray(const std::vector<Vertex> &vertices, const std::vector<std::vector<int>> &faces_indices);


    std::vector<int> facesIndices();

    std::vector<std::vector<int>> facesIndices2();
    std::vector<int> verticesPerFace();

    std::vector<int> faces_indices;
    std::vector<int> vertices_per_face;
    std::vector<int> edges_indices;

    std::vector<int> triangles_indices;
    std::vector<int> triangle_to_polygon;
    std::vector<int> polygon_to_triangles;

    void updateEdgesIndices();
    void updateFaceNormals();
    void updateHalfEdgeNormals();
    void fanTriangulate();
    void triangulate();

    int splitEdge(int index, float ratio);
    int splitFace(int vertex_A, int vertex_B);
    void addEdge(int index_v1, int index_v2);
    int addFace(std::vector<int> vertex_indices);
    int addVertexAndFace(std::vector<QVector3D> vertex_positions);
    void addFaceVertex(QVector3D position);
    void deleteFace(std::vector<int> vertex_indices);

//--------- subdiv

    int subdivision_level = 0;

#ifdef WITH_OPENSUBDIV
    OpenSubdiv::Far::TopologyRefiner* topology_refiner = nullptr;
    const OpenSubdiv::Far::StencilTable* stencil_table = nullptr;
    void createSubdivision();
    void updateSubdivision();
#endif

    //bool subdivision_active;
    //bool subdivision_cage;

    std::vector<Vertex> source_vertices;
    std::vector<Vertex> subdivision_vertices;
    std::vector<unsigned int> subdivision_triangle_indices;

    void printFaceIndex(HalfEdge *he) {
        HalfEdge *loop_start = he;
        HalfEdge *loop_current = he;
        std::vector<int> indices;
        do {
            indices.push_back(loop_current->header.id);
            loop_current = loop_current->loop_next;
        } while (loop_start != loop_current);
        qDebug() << indices;
    }
};

#endif // GEOMETRY_H
