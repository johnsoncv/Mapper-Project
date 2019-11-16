#include "Street.h"
#include "Store.h"
#include "StreetsDatabaseAPI.h"

Street::Street(unsigned id, std::string name) {
    streetID = id;
    streetName = name;
}

// Setters
void Street::addSegment(unsigned id) {
    streetSegments.push_back(id);
}

// Getters 
std::vector<unsigned> Street::getSegments() {
  return streetSegments;
}

std::string Street::getStreetName() {
    return streetName;
}

std::vector<unsigned> Street::getIntersections() {
  std::vector<unsigned> intersections;
  for (auto i = streetIntersections.begin(); i != streetIntersections.end(); i++) {
    intersections.push_back(i->first);  
  }

  return intersections;
}

// Traverses through the intersections maps for the corresponding streets
// If a common intersection exist, push into intersectionPoints
std::vector<unsigned> Street::intersects(const Street & street){
  std::vector<unsigned> intersectionPoints;

  for (auto i = street.streetIntersections.begin(); i != street.streetIntersections.end(); i++) {
    if (streetIntersections.find(i->first) != streetIntersections.end()) intersectionPoints.push_back(i->first);    
  }

  return intersectionPoints;
}

double Street::getStreetLength() {
  return streetLength;
}

// Utility functions
// For all the streetSegments for a Street, add the intersections it connects
void Street::generateIntersectionsList() {
  InfoStreetSegment info;
  for (auto i = streetSegments.begin(); i != streetSegments.end(); i++) {
      info = getInfoStreetSegment(*i);
        streetIntersections.insert(std::make_pair(info.to, info.to));
        streetIntersections.insert(std::make_pair(info.from, info.from));
  }
}

// Traverses all the segments, adds their lengths
void Street::calculateStreetLength() {
  for (auto i = streetSegments.begin(); i != streetSegments.end(); i++) {
      streetLength += store.SEGMENTS[*i]->getLength();
  }
}

// Sorts and find vector element where duplicates exist, and delete duplicates
void Street::clearSegmentDuplicates() {
  std::sort(streetSegments.begin(), streetSegments.end());
  streetSegments.erase(std::unique(streetSegments.begin(), streetSegments.end()), streetSegments.end());     
}
