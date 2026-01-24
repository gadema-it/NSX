#include "geometry.h"

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

#include <QDebug>

#ifdef WITH_OPENSUBDIV
#include <far/topologyDescriptor.h>
#include <far/stencilTableFactory.h>
#include <far/primvarRefiner.h>
#endif

static void test_loop_star(HalfEdge *he, bool inverse) {
    HalfEdge *test_A = he;
    HalfEdge *test_A_start = he;
    int guard = 0;
    qDebug() << "loop";
    do {
        if (test_A->face != nullptr) {
            qDebug() << "face" << test_A->face->header.id;
        }

        qDebug() << "vertex" << test_A->vertex->header.id;     

        if (guard > 30) {
            qDebug() << "face loop";
            Q_ASSERT(false);
        }

        if (inverse) {
            test_A = test_A->loop_prev;
        } else {
            test_A = test_A->loop_next;
        }

        guard++;
    } while (test_A != test_A_start);

    qDebug() << "star";
    test_A = he;
    test_A_start = he;
    do {
        if (test_A->face != nullptr) {
            qDebug() << "face" << test_A->face->header.id;
        }

        qDebug() << "vertex" << test_A->vertex->header.id;

        if (guard > 30) {
            qDebug() << "star loop";
            Q_ASSERT(false);
        }

        if (inverse) {
            test_A = test_A->star_prev;
        } else {
            test_A = test_A->star_next;
        }

        guard++;
    } while (test_A != test_A_start);
}


Geometry::Geometry():
    vertex_pool(100),
    face_pool(100),
    half_edge_pool(200)
{
}


int Geometry::addFace(std::vector<int> vertex_indices)
{



    return 0;
}


// no double edges
// no vertex checks
void Geometry::fromVertexFaceArray(const std::vector<Vertex> &vertices_data, const std::vector<std::vector<int>> &faces_indices)
{

    for (const Vertex &v : vertices_data) {
        Vertex *allocated_vertex = vertex_pool.allocate();
        *allocated_vertex = v;
        allocated_vertex->half_edge = nullptr;
        allocated_vertex->header.id = vertices.size();
        vertices.push_back(allocated_vertex);
    }

    for (std::vector<int> face_indices : faces_indices) {
        Face *allocated_face = face_pool.allocate();
        allocated_face->header.id = faces.size();
        allocated_face->vertex_count = face_indices.size();
        faces.push_back(allocated_face);
        HalfEdge *previous_he = nullptr;
        for(uint i = 0, j = 1; i < face_indices.size(); i++) { //loop from edge Vn --- V0  | V0 --- V1 ...
            Vertex *v1 = vertices[face_indices[i]];
            Vertex *v2 = vertices[face_indices[j]];
            HalfEdge *current_he = nullptr;

            if (v1->half_edge != nullptr) { // find free half edge from v1 to v2, in v1 half edge star
                HalfEdge *start_star_halfedge = v1->half_edge;
                HalfEdge *current_star_halfedge = start_star_halfedge;
                do {
                    if (current_star_halfedge->twin->vertex == v2 && current_star_halfedge->face == nullptr) { //TODO check double edges
                        current_he = current_star_halfedge;
                        break;
                    }
                    current_star_halfedge = current_star_halfedge->star_next;
                } while (current_star_halfedge != start_star_halfedge);
            }

            if (current_he == nullptr) { //no free he v1 -> v2
                HalfEdge *he_start = half_edge_pool.allocate();     // counter clockwise
                he_start->header.id = half_edges.size();
                half_edges.push_back(he_start);
                he_start->vertex = v1; // FIXME v1 e v2 invertiti?
                if (v1->half_edge == nullptr) {
                    v1->half_edge = he_start;
                    he_start->star_next = he_start;
                    he_start->star_prev = he_start;
                } else {
                    he_start->star_next = v1->half_edge->star_next;
                    v1->half_edge->star_next = he_start;
                    he_start->star_prev = v1->half_edge;
                    he_start->star_next->star_prev = he_start;
                }

                HalfEdge *he_end = half_edge_pool.allocate();
                he_end->header.id = half_edges.size();
                half_edges.push_back(he_end);
                he_end->vertex = v2;
                he_end->face = nullptr;
                he_end->loop_next = he_end;
                he_end->loop_prev = he_end;
                if (v2->half_edge == nullptr) {
                    v2->half_edge = he_end;
                    he_end->star_next = he_end;
                    he_end->star_prev = he_end;
                } else {
                    he_end->star_next = v2->half_edge->star_next;
                    v2->half_edge->star_next = he_end;
                    he_end->star_prev = v2->half_edge;
                    he_end->star_next->star_prev = he_end;
                }
                he_start->twin = he_end;
                he_end->twin = he_start;

                current_he = he_start;
            }

            if (previous_he == nullptr) {   // primo edge della faccia
                allocated_face->half_edge = current_he;
                current_he->loop_next = current_he;
                current_he->loop_prev = current_he;
            } else {
                current_he->loop_next = previous_he->loop_next;
                previous_he->loop_next = current_he;
                current_he->loop_prev = previous_he;
                current_he->loop_next->loop_prev = current_he;
            }

            current_he->face = allocated_face;

            previous_he = current_he;

            j++;
            if (j == face_indices.size()) j = 0;
        }
    }
}


std::vector<int> Geometry::facesIndices()
{
    faces_indices.clear();
    for(const Face *face: faces) {
        HalfEdge *loop_start = face->half_edge;
        HalfEdge *loop_current = loop_start;
        do {
            faces_indices.push_back(loop_current->vertex->header.id);
            loop_current = loop_current->loop_next;
        } while (loop_start != loop_current);
    }
    return faces_indices;
}


std::vector<std::vector<int>> Geometry::facesIndices2()
{
    std::vector<std::vector<int>> faces_indices;
    for(const Face *face: faces) {
        std::vector<int> face_indices;
        HalfEdge *loop_start = face->half_edge;
        HalfEdge *loop_current = loop_start;
        do {
            face_indices.push_back(loop_current->vertex->header.id);
            loop_current = loop_current->loop_next;
        } while (loop_start != loop_current);

        faces_indices.push_back(face_indices);
    }
    return faces_indices;
}


std::vector<int> Geometry::verticesPerFace()
{
    //std::vector<int> vertices_per_face;
    vertices_per_face.clear();
    for(const Face *face: faces) {
        vertices_per_face.push_back(face->vertex_count);
    }
    return vertices_per_face;
}


void Geometry::updateEdgesIndices()
{
    edges_indices.reserve(vertices.size());
    for (std::vector<HalfEdge*>::iterator it = half_edges.begin(); it != half_edges.end(); it += 2) {
        edges_indices.push_back((*it)->vertex->header.id);
        edges_indices.push_back((*it)->loop_next->vertex->header.id);
    }
}


void Geometry::fanTriangulate()
{
    triangles_indices.clear();
    polygon_to_triangles.clear();
    int triangle_count = 0;
    for (const auto &face : faces) {
        HalfEdge* start = face->half_edge;
        HalfEdge* it = face->half_edge->loop_next;
        while (start != it->loop_next) {
            triangles_indices.push_back(start->vertex->header.id);
            triangles_indices.push_back(it->vertex->header.id);
            it = it->loop_next;
            triangles_indices.push_back(it->vertex->header.id);
            triangle_to_polygon.push_back(face->header.id);
            //vertex_count += 2;
            triangle_count++;
        }
        //face->vertex_count = vertex_count;
        polygon_to_triangles.push_back(triangle_count);
    }
}


void Geometry::triangulate()
{
    triangles_indices.clear();
    polygon_to_triangles.clear();
    triangle_to_polygon.clear();
    int triangle_count = 0;
    HalfEdge* start;
    HalfEdge* it;
    for (const auto &face : faces) {
        polygon_to_triangles.push_back(triangle_count);

        switch (face->vertex_count) {
            case(3):
                start = face->half_edge;
                triangles_indices.push_back(start->vertex->header.id);
                it = start->loop_next;
                triangles_indices.push_back(it->vertex->header.id);
                it = it->loop_next;
                triangles_indices.push_back(it->vertex->header.id);
                triangle_to_polygon.push_back(face->header.id);
                ++triangle_count;
            break;

            case(4):
            {

                start = face->half_edge;

                QVector3D *a = &start->vertex->position;
                QVector3D *b = &start->loop_next->vertex->position;
                QVector3D *c = &start->loop_next->loop_next->vertex->position;
                QVector3D *d = &start->loop_prev->vertex->position;

                QVector3D vec1 =  QVector3D::crossProduct(*b - *a, *c - *a);
                QVector3D vec2 =  QVector3D::crossProduct(*d - *a, *d - *c);

                if (QVector3D::dotProduct(vec1, vec2) > 0) {
                    triangles_indices.push_back(start->vertex->header.id);
                    it = start->loop_next;
                    triangles_indices.push_back(it->vertex->header.id);
                    it = it->loop_next;
                    triangles_indices.push_back(it->vertex->header.id);
                    triangles_indices.push_back(it->vertex->header.id);
                    it = it->loop_next;
                    triangles_indices.push_back(it->vertex->header.id);
                    triangles_indices.push_back(start->vertex->header.id);
                } else {
                    triangles_indices.push_back(start->loop_prev->vertex->header.id);
                    triangles_indices.push_back(start->vertex->header.id);
                    it = start->loop_next;
                    triangles_indices.push_back(it->vertex->header.id);
                    triangles_indices.push_back(it->vertex->header.id);
                    it = it->loop_next;
                    triangles_indices.push_back(it->vertex->header.id);
                    it = it->loop_next;
                    triangles_indices.push_back(it->vertex->header.id);
                }
                triangle_to_polygon.push_back(face->header.id);
                ++triangle_count;
                triangle_to_polygon.push_back(face->header.id);
                ++triangle_count;
            }
            break;

            default: // ear clipping

            //project point
                QVector3D face_normal = face->normal;
                float fabsX = fabs(face_normal.x());
                float fabsY = fabs(face_normal.y());
                float fabsZ = fabs(face_normal.z());
                int axis;
                if (fabsX > fabsY) {
                    if (fabsX > fabsZ) {
                        axis = 0;
                    } else {
                        axis = 2;
                    }
                } else {
                    if (fabsY > fabsZ) {
                        axis = 1;
                    } else {
                        axis = 2;
                    }
                }

                //qDebug() << face->header.id << axis;

                HalfEdge *start = face->half_edge;
                HalfEdge *it = start;

               //build ear structure list
                std::vector<EarPoint> points;
                points.reserve(face->vertex_count);
                do {
                    QVector3D pos = it->vertex->position;
                    EarPoint ear;
                    switch (axis) {
                        case 0:
                            ear.point = QVector2D(pos.z(), pos.y());
                            break;
                        case 1:
                            ear.point = QVector2D(pos.x(), pos.z());
                            break;
                        case 2:
                            ear.point = QVector2D(pos.x(), pos.y());
                            break;
                    }
                    ear.index = it->vertex->header.id;
                    points.push_back(ear);
                    it = it->loop_next;
                } while (start != it);

                //set linked list
                int max_index = points.size() - 1;
                points[0].next = &points[1];
                points[0].prev = &points[max_index];
                for(int i = 1; i < max_index; i++) {
                    points[i].next = &points[i+1];
                    points[i].prev = &points[i-1];
                }
                points[max_index].next = &points[0];
                points[max_index].prev = &points[max_index - 1];

              //start ear clipping
               {
                    //bool polygon_positive = points[0].positive(); //??
                    float area = 0;
                    for (EarPoint &ear_point: points) {
                        area += ear_point.areaX2();
                    }
                    bool polygon_positive = area > 0;

                    std::list<EarPoint*> concave_points;

                    for(EarPoint &ear_point: points) {
                        ear_point.convex = ear_point.isConvex(polygon_positive);
                        if (!ear_point.convex) {
                            concave_points.push_back(&ear_point);
                        }
                    }

                 //   qDebug() << "concave_points size " << concave_points.size();

                   if (concave_points.empty()) {

                        // polygon is convex, fan triangulate

                        start = face->half_edge;
                        it = face->half_edge->loop_next;
                        while (start != it->loop_next) {
                            triangles_indices.push_back(start->vertex->header.id);
                            triangles_indices.push_back(it->vertex->header.id);
                            it = it->loop_next;
                            triangles_indices.push_back(it->vertex->header.id);
                            triangle_to_polygon.push_back(face->header.id);
                            triangle_count++;
                        }

                        continue;
                    }


                    int guard = 0;

                    EarPoint *start = &points[0];
                    EarPoint *it = start;
                    int tris = face->vertex_count - 2;
                    while (tris > 1) {
                      //  qDebug() << "tris " << tris;
                      //  qDebug() << "point " << it->index;
                     //   qDebug() << "convex " << it->convex;

                        if (it->convex && it->containPoints(concave_points)) {
                            //qDebug() << "ear";
                            triangles_indices.push_back(it->prev->index);
                            triangles_indices.push_back(it->index);
                            triangles_indices.push_back(it->next->index);
                            triangle_to_polygon.push_back(face->header.id);

                           // qDebug() << it->prev->index << it->index << it->next->index;
                            triangle_count++;
                            tris--;

                            it->prev->next = it->next;
                            it->next->prev = it->prev;

                            if (!it->prev->convex && it->prev->isConvex(polygon_positive)) {
                                it->prev->convex = true;
                                concave_points.remove(it->prev);
                            }

                            if (!it->next->convex && it->next->isConvex(polygon_positive)) {
                                it->next->convex = true;
                                concave_points.remove(it->next);
                            }
                        }

                        it = it->next;

                        guard++;
                        if (guard > 30) {
                            qDebug() << "Triangulate loop ---------------------------------------";
                            for(auto p:points) {
                                qDebug() << p.index << "isConvex" << p.convex;
                            }
                           // qDebug() << "concave_points" << concave_points;
                           // qDebug() << "points" << points;
                            break;
                        }

                    }

                    triangles_indices.push_back(it->prev->index);
                    triangles_indices.push_back(it->index);
                    triangles_indices.push_back(it->next->index);
                    triangle_to_polygon.push_back(face->header.id);
                    triangle_count++;
               }
        }
    }
}


void Geometry::updateFaceNormals()
{
    for (const auto &face : faces) {
            HalfEdge* start = face->half_edge;
            HalfEdge* it = face->half_edge;
            face->normal = {0.0f, 0.0f, 0.0f};
            do {
                QVector3D *v1 = &it->vertex->position;
                it = it->loop_next;
                QVector3D *v2 = &it->vertex->position;
                face->normal.setX(face->normal.x() + (v1->y() - v2->y()) * (v1->z() + v2->z()));
                face->normal.setY(face->normal.y() + (v1->z() - v2->z()) * (v1->x() + v2->x()));
                face->normal.setZ(face->normal.z() + (v1->x() - v2->x()) * (v1->y() + v2->y()));
            } while (start != it);
            face->normal.normalize();
           // qDebug() << face->header.id;
    }
}


void Geometry::updateHalfEdgeNormals()
{
    //half_edge_normals.resize(half_edges.size());

//    for (const auto &he : half_edges) {
//        he->header.visited = false;
//    }


    for (const auto &vertex : vertices) {
        //qDebug() << vertex->header.id;
        HalfEdge* start = vertex->half_edge;
        HalfEdge* it = vertex->half_edge;

        do  { // reset flag // TODO global counter
            it->header.visited = false;
            it = it->star_next;
            // qDebug() << it->header.id;
        } while (start != it);

      //  qDebug() << "trasssss";

        do {

         //   qDebug() << it->header.id;

            if (it->header.visited || it->face == nullptr) {
                it = it->star_next;
                continue; // if (start == it) break;
            }

            it->header.visited = true;

            HalfEdge* current = it;
            //QVector3D* normal = &half_edge_normals[current->header.id];
            //current->normal_index = current->header.id;
            QVector3D* normal = &current->stored_normal;
            current->normal = normal;

            *normal = current->face->normal;

            HalfEdge* next;
            do {
                next = current->twin;
                if (next->face == nullptr) break;
                next = next->loop_next;
                if (next->header.visited) break;

                if (QVector3D::dotProduct(current->face->normal, next->face->normal) > angle_treshold) {
                    *normal += next->face->normal;
                    //next->normal_index = current->normal_index;
                    next->normal = current->normal;
                    next->header.visited = true;
                    current = next;
                } else {
                    break;
                }
            } while(1);

            current = it;

           do { //counterclowise
                next = current->loop_prev->twin;
                if (next->header.visited || next->face == nullptr) break;
                //qDebug() << QVector3D::dotProduct(current->face->normal, next->face->normal);
                if (QVector3D::dotProduct(current->face->normal, next->face->normal) > angle_treshold) {
                    *normal += next->face->normal;
                    //next->normal_index = current->normal_index;
                    next->normal = current->normal;
                    next->header.visited = true;
                    current = next;
                } else {
                    break;
                }
            }  while(1);

            normal->normalize();

            it = it->star_next;
        } while (start != it);
    }
}


// UNDO save half_edge_v1_v2, half_edge_v2_v1; reconnect half_edge_v1_v2->next, half_edge_v2_v1->prev

int Geometry::splitEdge(int index, float ratio) //TODO index of edge
{
    HalfEdge *half_edge_v1_v2 = half_edges[index];
    HalfEdge *half_edge_v2_v1 = half_edge_v1_v2->twin;
    Vertex *v1 = half_edge_v1_v2->vertex;
    Vertex *v2 = half_edge_v2_v1->vertex;

   // printFaceIndex(half_edge_v1_v2);

    Vertex *v3 = vertex_pool.allocate();
    v3->header.id = vertices.size();
    vertices.push_back(v3);
// TODO new_vertex.interpolate(v1, v2, ratio); // iterpolate all properties
    //v3->position = v1->position + ratio * (v1->position - v2->position);
    v3->position = v1->position * ( 1 - ratio  ) +  v2->position * ( ratio );

    HalfEdge *new_he_v3_v2 = half_edge_pool.allocate();
    new_he_v3_v2->header.id = half_edges.size();
    half_edges.push_back(new_he_v3_v2);
    new_he_v3_v2->vertex = v3;
    v3->half_edge = new_he_v3_v2;

// TODO interpolate properties;
    new_he_v3_v2->face = half_edge_v1_v2->face;
    if (new_he_v3_v2->face != nullptr) {
         ++new_he_v3_v2->face->vertex_count;
        new_he_v3_v2->uv = (1 - ratio) * half_edge_v1_v2->uv +  ratio *  half_edge_v1_v2->loop_next->uv;
    }

// insert new after v1_v2 and before v1_v2->loop_next
    new_he_v3_v2->loop_next = half_edge_v1_v2->loop_next;
    new_he_v3_v2->loop_prev = half_edge_v1_v2;
    half_edge_v1_v2->loop_next->loop_prev = new_he_v3_v2;
    half_edge_v1_v2->loop_next = new_he_v3_v2;

//    HalfEdge *new_he_v3_v1 = half_edge_pool.allocate();
//    new_he_v3_v1->header.id = half_edges.size();
//    half_edges.push_back(new_he_v3_v1);
//    new_he_v3_v1->vertex = v3;
    HalfEdge *new_he_v2_v3 = half_edge_pool.allocate();
    new_he_v2_v3->header.id = half_edges.size();
    half_edges.push_back(new_he_v2_v3);

    new_he_v2_v3->vertex = half_edge_v2_v1->vertex; // swap vertex
    new_he_v2_v3->vertex->half_edge = new_he_v2_v3;
    half_edge_v2_v1->vertex = v3;

 // TODO interpolate properties
    new_he_v2_v3->face = half_edge_v2_v1->face;
    if (new_he_v2_v3->face != nullptr) {
        ++new_he_v2_v3->face->vertex_count;

         // copy
        new_he_v2_v3->uv = half_edge_v2_v1->uv;
        new_he_v2_v3->normal = half_edge_v2_v1->normal;
        new_he_v2_v3->stored_normal = half_edge_v2_v1->stored_normal;
        new_he_v2_v3->stored_normal = half_edge_v2_v1->stored_normal;

        half_edge_v2_v1->uv = ratio * half_edge_v2_v1->uv + ( 1 - ratio) * half_edge_v2_v1->loop_next->uv;
    }

    new_he_v3_v2->twin = new_he_v2_v3;
    new_he_v2_v3->twin = new_he_v3_v2;

    new_he_v2_v3->loop_next = half_edge_v2_v1;
    new_he_v2_v3->loop_prev = half_edge_v2_v1->loop_prev;
    half_edge_v2_v1->loop_prev->loop_next = new_he_v2_v3;
    half_edge_v2_v1->loop_prev = new_he_v2_v3;

    // loop star v2
    half_edge_v2_v1->star_next->star_prev = new_he_v2_v3;
    half_edge_v2_v1->star_prev->star_next = new_he_v2_v3;
    new_he_v2_v3->star_next = half_edge_v2_v1->star_next;
    new_he_v2_v3->star_prev = half_edge_v2_v1->star_prev;

    // loop star v3
    new_he_v3_v2->star_next = half_edge_v2_v1;
    new_he_v3_v2->star_prev = half_edge_v2_v1;
    half_edge_v2_v1->star_next = new_he_v3_v2;
    half_edge_v2_v1->star_prev = new_he_v3_v2;


    test_loop_star(new_he_v3_v2, false);
    test_loop_star(new_he_v2_v3, false);

    return v3->header.id; // TODO return new edge id
}



int Geometry::splitFace(int vertex_index_A, int vertex_index_B)
{
    Vertex *vertex_A = vertices[vertex_index_A];
    Vertex *vertex_B = vertices[vertex_index_B];

    HalfEdge *half_edge_start = vertex_A->half_edge;
    HalfEdge *half_edge_it;
    HalfEdge *half_edge_A = vertex_A->half_edge;
    HalfEdge *half_edge_B = nullptr;

    do {
        half_edge_it = half_edge_A;
        do {
            if (half_edge_it->face == nullptr) continue;
            if (half_edge_it->vertex == vertex_B) {
                if (half_edge_it->loop_next->vertex == vertex_A || half_edge_it->loop_prev->vertex == vertex_A) {
                    return -1;
                }
                half_edge_B = half_edge_it;
                goto same_face;
            }
            half_edge_it = half_edge_it->loop_next;
        } while (half_edge_it != half_edge_A);
        half_edge_A = half_edge_A->star_next;
    } while (half_edge_A != half_edge_start);

    return -1;

same_face:
    Face *face = half_edge_A->face;

    //qDebug() << "face" << face->header.id;


    Face *new_face = face_pool.allocate();
    new_face->header.id = faces.size();
    new_face->half_edge = vertex_A->half_edge;
    faces.push_back(new_face);

    HalfEdge *new_he_a_b = half_edge_pool.allocate();
    new_he_a_b->header.id = half_edges.size();
    new_he_a_b->vertex = vertex_A;
    new_he_a_b->face = face;
    half_edges.push_back(new_he_a_b);

    face->half_edge = new_he_a_b;

    HalfEdge *new_he_b_a = half_edge_pool.allocate();
    new_he_b_a->header.id = half_edges.size();
    new_he_b_a->vertex = vertex_B;
    new_he_b_a->face = new_face;
    half_edges.push_back(new_he_b_a);

    new_face->half_edge = new_he_b_a;

    new_he_a_b->twin = new_he_b_a;
    new_he_b_a->twin = new_he_a_b;

    //copy he attributes
    new_he_a_b->uv = half_edge_A->uv;
    new_he_b_a->uv = half_edge_B->uv;

    //fix loop A
    half_edge_A->loop_prev->loop_next = new_he_a_b;
    new_he_a_b->loop_prev = half_edge_A->loop_prev;
    half_edge_A->loop_prev = new_he_b_a;
    new_he_b_a->loop_next = half_edge_A;


    //fix star A
    half_edge_A->star_next->star_prev = new_he_a_b;
    new_he_a_b->star_next = half_edge_A->star_next;
    half_edge_A->star_next = new_he_a_b;
    new_he_a_b->star_prev = half_edge_A;

  //  qDebug () << "WTF";
    // set half edge new face and count vertices
    half_edge_it = half_edge_A;
    int vertex_count = 0;
    do {
        ++vertex_count;
        half_edge_it->face = new_face;
        half_edge_it = half_edge_it->loop_next;
    } while (half_edge_it != half_edge_B);
    new_face->vertex_count = vertex_count + 1;
    face->vertex_count -= (vertex_count - 1);


    //fix loop B
    half_edge_B->loop_prev->loop_next = new_he_b_a;
    new_he_b_a->loop_prev = half_edge_B->loop_prev;
    half_edge_B->loop_prev = new_he_a_b;
    new_he_a_b->loop_next = half_edge_B;

    //fix star B
    half_edge_B->star_next->star_prev = new_he_b_a;
    new_he_b_a->star_next = half_edge_B->star_next;
    half_edge_B->star_next = new_he_b_a;
    new_he_b_a->star_prev = half_edge_B;

//    qDebug () << "A---------------";
//    test_loop_star_prev(half_edge_A);
//    qDebug () << "B-------------";
//   test_loop_star_prev(half_edge_B);

    return new_face->header.id;
}


void Geometry::addEdge(int index_v1, int index_v2)
{

}


int Geometry::addVertexAndFace(std::vector<QVector3D> vertex_positions)
{
    Face *allocated_face = face_pool.allocate();
    allocated_face->header.id = faces.size();
    allocated_face->vertex_count = vertex_positions.size();
    faces.push_back(allocated_face);

    HalfEdge *previous_he = nullptr;
    HalfEdge *half_edge_1;
    HalfEdge *half_edge_2;

    for(int i = 0; i < vertex_positions.size(); i++) {
        Vertex *vertex_current = vertex_pool.allocate();
        vertex_current->position = vertex_positions[i];
        vertex_current->header.id = vertices.size();
        vertices.push_back(vertex_current);

        half_edge_1 = half_edge_pool.allocate();
        half_edge_1->header.id = half_edges.size();
        half_edges.push_back(half_edge_1);
        half_edge_1->face = nullptr;
        half_edge_1->vertex = vertex_current;
        vertex_current->half_edge = half_edge_1;

        if (allocated_face->half_edge == nullptr) {
            allocated_face->half_edge = half_edge_1;
        }

        half_edge_2 = half_edge_pool.allocate();
        half_edge_2->header.id = half_edges.size();
        half_edges.push_back(half_edge_2);
        half_edge_2->face = allocated_face;
        //half_edge_2->vertex = vertex_current;

        half_edge_1->twin = half_edge_2;
        half_edge_2->twin = half_edge_1;

        if (previous_he != nullptr) {
            previous_he->loop_next = half_edge_1;
            half_edge_1->loop_prev = previous_he;

            previous_he->twin->loop_prev = half_edge_2;
            half_edge_2->loop_next = previous_he->twin;

            previous_he->twin->vertex = vertex_current;

            half_edge_1->star_next = previous_he->twin;
            half_edge_1->star_prev = previous_he->twin;
            previous_he->twin->star_next = half_edge_1;
            previous_he->twin->star_prev = half_edge_1;
        }

        previous_he = half_edge_1;
    }

    half_edge_1 = allocated_face->half_edge;
    half_edge_2 = half_edge_1->twin;

    previous_he->loop_next = half_edge_1;
    half_edge_1->loop_prev = previous_he;

    previous_he->twin->loop_prev = half_edge_2;
    half_edge_2->loop_next = previous_he->twin;

    previous_he->twin->vertex = half_edge_1->vertex;

    half_edge_1->star_next = previous_he->twin;
    half_edge_1->star_prev = previous_he->twin;
    previous_he->twin->star_next = half_edge_1;
    previous_he->twin->star_prev = half_edge_1;

    allocated_face->half_edge = half_edge_1->twin;

    return allocated_face->header.id;
}

void Geometry::addFaceVertex(QVector3D position)
{

}


void Geometry::deleteFace(std::vector<int> vertex_indices)
{
    std::vector<Face*> removed_faces;
    std::vector<Vertex*> removed_vertices;
    std::vector<HalfEdge*> removed_halfedges;
    std::vector<HalfEdge*> face_halfedge; // internal he of the removed faces

    int face_max_index = faces.size() - 1;
    for (int index: vertex_indices) {
        if (index > face_max_index) continue;
        removed_faces.push_back(faces[index]);
    }

    for (Face *f: removed_faces) {
        int index = f->header.id;
        faces[index] = faces.back();
        faces[index]->header.id = index;
        faces.pop_back();

        HalfEdge *start = f->half_edge;
        HalfEdge *it = start;
        do {
            face_halfedge.push_back(it);
            it = it->loop_next;
        } while (start != it);
    }


    for(HalfEdge *it: face_halfedge) {
            qDebug() << "to remove -------- " << it->header.id;
    }


    for(HalfEdge *it: face_halfedge) {
        if (it->twin->face == nullptr) {  // edge da eliminare

            // aggiorna lista puntatori, sposta entrambi half edges
            int halfedge_index = (it->header.id / 2) * 2;
            qDebug() << "remove" << halfedge_index/2;

            HalfEdge *removed_halfedge = half_edges[halfedge_index];
            removed_halfedges.push_back(removed_halfedge);
            removed_halfedges.push_back(removed_halfedge->twin);
            //removed_halfedges.push_back(half_edges[halfedge_index + 1]);

            if (halfedge_index + 1 == half_edges.size() - 1) {
                half_edges.pop_back();  // se ultimi
                half_edges.pop_back();
            } else {
                HalfEdge *last_he = half_edges.back();
                last_he->header.id = removed_halfedge->twin->header.id;
                half_edges[removed_halfedge->twin->header.id] = last_he;
                half_edges.pop_back();

                last_he = half_edges.back();
                last_he->header.id = halfedge_index;
                half_edges[halfedge_index] = last_he;
                half_edges.pop_back();
            }

            // controlla vertex
            if (it->star_prev == it->star_next) {

                // rimuovi anche vertex, ha solo 2 edge

                int vertex_index = it->vertex->header.id;
                removed_vertices.push_back(vertices[vertex_index]);
                vertices[vertex_index] = vertices.back();
                vertices[vertex_index]->header.id = vertex_index;
                vertices.pop_back();          
            } else { // rimuovi halfedge da star
                it->star_next->star_prev = it->star_prev;
                it->star_prev->star_next = it->star_next;
                qDebug() << "star delete" << it->vertex->header.id;

                HalfEdge *prev_twin = it->loop_prev->twin;
                if (prev_twin->face == nullptr) {
                    prev_twin->star_next->star_prev = prev_twin->star_prev;
                    prev_twin->star_prev->star_next = prev_twin->star_next;
                    qDebug() << "star delete" << prev_twin->vertex->header.id;
                }

                it->vertex->half_edge = prev_twin->star_next;
                qDebug() << " it->vertex->half_edge --------------" << prev_twin->star_next->header.id;

            }

            // face loop
            HalfEdge *prev = it->loop_prev;
            if (prev->twin->face != nullptr) {

                // l'edge prev rimane, diventa confine

                prev->face = nullptr;
                it->twin->loop_next->loop_prev = prev;
                prev->loop_next = it->twin->loop_next;
            }

             qDebug() << "end remove";

        } else {
             qDebug() << "it other" << it->header.id / 2;
             qDebug() << "it other next" << it->loop_next->header.id / 2;

            HalfEdge *disk_prev = it->twin->loop_next->twin;
            HalfEdge *disk_prev_it = disk_prev;
            do {

                // disk loop

                if (disk_prev_it->face == nullptr) {

                    if (it->loop_prev->twin->face != nullptr) { // se rimane anche l'altro edge

                        if (it->loop_prev != disk_prev_it) {
                            // nm vertex
                            qDebug() << " nm vertex  nm vertex  nm vertex";
                            it->loop_prev->loop_next = disk_prev_it->loop_next;
                            disk_prev_it->loop_next->loop_prev = it->loop_prev;
                        }

                    } else {
                        // remove star
                        qDebug() << "star it" << it->vertex->header.id;
                        qDebug() << "star wtf" << it->star_prev->vertex->header.id;
                        qDebug() << "star" << it->loop_prev->twin->vertex->header.id;

                        HalfEdge *star_to_remove = it->loop_prev->twin;
                        //HalfEdge *star_to_remove = disk_prev_it->loop_next;
                        star_to_remove->star_prev->star_next = star_to_remove->star_next;
                        star_to_remove->star_next->star_prev = star_to_remove->star_prev;
                        it->vertex->half_edge = star_to_remove->star_next;
                    }

                    // si crea un confine
                    it->loop_prev = disk_prev_it;
                    disk_prev_it->loop_next = it;


                    qDebug() << "wtf --------------------------------" << it->loop_next->header.id  / 2;
                    qDebug() << "wtf " << it->loop_next->loop_next->header.id  / 2;
                    qDebug() << "wtf " << it->loop_next->loop_next->loop_next->header.id  / 2;

                    break;
                }
                disk_prev_it = disk_prev_it->loop_next->twin;
            } while (disk_prev != disk_prev_it);

            it->face = nullptr;
        }
    }


    for(HalfEdge *it: face_halfedge) {
        //std::vector<HalfEdge*>::iterator deleted_it = find (removed_halfedges.begin(), removed_halfedges.end(), it);
        //if (deleted_it != removed_halfedges.end()) {
        if (it->twin->face != nullptr) {
            test_loop_star(it, false);
            test_loop_star(it, true);
            test_loop_star(it->twin, false);
            test_loop_star(it->twin, true);
        }

    }


    qDebug() << "end delete";

}




//-------------------------------- subdivisions
#ifdef WITH_OPENSUBDIV
using namespace OpenSubdiv;

void Geometry::createSubdivision()
{
    if (subdivision_level == 0) return;

    typedef Far::TopologyDescriptor Descriptor;

    Sdc::SchemeType type = OpenSubdiv::Sdc::SCHEME_CATMARK;

    Sdc::Options sdc_options;
    sdc_options.SetVtxBoundaryInterpolation(OpenSubdiv::Sdc::Options::VTX_BOUNDARY_EDGE_ONLY);

    verticesPerFace(); // TODO refactor
    facesIndices(); // TODO refactor

    Descriptor desc;
    desc.numVertices  = vertices.size();
    desc.numFaces     = faces.size();
    desc.numVertsPerFace = vertices_per_face.data();
    desc.vertIndicesPerFace = faces_indices.data();

    // Instantiate a Far::TopologyRefiner from the descriptor
    if (topology_refiner != nullptr) delete topology_refiner;
    topology_refiner = Far::TopologyRefinerFactory<Descriptor>::Create(desc,
                                            Far::TopologyRefinerFactory<Descriptor>::Options(type, sdc_options));

    // Uniformly refine the topology up to 'maxlevel'
    topology_refiner->RefineUniform(Far::TopologyRefiner::UniformOptions(subdivision_level));

    // Use the Far::StencilTable factory to create discrete stencil table
    // note: we only want stencils for the highest refinement level.
    Far::StencilTableFactory::Options options;
    options.generateIntermediateLevels=false;
    options.generateOffsets=true;

    if (stencil_table != nullptr) delete stencil_table;
    stencil_table = Far::StencilTableFactory::Create(*topology_refiner, options);

// Allocate vertex primvar buffer (1 stencil for each vertex)
    int nstencils = stencil_table->GetNumStencils();
    subdivision_vertices.resize(nstencils);

     source_vertices.resize(vertices.size());

// save triangles
    Far::TopologyLevel const & refLastLevel = topology_refiner->GetLevel(subdivision_level);
    int nverts = refLastLevel.GetNumVertices();
    int nfaces = refLastLevel.GetNumFaces();
    int firstOfLastVerts = topology_refiner->GetNumVerticesTotal() - nverts;

    for (int face = 0; face < nfaces; ++face) {
        Far::ConstIndexArray fverts = refLastLevel.GetFaceVertices(face);
        subdivision_triangle_indices.push_back(fverts[0]);
        subdivision_triangle_indices.push_back(fverts[1]);
        subdivision_triangle_indices.push_back(fverts[2]);
        subdivision_triangle_indices.push_back(fverts[0]);
        subdivision_triangle_indices.push_back(fverts[2]);
        subdivision_triangle_indices.push_back(fverts[3]);
    }
}


void Geometry::updateSubdivision()
{
   if (subdivision_level == 0) return;

   for (int i = 0; i < source_vertices.size(); i++) {
       source_vertices[i] = *vertices[i];
    }

    stencil_table->UpdateValues(&source_vertices[0], &subdivision_vertices[0]);

   // normals
   Far::TopologyLevel const & refLastLevel = topology_refiner->GetLevel(subdivision_level);
   int nverts = refLastLevel.GetNumVertices();
   int nfaces = refLastLevel.GetNumFaces();

   for (int f = 0; f < nfaces; f++) {
       Far::ConstIndexArray faceVertices = refLastLevel.GetFaceVertices(f);

       const float * v0 = subdivision_vertices[  faceVertices[0] ].GetPosition();
       const float * v1 = subdivision_vertices[  faceVertices[1] ].GetPosition();
       const float * v2 = subdivision_vertices[  faceVertices[2] ].GetPosition();
       const float * v3 = subdivision_vertices[  faceVertices[3] ].GetPosition();

       QVector3D a = { v2[0]-v0[0], v2[1]-v0[1], v2[2]-v0[2] };
       QVector3D b = { v3[0]-v1[0], v3[1]-v1[1], v3[2]-v1[2] };
       QVector3D normalCalculated = QVector3D::crossProduct(a, b);
       normalCalculated.normalize();

       for (int vInFace = 0; vInFace < faceVertices.size() ; vInFace++ ) {
           subdivision_vertices[faceVertices[vInFace]].normal += normalCalculated;
       }
   }

   for (int vert = 0; vert < nverts; ++vert) {
       subdivision_vertices[vert].normal.normalize();
   }

}
#endif
