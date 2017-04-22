/**
    Copyright (c) 2017 Ryan Porter    
    You may use, distribute, or modify this code under the terms of the MIT license.
*/

#include "meshData.h"
#include "meshTopology.h"
#include "topologyPath.h"

#include <queue>
#include <iomanip>
#include <stdio.h>
#include <vector>
#include <limits.h>

#include <maya/MDagPath.h>
#include <maya/MFnMesh.h>
#include <maya/MGlobal.h>
#include <maya/MString.h>


MeshTopology::MeshTopology() {}


MeshTopology::MeshTopology(MDagPath &mesh)
{
    this->setMesh(mesh);
}


MeshTopology::~MeshTopology() {}


void MeshTopology::setMesh(MDagPath &mesh)
{
    this->mesh = mesh;
    meshData.unpackMesh(mesh);

    this->reset();
}


void MeshTopology::reset()
{
    edgePath.resize(meshData.numberOfEdges);    
    facePath.resize(meshData.numberOfFaces);
    vertexPath.resize(meshData.numberOfVertices);

    shellId = 0; 
}


bool MeshTopology::isComplete()
{
    return vertexPath.isComplete();
}


bool MeshTopology::hasVisitedVertex(int i)
{
    return vertexPath.visited(i);
}


void MeshTopology::walk(polyReorder::ComponentSelection &startAt)
{
    walkStartingFace(startAt);

    while (!edgePath.empty())
    {
        int nextEdge = edgePath.next();

        for (int &faceIndex : meshData.edgeData[nextEdge].connectedFaces)
        {
            if (!facePath.visited(faceIndex))
            {
                walkVerticesOnFace(faceIndex);
            }
        }
    }

    shellId++;
}


void MeshTopology::walkStartingFace(polyReorder::ComponentSelection &startAt)
{
    vertexPath.visit(startAt.vertexIndex, shellId);
    edgePath.visit(startAt.edgeIndex, shellId);

    walkVerticesOnFace(startAt.faceIndex);
}


void MeshTopology::walkVerticesOnFace(int &faceIndex)
{
    int nextEdge  = getFirstVisited(meshData.faceData[faceIndex].connectedEdges, edgePath);
    
    int firstVertex = getFirstVisited(meshData.edgeData[nextEdge].connectedVertices, vertexPath);
    int prevVertex = firstVertex;
    int nextVertex = getOppositeVertex(nextEdge, prevVertex);
   
    int tmp;

    while (true)
    {
        vertexPath.visit(nextVertex, shellId);
        edgePath.visit(nextEdge, shellId);
        edgePath.push(nextEdge);

        tmp = nextVertex;
        nextVertex = getNextVertexSibling(prevVertex, nextVertex, faceIndex);
        prevVertex = tmp;

        nextEdge = getTraversedEdge(prevVertex, nextVertex);

        if (nextVertex == firstVertex) 
        { 
            break;
        }
    }

    facePath.visit(faceIndex, shellId);
}


int MeshTopology::getFirstVisited(std::vector<int> &components, TopologyPath &path)
{
    int result = -1;
    int visitOrder = INT_MAX;

    for (int &idx : components)
    {
        int vo = path.visitedAt(idx);

        if (vo != -1 && vo < visitOrder) 
        {   
            result = idx;
            visitOrder = vo;            
        }
    }

    return result;
}


int MeshTopology::getOppositeVertex(int &edgeIndex, int &vertexIndex)
{
    int result = -1;

    for (int &v : meshData.edgeData[edgeIndex].connectedVertices)
    {
        if (v != vertexIndex)
        {
            result = v;
            break;
        }
    }

    return result;
}


int MeshTopology::getTraversedEdge(int &prevVertex, int &nextVertex)
{
    int result = -1;

    auto edgeTraversed = intersection(
        meshData.vertexData[prevVertex].connectedEdges,
        meshData.vertexData[nextVertex].connectedEdges
    );

    if (!edgeTraversed.empty())
    {
        result = edgeTraversed[0];
    }

    return result;
}


int MeshTopology::getNextVertexSibling(int &lastVertex, int &vertex, int &faceIndex)
{
    int result = -1;

    auto vertexSiblings = intersection(
        meshData.vertexData[vertex].connectedVertices,
        meshData.vertexData[vertex].faceSiblings[faceIndex]
    );

    for (int &v : vertexSiblings)
    {
        if (v != lastVertex)
        {
            result = v;
            break;
        }
    }

    return result;
}

bool MeshTopology::hasSameTopology(MDagPath &a, MDagPath &b)
{
    MFnMesh fA(a);
    MFnMesh fB(b);

    return (
           fA.numVertices() == fB.numVertices()
        && fA.numEdges()    == fB.numEdges()
        && fA.numPolygons() == fB.numPolygons()
    );
}