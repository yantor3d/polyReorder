/**
    Copyright (c) 2017 Ryan Porter    
    You may use, distribute, or modify this code under the terms of the MIT license.
*/

#ifndef PARSE_ARGS_H
#define PARSE_ARGS_H

#include <maya/MArgDatabase.h>
#include <maya/MDagPath.h>
#include <maya/MFn.h>
#include <maya/MObject.h>
#include <maya/MSelectionList.h>
#include <maya/MStatus.h>

namespace parseArgs
{
    MStatus getComponentArgument(MSelectionList &selection, MFn::Type componentType, MDagPath &mesh, MObject &components);
    int getComponentIndex(MObject &component);

    MStatus getNodeArgument(MArgDatabase &argsData, const char* flag, MObject &node, bool required);
    MStatus getDagPathArgument(MArgDatabase &argsData, const char* flag, MDagPath &path, bool required);

    MStatus getBooleanArgument(MArgDatabase &argsData, const char* flag, bool &value, bool default_=true);
   
    bool isNodeType(MObject &node, MFn::Type nodeType);
    bool isNodeType(MDagPath &path, MFn::Type nodeType);

    void extendToShape(MDagPath &dagPath);
}

#endif