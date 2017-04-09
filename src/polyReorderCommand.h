/**
    Copyright (c) 2017 Ryan Porter    
    You may use, distribute, or modify this code under the terms of the MIT license.
*/

#ifndef POLY_REORDER_COMMAND_H
#define POLY_REORDER_COMMAND_H

#include "polyReorder.h"

#include <vector>

#include <maya/MArgDatabase.h>
#include <maya/MArgList.h>
#include <maya/MDagPath.h>
#include <maya/MFloatPointArray.h>
#include <maya/MIntArray.h>
#include <maya/MObject.h>
#include <maya/MPointArray.h>
#include <maya/MPxCommand.h>
#include <maya/MString.h>
#include <maya/MStatus.h>
#include <maya/MSyntax.h>
#include <maya/MVectorArray.h>

#define CONSTUCTION_HISTORY_FLAG            "-ch"
#define CONSTUCTION_HISTORY_LONG_FLAG       "-constructionHistory"

#define REPLACE_ORIGINAL_FLAG               "-rpo"
#define REPLACE_ORIGINAL_LONG_FLAG          "-replaceOriginal"

#define SOURCE_MESH_FLAG                    "-sm"
#define SOURCE_MESH_LONG_FLAG               "-sourceMesh"

#define DESTINATION_MESH_FLAG               "-dm"
#define DESTINATION_MESH_LONG_FLAG          "-destinationMesh"

#define SOURCE_COMPONENTS_FLAG              "-sc"
#define SOURCE_COMPONENTS_LONG_FLAG         "-sourceComponents"

#define DESTINATION_COMPONENTS_FLAG         "-dc"
#define DESTINATION_COMPONENTS_LONG_FLAG    "-destinationComponents"

class PolyReorderCommand : public MPxCommand
{
public:
                        PolyReorderCommand();
    virtual             ~PolyReorderCommand();

    static void*        creator();

    static MSyntax      getSyntax();
    virtual MStatus     parseArguments(MArgDatabase &argsData);
    virtual MStatus     parseComponentArguments(MArgDatabase &argsData, 
                                                const char* flag, MDagPath &mesh, 
                                                std::vector<polyReorder::ComponentSelection> &componentSelection);

    virtual MStatus     validateArguments();

    virtual MStatus     doIt(const MArgList& argList);
    virtual MStatus     redoIt();
    virtual MStatus     undoIt();

    virtual MStatus     saveOriginalMesh();
    virtual MStatus     restoreOriginalMesh();

    virtual MIntArray   getPointOrder(MStatus *status);
    virtual MStatus     createPolyReorderNode(MIntArray &pointOrder);
    virtual MStatus     createNewMesh();
    
    virtual MStatus     connectPolyReorderNode();
    virtual MStatus     disconnectPolyReorderNode();
    virtual MStatus     connectPolyReorderNodeToCreatedMesh();

    virtual bool        isUndoable() const { return true; }
    virtual bool        hasSyntax()  const { return true; }

public:
    static MString      COMMAND_NAME;

private:
    std::vector<polyReorder::ComponentSelection> sourceComponents;
    std::vector<polyReorder::ComponentSelection> destinationComponents;

    bool                    replaceOriginal     = true;
    bool                    constructionHistory = false;
        
    MDagPath                sourceMesh;
    MDagPath                destinationMesh;

    MObject                 undoOriginalMesh;
    MObject                 undoCreatedNode;
    MObject                 undoCreatedMesh;
};

#endif