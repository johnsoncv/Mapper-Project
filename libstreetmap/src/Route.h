#ifndef ROUTE_H
#define ROUTE_H

#include <string>
#include <vector>
#include "StreetsDatabaseAPI.h"

class Route {
public:
    Route(int tag, std::string name);
    
    // Getters
    std::vector<std::pair<LatLon, int>> getStopPoints();
    int getRouteTag();
    std::string getRouteName();
    
    // Setters
    void setRouteStops(std::vector<std::pair<LatLon, int>> stops);
    
    // Public bool for check if already cached bus stops
    bool alreadyBuilt = false;
    
private:
    int routeTag;
    std::string routeName;
    std::vector<std::pair<LatLon, int>> stopPoints;
};

#endif /* ROUTE_H */

