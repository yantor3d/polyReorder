/**
    Copyright (c) 2017 Ryan Porter    
    You may use, distribute, or modify this code under the terms of the MIT license.
*/

#ifndef YANTOR3D_POLY_REORDER_TOOL_H
#define YANTOR3D_POLY_REORDER_TOOL_H

#include "meshData.h"
#include "meshTopology.h"
#include "polyReorder.h"

#include <list>

#include <maya/MColorArray.h>
#include <maya/MDagPath.h>
#include <maya/MEvent.h>
#include <maya/MPxContext.h>
#include <maya/MPxContextCommand.h>
#include <maya/MPxSelectionContext.h>
#include <maya/MString.h>
#include <maya/MStatus.h>

namespace polyReorder 
{
    enum ToolState
    {
        SELECT_SOURCE_MESH,
        SELECT_DESTINATION_MESH,
        SELECT_COMPONENTS,
        SELECT_OR_COMPLETE,
        COMPLETE
    };
}

class PolyReorderTool : public MPxSelectionContext
{
public:
                        PolyReorderTool();
    virtual             ~PolyReorderTool();

    virtual void        toolOnSetup(MEvent &event);
    virtual void        toolOffCleanup();

    virtual MStatus     helpStateHasChanged(MEvent &event);
    virtual void        updateHelpString();

    virtual void        abortAction();
    virtual void        completeAction();
    virtual void        deleteAction();

    virtual bool        readyToComplete();
    virtual void        doToolCommand();

    virtual void        setState(polyReorder::ToolState newState);

    virtual MStatus     getSelectedMesh();
    virtual MStatus     clearSelectedMesh();

    virtual MStatus     getDisplayColors(MDagPath &mesh, bool &value);
    virtual MStatus     updateDisplayColors(MDagPath &mesh, MeshTopology &topology);
    virtual MStatus     clearDisplayColors(MDagPath &mesh, bool &originalValue);

    virtual MStatus     getSelectedComponents();
    virtual MStatus     getSelectedComponentsOnMesh(
                            MSelectionList &activeSelection, 
                            MDagPath &mesh, 
                            MeshData &meshData, 
                            polyReorder::ComponentSelection &componentSelection
                        );

    virtual MStatus     popSelectedComponents();

private:
    MDagPath                        sourceMesh;
    MDagPath                        destinationMesh;

    MeshData                        sourceMeshData;
    MeshData                        destinationMeshData;

    MeshTopology                    sourceMeshTopology;
    MeshTopology                    destinationMeshTopology;

    polyReorder::ToolState          state = polyReorder::ToolState::SELECT_SOURCE_MESH;

    MColorArray                     originalSourceVertexColors;
    MColorArray                     originalDestinationVertexColors;

    bool                            originalSourceDisplayColors = false;
    bool                            originalDestinationDisplayColors = false;

    std::list<polyReorder::ComponentSelection>  sourceComponents;
    std::list<polyReorder::ComponentSelection>  destinationComponents;
};

class PolyReorderContextCmd : public MPxContextCommand
{
public:
    PolyReorderContextCmd();
    ~PolyReorderContextCmd();

    virtual MPxContext*     makeObj();
    static void*            creator();

public:
    static MString          COMMAND_NAME;
};

#endif