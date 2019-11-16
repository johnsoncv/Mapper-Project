/* Global store object that acts as the main database for all the map data
 * Includes user entered data to be used by refresh callbacks, and set by handlers
 * To be initialized once 
 */

#ifndef STORE_H
#define STORE_H

#include <string>

#include "m1.h"
#include "StreetsDatabaseAPI.h"
#include "Intersection.h"
#include "IntersectionSearchNode.h"
#include "Street.h"
#include "StreetSegment.h"
#include "Route.h"
#include "Feature.h"
#include "InternalFeature.h"
#include "ezgl/graphics.hpp"

#include <unordered_map>
#include <set>

class Store {
public:
    void resetStore();
    
    // Load Map Data (constant for each map, caps it easy to relate value to a Store value)
    std::vector<Intersection*> INTERSECTIONS;
    std::vector<Street*> STREETS;
    std::vector<StreetSegment*> SEGMENTS;
    std::vector<IntersectionSearchNode*> intersectionSearchNodes;
    
    std::vector<std::vector<unsigned>> SEGMENTS_IDS;
    
    //Using multimaps since multiple entries may have the same key 
    // (constant for each map, caps it easy to relate value to a Store value)
    std::multimap<std::string, unsigned> STREETS_DICTIONARY;
    std::multimap<std::string, unsigned> POI_DICTIONARY;
    std::multimap<OSMID, unsigned> OSMID_SEGMENTID_MAP;
    std::multimap<FeatureType, InternalFeature*> FEATURES_TYPE_MAP;
    std::map<std::string, std::vector<unsigned>> INTERSECTION_DICTIONARY;
    
    // Cached PNG surfaces, unordered_map to acheive constant lookup
    std::unordered_map<std::string, ezgl::surface*> PNG_MAP;
    std::set<std::string> completionDictionary;
    
    // Map constants
    double LEFT_TURN_PENALTY = 13;
    double RIGHT_TURN_PENALTY = 7;
    double LAT_AVG;
    
    // Map state data
    bool reloadFlag = false;
    bool newMapLoadFlag = true;
    bool networkingFlag = false;
    bool loadSuccessFlag = true;
   
    // User data
    std::string prevMap;
    std::string mapName = "toronto_canada";
    std::vector<unsigned> highlightedIntersections;
    std::vector<unsigned> highlightedPOIs;
    std::string searchString = "";
    std::string destinationSearchString = "";
    int zoomLevel = 1;
    int focusedRoute = -1; 
    int focusedBusStop = -1;
    LatLon userLocation;
    
    // Clicked Intersections
    std::vector<unsigned> clicked;
    
    // Live data
    std::map<int, Route*> routes;
    
    // libcurl response buffer
    std::string responseBuffer = "";

    // direction/path related
    std::vector<unsigned> path;
    bool drawPath = false;
    double topSpeedLimit = 0;
    
    // help commands
    std::vector<std::string> commands;
};

extern Store store;

#endif /* STORE_H */

