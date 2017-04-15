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
#include <maya/MItMeshEdge.h>
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
    MFnMesh meshFn(mesh);
    meshFn.getVertices(polyCounts, polyConnects);

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


MStatus polyReorder::getFaceVertexNormals(MObject &mesh, MIntArray &pointOrder, MVectorArray &vertexNormals)
{
    MStatus status;

    MFnMesh meshFn(mesh);
        
    MFloatVectorArray vertexFloatNormals;
    meshFn.getNormals(vertexFloatNormals, MSpace::kObject);

    uint numNormals = vertexFloatNormals.length();

    vertexNormals.setLength(numNormals);

    for (uint i = 0; i < numNormals; i++)
    {
        vertexNormals.set(MVector(vertexFloatNormals[i]), i);
    }

    return MStatus::kSuccess;
}


MStatus polyReorder::setFaceVertexNormals(MObject &mesh, MIntArray &polyCounts, MIntArray &polyConnects, MVectorArray &vertexNormals)
{
    MStatus status;

    MFnMesh meshFn(mesh);
    
    MIntArray normalCounts;
    MIntArray normalIds;
    
    meshFn.getNormalIds(normalCounts, normalIds);

    uint numPolys = polyCounts.length();
    uint numNormals = polyConnects.length();

    MIntArray faceList(numNormals);
    MIntArray vertexList(numNormals);

    uint idx = 0;

    for (uint i = 0; i < numPolys; i++)
    {
        for (int j = 0; j < polyCounts[i]; j++, idx++)
        {
            faceList[idx] = i;
            vertexList[idx] = polyConnects[idx];
        }
    }
    
    status = meshFn.setFaceVertexNormals(vertexNormals, faceList, vertexList, MSpace::kObject);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return MStatus::kSuccess;
}

MStatus polyReorder::getFaceVertexLocks(MObject &mesh, MIntArray &pointOrder, MIntArray &faceList, MIntArray &vertexList, MIntArray &lockedList)
{
    MStatus status;

    MFnMesh meshFn(mesh);

    MIntArray normalCounts;
    MIntArray normalIds;

    MIntArray polyCounts;
    MIntArray polyConnects;

    status = meshFn.getNormalIds(normalCounts, normalIds);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = meshFn.getVertices(polyCounts, polyConnects);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    uint numPolys = polyCounts.length();
    uint numNormals = polyConnects.length();

    faceList.setLength(numNormals);
    vertexList.setLength(numNormals);
    lockedList.setLength(numNormals);

    uint i = 0;
    
    for (uint f = 0; f < numPolys; f++)
    {
        for (int fv = 0; fv < polyCounts[f]; fv++)
        {
            faceList[i] = f;
            vertexList[i] = pointOrder[polyConnects[i]];
            lockedList[i] = meshFn.isNormalLocked(normalIds[i]);
            i++;
        }
    }

    return MStatus::kSuccess;
}

MStatus polyReorder::setFaceVertexLocks(MObject &mesh, MIntArray &faceList, MIntArray &vertexList, MIntArray &lockedList)
{
    MStatus status;

    MFnMesh meshFn(mesh);

    uint numNormals = faceList.length();

    MIntArray lockedFaceList(numNormals);
    MIntArray lockedVertList(numNormals);

    MIntArray unlockedFaceList(numNormals);
    MIntArray unlockedVertList(numNormals);

    uint L = 0;
    uint UL = 0;

    for (uint i = 0; i < numNormals; i++)
    {
        if (lockedList[i] == 1)
        {
            lockedFaceList[L] = faceList[i];
            lockedVertList[L] = vertexList[i];
            L++;
        } else {
            unlockedFaceList[UL] = faceList[i];
            unlockedVertList[UL] = vertexList[i];
            UL++;
        }
    }

    if (UL)
    {
        unlockedFaceList.setLength(UL);
        unlockedVertList.setLength(UL);
        status = meshFn.unlockFaceVertexNormals(unlockedFaceList, unlockedVertList);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    if (L)
    {
        lockedFaceList.setLength(L);
        lockedVertList.setLength(L);
        status = meshFn.lockFaceVertexNormals(lockedFaceList, lockedVertList);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    return MStatus::kSuccess;
}


MStatus polyReorder::getEdgeSmoothing(MObject &mesh, std::unordered_map<uint64_t, bool> &edgeSmoothing)
{
    MStatus status;

    MItMeshEdge itEdge(mesh);

    while (!itEdge.isDone())
    {
        int v0 = itEdge.index(0);
        int v1 = itEdge.index(1);

        uint64_t edgeKey = polyReorder::twoIntKey(v0, v1);

        edgeSmoothing.emplace(edgeKey, itEdge.isSmooth());

        itEdge.next();
    }

    return MStatus::kSuccess;
}


MStatus polyReorder::setEdgeSmoothing(MObject &mesh, MIntArray &pointOrder, std::unordered_map<uint64_t, bool> &edgeSmoothing)
{
    MStatus status;

    MItMeshEdge itEdge(mesh);

    uint n = pointOrder.length();
    MIntArray pointReorder(n);

    for (uint i = 0; i < n; i++)
    {
        pointReorder[pointOrder[i]] = int(i);
    }

    while (!itEdge.isDone())
    {
        int v0 = pointReorder[itEdge.index(0)];
        int v1 = pointReorder[itEdge.index(1)];

        uint64_t edgeKey = polyReorder::twoIntKey(v0, v1);

        itEdge.setSmoothing(edgeSmoothing[edgeKey]);

        itEdge.next();
    }

    return MStatus::kSuccess;
}


MStatus polyReorder::getUVs(MObject &mesh, std::vector<UVSetData> &uvSets)
{
    MStatus status;

    MFnMesh meshFn(mesh);

    for (UVSetData &uvData : uvSets)
    {
        status = meshFn.getUVs(uvData.uArray, uvData.vArray, &(uvData.name));
        CHECK_MSTATUS_AND_RETURN_IT(status);

        status = meshFn.getAssignedUVs(uvData.uvCounts, uvData.uvIds, &(uvData.name));
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    return MStatus::kSuccess;
}


MStatus polyReorder::setUVs(MObject &mesh, std::vector<UVSetData> &uvSets)
{
    MStatus status;

    MFnMesh meshFn(mesh);

    for (UVSetData &uvData : uvSets)
    {
        if (uvData.name != "map1")
        {
            status = meshFn.createUVSet(uvData.name);
            CHECK_MSTATUS(status);
        }

        status = meshFn.clearUVs(&(uvData.name));
        CHECK_MSTATUS_AND_RETURN_IT(status);

        status = meshFn.setUVs(uvData.uArray, uvData.vArray, &(uvData.name));
        CHECK_MSTATUS_AND_RETURN_IT(status);

        status = meshFn.assignUVs(uvData.uvCounts, uvData.uvIds, &(uvData.name));
        CHECK_MSTATUS_AND_RETURN_IT(status);
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

    MIntArray faceList;
    MIntArray vertexList;
    MIntArray lockedList;

    MVectorArray vertexNormals;

    std::unordered_map<uint64_t, bool> edgeSmoothing;

    int numUVSets = isMeshData ? srcMeshFn.numUVSets() : tgtMeshFn.numUVSets();
    MStringArray uvSetNames;
    std::vector<UVSetData> uvSets(numUVSets);

    if (isMeshData)
    {
        srcMeshFn.getUVSetNames(uvSetNames);
    } else {
        tgtMeshFn.getUVSetNames(uvSetNames);
    }
    
    for (int i = 0; i < numUVSets; i++)
    {
        uvSets[i].name = uvSetNames[i];
    }

    polyReorder::getPoints(targetMesh, pointOrder, points, true);
    polyReorder::getPolys(sourceMesh, pointOrder, polyCounts, polyConnects, isMeshData);
    polyReorder::getFaceVertexNormals(targetMesh, pointOrder, vertexNormals);
    polyReorder::getFaceVertexLocks(targetMesh, pointOrder, faceList, vertexList, lockedList);
    polyReorder::getEdgeSmoothing(targetMesh, edgeSmoothing);
    polyReorder::getUVs(isMeshData ? sourceMesh : targetMesh, uvSets);

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

    status = polyReorder::setUVs(outMesh, uvSets);
    RETURN_IF_ERROR(status);

    if (vertexNormals.length() == polyConnects.length())
    {
        status = polyReorder::setFaceVertexNormals(outMesh, polyCounts, polyConnects, vertexNormals);
        RETURN_IF_ERROR(status);

        status = polyReorder::setFaceVertexLocks(outMesh, faceList, vertexList, lockedList);
        RETURN_IF_ERROR(status);
    }

    status = polyReorder::setEdgeSmoothing(outMesh, pointOrder, edgeSmoothing);
    RETURN_IF_ERROR(status);

    return MStatus::kSuccess;
}