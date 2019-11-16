#include "util.h"

double lonToX(double lon) {
    return lon*cos(store.LAT_AVG) * DEG_TO_RAD;
}

double latToY(double lat) {
    return lat * DEG_TO_RAD;
}

double xToLon(double x) {
    return x / cos(store.LAT_AVG) / DEG_TO_RAD;
}

double yToLat(double y) {
    return y / DEG_TO_RAD;
}

//Returns the angle by which the street segment is rotated in degrees
double segmentRotationAngle(LatLon from, LatLon to){
    double x1 = lonToX(from.lon());
    double y1 = latToY(from.lat());
    
    double x2 = lonToX(to.lon());
    double y2 = latToY(to.lat());
    
    return atan((y2-y1)/(x2-x1)) / DEG_TO_RAD;
}

//Returns the LatLon of the center of a straight line(segment) 
LatLon averageLatLon(LatLon from, LatLon to){
    double lat_avg = (from.lat() + to.lat()) / 2;
    double lon_avg = (from.lon() + to.lon()) / 2;
    
    LatLon avg(lat_avg, lon_avg);
    
    return avg;
}

// Returns substring found between delimiters delimOne and delimTwo
// provided they are unequal and delimTwo comes after delimOne
std::string stringSplit(std::string text, std::string delimOne, std::string delimTwo){
    auto firstPos = text.find(delimOne);
    auto secondPos = text.find(delimTwo);
    
    int charLength = secondPos - firstPos - 1;
    
    std::string result = text.substr(firstPos+1, charLength);
    
    return result;
}

ezgl::color getStreetColour(std::string streetType) {
    // Highways
    if (streetType == "motorway" || streetType == "motorway_link") return ezgl::HIGHWAY;
    if (streetType == "trunk" || streetType == "trunk_link" ) return ezgl::HIGHWAY;
    
    // Inter-city large streets
    if (streetType == "primary" || streetType == "primary_link") return ezgl::WHITE;
    if (streetType == "secondary" || streetType == "secondary_link") return ezgl::WHITE;
    
    if (streetType == "tertiary" || streetType == "tertiary_link") return ezgl::WHITE;
    if (streetType == "unclassified") return ezgl::WHITE;
    if (streetType == "residential") return ezgl::WHITE;
    if (streetType == "service") return ezgl::WHITE; 
    
    return ezgl::WHITE;
}

//Helps to determine appropriate width of a street based on the zoom level
float streetZoomScale(float slope) {
    if (store.zoomLevel >= 7) return slope * (store.zoomLevel % 7) + 10;
    else if (store.zoomLevel <= 2) return 1.5;
    else if (store.zoomLevel <= 4) return 2;
    else if (store.zoomLevel == 5) return 3;
    else if (store.zoomLevel == 6) return 5;
    
    return 1;
}

float getStreetWidth(std::string streetType) {
    // Highways
    if (streetType == "motorway" || streetType == "motorway_link" || streetType == "trunk" || streetType == "trunk_link") {
        return streetZoomScale(9);
    } 
    
    // Primary roads of the city
    if (streetType == "primary" || streetType == "primary_link" ||streetType == "secondary" || streetType == "secondary_link") {
        return streetZoomScale(7);
    }
    
    // Common small roadways between primary and secondary 
    if (streetType == "tertiary" || streetType == "tertiary_link" || streetType == "residential") {
        return streetZoomScale(5) / 2;
    }
    
    // Parking/entry roads to malls, etc (constant, since only visualized at zoom >= 8)
    if (streetType == "unclassified") return 4;
    if (streetType == "service") return 4; 
    
    return 4;
}

int getStreetDrawLevel(std::string streetType) {
    // Most significant roadways
    if (streetType == "motorway" || streetType == "motorway_link") return 1;
    if (streetType == "trunk" || streetType == "trunk_link" ) return 1;
    if (streetType == "primary" || streetType == "primary_link") return 1;
    if (streetType == "secondary" || streetType == "secondary_link") return 1;
    
    
    if (streetType == "tertiary" || streetType == "tertiary_link") return 5;
    if (streetType == "residential") return 5;
    
    if (streetType == "unclassified") return 8;
    if (streetType == "service") return 8;
    
    return 1;
}

size_t curl_write(void *ptr, size_t size, size_t nmemb, void *stream) {
    (void) stream;
    
    store.responseBuffer.append((char*)ptr, size * nmemb);
    return size * nmemb;
}

std::vector<unsigned> find_intersection_ids_from_partial_intersection_name(std::string intersection_prefix){
    std::vector<unsigned> inter_ids;

    if (intersection_prefix.length() == 0) return inter_ids;
    
    // Sanitize input to be lowercase
    std::transform(intersection_prefix.begin(), intersection_prefix.end(), intersection_prefix.begin(), ::tolower);
  
    // Split the string using spaces as delimiters
    std::vector<std::string> strs;
    boost::split(strs, intersection_prefix, boost::is_any_of(" "));
    
    // Prepares string to be used as the regular expression
    std::string reg_prefix;
    for(auto it = strs.begin(); it !=  strs.end(); it++){
        if(it == strs.begin()) reg_prefix += ".*(" + *it + ").*";
        else reg_prefix += "(" + *it + ").*";
    }
    
    std::regex base_regex(reg_prefix);
    
    // Finds the closest matches and appends the ids
    for (auto it = store.INTERSECTION_DICTIONARY.lower_bound(intersection_prefix); it != store.INTERSECTION_DICTIONARY.end(); it++) {
        if(std::regex_match(it->first, base_regex)) inter_ids.insert(inter_ids.end(), it->second.begin(), it->second.end());
    }
    
    return inter_ids;
}

std::pair<LatLon, int> findClosestBusStop(LatLon position) {
    std::pair<LatLon, int> stop;
    if (store.routes.find(store.focusedRoute) == store.routes.end()) return std::make_pair(LatLon(0, 0), -1);
    
    //get bus stop points to search through
    const std::vector<std::pair<LatLon, int>>& points = store.routes.find(store.focusedRoute)->second->getStopPoints();
    int currentDistance, shortestDistance;
    
    shortestDistance = find_distance_between_two_points(position, points[0].first);
    stop = points[0];
    
    //searching for closest bus stop to given position
    for (auto i = points.begin(); i != points.end(); i++) {
        currentDistance = find_distance_between_two_points(position, i->first);
        
        if (currentDistance < shortestDistance) {
            shortestDistance = currentDistance;
            stop = *i;
        }
    }
    
    return stop;
}

// Astar heursitic function, estimate the time to goal node using eucliudian distance/ topCitySpeed
double heuristic(const unsigned node, const unsigned goalNode) {
    LatLon nodePosition = getIntersectionPosition(node);
    LatLon goalPosition = getIntersectionPosition(goalNode);
    
    double distance = find_distance_between_two_points(nodePosition, goalPosition);
    
    return distance / (store.topSpeedLimit / KM_H_TO_M_S);
}

// Returns the LatLon of the location of the user by calling an API
LatLon getUserLatLon(){
    using namespace boost::property_tree;
    ptree propertyTree;
    
    float lat = 0, lon = 0;
    
    CURL *curlHandle = curl_easy_init();
    store.responseBuffer = "";
    
    std::string url = "https://api.ipdata.co/?api-key=570c726f0272bf61a8acce72ff76e37eb4e871045bdfa305c1bed191";
    
    curl_easy_setopt(curlHandle, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curlHandle, CURLOPT_WRITEFUNCTION, curl_write);  
    
    // Get response
    curl_easy_perform(curlHandle);

    // Parse into property trees
    std::istringstream inputStream(store.responseBuffer);
    read_json(inputStream, propertyTree);
    
    for(auto i=propertyTree.begin(); i!=propertyTree.end(); i++){
        if(i->first == "latitude") lat = i->second.get_value<float>();
        if(i->first == "longitude") lon = i->second.get_value<float>();
    }
    
    LatLon position(lat, lon);
    
    curl_easy_cleanup(curlHandle);
    
    return position;
}

// Wrapper function for finding path b/w intersections
std::unordered_map<unsigned, std::pair<double, std::vector<unsigned>>> find_paths_to_destinations (const unsigned intersect_id_start, 
        const std::unordered_set<unsigned>& destinations, 
        const double right_turn_penalty, 
        const double left_turn_penalty) {
    
    std::unordered_map<unsigned, std::pair<double, std::vector<unsigned>>> result;
    std::vector<IntersectionSearchNode*> searchNodes;
    searchNodes.resize(getNumIntersections());
    
    
    for (int intersection_id = 0; intersection_id < getNumIntersections(); intersection_id++) {
        searchNodes[intersection_id] = new IntersectionSearchNode(intersection_id, store.SEGMENTS_IDS[intersection_id]);
    }
    
    // If path exist, then traceBack, otherwise path is empty
    if (searchMultiPath(intersect_id_start, destinations, searchNodes, right_turn_penalty, left_turn_penalty)) { 
        for (auto i = destinations.begin(); i != destinations.end(); i++) {
            std::vector<unsigned> path = trackBackMulti(*i, searchNodes);
            double time = compute_path_travel_time(path, right_turn_penalty, left_turn_penalty);
            result.insert(std::make_pair(*i, std::make_pair(time, path))); 
        }
    }
    
    for (int intersection_id = 0; intersection_id < getNumIntersections(); intersection_id++) {
        delete searchNodes[intersection_id];
    }
    
    return result;
}

// Trace back the path found by searchPath, using the destID, also resets all the segments after finished
std::vector<unsigned> trackBackMulti(const unsigned destID, const std::vector<IntersectionSearchNode*>& searchNodes) {
    std::vector<unsigned> path;
    
    IntersectionSearchNode* current = searchNodes[destID];
    int segmentID = current->getReachingSegment();
    
    // Go through all the reachingSegments required to get to the destination 
    while (segmentID != NO_EDGE) {
        InfoStreetSegment info = getInfoStreetSegment(segmentID);
        int nextIntersection = (unsigned)info.to == current->getID() ? info.from : info.to;
        
        current = searchNodes[nextIntersection];
        
        path.push_back(segmentID);
        segmentID = current->getReachingSegment();  
        
    }  
    
    // Reverse to get source -> dest
    std::reverse(path.begin(), path.end());
    
    return path;
}

// A star path search with a straight-line estimatedTime to goal node heuristic
// Returns whether a path exists or not, and leaves a path to trace back if there exists one
bool searchMultiPath (const unsigned intersection_id_start, const std::unordered_set<unsigned>& destinations, 
        std::vector<IntersectionSearchNode*>& searchNodes, const double right_turn_penalty, const double left_turn_penalty) {
    // Wavefront queue, sorted by the aStar heuristic
    std::priority_queue<WaveElement, std::vector<WaveElement>, std::greater<WaveElement>> wavefront;
    
    // Add the source as the first element
    IntersectionSearchNode* source = searchNodes[intersection_id_start];
    wavefront.push(WaveElement(source, NO_EDGE, 0, 0));
    
    unsigned destinationsReached = 0;
    while (!wavefront.empty() && destinationsReached != destinations.size()) {
        // Pop off the top of the queue, most promising node
        WaveElement wave = wavefront.top();
        wavefront.pop();
        
        // Get its corresponding searchNode
        IntersectionSearchNode* currentNode = wave.getSearchNode();
        if ((wave.getTravelTime() < currentNode->getBestTime() || currentNode->getBestTime() == UNDEFINED)) {
            // Leave track for backTracing
            currentNode->setReachingSegment(wave.getSegmentID());
            // If time to get there is the faster than update it
            currentNode->setBestTime(wave.getTravelTime());
            
            // Found a destination
            if (destinations.find(currentNode->getID()) != destinations.end()) destinationsReached++;
            
            // Get the parent's outgoingSegments and children intersections
            const std::vector<unsigned>& outgoingSegments = currentNode->getSegments();
            const std::vector<unsigned>& adjacentIntersections = find_adjacent_intersections(currentNode->getID());
            
            // For each of the outgoingSegments match it with the adjacent intersection and create a new WaveElement
            for (auto edge = outgoingSegments.begin(); edge != outgoingSegments.end(); edge++) {
                InfoStreetSegment info = getInfoStreetSegment(*edge);
                double turn_penalty = 0;
                
                // Get the turn penalty associated with this new node to be explored
                if (wave.getSegmentID() != NO_EDGE) {
                    TurnType turn_type = find_turn_type(wave.getSegmentID(), *edge);
                
                    switch (turn_type) {
                        case TurnType::LEFT: turn_penalty = left_turn_penalty; break;
                        case TurnType::RIGHT: turn_penalty = right_turn_penalty; break;
                        default: turn_penalty = 0; break;
                    }
                }

                for (auto adj = adjacentIntersections.begin(); adj != adjacentIntersections.end(); adj++) {
                    IntersectionSearchNode* searchNode = searchNodes[*adj];    
                    
                    // Add the node to the wavefront, as long as it is legal
                    if ((*adj == (unsigned)info.to || (*adj == (unsigned)info.from && !info.oneWay))) {          
                        double nodeToNodeCost = currentNode->getBestTime() + store.SEGMENTS[*edge]->getTravelTime() + turn_penalty;

                        wavefront.push(WaveElement(searchNode, *edge, nodeToNodeCost, nodeToNodeCost));
                    }
                }
            }
        } 
    }
    
    if (destinations.size() > 0) return true;
    
    return false;
}