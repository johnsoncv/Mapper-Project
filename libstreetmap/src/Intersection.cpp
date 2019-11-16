#include "Intersection.h"
#include "m1.cpp"
// Constructor builds Intersection from id, vector of connected segment names,
// and adjacent intersections
Intersection::Intersection(
    unsigned id, std::string name,
    std::vector<std::string> connectedSegmentNames, 
    std::vector<unsigned> adjIntersections, LatLon coords) {
  
    intersectionID = id;
    intersectionName = name;
    segmentNames = connectedSegmentNames;
    adjacentIntersections = adjIntersections;
    position = coords;
}

// Getters
std::vector<std::string> Intersection::getSegmentNames() {
  return segmentNames;
}

std::vector<unsigned> Intersection::getAdjIntersections() {
  return adjacentIntersections;
}

unsigned Intersection::getID() {
  return intersectionID;
}

std::string Intersection::getName() {
    return intersectionName;
}

LatLon Intersection::getPosition() {
    return position;
}

//gets nearby Points of Interest in a radius of 50, stores them in nearbyPOIs
std::vector<unsigned> Intersection::getNearbyPOIs(){
    int distance;
    double radius = 50;
  
    for(int i = 0; i < getNumPointsOfInterest(); i++){
        LatLon poi_position = getPointOfInterestPosition(i); //each poi's position
        distance = find_distance_between_two_points(position, poi_position);    

        if(distance < radius){                     
            nearbyPOIs.push_back(i);
        }
    }
    return nearbyPOIs;
}

// Checks whether the same intersectionID is the same as itself
// If not, does a linear search through adjcentIntersections
// Worst Case: O(n) 
bool Intersection::directlyConnected(unsigned intersection_id) {
  if(this->intersectionID == intersection_id) return true; // Corner case, intersection intersects itself

  for (auto i = adjacentIntersections.begin(); i != adjacentIntersections.end(); i++) {
    if (*i == intersection_id) return true;
  }

  return false;
}
