/**
    Copyright (c) 2017 Ryan Porter    
    You may use, distribute, or modify this code under the terms of the MIT license.
*/

#include "topologyPath.h"

#include <queue>
#include <vector>


TopologyPath::TopologyPath() {}


TopologyPath::TopologyPath(int &numberOfComponents)
{
    this->resize(numberOfComponents);
}


TopologyPath::~TopologyPath() {}


void TopologyPath::resize(int &numberOfComponents)
{
    numVisited = 0;
    
    indexVisitOrder.clear();
    componentShellId.clear();
    visitedIndices.clear();

    indexVisitOrder.resize(numberOfComponents, -1);
    componentShellId.resize(numberOfComponents, 0);
    visitedIndices.resize(numberOfComponents, -1);
}


bool TopologyPath::isComplete()
{
    return numVisited > 0 && numVisited == visitedIndices.size();
}


bool TopologyPath::visit(int &index, int &shellId)
{
    bool result = false;

    if (!visited(index)) 
    { 
        visitedIndices[numVisited] = index;
        indexVisitOrder[index] = numVisited++;
        result =  true;

        componentShellId[index] = shellId;
    }    

    return result;
}


bool TopologyPath::visited(int &index)
{
    return indexVisitOrder[index] != -1;
}


int TopologyPath::visitedAt(int &index)
{
    return indexVisitOrder[index];
}


int TopologyPath::shellId(int &index)
{
    return componentShellId[index];
}


void TopologyPath::push(int &index)
{
    nextToVisit.push(index);
}


bool TopologyPath::empty()
{
    return nextToVisit.empty();
}


int TopologyPath::next()
{
    int result = -1;

    if (!nextToVisit.empty())
    {
        result = nextToVisit.front();
        nextToVisit.pop();
    }

    return result;
}