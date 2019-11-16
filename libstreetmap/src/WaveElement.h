#ifndef WAVEELEMENT_H
#define WAVEELEMENT_H
#define NO_EDGE -1

#include "IntersectionSearchNode.h"

class WaveElement {
public:
    WaveElement(IntersectionSearchNode* n, int id, double time, double estTime);

    int getSegmentID();
    IntersectionSearchNode* getSearchNode();
    double getTravelTime();
    double getEstimatedTime();
    
    friend bool operator<(const WaveElement& lhs, const WaveElement& rhs);
    friend bool operator>(const WaveElement& lhs, const WaveElement& rhs);
    
private:
    IntersectionSearchNode* node;
    int segmentID;
    double travelTime;
    double estimatedTime;
};

#endif /* WAVEELEMENT_H */

