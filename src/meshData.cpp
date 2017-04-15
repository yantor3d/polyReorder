/**
    Copyright (c) 2017 Ryan Porter    
    You may use, distribute, or modify this code under the terms of the MIT license.
*/

#include "meshData.h"

#include <algorithm>
#include <vector>
#include <unordered_map>

#include <maya/MDagPath.h>
#include <maya/MIntArray.h>
#include <maya/MItMeshEdge.h>
#include <maya/MItMeshPolygon.h>
#include <maya/MItMeshVertex.h>


MeshData::MeshData() {}


MeshData::~MeshData() {}


void MeshData::clear()
{
    numberOfVertices = 0;
    numberOfEdges = 0;
    numberOfFaces = 0;

    vertexData.clear();
    edgeData.clear();
    faceData.clear();
}


void MeshData::unpackMesh(MDagPath &meshDagPath)
{    
    this->unpackEdges(meshDagPath);
    this->unpackFaces(meshDagPath);
    this->unpackVertices(meshDagPath);

    this->unpackVertexSiblings();
}


void MeshData::unpackEdges(MDagPath &meshDagPath)
{
    MItMeshEdge edges(meshDagPath);

    this->numberOfEdges = edges.count();
    this->edgeData.resize(this->numberOfEdges);

    MIntArray connectedEdges;
    MIntArray connectedFaces;

    edges.reset();

    while (!edges.isDone())
    {
        ComponentData &edge = edgeData[edges.index()];

        edge.connectedVertices.resize(2);
        edge.connectedVertices[0] = edges.index(0);
        edge.connectedVertices[1] = edges.index(1);

        sort(edge.connectedVertices.begin(), edge.connectedVertices.end());
        
        edges.getConnectedFaces(connectedFaces);
        edges.getConnectedEdges(connectedEdges);

        insertAll(connectedFaces, edge.connectedFaces);
        insertAll(connectedEdges, edge.connectedEdges);

        edges.next();
    }
}


void MeshData::unpackFaces(MDagPath &meshDagPath)
{
    MItMeshPolygon faces(meshDagPath);

    this->numberOfFaces = faces.count();
    this->faceData.resize(this->numberOfFaces);

    MIntArray connectedEdges;
    MIntArray connectedFaces;
    MIntArray connectedVertices;

    faces.reset();

    while (!faces.isDone())
    {
        ComponentData &face = faceData[faces.index()];

        faces.getEdges(connectedEdges);
        faces.getConnectedFaces(connectedFaces);
        faces.getVertices(connectedVertices);

        insertAll(connectedFaces, face.connectedFaces);
        insertAll(connectedEdges, face.connectedEdges);
        insertAll(connectedVertices, face.connectedVertices);

        faces.next();
    }
}


void MeshData::unpackVertices(MDagPath &meshDagPath)
{
    MItMeshVertex vertices(meshDagPath);

    this->numberOfVertices = vertices.count();
    this->vertexData.resize(this->numberOfVertices);

    MIntArray connectedEdges;
    MIntArray connectedFaces;
    MIntArray connectedVertices;

    vertices.reset();

    while (!vertices.isDone())
    {
        ComponentData &vertex = vertexData[vertices.index()];

        vertices.getConnectedEdges(connectedEdges);
        vertices.getConnectedFaces(connectedFaces);
        vertices.getConnectedVertices(connectedVertices);

        insertAll(connectedFaces, vertex.connectedFaces);
        insertAll(connectedEdges, vertex.connectedEdges);
        insertAll(connectedVertices, vertex.connectedVertices);

        vertices.next();
    }
}


void MeshData::unpackVertexSiblings()
{
    for (int vertexIndex = 0; vertexIndex < this->numberOfVertices; vertexIndex++)
    {
        for (int &faceIndex : vertexData[vertexIndex].connectedFaces)
        {
            vertexData[vertexIndex].faceSiblings.emplace(faceIndex, std::vector<int>());

            for (int &faceVertexIndex : faceData[faceIndex].connectedVertices)
            {
                if (contains(vertexData[faceVertexIndex].connectedVertices, vertexIndex))
                {
                    vertexData[vertexIndex].faceSiblings[faceIndex].push_back(faceVertexIndex);
                }
            }

            sort(
                vertexData[vertexIndex].faceSiblings[faceIndex].begin(), 
                vertexData[vertexIndex].faceSiblings[faceIndex].end()
            );
        }
    }
}


void MeshData::insertAll(MIntArray &src, std::vector<int> &dest)
{
    dest.resize(src.length());

    for (uint i = 0; i < src.length(); i++)
    {
        dest[i] = src[i];
    }

    sort(dest.begin(), dest.end());
}


bool contains(std::vector<int> &items, int &item)
{
    return binary_search(items.begin(), items.end(), item);
}


std::vector<int> intersection(std::vector<int> &a, std::vector<int> &b)
{
    std::vector<int> result(a.size() + b.size());
    std::vector<int>::iterator it; 

    it = set_intersection(
        a.begin(),
        a.end(),
        b.begin(),
        b.end(),
        result.begin()
    );

    result.resize(it - result.begin());

    return result;    
}