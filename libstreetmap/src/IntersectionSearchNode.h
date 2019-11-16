#ifndef INTERSECTIONSEARCHNODE_H
#define INTERSECTIONSEARCHNODE_H

#include <vector>

#define UNDEFINED -1
#define NO_EDGE -1

class IntersectionSearchNode {
public:
    IntersectionSearchNode(unsigned id, std::vector<unsigned> segments);
    
    std::vector<unsigned> getSegments();
    unsigned getID();
    int getReachingSegment();
    double getBestTime();
    void setReachingSegment(int id);
    void setBestTime(double time);
private:
    unsigned intersectionID;
    std::vector<unsigned> intersectionSegments;
    int reachingSegment = NO_EDGE;
    double bestTimeToNode = UNDEFINED;
};

#endif /* INTERSECTIONSEARCHNODE_H */

