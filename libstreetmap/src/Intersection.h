/* Intersection class holds information about its id, connecting street
 * segments (vector<std::string>), and the adjacent intersections(vector<unsigned>) 
 */

#ifndef INTERSECTION_H
#define INTERSECTION_H

#include <vector>
#include <string>

#include "StreetsDatabaseAPI.h"

class Intersection {
  private:
        unsigned intersectionID;
        std::string intersectionName;
        std::vector<std::string> segmentNames;
        std::vector<unsigned> adjacentIntersections;
        LatLon position;
        std::vector<unsigned> nearbyPOIs;
        
  public:
    Intersection(
        unsigned id, 
        std::string name,
        std::vector<std::string> connectedSegmentNames, 
        std::vector<unsigned> adjIntersections,
        LatLon coords
    );
    
    // Getters
    unsigned getID();
    std::vector<std::string> getSegmentNames();
    std::vector<unsigned> getAdjIntersections();
    std::string getName();
    LatLon getPosition();
    std::vector<unsigned> getNearbyPOIs();
    
    // Given an intersectionID, returns whether the two intersections are
    // connected
    // Note: an intersection intersects with itself
    bool directlyConnected(unsigned intersection_id);
};

#endif 
