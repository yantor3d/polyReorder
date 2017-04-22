/**
    Copyright (c) 2017 Ryan Porter    
    You may use, distribute, or modify this code under the terms of the MIT license.
*/

#include "parseArgs.h"

#include <maya/MArgDatabase.h>
#include <maya/MDagPath.h>
#include <maya/MFnDagNode.h>
#include <maya/MGlobal.h>
#include <maya/MItSelectionList.h>
#include <maya/MObject.h>
#include <maya/MSelectionList.h>
#include <maya/MFnSingleIndexedComponent.h>
#include <maya/MStatus.h>
#include <maya/MString.h>


MStatus parseArgs::getNodeArgument(MArgDatabase &argsData, const char* flag, MObject &node, bool required)
{
    MStatus status;

    if (argsData.isFlagSet(flag))
    {
        MSelectionList selection;
        MString objectName;

        argsData.getFlagArgument(flag, 0, objectName);        
        status = selection.add(objectName);
        
        if (!status)
        {
            MString errorMsg("Object '^1s'' does not exist.");
            errorMsg.format(errorMsg, objectName);
            MGlobal::displayError(errorMsg);
            return status;
        }

        selection.getDependNode(0, node);        
    } else if (required) {
        MString errorMsg("The ^1s flag is required.");
        errorMsg.format(errorMsg, MString(flag));
        MGlobal::displayError(errorMsg);
        return MStatus::kFailure;
    }

    return status; 
}


MStatus parseArgs::getDagPathArgument(MArgDatabase &argsData, const char* flag, MDagPath &path, bool required)
{
    MStatus status;

    MObject obj;
    status = getNodeArgument(argsData, flag, obj, required);

    if (status)
    {
        MDagPath::getAPathTo(obj, path);
    }

    return status; 
}


MStatus parseArgs::getComponentArgument(MSelectionList &selection, MFn::Type componentType, MDagPath &mesh, MObject &components)
{
    MStatus status;

    MItSelectionList iterSel(selection, componentType);

    MDagPath dag;
    MObject obj;

    MSelectionList filteredSelection;

    while (!iterSel.isDone())
    {
        iterSel.getDagPath(dag, obj);

        if (!dag.node().hasFn(MFn::kTransform)) { dag.pop(); }

        if (dag == mesh) 
        {
            filteredSelection.add(dag, obj, true);
        }

        iterSel.next();
    }

    if (filteredSelection.length() == 1)
    {
        filteredSelection.getDagPath(0, dag, components);
        status = MStatus::kSuccess;
    } else {
        status = MStatus::kFailure;
    }

    return status;
}


MStatus parseArgs::getBooleanArgument(MArgDatabase &argsData, const char* flag, bool &value, bool default_)
{
    MStatus status;

    bool flagIsSet = argsData.isFlagSet(flag, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    if (flagIsSet)
    {
        status = argsData.getFlagArgument(flag, 0, value);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    } else {
        value = default_;
    }
    
    return status;
}


int parseArgs::getComponentIndex(MObject &component)
{
    MFnSingleIndexedComponent sic(component);
    return sic.element(0);
}


bool parseArgs::isNodeType(MObject &node, MFn::Type nodeType)
{
    return !node.isNull() && node.hasFn(nodeType);
}


bool parseArgs::isNodeType(MDagPath &path, MFn::Type nodeType)
{
    return path.isValid() && path.hasFn(nodeType);
}


bool parseArgs::isSameTransform(MDagPath &a, MDagPath &b)
{
    MDagPath a_(a);
    MDagPath b_(b);

    parseArgs::toTransform(a_);
    parseArgs::toTransform(b_);

    return a_ == b_;
}


void parseArgs::extendToShape(MDagPath &dagPath)
{
    if (!dagPath.node().hasFn(MFn::kMesh))
    {
        uint numShapes;
        dagPath.numberOfShapesDirectlyBelow(numShapes);

        for (uint i = 0; i < numShapes; i++)
        {
            dagPath.extendToShapeDirectlyBelow(i);

            if (MFnDagNode(dagPath).isIntermediateObject())
            {
                dagPath.pop();
            } else {
                break;
            }
        }        
    }
}


void parseArgs::toTransform(MDagPath &dagPath)
{
    if (!dagPath.node().hasFn(MFn::kTransform))
    {
        dagPath.pop();
    }
}