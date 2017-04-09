/**
    Copyright (c) 2017 Ryan Porter    
    You may use, distribute, or modify this code under the terms of the MIT license.
*/

#ifndef MESH_DATA_CMD_H
#define MESH_DATA_CMD_H

#include <vector>
#include <unordered_map>

#include <maya/MDagPath.h>
#include <maya/MItMeshEdge.h>
#include <maya/MItMeshPolygon.h>
#include <maya/MItMeshVertex.h>


struct ComponentData
{
    std::vector<int> connectedVertices;
    std::vector<int> connectedEdges;
    std::vector<int> connectedFaces;
};

struct VertexData : ComponentData
{
    std::unordered_map<int, std::vector<int>> faceVertexSiblings;
};
    
struct EdgeData : ComponentData {};
struct FaceData : ComponentData {};

class MeshData
{
public:
    MeshData();
    virtual ~MeshData();

    virtual void            unpackMesh(MDagPath &meshDagPath);
    virtual void            clear();

private:
    
    virtual void        unpackEdges(MItMeshEdge &edges);
    virtual void        unpackFaces(MItMeshPolygon &faces);
    virtual void        unpackVertices(MItMeshVertex &vertices);
    virtual void        unpackVertexSiblings();
    
    virtual void        insertAll(MIntArray &src, std::vector<int> &dest);

public:
    int                     numberOfVertices = 0;
    int                     numberOfEdges = 0;
    int                     numberOfFaces = 0;
    
    std::vector<VertexData> vertexData;
    std::vector<EdgeData>   edgeData;
    std::vector<FaceData>   faceData;
};

std::vector<int> intersection(std::vector<int> &a, std::vector<int> &b);
bool             contains(std::vector<int> &items, int &value);

#endif