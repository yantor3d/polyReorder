/**
    Copyright (c) 2017 Ryan Porter    
    You may use, distribute, or modify this code under the terms of the MIT license.
*/

#ifndef YANTOR3D_TOPOLOGY_PATH_H
#define YANTOR3D_TOPOLOGY_PATH_H

#include <queue>
#include <vector>

class TopologyPath
{
public:
                        TopologyPath();
                        TopologyPath(int &numberOfComponents);
    virtual             ~TopologyPath();
    
    bool                isComplete();

    bool                visit(int &index);
    bool                visited(int &index);
    int                 visitedAt(int &index);

    bool                empty();
    int                 next();
    void                push(int &index);

    int&                operator[] (int i) { return visitedIndices[i]; }

private:
    std::vector<int>    visitedIndices;
    std::vector<int>    indexVisitOrder;
    std::queue<int>     nextToVisit;

    int                 numVisited = 0;
};

#endif