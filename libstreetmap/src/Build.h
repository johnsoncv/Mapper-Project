/* 
 * Contains all the build routines for the map
 * Every function will affect the data in the Store object
 * To be called at most once per map load
 */

#ifndef BUILD_H
#define BUILD_H

#include "util.h"

// M1 build functions, for use in load_map, and initialize data sets
void buildIntersectionsVector();
void buildStreets();
void buildStreetSegments(); 
void buildOSMWays();
void buildPOIDictionary();
void buildCompletionDictionary();

// Builds PNG database
// To be called only once, during the first initial canvas refresh
void buildPNG(ezgl::renderer &g);

// Build draw property objects
void buildFeatureMap();

// Network related build functions to get live bus data
void buildBusRoutes();
void buildBusStops(int routeTag);

void buildCommandList();

#endif /* BUILD_H */

