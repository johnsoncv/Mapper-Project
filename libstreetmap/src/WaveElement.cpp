#include "WaveElement.h"
WaveElement::WaveElement (IntersectionSearchNode* n, int id, double time, double estTime) {
    node = n;
    segmentID = id;
    travelTime = time;
    estimatedTime = estTime;
}

int WaveElement::getSegmentID() {
    return segmentID;
}

IntersectionSearchNode* WaveElement::getSearchNode() {
    return node;
}

double WaveElement::getTravelTime() {
    return travelTime;
}

double WaveElement::getEstimatedTime() {
    return estimatedTime;
}

//comparison between estimated times for aStar
bool operator<(const WaveElement& lhs, const WaveElement& rhs) {
    return lhs.estimatedTime < rhs.estimatedTime;
}

//comparison between estimated times for aStar
bool operator>(const WaveElement& lhs, const WaveElement& rhs) {
    return lhs.estimatedTime > rhs.estimatedTime;
}