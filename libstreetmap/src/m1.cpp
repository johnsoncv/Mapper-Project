/* 
 * Copyright 2018 University of Toronto
 *
 * Permission is hereby granted, to use this software and associated 
 * documentation files (the "Software") in course work at the University 
 * of Toronto, or for personal use. Other uses are prohibited, in 
 * particular the distribution of the Software either publicly or to third 
 * parties.
 *
 * The above copyright notice and this permission notice shall be included in 
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "StreetsDatabaseAPI.h"
#include "OSMDatabaseAPI.h"

#include "Store.h"
#include "m1.h"
#include "util.h"
#include "Build.h"

#include <math.h>
#include <thread>
#include <set>

// Init global store
Store store;

// Flag for close_map to check if the map was loaded
bool loadedSuccessfully;

bool load_map(std::string map_name) {
   
    // Build map paths for osm
    std::string main_path = map_name.substr(0, map_name.find("."));
    std::string map_OSM_path = main_path + ".osm.bin";

    // Initialize Database API
    if(!loadStreetsDatabaseBIN(map_name) || !loadOSMDatabaseBIN(map_OSM_path)) return false; 
    
    store.STREETS.resize(getNumStreets());
    store.SEGMENTS.resize(getNumStreetSegments());
    store.INTERSECTIONS.resize(getNumIntersections());
    store.intersectionSearchNodes.resize(getNumIntersections());
    store.SEGMENTS_IDS.resize(getNumIntersections());
    
    store.userLocation = getUserLatLon();
    
    buildCommandList();
    
    // Build threads
    std::thread t1(buildIntersectionsVector);
    std::thread t2(buildFeatureMap);
    std::thread t3(buildPOIDictionary);
    std::thread t4(buildCompletionDictionary);
    
    buildStreetSegments();
    buildStreets();
    buildOSMWays();
    
    // Wait for threads to finish
    t1.join();
    t2.join();
    t3.join();
    t4.join();
    
    // Safety flag for close_map
    loadedSuccessfully = true;
    
    return true;
}

void close_map() {
    if(!loadedSuccessfully) return;  

    // Deallocate data structures
    for (int intersection_id = 0; intersection_id < getNumIntersections(); intersection_id++) {
        delete store.INTERSECTIONS[intersection_id];
        delete store.intersectionSearchNodes[intersection_id];
    }  
    
    for (int street_id = 0; street_id < getNumStreets(); street_id++) {
        delete store.STREETS[street_id];
    }
    
    for (int street_id = 0; street_id < getNumStreetSegments(); street_id++) {
        delete store.SEGMENTS[street_id];
    }
    
    for (auto it = store.FEATURES_TYPE_MAP.begin(); it != store.FEATURES_TYPE_MAP.end(); it++) {
        delete it->second;
    }
    
    for (auto it = store.routes.begin(); it != store.routes.end(); it++) {
        delete it->second;
    }
    
    for (auto it = store.PNG_MAP.begin(); it != store.PNG_MAP.end(); it++) {
        ezgl::renderer::free_surface(it->second);
    }
    
    store.PNG_MAP.clear();
    store.newMapLoadFlag = true;
    store.STREETS_DICTIONARY.clear();
    store.OSMID_SEGMENTID_MAP.clear();
    store.FEATURES_TYPE_MAP.clear();
    store.routes.clear();
    store.commands.clear();
    store.completionDictionary.clear();
    store.clicked.clear();
    store.SEGMENTS_IDS.clear();

    // Close Databases
    closeStreetDatabase();    
    closeOSMDatabase();
}

std::vector<unsigned> find_intersection_street_segments(unsigned intersection_id){
    return store.intersectionSearchNodes[intersection_id]->getSegments();
}

std::vector<std::string> find_intersection_street_names(unsigned intersection_id) {
  return store.INTERSECTIONS[intersection_id]->getSegmentNames();
}

bool are_directly_connected(unsigned intersection_id1, unsigned intersection_id2) {
  return store.INTERSECTIONS[intersection_id1]->directlyConnected(intersection_id2);
}

std::vector<unsigned> find_adjacent_intersections(unsigned intersection_id) {
  return store.INTERSECTIONS[intersection_id]->getAdjIntersections();
}

std::vector<unsigned> find_street_street_segments(unsigned street_id) {
  return store.STREETS[street_id]->getSegments();
}

std::vector<unsigned> find_all_street_intersections(unsigned street_id) {
  return store.STREETS[street_id]->getIntersections();
}

std::vector<unsigned> find_intersection_ids_from_street_ids(unsigned street_id1, 
                                                              unsigned street_id2) {
  return store.STREETS[street_id1]->intersects(*store.STREETS[street_id2]);
}

// Calculated distance between two points with Pythagoras’ theorem on an equirectangular projection (in meters)
double find_distance_between_two_points(LatLon point1, LatLon point2) {
  double total_distance = 0;
  
  double averageLatitude = (point1.lat() + point2.lat()) / 2 * DEG_TO_RAD;
  
  double x1 = point1.lon() * cos(averageLatitude) * DEG_TO_RAD;
  double y1 = point1.lat() * DEG_TO_RAD;
  
  double x2 = point2.lon() * cos(averageLatitude) * DEG_TO_RAD;
  double y2 = point2.lat() * DEG_TO_RAD;
  
  total_distance = EARTH_RADIUS_IN_METERS * sqrt(pow((y2-y1),2) + pow((x2-x1),2));

  return total_distance;
}

double find_street_segment_length(unsigned street_segment_id) {
  return store.SEGMENTS[street_segment_id]->getLength();
}

double find_street_length(unsigned street_id) {
  return store.STREETS[street_id]->getStreetLength();
}

double find_street_segment_travel_time(unsigned street_segment_id) {
  return store.SEGMENTS[street_segment_id]->getTravelTime();
}

unsigned find_closest_point_of_interest(LatLon my_position) {
  unsigned closest_point_interest_id = 0;
  
  double distance, shortest_distance;
  
  // Initialize shortest distance to first POI as reference
  shortest_distance = find_distance_between_two_points(my_position, getPointOfInterestPosition(0));
  
  for(int i = 1; i < getNumPointsOfInterest(); i++){
      LatLon poi_position = getPointOfInterestPosition(i); //each poi's position
      distance = find_distance_between_two_points(my_position, poi_position);    
    
    if(distance < shortest_distance){                     
        shortest_distance = distance;
        closest_point_interest_id = i;
    }
  }
  
  return closest_point_interest_id;
}
//Returns the nearest intersection to the given position
unsigned find_closest_intersection(LatLon my_position) {
  unsigned closest_intersection_id = 0;                             
          
  double distance, shortest_distance;    
  
  // Initialize shortest distance to first Intersection as reference
  shortest_distance = find_distance_between_two_points(my_position, getIntersectionPosition(0));
  
  for(int i = 1; i < getNumIntersections(); i++){
    LatLon intersection_position = getIntersectionPosition(i);        
    distance = find_distance_between_two_points(my_position, intersection_position);
    
        if(distance < shortest_distance) {                                 
            shortest_distance = distance;
            closest_intersection_id = i;
        }
    }
   
  return closest_intersection_id;
}

std::vector<unsigned> find_street_ids_from_partial_street_name(std::string street_prefix) {
    std::vector<unsigned> street_ids;
    if (street_prefix.length() == 0) return street_ids;
    
    // Sanitize input to be lowercase
    std::transform(street_prefix.begin(), street_prefix.end(), street_prefix.begin(), ::tolower);

    auto lowerbound = store.STREETS_DICTIONARY.lower_bound(street_prefix);

    for (auto it = lowerbound; it != store.STREETS_DICTIONARY.end(); it++) {
        if (it->first.find(street_prefix) == std::string::npos) break;
        street_ids.push_back(it->second);
    }

    return street_ids;
}
