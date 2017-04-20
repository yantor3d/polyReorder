/**
    Copyright (c) 2017 Ryan Porter    
    You may use, distribute, or modify this code under the terms of the MIT license.
*/

#include "polyReorderCommand.h"
#include "polyReorderNode.h"
#include "polyReorderTool.h"

#include <maya/MFnPlugin.h>
#include <maya/MGlobal.h>
#include <maya/MTypeId.h>
#include <maya/MString.h>
#include <maya/MStatus.h>


const char* kAUTHOR = "Ryan Porter";
const char* kVERSION = "0.3.0";
const char* kREQUIRED_API_VERSION = "Any";


MString PolyReorderNode::NODE_NAME = "polyReorder";
MTypeId PolyReorderNode::NODE_ID = 0x00126b0e;

MString PolyReorderCommand::COMMAND_NAME = "polyReorder";

MString PolyReorderContextCmd::COMMAND_NAME = "polyReorderCtx";


bool menuCreated = false;


MStatus initializePlugin(MObject obj)
{
    MStatus status;
    MFnPlugin fnPlugin(obj, kAUTHOR, kVERSION, kREQUIRED_API_VERSION);

    status = fnPlugin.registerContextCommand(
        PolyReorderContextCmd::COMMAND_NAME,
        PolyReorderContextCmd::creator
    );

    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = fnPlugin.registerCommand(
        PolyReorderCommand::COMMAND_NAME, 
        PolyReorderCommand::creator, 
        PolyReorderCommand::getSyntax
    );

    status = fnPlugin.registerNode(
	    PolyReorderNode::NODE_NAME,
        PolyReorderNode::NODE_ID,
        PolyReorderNode::creator,
        PolyReorderNode::initialize,
		MPxNode::kDependNode
    );

    CHECK_MSTATUS_AND_RETURN_IT(status);

    if (MGlobal::mayaState() == MGlobal::kInteractive)
    {
        status = MGlobal::executePythonCommand("import polyReorder");

        if (status) 
        {
            MGlobal::executePythonCommand("polyReorder._initializePlugin()");
            menuCreated = true;
        } else {
            MGlobal::displayWarning("polyReorder module has not been installed - cannot create a tools menu.");
        }
    }

    return MS::kSuccess;
}


MStatus uninitializePlugin(MObject obj)
{
    MStatus status;
    MFnPlugin fnPlugin(obj, kAUTHOR, kVERSION, kREQUIRED_API_VERSION);

    status = fnPlugin.deregisterContextCommand(PolyReorderContextCmd::COMMAND_NAME);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = fnPlugin.deregisterCommand(PolyReorderCommand::COMMAND_NAME);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = fnPlugin.deregisterNode(PolyReorderNode::NODE_ID);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    if (MGlobal::mayaState() == MGlobal::kInteractive && menuCreated)
    {
        status = MGlobal::executePythonCommand("import polyReorder");

        if (status) 
        {
            MGlobal::executePythonCommand("polyReorder._uninitializePlugin()");
        }
    }


    return MS::kSuccess;
}