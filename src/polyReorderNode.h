/**
    Copyright (c) 2017 Ryan Porter    
    You may use, distribute, or modify this code under the terms of the MIT license.
*/

#ifndef POLY_REORDER_NODE_H
#define POLY_REORDER_NODE_H

#include <maya/MDataBlock.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MPxNode.h>
#include <maya/MString.h>
#include <maya/MStatus.h>
#include <maya/MTypeId.h>

class PolyReorderNode : public MPxNode
{
public:
    static  void*       creator();
    static  MStatus     initialize();
    
    virtual MStatus     compute(const MPlug &plug, MDataBlock &dataBlock);

public:
    static MString      NODE_NAME;
    static MTypeId      NODE_ID;
    
    static MObject      inMeshAttr;
    static MObject      pointOrderAttr;
    static MObject      outMeshAttr;
};

#endif