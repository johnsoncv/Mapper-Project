#include "Route.h"

Route::Route(int tag, std::string name) {
    routeTag = tag;
    routeName = name;
}

// Getters
std::vector<std::pair<LatLon, int>> Route::getStopPoints() {
    return stopPoints;
}
int Route::getRouteTag() {
    return routeTag;
}
std::string Route::getRouteName() {
    return routeName;
}

// Setters
void Route::setRouteStops(std::vector<std::pair<LatLon, int>> stops) {
    stopPoints = stops;
}

