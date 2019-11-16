#ifndef UTIL_H
#define UTIL_H

#include "m1.h"
#include "m3.h"
#include "m4.h"
#include "OSMDatabaseAPI.h"
#include "math.h"
#include "Store.h"
#include "LatLon.h"
#include "WaveElement.h"

#include <regex>
#include <curl/curl.h>
#include <boost/algorithm/string.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/foreach.hpp>
#include <queue>
#include <unordered_set>

#include <ezgl/application.hpp>
#include <ezgl/graphics.hpp>
#include <ezgl/rectangle.hpp>
#include <ezgl/color.hpp>

#include "InternalFeature.h"

#define KM_H_TO_M_S 3.6
#define DEFAULT_ZOOM_SCALE 5.0/3.0

// Coordinate conversion functions
double lonToX(double lon);
double latToY(double lat);
double xToLon(double x);
double yToLat(double y);

// Street drawing configuation look-up functions
ezgl::color getStreetColour(std::string streetType);
float getStreetWidth(std::string streetType);
int getStreetDrawLevel(std::string streetType);
float streetZoomScale(float slope);

// libcurl write buffer write callback
size_t curl_write(void *ptr, size_t size, size_t nmemb, void *stream);

// Misc. functions
LatLon averageLatLon(LatLon from, LatLon to);
double segmentRotationAngle(LatLon from, LatLon to);
std::pair<LatLon, int> findClosestBusStop(LatLon position);
std::string stringSplit(std::string text, std::string delimOne, std::string delimTwo);

// Location functions
LatLon getUserLatLon();

bool searchPath (const unsigned intersection_id_start, const unsigned intersection_id_end, const double right_turn_penalty, const double left_turn_penalty) ;
std::vector<unsigned> traceBack(const unsigned destID);
double heuristic(const unsigned node, const unsigned goalNode);

// intersection ids from partial intersection name
std::vector<unsigned> find_intersection_ids_from_partial_intersection_name(std::string intersection_prefix); 

std::unordered_map<unsigned, std::pair<double, std::vector<unsigned>>> find_paths_to_destinations (const unsigned intersect_id_start, 
        const std::unordered_set<unsigned>& destinations, 
        const double right_turn_penalty, 
        const double left_turn_penalty);
bool searchMultiPath (const unsigned intersection_id_start, const std::unordered_set<unsigned>& destinations, 
        std::vector<IntersectionSearchNode*>& searchNodes, const double right_turn_penalty, const double left_turn_penalty);
std::vector<unsigned> trackBackMulti(const unsigned destID, const std::vector<IntersectionSearchNode*>& searchNodes);

#endif /* UTIL_H */

