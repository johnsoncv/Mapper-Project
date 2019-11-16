#include "IntersectionSearchNode.h"

IntersectionSearchNode::IntersectionSearchNode(unsigned id, std::vector<unsigned> segments) {
    intersectionID = id;
    intersectionSegments = segments;
}

void IntersectionSearchNode::setReachingSegment(int id) {
    reachingSegment = id;
}

std::vector<unsigned> IntersectionSearchNode::getSegments() {
    return intersectionSegments;
}

int IntersectionSearchNode::getReachingSegment() {
    return reachingSegment;
}

unsigned IntersectionSearchNode::getID() {
    return intersectionID;
}

double IntersectionSearchNode::getBestTime() {
    return bestTimeToNode;
}

void IntersectionSearchNode::setBestTime(double time) {
    bestTimeToNode = time;
}
