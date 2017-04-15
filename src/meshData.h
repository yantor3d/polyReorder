/**
    Copyright (c) 2017 Ryan Porter    
    You may use, distribute, or modify this code under the terms of the MIT license.
*/

#ifndef MESH_DATA_CMD_H
#define MESH_DATA_CMD_H

#include <vector>
#include <unordered_map>

#include <maya/MDagPath.h>
#include <maya/MIntArray.h>


struct ComponentData
{
    std::vector<int> connectedVertices;
    std::vector<int> connectedEdges;
    std::vector<int> connectedFaces;
    std::unordered_map<int, std::vector<int>> faceSiblings;
};


class MeshData
{
public:
                            MeshData();
    virtual                 ~MeshData();
    
    virtual void            clear();
    virtual void            unpackMesh(MDagPath &meshDagPath);

private:    
    virtual void            unpackEdges(MDagPath &meshDagPath);
    virtual void            unpackFaces(MDagPath &meshDagPath);
    virtual void            unpackVertices(MDagPath &meshDagPath);
    virtual void            unpackVertexSiblings();
    
    virtual void            insertAll(MIntArray &src, std::vector<int> &dest);

public:
    int                             numberOfVertices = 0;
    int                             numberOfEdges = 0;
    int                             numberOfFaces = 0;
    
    std::vector<ComponentData>      vertexData;
    std::vector<ComponentData>      edgeData;
    std::vector<ComponentData>      faceData;
};

bool                contains(std::vector<int> &items, int &item);
std::vector<int>    intersection(std::vector<int> &a, std::vector<int> &b);

#endif