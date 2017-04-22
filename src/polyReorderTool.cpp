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

#include <maya/MColorArray.h>
#include <maya/MDagPath.h>
#include <maya/MEvent.h>
#include <maya/MFnMesh.h>
#include <maya/MGlobal.h>
#include <maya/MItSelectionList.h>
#include <maya/MPlug.h>
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
                    setState(polyReorder::ToolState::SELECT_OR_COMPLETE); 
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
            setState(polyReorder::ToolState::SELECT_SOURCE_MESH);
        break;

        case polyReorder::ToolState::SELECT_COMPONENTS:
            this->clearSelectedMesh();            
            setState(polyReorder::ToolState::SELECT_DESTINATION_MESH);
        break;

        case polyReorder::ToolState::SELECT_OR_COMPLETE:
            this->popSelectedComponents();

            if (sourceComponents.empty())
            {
                setState(polyReorder::ToolState::SELECT_COMPONENTS);
            } else {
                setState(polyReorder::ToolState::SELECT_DESTINATION_MESH);
            }
            
        break;
    }
}


void PolyReorderTool::abortAction()
{
    MGlobal::executeCommand("escapeCurrentTool");
}


bool PolyReorderTool::readyToComplete()
{
    sourceMeshTopology.reset();
    destinationMeshTopology.reset();

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
    clearDisplayColors(sourceMesh, originalSourceDisplayColors);
    clearDisplayColors(destinationMesh, originalDestinationDisplayColors);

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

    MDagPath mesh;

    if (activeSelection.isEmpty())
    {
        return MStatus::kFailure;
    }

    MItSelectionList iterSelection(activeSelection);
    MDagPath selectedMesh;

    while (!iterSelection.isDone())
    {
        iterSelection.getDagPath(selectedMesh);

        if (selectedMesh.hasFn(MFn::kMesh))
        {
            mesh.set(selectedMesh);
            break;
        }
    }

    if (!mesh.isValid())
    {
        return MStatus::kFailure;
    }

    switch(state)
    {
        case polyReorder::ToolState::SELECT_SOURCE_MESH:           
            sourceMesh.set(mesh);
            
            sourceMeshData.unpackMesh(sourceMesh);
            sourceMeshTopology.setMesh(sourceMesh);
            getDisplayColors(sourceMesh, originalSourceDisplayColors);

            parseArgs::toTransform(sourceMesh);
        break;

        case polyReorder::ToolState::SELECT_DESTINATION_MESH:
            if (parseArgs::isSameTransform(mesh, sourceMesh))
            {
                MGlobal::displayError("Source and destination meshes cannot be the same.");
                return MStatus::kFailure;
            } 

            if (!MeshTopology::hasSameTopology(mesh, sourceMesh))
            {
                MGlobal::displayError("Source and destination meshes must have the same topology.");
                return MStatus::kFailure;
            }

            destinationMesh.set(mesh);

            destinationMeshData.unpackMesh(destinationMesh);
            destinationMeshTopology.setMesh(destinationMesh);
            getDisplayColors(destinationMesh, originalDestinationDisplayColors);

            parseArgs::toTransform(destinationMesh);
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
            clearDisplayColors(sourceMesh, originalSourceDisplayColors);
            sourceMesh.set(MDagPath());
            sourceMeshData.clear();
        break;

        case polyReorder::ToolState::SELECT_COMPONENTS:
            clearDisplayColors(destinationMesh, originalDestinationDisplayColors);
            destinationMesh.set(MDagPath());
            destinationMeshData.clear();
        break;
    }

    return MStatus::kSuccess;
}


MStatus PolyReorderTool::getDisplayColors(MDagPath &mesh, bool &value)
{
    MStatus status;

    parseArgs::extendToShape(mesh);

    MFnDagNode meshFn(mesh);
    MPlug displayColorsPlug = meshFn.findPlug("displayColors");
    value = displayColorsPlug.asBool();

    parseArgs::toTransform(mesh);

    return MStatus::kSuccess;
}


MStatus PolyReorderTool::updateDisplayColors(MDagPath &mesh, MeshTopology &topology)
{
    MStatus status;

    if (mesh.isValid())
    {
        parseArgs::extendToShape(mesh);

        MIntArray   vertexList(topology.numberOfVertices());
        MColorArray colors(topology.numberOfVertices());

        for (int i = 0; i < topology.numberOfVertices(); i++)
        {
            bool visited = topology.hasVisitedVertex(i);

            float r = visited ? 1.0f : 0.5f;
            float g = visited ? 1.0f : 0.5f;
            float b = 0.5f;

            vertexList[i] = i;
            colors[i] = MColor(r, g, b);
        }

        MFnMesh meshFn(mesh);
        MPlug displayColorsPlug = meshFn.findPlug("displayColors");
        displayColorsPlug.setBool(true);
        meshFn.setVertexColors(colors, vertexList);

        parseArgs::toTransform(mesh);
    }

    return MStatus::kSuccess;
}


MStatus PolyReorderTool::clearDisplayColors(MDagPath &mesh, bool &originalValue)
{
    MStatus status;

    parseArgs::extendToShape(mesh);

    MFnMesh meshFn(mesh);

    MPlug displayColorsPlug = meshFn.findPlug("displayColors");
    displayColorsPlug.setBool(originalValue);

    meshFn.clearColors();

    parseArgs::toTransform(mesh);

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

    return MStatus::kSuccess;
}


void PolyReorderTool::setState(polyReorder::ToolState newState)
{
    this->state = newState;

    updateHelpString();

    sourceMeshTopology.reset();
    destinationMeshTopology.reset();

    for (polyReorder::ComponentSelection &cs : sourceComponents)
    {
        sourceMeshTopology.walk(cs);
    }

    for (polyReorder::ComponentSelection &cs : destinationComponents)
    {
        destinationMeshTopology.walk(cs);
    }

    updateDisplayColors(sourceMesh, sourceMeshTopology);
    updateDisplayColors(destinationMesh, destinationMeshTopology);
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