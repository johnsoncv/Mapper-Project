/* Data structure for holding Street related information. Initially constructed
 * empty, and built using addIntersection, and addSegment. After constructed,
 * utility functions can be used to filter the segments, and generate the streetLength, 
 * and associated Intersections as properties
 */

#ifndef STREET_H
#define STREET_H

#include "StreetSegment.h"

#include <vector>
#include <map>
#include <algorithm>

class Street {
  private:
    unsigned streetID;
    std::string streetName;
    double streetLength = 0;
    std::vector<unsigned> streetSegments;
    std::map<unsigned, unsigned> streetIntersections;
    
  public:
    Street(unsigned streetID, std::string name);
    
    // Setters
    void addSegment(unsigned id);
   
    // Getters 
    double getStreetLength();
    std::string getStreetName();
    float getStreetWidth();
    std::vector<unsigned> getSegments();
    std::vector<unsigned> getIntersections();
    
    // Given another Street object, check whether the two streets intersects
    std::vector<unsigned> intersects(const Street & street);

    // Utility functions
    void generateIntersectionsList();
    void calculateStreetLength();
    void clearSegmentDuplicates();
};

#endif 
