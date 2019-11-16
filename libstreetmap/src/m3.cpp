#include <queue>
#include <set>

#include "m1.h"
#include "m3.h"
#include "WaveElement.h"
#include "util.h"
#include "Store.h"

// Caluculates the turn type between two segments using cross product, if the cross product is positive left turn, else right
TurnType find_turn_type(unsigned street_segment1, unsigned street_segment2) {
    StreetSegment* segment1 = store.SEGMENTS[street_segment1];
    StreetSegment* segment2 = store.SEGMENTS[street_segment2];
    
    IntersectionIndex commonIntersection = -1;
    std::set<IntersectionIndex> intersections;
    
    // If same segment, STRAIGHT
    if(segment1->getStreetID() == segment2->getStreetID()) return TurnType::STRAIGHT;
    
    InfoStreetSegment infoSegment1 = getInfoStreetSegment(street_segment1);
    InfoStreetSegment infoSegment2 = getInfoStreetSegment(street_segment2);
    
    // Find common intersection by adding to a set, and checking when the set is added with the same intersectionID
    intersections.insert(infoSegment1.to);
    intersections.insert(infoSegment1.from);
    if (intersections.insert(infoSegment2.to).second == false) commonIntersection = infoSegment2.to;
    if (intersections.insert(infoSegment2.from).second == false) commonIntersection = infoSegment2.from;
    
    if (commonIntersection != -1) intersections.erase(commonIntersection);
    else return TurnType::NONE; // no common intersection, therefore no turn possible
    
    LatLon commonPosition = getIntersectionPosition(commonIntersection);
    LatLon startPosition; 
    LatLon endPosition;
    
    // get segment curve points, ordered as from -> to
    std::vector<LatLon> segment1CurvePoints = segment1->getSegmentPoints();
    std::vector<LatLon> segment2CurvePoints = segment2->getSegmentPoints();
    
    // Finds the closest curvePoint to the commonIntersection between the two segments
    if (infoSegment1.curvePointCount == 0) startPosition = intersections.find(infoSegment1.to) != intersections.end() ? getIntersectionPosition(infoSegment1.to) : getIntersectionPosition(infoSegment1.from);
    else if (commonIntersection == infoSegment1.from) startPosition = segment1CurvePoints[1]; 
    else startPosition = segment1CurvePoints[segment1CurvePoints.size() - 2];
    if (infoSegment2.curvePointCount == 0) endPosition = intersections.find(infoSegment2.to) != intersections.end() ? getIntersectionPosition(infoSegment2.to) : getIntersectionPosition(infoSegment2.from);
    else if (commonIntersection == infoSegment2.from) endPosition = segment2CurvePoints[1];
    else endPosition = segment2CurvePoints[segment2CurvePoints.size() - 2];
    
    // Use the closest curvePoints and the commonIntersection to define a vector of fromClosestCurvePoint -> commonIntersection, and commonIntersection -> toClosestCurvePoint
    std::pair<double, double> startToCommon = std::make_pair(commonPosition.lon() - startPosition.lon(), commonPosition.lat() - startPosition.lat());
    std::pair<double, double> commonToEnd  = std::make_pair(endPosition.lon() - commonPosition.lon(), endPosition.lat() - commonPosition.lat()); 
    
    // Cross product with only z component as x, and y components are zeroed out
    double crossProduct = (startToCommon.first * commonToEnd.second) - (startToCommon.second * commonToEnd.first);
    
    // Positive crossProduct means a LEFT turn
    if (crossProduct > 0) return TurnType::LEFT;
    
    // else RIGHT turn
    return TurnType::RIGHT;
}

// Computes the path travel time given the path, and the corresponding turn penalties
double compute_path_travel_time(const std::vector<unsigned>& path, const double right_turn_penalty, const double left_turn_penalty) {
    
    double travelTime = 0;
    
    // If no path, travelTime is 0
    if (path.size() == 0) return DBL_MAX;
    
    // For each segment in the path, add up the associated turn penalty and segment's travelTime
    for (auto segment = path.begin(); segment != path.end() - 1; segment++) {
        double turn_penalty = 0;
        TurnType turn_type = find_turn_type(*segment, *(segment+1));
        
        switch (turn_type) {
            case TurnType::LEFT: turn_penalty = left_turn_penalty; break;
            case TurnType::RIGHT: turn_penalty = right_turn_penalty; break;
            default: turn_penalty = 0; break;
        }
        
        travelTime += store.SEGMENTS[*segment]->getTravelTime() + turn_penalty;
    }
    
    // Adds the last segment, w/o turn
    travelTime += store.SEGMENTS[path.back()]->getTravelTime();
    
    return travelTime;
}

// Wrapper function for finding path b/w intersections
std::vector<unsigned> find_path_between_intersections (const unsigned intersect_id_start, 
        const unsigned intersection_id_end, 
        const double right_turn_penalty, 
        const double left_turn_penalty) {
    
    std::vector<unsigned> path;
    
    // If path exist, then traceBack, otherwise path is empty
    if (searchPath(intersect_id_start, intersection_id_end, right_turn_penalty, left_turn_penalty)) path = traceBack(intersection_id_end);
    
        // Reset all bestTimes 
    for (auto i = store.intersectionSearchNodes.begin(); i != store.intersectionSearchNodes.end(); i++) {
            (*i)->setBestTime(UNDEFINED);
    }
        
    return path;
}

// A star path search with a straight-line estimatedTime to goal node heuristic
// Returns whether a path exists or not, and leaves a path to trace back if there exists one
bool searchPath (const unsigned intersection_id_start, const unsigned intersection_id_end, const double right_turn_penalty, const double left_turn_penalty) {
    // Wavefront queue, sorted by the aStar heuristic
    std::priority_queue<WaveElement, std::vector<WaveElement>, std::greater<WaveElement>> wavefront;
    
    double aStarCost;
    
    // Add the source as the first element
    IntersectionSearchNode* source = store.intersectionSearchNodes[intersection_id_start];
    wavefront.push(WaveElement(source, NO_EDGE, 0, 0));
    
    while (!wavefront.empty()) {
        // Pop off the top of the queue, most promising node
        WaveElement wave = wavefront.top();
        wavefront.pop();
        
        // Get its corresponding searchNode
        IntersectionSearchNode* currentNode = wave.getSearchNode();
        if ((wave.getTravelTime() < currentNode->getBestTime() || currentNode->getBestTime() == UNDEFINED)) {
            // Leave track for backTracing
            currentNode->setReachingSegment(wave.getSegmentID());
            // If time to get there is the faster than update it
            currentNode->setBestTime(wave.getTravelTime());
            
            // Found destination
            if (currentNode->getID() == intersection_id_end) return true;
            
            // Get the parent's outgoingSegments and children intersections
            const std::vector<unsigned>& outgoingSegments = currentNode->getSegments();
            const std::vector<unsigned>& adjacentIntersections = find_adjacent_intersections(currentNode->getID());
            
            // For each of the outgoingSegments match it with the adjacent intersection and create a new WaveElement
            for (auto edge = outgoingSegments.begin(); edge != outgoingSegments.end(); edge++) {
                InfoStreetSegment info = getInfoStreetSegment(*edge);
                double turn_penalty = 0;
                
                // Get the turn penalty associated with this new node to be explored
                if (wave.getSegmentID() != NO_EDGE) {
                    TurnType turn_type = find_turn_type(wave.getSegmentID(), *edge);
                
                    switch (turn_type) {
                        case TurnType::LEFT: turn_penalty = left_turn_penalty; break;
                        case TurnType::RIGHT: turn_penalty = right_turn_penalty; break;
                        default: turn_penalty = 0; break;
                    }
                }

                for (auto adj = adjacentIntersections.begin(); adj != adjacentIntersections.end(); adj++) {
                    IntersectionSearchNode* searchNode = store.intersectionSearchNodes[*adj];    
                    
                    // Add the node to the wavefront, as long as it is legal
                    if ((*adj == (unsigned)info.to || (*adj == (unsigned)info.from && !info.oneWay))) {          
                        double nodeToNodeCost = currentNode->getBestTime() + store.SEGMENTS[*edge]->getTravelTime() + turn_penalty;

                        // get heuristic value
                        aStarCost = heuristic(searchNode->getID(), intersection_id_end);

                        wavefront.push(WaveElement(searchNode, *edge, nodeToNodeCost, nodeToNodeCost + aStarCost));
                    }
                }
            }
        } 
    }
    return false;
}

// Trace back the path found by searchPath, using the destID, also resets all the segments after finished
std::vector<unsigned> traceBack(const unsigned destID) {
    std::vector<unsigned> path;
    
    IntersectionSearchNode* current = store.intersectionSearchNodes[destID];
    int segmentID = current->getReachingSegment();
    
    // Go through all the reachingSegments required to get to the destination 
    while (segmentID != NO_EDGE) {
        InfoStreetSegment info = getInfoStreetSegment(segmentID);
        int nextIntersection = (unsigned)info.to == current->getID() ? info.from : info.to;
        
        current = store.intersectionSearchNodes[nextIntersection];
        
        path.push_back(segmentID);
        segmentID = current->getReachingSegment();  
        
    }  
    
    // Reverse to get source -> dest
    std::reverse(path.begin(), path.end());
    
    return path;
}
