/**
    Copyright (c) 2017 Ryan Porter    
    You may use, distribute, or modify this code under the terms of the MIT license.
*/

#ifndef YANTOR3D_POLY_REORDER_H
#define YANTOR3D_POLY_REORDER_H

#include "meshData.h"

#include <cstdint>
#include <deque>
#include <vector>
#include <unordered_map>

#include <maya/MFloatArray.h>
#include <maya/MFloatPointArray.h>
#include <maya/MFloatVectorArray.h>
#include <maya/MIntArray.h>
#include <maya/MObject.h>
#include <maya/MPointArray.h>
#include <maya/MStatus.h>
#include <maya/MString.h>
#include <maya/MVectorArray.h>


namespace polyReorder
{
    struct ComponentSelection
    {
        int vertexIndex;
        int edgeIndex;
        int faceIndex;

        ComponentSelection() {}
    };

    struct UVSetData
    {
        MIntArray   uvCounts;
        MIntArray   uvIds;

        MFloatArray uArray;
        MFloatArray vArray;

        MString     name;
    };

    inline uint64_t twoIntKey(int id1, int id2) 
    { 
        int o1 = id1 < id2;
        int o2 = id2 < id1;

        uint32_t vtx0 = (id1 * o1) + (id2 * o2);
        uint32_t vtx1 = (id2 * o1) + (id1 * o2);

        return uint64_t(id1) | (uint64_t(id2) << 32); 
    };

    MStatus getPoints(MObject &mesh, MIntArray &pointOrder, MPointArray &outPoints, bool reorderPoints);
    MStatus getPolys(MObject &mesh, MIntArray &pointOrder, MIntArray &polyCounts, MIntArray &polyConnects, bool reorderPoints);
    MStatus getFaceVertexNormals(MObject &mesh, MIntArray &pointOrder, MVectorArray &vertexNormals);
    MStatus setFaceVertexNormals(MObject &mesh, MIntArray &polyCounts, MIntArray &polyConnects, MVectorArray &vertexNormals);
    MStatus getFaceVertexLocks(MObject &mesh, MIntArray &pointOrder, MIntArray &faceList, MIntArray &vertexList, MIntArray &lockedList);
    MStatus setFaceVertexLocks(MObject &mesh, MIntArray &faceList, MIntArray &vertexList, MIntArray &lockedList);

    MStatus getEdgeSmoothing(MObject &mesh, std::unordered_map<uint64_t, bool> &edgeSmoothing);
    MStatus setEdgeSmoothing(MObject &mesh, MIntArray &pointOrder, std::unordered_map<uint64_t, bool> &edgeSmoothing);

    MStatus getUVs(MObject &mesh, std::vector<UVSetData> &uvSets);
    MStatus setUVs(MObject &mesh, std::vector<UVSetData> &uvSets);
    
    MStatus reorderMesh(MObject &sourceMesh, MObject &targetMesh, MIntArray &pointOrder, MObject &outMesh, bool isMeshData=false);
}

#endif