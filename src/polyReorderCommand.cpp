/**
    Copyright (c) 2017 Ryan Porter    
    You may use, distribute, or modify this code under the terms of the MIT license.
*/

#include "meshTopology.h"
#include "parseArgs.h"
#include "polyReorder.h"
#include "polyReorderCommand.h"
#include "polyReorderNode.h"

#include <maya/MArgDatabase.h>
#include <maya/MArgList.h>
#include <maya/MDagPath.h>
#include <maya/MDGModifier.h>
#include <maya/MDAGModifier.h>
#include <maya/MFn.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnIntArrayData.h>
#include <maya/MFnMesh.h>
#include <maya/MFnMeshData.h>
#include <maya/MFnSingleIndexedComponent.h>
#include <maya/MGlobal.h>
#include <maya/MItMeshEdge.h>
#include <maya/MItSelectionList.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MPlugArray.h>
#include <maya/MSelectionList.h>
#include <maya/MStatus.h>
#include <maya/MSyntax.h>


#define RETURN_IF_ERROR(s) if (!s) { return s; }


PolyReorderCommand::PolyReorderCommand() {}


PolyReorderCommand::~PolyReorderCommand() {}


void* PolyReorderCommand::creator()
{
    return new PolyReorderCommand();
}


MSyntax PolyReorderCommand::getSyntax()
{
    MSyntax syntax;

    syntax.addFlag(
        SOURCE_COMPONENTS_FLAG, 
        SOURCE_COMPONENTS_LONG_FLAG, 
        MSyntax::MArgType::kString,
        MSyntax::MArgType::kString,
        MSyntax::MArgType::kString
    );

    syntax.makeFlagMultiUse(SOURCE_COMPONENTS_FLAG);

    syntax.addFlag(
        DESTINATION_COMPONENTS_FLAG, 
        DESTINATION_COMPONENTS_LONG_FLAG, 
        MSyntax::MArgType::kString,
        MSyntax::MArgType::kString,
        MSyntax::MArgType::kString
    );

    syntax.makeFlagMultiUse(DESTINATION_COMPONENTS_FLAG);

    syntax.addFlag(SOURCE_MESH_FLAG, SOURCE_MESH_LONG_FLAG, MSyntax::kString);
    syntax.addFlag(DESTINATION_MESH_FLAG, DESTINATION_MESH_LONG_FLAG, MSyntax::kString);

    syntax.addFlag(REPLACE_ORIGINAL_FLAG, REPLACE_ORIGINAL_LONG_FLAG, MSyntax::kBoolean);
    syntax.addFlag(CONSTUCTION_HISTORY_FLAG, CONSTUCTION_HISTORY_LONG_FLAG, MSyntax::kBoolean);

    return syntax;
}


MStatus PolyReorderCommand::parseArguments(MArgDatabase &argsData)
{
    MStatus status;

    status = parseArgs::getDagPathArgument(argsData, SOURCE_MESH_FLAG, this->sourceMesh, true);
    RETURN_IF_ERROR(status);

    status = parseArgs::getDagPathArgument(argsData, DESTINATION_MESH_FLAG, this->destinationMesh, true);
    RETURN_IF_ERROR(status);

    status = parseComponentArguments(argsData, SOURCE_COMPONENTS_FLAG, this->sourceMesh, sourceComponents);
    RETURN_IF_ERROR(status);

    status = parseComponentArguments(argsData, DESTINATION_COMPONENTS_FLAG, this->destinationMesh, destinationComponents);
    RETURN_IF_ERROR(status);

    status = parseArgs::getBooleanArgument(argsData, CONSTUCTION_HISTORY_FLAG, this->constructionHistory, true);
    RETURN_IF_ERROR(status);

    status = parseArgs::getBooleanArgument(argsData, REPLACE_ORIGINAL_FLAG, this->replaceOriginal, true);
    RETURN_IF_ERROR(status);

    return status;
}


MStatus PolyReorderCommand::validateArguments()
{
    MStatus status;

    if (!parseArgs::isNodeType(this->sourceMesh, MFn::kMesh))
    {
        MString errorMessage("^1s/^2s expects a mesh.");
        errorMessage.format(errorMessage, MString(SOURCE_MESH_LONG_FLAG), MString(SOURCE_MESH_FLAG));

        this->displayError(errorMessage);
        return MStatus::kFailure;
    }

    if (!parseArgs::isNodeType(this->destinationMesh, MFn::kMesh))
    {
        MString errorMessage("^1s/^2s expects a mesh.");
        errorMessage.format(errorMessage, MString(DESTINATION_MESH_LONG_FLAG), MString(DESTINATION_MESH_FLAG));

        this->displayError(errorMessage);
        return MStatus::kFailure;
    }

    parseArgs::extendToShape(this->sourceMesh);
    parseArgs::extendToShape(this->destinationMesh);

    if (this->sourceMesh == this->destinationMesh)
    {
        MString errorMessage("Must specify difference meshes for ^1s and ^2s flags.");
        errorMessage.format(errorMessage, MString(SOURCE_MESH_LONG_FLAG), MString(DESTINATION_MESH_LONG_FLAG));

        this->displayError(errorMessage);
        return MStatus::kFailure;
    }

    MFnMesh sourceMeshFn(sourceMesh);
    MFnMesh destinationMeshFn(destinationMesh);

    bool vertCountMatch = sourceMeshFn.numVertices() == destinationMeshFn.numVertices();
    bool edgeCountMatch = sourceMeshFn.numEdges() == destinationMeshFn.numEdges();
    bool polyCountMatch = sourceMeshFn.numPolygons() == destinationMeshFn.numPolygons();

    bool topologyMatches = vertCountMatch && edgeCountMatch && polyCountMatch;

    if (!topologyMatches)
    {
        MString errorMessage("Must specify meshes with identical toplogy for ^1s and ^2s flags.");
        errorMessage.format(errorMessage, MString(SOURCE_MESH_LONG_FLAG), MString(DESTINATION_MESH_LONG_FLAG));
        this->displayError(errorMessage);

        return MStatus::kFailure;
    }

    int numSourceComponents = (int) sourceComponents.size();
    int numDestinationComponents = (int) destinationComponents.size();

    if (numSourceComponents == 0)
    {
        MString errorMessage("^1s/^2s flag(s) are required.");
        errorMessage.format(errorMessage, MString(SOURCE_COMPONENTS_LONG_FLAG), MString(SOURCE_COMPONENTS_FLAG));

        this->displayError(errorMessage);
        return MStatus::kFailure;
    }

    if (numDestinationComponents == 0)
    {
        MString errorMessage("^1s/^2s flag(s) are required.");
        errorMessage.format(errorMessage, MString(DESTINATION_COMPONENTS_LONG_FLAG), MString(DESTINATION_COMPONENTS_FLAG));

        this->displayError(errorMessage);
        return MStatus::kFailure;
    }

    if (numSourceComponents != numDestinationComponents)
    {
        MString errorMessage("Must pass the same number of ^1s/^2s and ^3s/^4s flags.");
        errorMessage.format(
            errorMessage,
            MString(SOURCE_COMPONENTS_LONG_FLAG), 
            MString(SOURCE_COMPONENTS_FLAG),
            MString(DESTINATION_COMPONENTS_LONG_FLAG),
            MString(DESTINATION_COMPONENTS_FLAG)
        );

        this->displayError(errorMessage);
        return MStatus::kFailure;
    }

    return status;
}


MStatus PolyReorderCommand::parseComponentArguments(
    MArgDatabase &argsData,
    const char* flag,
    MDagPath &mesh, 
    std::vector<polyReorder::ComponentSelection> &componentSelection
) {
    MStatus status;

    uint numComponents = argsData.numberOfFlagUses(flag);

    componentSelection.resize(numComponents);

    for (uint i = 0; i < numComponents; i++)
    {
        MStatus s;

        MArgList args;
        MSelectionList selection;

        status = argsData.getFlagArgumentList(flag, i, args);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        for (uint argIndex = 0; argIndex < args.length(); argIndex++)
        {
            MString obj = args.asString(argIndex, &status);
            CHECK_MSTATUS_AND_RETURN_IT(status);           

            if (obj.index('.') == -1)
            {
                obj = mesh.partialPathName() + '.' + obj;
            }

            status = selection.add(obj);

            if (!status) 
            { 
                MGlobal::displayError("No object matches name: " + obj);
                return status;
            }
        }

        MObject edge;
        MObject face;
        MObject vertex;

        status = parseArgs::getComponentArgument(selection, MFn::Type::kMeshEdgeComponent, mesh, edge);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        status = parseArgs::getComponentArgument(selection, MFn::Type::kMeshPolygonComponent, mesh, face);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        status = parseArgs::getComponentArgument(selection, MFn::Type::kMeshVertComponent, mesh, vertex);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        componentSelection[i].edgeIndex   = parseArgs::getComponentIndex(edge);
        componentSelection[i].faceIndex   = parseArgs::getComponentIndex(face);
        componentSelection[i].vertexIndex = parseArgs::getComponentIndex(vertex);
    }

    return MStatus::kSuccess;
}


MStatus PolyReorderCommand::doIt(const MArgList& argList)
{
    MStatus status;

    MArgDatabase argsData(syntax(), argList, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = this->parseArguments(argsData);        
    RETURN_IF_ERROR(status);

    status = this->validateArguments();
    RETURN_IF_ERROR(status);

    status = this->redoIt();

    return status;
}


MStatus PolyReorderCommand::redoIt()
{
    MStatus status;

    MIntArray pointOrder = getPointOrder(&status);
    RETURN_IF_ERROR(status);

    MFnDependencyNode destinationFn(destinationMesh.node());
    MPlug inMeshPlug = destinationFn.findPlug("inMesh");

    MPlugArray incomingConnection;
    inMeshPlug.connectedTo(incomingConnection, true, false);

    bool hasHistory = incomingConnection.length() != 0;    
    bool shouldCreateNode = constructionHistory || (replaceOriginal && hasHistory);
    bool shouldCreateMesh = !replaceOriginal;

    if (replaceOriginal)
    {
        if (!constructionHistory && hasHistory)
        {
            MGlobal::displayWarning("History will be on for the command since the selected item has history.");
            shouldCreateNode = true;
        } else if (constructionHistory && !hasHistory) {
            MGlobal::displayWarning("History will be off for the command since the selected item has no history.");
            shouldCreateNode = false;
        }
    }

    if (shouldCreateMesh) { createNewMesh(); }
    if (shouldCreateNode) { createPolyReorderNode(pointOrder); }

    MObject sourceMeshObj = sourceMesh.node();
    MObject destinationMeshObj = destinationMesh.node();

    if (shouldCreateNode && shouldCreateMesh)
    {        
        connectPolyReorderNodeToCreatedMesh();
    } else if (shouldCreateNode) { 
        connectPolyReorderNode();
    } else if (shouldCreateMesh) {
        polyReorder::reorderMesh(sourceMeshObj, destinationMeshObj, pointOrder, undoCreatedMesh);
    } else {    
        status = saveOriginalMesh();
        RETURN_IF_ERROR(status);

        polyReorder::reorderMesh(sourceMeshObj, destinationMeshObj, pointOrder, destinationMeshObj);
    }

    MDagPath oldMesh(destinationMesh);

    if (oldMesh.node().hasFn(MFn::kMesh))
    {
        oldMesh.pop();
    }

    if (undoCreatedMesh.isNull())
    {

        this->appendToResult(oldMesh.partialPathName());
    } else {
        MDagPath createdMesh;
        status = MDagPath::getAPathTo(undoCreatedMesh, createdMesh);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        if (createdMesh.node().hasFn(MFn::kMesh))
        {
            createdMesh.pop();
            undoCreatedMesh = createdMesh.node();
        }

        MDGModifier dgMod;
        status = dgMod.renameNode(undoCreatedMesh, oldMesh.partialPathName() + "Reorder");
        CHECK_MSTATUS_AND_RETURN_IT(status);
        status = dgMod.doIt();
        CHECK_MSTATUS_AND_RETURN_IT(status);

        this->appendToResult(createdMesh.partialPathName());
    }

    if (!undoCreatedNode.isNull())
    {
        this->appendToResult(MFnDependencyNode(undoCreatedNode).name());
    }

    CHECK_MSTATUS(status);

    return status;
}


MIntArray PolyReorderCommand::getPointOrder(MStatus *status)
{    
    MeshTopology sourceMeshTopology(this->sourceMesh);
    MeshTopology destinationMeshTopology(this->destinationMesh);

    for (polyReorder::ComponentSelection &cs : this->sourceComponents)
    {
        sourceMeshTopology.walk(cs);
    }

    for (polyReorder::ComponentSelection &cs : this->destinationComponents)
    {
        destinationMeshTopology.walk(cs);
    }

    MIntArray pointOrder(sourceMeshTopology.numberOfVertices(), -1);

    for (uint i = 0; i < pointOrder.length(); i++)
    {
        if (destinationMeshTopology[i] == -1 || sourceMeshTopology[i] == -1)
        {
            status = new MStatus(MStatus::kFailure);
            break;
        }

        pointOrder[destinationMeshTopology[i]] = sourceMeshTopology[i];
    }

    if (!status)
    {
        MGlobal::displayError(
            "Not all vertices were remapped. Make sure that your component selects were topologically identical."
        );
    }

    return pointOrder;
}


MStatus PolyReorderCommand::saveOriginalMesh()
{
    MStatus status;

    MFnMeshData undoOriginalMeshData;
    undoOriginalMesh = undoOriginalMeshData.create();

    MFnMesh destinationMeshFn(destinationMesh);
    MObject originalMesh = destinationMesh.node();
    destinationMeshFn.copy(originalMesh, undoOriginalMesh, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return MStatus::kSuccess;
}


MStatus PolyReorderCommand::restoreOriginalMesh()
{
    MStatus status;
    parseArgs::extendToShape(destinationMesh);

    MFnMesh destinationMeshFn(destinationMesh);
    status = destinationMeshFn.copyInPlace(undoOriginalMesh);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return status;
}


MStatus PolyReorderCommand::createPolyReorderNode(MIntArray &pointOrder)
{    
    MStatus status;

    MDGModifier mod;

    undoCreatedNode = mod.createNode(PolyReorderNode::NODE_NAME, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = mod.doIt();
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MFnDependencyNode createdNodeFn(undoCreatedNode, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MPlug pointOrderPlug = createdNodeFn.findPlug("pointOrder", false, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MFnIntArrayData pointOrderArrayData;
    MObject pointOrderData = pointOrderArrayData.create(pointOrder, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = pointOrderPlug.setMObject(pointOrderData);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return MStatus::kSuccess;
}


MStatus PolyReorderCommand::createNewMesh()
{    
    MStatus status;

    MDagModifier mod;
    MObject newTransform = mod.createNode("transform", MObject::kNullObj, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = mod.doIt();
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MDagPath result;
    status = MDagPath::getAPathTo(newTransform, result);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MFnMesh copyMesh;
    undoCreatedMesh = copyMesh.copy(destinationMesh.node(), newTransform, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MFnDagNode createdMeshFn(result);

    MGlobal::executeCommand("sets -e -forceElement initialShadingGroup " + result.partialPathName());

    return MStatus::kSuccess;
}


MStatus PolyReorderCommand::connectPolyReorderNode()
{
    MStatus status;

    MPlug mesh_inMeshPlug = MFnDagNode(destinationMesh).findPlug("inMesh", false, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MPlugArray mesh_inPlugConnections;
    mesh_inMeshPlug.connectedTo(mesh_inPlugConnections, true, false, &status);    
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MFnDependencyNode polyReorderNode(undoCreatedNode);
    
    MPlug node_inMeshPlug = polyReorderNode.findPlug("inMesh", false, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MPlug node_outMeshPlug = polyReorderNode.findPlug("outMesh", false, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MDGModifier dgMod;

    status = dgMod.disconnect(mesh_inPlugConnections[0], mesh_inMeshPlug);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = dgMod.doIt();
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = dgMod.connect(mesh_inPlugConnections[0], node_inMeshPlug);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = dgMod.doIt();
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = dgMod.connect(node_outMeshPlug, mesh_inMeshPlug);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = dgMod.doIt();
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return MStatus::kSuccess;
}


MStatus PolyReorderCommand::disconnectPolyReorderNode()
{
    MStatus status;

    MFnDependencyNode polyReorderNode(undoCreatedNode);

    MPlug node_inMeshPlug = polyReorderNode.findPlug("inMesh", false, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MPlug node_outMeshPlug = polyReorderNode.findPlug("outMesh", false, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MPlug mesh_inMeshPlug = MFnDagNode(destinationMesh).findPlug("inMesh", false, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MPlugArray node_inMeshPlugConnections;
    node_inMeshPlug.connectedTo(node_inMeshPlugConnections, true, false, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MPlugArray mesh_inMeshPlugConnections;
    mesh_inMeshPlug.connectedTo(mesh_inMeshPlugConnections, true, false, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MString connectAttrCmd("connectAttr -f ^1s ^2s");
    connectAttrCmd.format(
        connectAttrCmd, 
        node_inMeshPlugConnections[0].name(),
        mesh_inMeshPlug.name()
    );

    status = MGlobal::executeCommand(connectAttrCmd);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return MStatus::kSuccess;
}


MStatus PolyReorderCommand::connectPolyReorderNodeToCreatedMesh()
{
    MStatus status;

    MDagPath createdMesh;
    status = MDagPath::getAPathTo(undoCreatedMesh, createdMesh);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    parseArgs::extendToShape(createdMesh);

    MFnDependencyNode polyReorderNode(undoCreatedNode);
    
    MPlug oldMesh_outMeshPlug = MFnDagNode(destinationMesh).findPlug("outMesh");
    MPlug newMesh_inMeshPlug = MFnDagNode(createdMesh).findPlug("inMesh");

    MPlug node_inMeshPlug = polyReorderNode.findPlug("inMesh");
    MPlug node_outMeshPlug = polyReorderNode.findPlug("outMesh");

    MDGModifier dgMod;

    dgMod.connect(oldMesh_outMeshPlug, node_inMeshPlug);
    status = dgMod.doIt();
    CHECK_MSTATUS_AND_RETURN_IT(status);

    dgMod.connect(node_outMeshPlug, newMesh_inMeshPlug);
    status = dgMod.doIt();
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return MStatus::kSuccess;
}


MStatus PolyReorderCommand::undoIt()
{
    MStatus status;

    bool createdMesh = !undoCreatedMesh.isNull();
    bool createdNode = !undoCreatedNode.isNull();

    if (createdNode && createdMesh)
    {
        MString deleteCmd("delete ^1s ^2s");
        deleteCmd.format(
            deleteCmd,
            MFnDependencyNode(undoCreatedNode).name(),
            MFnDependencyNode(undoCreatedMesh).name()
        );

        status = MGlobal::executeCommand(deleteCmd);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    } else if (createdNode) { 
        status = disconnectPolyReorderNode();
        CHECK_MSTATUS_AND_RETURN_IT(status);

        MString deleteCmd("delete ^1s");
        deleteCmd.format(
            deleteCmd,
            MFnDependencyNode(undoCreatedNode).name()
        );

        status = MGlobal::executeCommand(deleteCmd);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    } else if (createdMesh) {
        MString deleteCmd("delete ^1s");
        deleteCmd.format(
            deleteCmd,
            MFnDependencyNode(undoCreatedMesh).name()
        );

        status = MGlobal::executeCommand(deleteCmd);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    } else {
        status = restoreOriginalMesh();
    }

    return status;
}