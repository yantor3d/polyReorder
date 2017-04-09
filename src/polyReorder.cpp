/**
    Copyright (c) 2017 Ryan Porter    
    You may use, distribute, or modify this code under the terms of the MIT license.
*/

#include "polyReorder.h"

#include <vector>

#include <maya/MFloatPointArray.h>
#include <maya/MFloatVectorArray.h>
#include <maya/MFnMesh.h>
#include <maya/MGlobal.h>
#include <maya/MIntArray.h>
#include <maya/MItMeshPolygon.h>
#include <maya/MStatus.h>
#include <maya/MPointArray.h>
#include <maya/MVectorArray.h>


#define RETURN_IF_ERROR(s) if (!s) { return s; }


MStatus polyReorder::getPoints(MObject &mesh, MIntArray &pointOrder, MPointArray &outPoints, bool reorderPoints)
{
    MFnMesh meshFn(mesh);

    uint numVertices = meshFn.numVertices();

    MPointArray inPoints(numVertices);
    outPoints.setLength(numVertices);

    meshFn.getPoints(inPoints, MSpace::kObject);
    
    for (uint i = 0; i < numVertices; i++)
    {
        if (reorderPoints)
        {
            outPoints.set(inPoints[i], pointOrder[i]);
        } else {
            outPoints.set(inPoints[i], i);
        }
    }

    return MStatus::kSuccess;
}


MStatus polyReorder::getPolys(MObject &mesh, MIntArray &pointOrder, MIntArray &polyCounts, MIntArray &polyConnects, bool reorderPoints)
{
    MFnMesh fnMesh(mesh);
    fnMesh.getVertices(polyCounts, polyConnects);

    uint pci = 0;

    MItMeshPolygon iterPoly(mesh);

    while (!iterPoly.isDone())
    {
        MIntArray vertices;
        iterPoly.getVertices(vertices);

        polyCounts[iterPoly.index()] = (int) iterPoly.polygonVertexCount();

        uint nv = vertices.length();

        for (uint i = 0; i < nv; i++)
        {
            polyConnects[pci++] = reorderPoints ? pointOrder[vertices[i]] : vertices[i];
        }

        iterPoly.next();
    }

    return MStatus::kSuccess;
}


MStatus polyReorder::getVertexNormals(MObject &mesh, MIntArray &pointOrder, MVectorArray &vertexNormals)
{
    MStatus status;

    MFnMesh fnMesh(mesh);
    MFloatVectorArray currentVertexNormals;

    fnMesh.getVertexNormals(true, currentVertexNormals, MSpace::kObject);
    uint numNormals = currentVertexNormals.length();

    vertexNormals.setLength(numNormals);

    for (uint i = 0; i < numNormals; i++)
    {
        MVector v(currentVertexNormals[i]);
        vertexNormals.set(v, pointOrder[i]);
    }

    return MStatus::kSuccess;
}


MStatus polyReorder::reorderMesh(MObject &sourceMesh, MObject &targetMesh, MIntArray &pointOrder, MObject &outMesh, bool isMeshData)
{
    MStatus status;

    MFnMesh srcMeshFn(sourceMesh);
    MFnMesh tgtMeshFn(targetMesh);

    uint numVertices = srcMeshFn.numVertices();
    uint numPolys = srcMeshFn.numPolygons();

    MPointArray points;
    MIntArray polyCounts;
    MIntArray polyConnects;

    MVectorArray vertexNormals;
    
    polyReorder::getPoints(targetMesh, pointOrder, points, true);
    polyReorder::getPolys(sourceMesh, pointOrder, polyCounts, polyConnects, isMeshData);
    polyReorder::getVertexNormals(targetMesh, pointOrder, vertexNormals);

    if (isMeshData)
    {
        MFnMesh outMeshFn;

        outMeshFn.create(
            numVertices, 
            numPolys,
            points,
            polyCounts,
            polyConnects,
            outMesh,
            &status
        );

        CHECK_MSTATUS_AND_RETURN_IT(status);
    } else {
        MFnMesh outMeshFn(outMesh);
        MFloatPointArray floatPoints(numVertices);

        for (uint i = 0; i < numVertices; i++)
        {
            floatPoints[i].setCast(points[i]);
        }

        outMeshFn.createInPlace(
            numVertices, 
            numPolys,
            floatPoints,
            polyCounts,
            polyConnects
        );

        CHECK_MSTATUS_AND_RETURN_IT(status);
    }
  
    MFnMesh meshFn(outMesh, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MIntArray vertices(numVertices);
    for (uint i = 0; i < numVertices; i++)
    {
        vertices.set((int) i, i);
    }

    status = meshFn.setVertexNormals(vertexNormals, vertices, MSpace::kObject);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return MStatus::kSuccess;
}