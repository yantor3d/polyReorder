/**
    Copyright (c) 2017 Ryan Porter    
    You may use, distribute, or modify this code under the terms of the MIT license.
*/

#include "polyReorder.h"
#include "polyReorderNode.h"

#include <stdio.h>
#include <vector>

#include <maya/MDataBlock.h>
#include <maya/MDataHandle.h>
#include <maya/MFloatVector.h>
#include <maya/MFloatVectorArray.h>
#include <maya/MFnData.h>
#include <maya/MFnIntArrayData.h>
#include <maya/MFnMesh.h>
#include <maya/MFnMeshData.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MGlobal.h>
#include <maya/MIntArray.h>
#include <maya/MItMeshPolygon.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MPoint.h>
#include <maya/MPointArray.h>
#include <maya/MPxNode.h>
#include <maya/MString.h>
#include <maya/MStatus.h>
#include <maya/MTypeId.h>
#include <maya/MVector.h>
#include <maya/MVectorArray.h>


#define RETURN_IF_ERROR(s) if (!s) { return s; }


MObject PolyReorderNode::inMeshAttr;
MObject PolyReorderNode::pointOrderAttr;
MObject PolyReorderNode::outMeshAttr;


void* PolyReorderNode::creator()
{
    return new PolyReorderNode();
}


MStatus PolyReorderNode::initialize()
{
    MStatus status;

    MFnTypedAttribute T;
    
    inMeshAttr = T.create("inMesh", "im", MFnData::kMesh, MObject::kNullObj, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    pointOrderAttr = T.create("pointOrder", "po", MFnData::kIntArray, MObject::kNullObj, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    outMeshAttr = T.create("outMesh", "om", MFnData::kMesh, MObject::kNullObj, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    T.setStorable(false);

    CHECK_MSTATUS_AND_RETURN_IT(addAttribute(inMeshAttr));
    CHECK_MSTATUS_AND_RETURN_IT(addAttribute(pointOrderAttr));
    CHECK_MSTATUS_AND_RETURN_IT(addAttribute(outMeshAttr));

    CHECK_MSTATUS_AND_RETURN_IT(attributeAffects(inMeshAttr, outMeshAttr));
    CHECK_MSTATUS_AND_RETURN_IT(attributeAffects(pointOrderAttr, outMeshAttr));

    return status;
}


MStatus PolyReorderNode::compute(const MPlug &plug, MDataBlock &dataBlock)
{
    MStatus status;

    if (plug != outMeshAttr) { return MStatus::kUnknownParameter; }

    MDataHandle inMeshHandle = dataBlock.inputValue(inMeshAttr, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MDataHandle pointOrderHandle = dataBlock.inputValue(pointOrderAttr, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MDataHandle outMeshHandle = dataBlock.outputValue(outMeshAttr, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MObject inMesh = inMeshHandle.asMesh();
    MObject pointOrderData = pointOrderHandle.data();

    MIntArray pointOrder;

    if (!pointOrderData.isNull())
    {
        MFnIntArrayData pointOrderDataFn(pointOrderData, &status);               
        CHECK_MSTATUS_AND_RETURN_IT(status);
        pointOrder = pointOrderDataFn.array(&status);                      
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    MFnMeshData outMeshData;
    MObject outMesh = outMeshData.create(&status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    if (!inMesh.isNull())
    {
        status = polyReorder::reorderMesh(inMesh, inMesh, pointOrder, outMesh, true);
    }

    if (outMesh.isNull() || !status)
    {
        CHECK_MSTATUS(status);
        MGlobal::displayError("Mesh reorder failed.");
        outMesh = inMesh;
    }

    if (!outMesh.isNull())
    {
        status = outMeshHandle.setMObject(outMesh);    
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }
    
    outMeshHandle.setClean();

    return status;
}