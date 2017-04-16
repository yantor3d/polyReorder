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
        int o1 = id1 <= id2;
        int o2 = 1 - o1;

        uint32_t vtx0 = (id1 * o1) + (id2 * o2);
        uint32_t vtx1 = (id2 * o1) + (id1 * o2);

        return uint64_t(vtx0) | (uint64_t(vtx1) << 32); 
    };

    void    getFaceVertexList(MIntArray &polyCounts, MIntArray &polyConnects, MIntArray &faceList, MIntArray &vertexList);

    MStatus getPoints(MObject &mesh, MIntArray &pointOrder, MPointArray &outPoints);
    MStatus getPolys(MObject &mesh, MIntArray &pointOrder, MIntArray &polyCounts, MIntArray &polyConnects, bool reorderPoints);
    MStatus getFaceVertexNormals(MObject &mesh, MVectorArray &vertexNormals);
    MStatus setFaceVertexNormals(MObject &mesh, MIntArray &polyCounts, MIntArray &polyConnects, MVectorArray &vertexNormals);
    MStatus getFaceVertexLocks(MObject &mesh, MIntArray &lockedList);
    MStatus setFaceVertexLocks(MObject &mesh, MIntArray &lockedList);

    MStatus getEdgeSmoothing(MObject &mesh, MIntArray &pointOrder, std::unordered_map<uint64_t, bool> &edgeSmoothing);
    MStatus setEdgeSmoothing(MObject &mesh, std::unordered_map<uint64_t, bool> &edgeSmoothing);

    MStatus getUVs(MObject &mesh, std::vector<UVSetData> &uvSets);
    MStatus setUVs(MObject &mesh, std::vector<UVSetData> &uvSets);
    
    MStatus reorderMesh(MObject &sourceMesh, MObject &targetMesh, MIntArray &pointOrder, MObject &outMesh, bool isMeshData=false);
}

#endif