/**
    Copyright (c) 2017 Ryan Porter    
    You may use, distribute, or modify this code under the terms of the MIT license.
*/

#include "meshData.h"
#include "parseArgs.h"
#include "meshTopology.h"
#include "polyReorderCommand.h"
#include "polyReorderTool.h"

#include <list>
#include <sstream>

#include <maya/MDagPath.h>
#include <maya/MEvent.h>
#include <maya/MFnMesh.h>
#include <maya/MGlobal.h>
#include <maya/MItSelectionList.h>
#include <maya/MPxContext.h>
#include <maya/MPxContextCommand.h>
#include <maya/MPxSelectionContext.h>
#include <maya/MSelectionList.h>
#include <maya/MString.h>
#include <maya/MStatus.h>


#define RETURN_IF_ERROR(s) if (!s) { return s; }


PolyReorderTool::PolyReorderTool() {}
PolyReorderTool::~PolyReorderTool() 
{
    sourceComponents.clear();
    destinationComponents.clear();
}

void PolyReorderTool::toolOnSetup(MEvent &event) 
{
    sourceMeshData = MeshData();
    destinationMeshData = MeshData();

    this->updateHelpString();
}


void PolyReorderTool::toolOffCleanup() 
{

}


MStatus PolyReorderTool::helpStateHasChanged(MEvent &event)
{
    this->updateHelpString();

    return MStatus::kSuccess;
}


void PolyReorderTool::updateHelpString() 
{
    switch (state)
    {
        case polyReorder::ToolState::SELECT_SOURCE_MESH:                    
            this->setHelpString("Select the source mesh whose point order will be matched.");
        break;

        case polyReorder::ToolState::SELECT_DESTINATION_MESH:
            this->setHelpString("Select the destination mesh whose point order will be modified.");
        break;

        case polyReorder::ToolState::SELECT_COMPONENTS:
            this->setHelpString("Select components on both meshes.");
        break;

        case polyReorder::ToolState::SELECT_OR_COMPLETE:
            this->setHelpString("Select components on both meshes, or press ENTER to complete the tool.");
        break;
    }
}


void PolyReorderTool::completeAction() 
{
    switch (state)
    {
        case polyReorder::ToolState::SELECT_SOURCE_MESH:                    
            if (this->getSelectedMesh())            
            { 
                setState(polyReorder::ToolState::SELECT_DESTINATION_MESH); 
            }
        break;

        case polyReorder::ToolState::SELECT_DESTINATION_MESH:
            if (this->getSelectedMesh())            
            { 
                setState(polyReorder::ToolState::SELECT_COMPONENTS); 
            }
        break;

        case polyReorder::ToolState::SELECT_COMPONENTS:
            if (this->getSelectedComponents())            
            { 
                MGlobal::clearSelectionList();
                setState(polyReorder::ToolState::SELECT_OR_COMPLETE); 
            }
        break;

        case polyReorder::ToolState::SELECT_OR_COMPLETE:
            MSelectionList activeSelection;
            MGlobal::getActiveSelectionList(activeSelection);

            if (activeSelection.isEmpty())
            {
                if (readyToComplete())
                {
                    this->doToolCommand();
                } else {
                    MGlobal::displayError("Incomplete selection - must select components on all shell.");
                }
            } else {
                if (this->getSelectedComponents())
                {
                    MGlobal::clearSelectionList();
                }
            }
        break;
    }
}


void PolyReorderTool::deleteAction() 
{
    switch (state)
    {
        case polyReorder::ToolState::SELECT_SOURCE_MESH:  
            MGlobal::displayWarning("Select the source mesh whose point order will be matched, or press ESC to exit the tool.");
        break;

        case polyReorder::ToolState::SELECT_DESTINATION_MESH:
            this->clearSelectedMesh();
        break;

        case polyReorder::ToolState::SELECT_COMPONENTS:
            this->clearSelectedMesh();
        break;

        case polyReorder::ToolState::SELECT_OR_COMPLETE:
            this->popSelectedComponents();
        break;
    }
}


void PolyReorderTool::abortAction()
{
    MGlobal::executeCommand("escapeCurrentTool");
}


bool PolyReorderTool::readyToComplete()
{
    bool result = false;

    MeshTopology sourceMeshTopology(sourceMesh);
    MeshTopology destinationMeshTopology(destinationMesh);

    for (polyReorder::ComponentSelection &cs : sourceComponents)
    {
        sourceMeshTopology.walk(cs);
    }

    for (polyReorder::ComponentSelection &cs : destinationComponents)
    {
        destinationMeshTopology.walk(cs);
    }
    
    return sourceMeshTopology.isComplete() && destinationMeshTopology.isComplete();
}


void PolyReorderTool::doToolCommand() 
{
    MGlobal::executeCommand("escapeCurrentTool");

    std::stringstream ss;

    ss << PolyReorderCommand::COMMAND_NAME << ' ';
    
    ss << SOURCE_MESH_FLAG << ' ' << sourceMesh.partialPathName() << ' ';

    for (polyReorder::ComponentSelection &s : sourceComponents)
    {
        ss << SOURCE_COMPONENTS_FLAG << ' ';

        ss << "e[" << s.edgeIndex << "] ";
        ss << "f[" << s.faceIndex << "] ";
        ss << "vtx[" << s.vertexIndex << "] ";
    }

    ss << DESTINATION_MESH_FLAG << ' ' << destinationMesh.partialPathName() << ' ';

    for (polyReorder::ComponentSelection &s : destinationComponents)
    {
        ss << DESTINATION_COMPONENTS_FLAG << ' ';

        ss << "e[" << s.edgeIndex << "] ";
        ss << "f[" << s.faceIndex << "] ";
        ss << "vtx[" << s.vertexIndex << "] ";
    }

    MGlobal::executeCommand(ss.str().c_str(), true, true);
}


MStatus PolyReorderTool::getSelectedMesh()
{
    MStatus status;

    MSelectionList activeSelection;
    MGlobal::getActiveSelectionList(activeSelection);

    if (!activeSelection.isEmpty())
    {
        MItSelectionList iterSelection(activeSelection);
        MDagPath object;

        while (!iterSelection.isDone())
        {
            iterSelection.getDagPath(object);

            if (object.hasFn(MFn::kMesh))
            {
                switch (state)
                {
                    case polyReorder::ToolState::SELECT_SOURCE_MESH:                    
                        this->sourceMesh.set(object);
                    break;
                    case polyReorder::ToolState::SELECT_DESTINATION_MESH:
                        this->destinationMesh.set(object);
                    break;
                }
                break;
            }
        }
    }

    switch(state)
    {
        case polyReorder::ToolState::SELECT_SOURCE_MESH:
            if (sourceMesh.isValid())
            {
                sourceMeshData.unpackMesh(sourceMesh);

                if (!sourceMesh.node().hasFn(MFn::kTransform)) { sourceMesh.pop(); }
            } else {
                status = MStatus::kFailure;
            }
        break;

        case polyReorder::ToolState::SELECT_DESTINATION_MESH:
            if (destinationMesh.isValid())
            {
                if (!destinationMesh.node().hasFn(MFn::kTransform)) { destinationMesh.pop(); }

                if (sourceMesh == destinationMesh)
                {
                    MGlobal::displayError("Source and destination meshes cannot be the same.");
                    destinationMesh.set(MDagPath());
                    status = MStatus::kFailure;
                } else {
                    MFnMesh sourceMeshFn(sourceMesh);
                    MFnMesh destinationMeshFn(destinationMesh);

                    if (
                        sourceMeshFn.numVertices() == destinationMeshFn.numVertices()
                        && sourceMeshFn.numEdges() ==  destinationMeshFn.numEdges()
                        && sourceMeshFn.numPolygons() == destinationMeshFn.numPolygons()
                    ) {
                        destinationMeshData.unpackMesh(destinationMesh);
                    } else {
                        MGlobal::displayError("Source and destination meshes must have the same topology.");
                        destinationMesh.set(MDagPath());
                        status = MStatus::kFailure;
                    }
                }
            } else {
                status = MStatus::kFailure;
            }
        break;
    }

    return status;
}


MStatus PolyReorderTool::clearSelectedMesh()
{
    MStatus status;

    switch(state)
    {
        case polyReorder::ToolState::SELECT_DESTINATION_MESH:
            sourceMesh.set(MDagPath());
            sourceMeshData.clear();
            setState(polyReorder::ToolState::SELECT_SOURCE_MESH);
        break;

        case polyReorder::ToolState::SELECT_COMPONENTS:
            destinationMesh.set(MDagPath());
            destinationMeshData.clear();
            setState(polyReorder::ToolState::SELECT_DESTINATION_MESH);
        break;
    }

    return MStatus::kSuccess;
}


MStatus PolyReorderTool::getSelectedComponents()
{
    MStatus status;

    MSelectionList activeSelection;
    MGlobal::getActiveSelectionList(activeSelection);

    if (activeSelection.isEmpty())
    {
        return MStatus::kFailure;
    }

    polyReorder::ComponentSelection src;
    polyReorder::ComponentSelection dst;

    status = getSelectedComponentsOnMesh(activeSelection, sourceMesh, sourceMeshData, src);
    RETURN_IF_ERROR(status);

    status = getSelectedComponentsOnMesh(activeSelection, destinationMesh, destinationMeshData, dst);
    RETURN_IF_ERROR(status);

    sourceComponents.push_back(src);
    destinationComponents.push_back(dst);

    return MStatus::kSuccess;
}


MStatus PolyReorderTool::getSelectedComponentsOnMesh(
    MSelectionList &activeSelection, 
    MDagPath &mesh,
    MeshData &meshData, 
    polyReorder::ComponentSelection &componentSelection
) {    
    MStatus status; 

    MObject edge;
    MObject face;
    MObject vertex;

    parseArgs::getComponentArgument(activeSelection, MFn::Type::kMeshEdgeComponent, mesh, edge);
    parseArgs::getComponentArgument(activeSelection, MFn::Type::kMeshPolygonComponent, mesh, face);
    parseArgs::getComponentArgument(activeSelection, MFn::Type::kMeshVertComponent, mesh, vertex);
 
    bool edgeSelected = !edge.isNull();
    bool faceSelected = !face.isNull();
    bool vertexSelected = !vertex.isNull();

    if (edgeSelected && faceSelected && vertexSelected)
    {
        polyReorder::ComponentSelection s;

        s.edgeIndex = parseArgs::getComponentIndex(edge);
        s.faceIndex = parseArgs::getComponentIndex(face);
        s.vertexIndex = parseArgs::getComponentIndex(vertex);

        bool edgeOnFace = contains(meshData.faceData[s.faceIndex].connectedEdges, s.edgeIndex);
        bool vertexOnEdge = contains(meshData.edgeData[s.edgeIndex].connectedVertices, s.vertexIndex);

        if (edgeOnFace && vertexOnEdge)
        {
            componentSelection = s;
        } else {
            MString errorMessage;

            if (!edgeOnFace && !vertexOnEdge)
            {
                errorMessage = "Invalid components selected on ^1s - selected edge must be on selected face, selected vertex must be on selected edge.";
            } else if (!edgeOnFace) {
                errorMessage = "Invalid components selected on ^1s - selected edge must be on selected face.";
            } else if (!vertexOnEdge) {
                errorMessage = "Invalid components selected on ^1s - selected vertex must be on selected edge.";
            }

            errorMessage.format(errorMessage, mesh.partialPathName());
            MGlobal::displayError(errorMessage);
            status = MStatus::kFailure;
        }
    } else {
        MString errorMessage("Invalid components selected on ^1s - must select a face, edge, and vertex.");
        errorMessage.format(errorMessage, mesh.partialPathName());
        MGlobal::displayError(errorMessage);
        status = MStatus::kFailure;
    }
    

    return status;
}


MStatus PolyReorderTool::popSelectedComponents()
{
    MStatus status;

    sourceComponents.pop_back();
    destinationComponents.pop_back();

    if (sourceComponents.empty())
    {
        setState(polyReorder::ToolState::SELECT_COMPONENTS);
    }

    return MStatus::kSuccess;
}


void PolyReorderTool::setState(polyReorder::ToolState newState)
{
    state = newState;
    updateHelpString();
}


PolyReorderContextCmd::PolyReorderContextCmd() {}
PolyReorderContextCmd::~PolyReorderContextCmd() {}


MPxContext* PolyReorderContextCmd::makeObj()
{
    return new PolyReorderTool();
}


void* PolyReorderContextCmd::creator()
{
    return new PolyReorderContextCmd();
}