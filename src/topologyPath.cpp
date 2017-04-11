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
    indexVisitOrder.resize(numberOfComponents, -1);
    visitedIndices.resize(numberOfComponents, -1);
}


TopologyPath::~TopologyPath() {}


bool TopologyPath::visit(int &index)
{
    bool result = false;

    if (!visited(index)) 
    { 
        visitedIndices[numVisited] = index;
        indexVisitOrder[index] = numVisited++;
        result =  true;
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