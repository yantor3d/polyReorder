/**
    Copyright (c) 2017 Ryan Porter    
    You may use, distribute, or modify this code under the terms of the MIT license.
*/

#ifndef YANTOR3D_POLY_REORDER_H
#define YANTOR3D_POLY_REORDER_H

#include "meshData.h"

#include <vector>
#include <deque>

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

        bool isValid() { return vertexIndex >= 0 && edgeIndex >= 0 && faceIndex >= 0; }
    };

    struct UVSetData
    {
        MIntArray   uvCounts;
        MIntArray   uvIds;

        MFloatArray uArray;
        MFloatArray vArray;

        MString     name;
    };

    MStatus getPoints(MObject &mesh, MIntArray &pointOrder, MPointArray &outPoints, bool reorderPoints);
    MStatus getPolys(MObject &mesh, MIntArray &pointOrder, MIntArray &polyCounts, MIntArray &polyConnects, bool reorderPoints);
    MStatus getVertexNormals(MObject &mesh, MIntArray &pointOrder, MVectorArray &vertexNormals);                       
    MStatus getUVs(MObject &mesh, std::vector<UVSetData> &uvSets);
    MStatus setUVs(MObject &mesh, std::vector<UVSetData> &uvSets);
    
    MStatus reorderMesh(MObject &sourceMesh, MObject &targetMesh, MIntArray &pointOrder, MObject &outMesh, bool isMeshData=false);
}

#endif