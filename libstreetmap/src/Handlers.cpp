#include "Handlers.h"

/* Search handler for the search bar, works mainly on finding the key delimiter of the command
 * Controls Store values, and refresh/restarts the canvas as necessary
 * @params ezgl::application pointer
 * @returns void
 */
void searchHandler(ezgl::application *application) {
    // Get input from search bar, and store it in store.SEARCH_STRING
    application->get_input(); 
    application->get_destination();
    
    // Sanitize to lower case to make it case insensitive
    std::transform(store.searchString.begin(), store.searchString.end(), store.searchString.begin(), ::tolower);
    std::transform(store.destinationSearchString.begin(), store.destinationSearchString.end(), store.destinationSearchString.begin(), ::tolower);
    
    if (store.destinationSearchString.length() != 0 && store.searchString.length() != 0) {
        searchPathHandler(application, " & ");
        return;
    }
    
    // route command
    if (store.searchString.find("route ") != std::string::npos) routeHandler("route ");
    // find intersection command
    //else if (store.SEARCH_STRING.find(" & ") != std::string::npos) intersectionHandler(" & ", application);
    // switch map command w/ city & country
    else if (store.searchString.find(", ") != std::string::npos) { 
        mapChangeCityHandler(", ");
        application->quit();
    // enable networking command
    } else if (store.searchString.find("/network") != std::string::npos){
        store.networkingFlag = true;
        store.mapName = "toronto_canada";
        store.reloadFlag = true;
        store.resetStore();
        application->quit();
    // refresh bus command
    } else if (store.searchString.find("/refresh") != std::string::npos) {
        busPredictionHandler(application);
    // get weather command
    } else if (store.searchString.find("/weather") != std::string::npos) {
        weatherHandler(application, "toronto");
    // get help command
    } else if (store.searchString.find("/help") != std::string::npos) {
        helpHandler(application);
    // search string is taken as a POI, if POI does not exist, taken that input was 
    // map country name
    } else if (store.searchString.find(" & ") != std::string::npos) {
        streetIntersectionHandler(" & ", application);
    }
    else {
        if(!poiHandler(application)) {
            if(!intersectionHandler(application)){
                mapChangeCountryHandler();
                application->quit();
            }
        }
    }
    // Refresh application only if the application hasn't quit already
    application->refresh_drawing();
  
}

void routeHandler(std::string delimiter) {
    // Get route number from search
    std::string routeNum = store.searchString.substr(store.searchString.find(delimiter) + delimiter.size(), store.searchString.length());
    
    // Call build bus stops only if not already built
    if (store.routes.find(std::stoi(routeNum)) != store.routes.end()) {
        // Set focused route only if exist
        store.focusedRoute = std::stoi(routeNum);
        
        // Build only if not already cached
        if (!store.routes.find(std::stoi(routeNum))->second->alreadyBuilt) buildBusStops(std::stoi(routeNum));
    }
}

void streetIntersectionHandler(std::string delimiter, ezgl::application *application) {
    // Get streets from search string
    std::string street1 = store.searchString.substr(0, store.searchString.find(delimiter));
    std::string street2 = store.searchString.substr(store.searchString.find(delimiter) + delimiter.size(), store.searchString.length());
        
    // Get streetIDs between intersections
    std::vector<unsigned> streetID1, streetID2, commonIntersections;
    streetID1 = find_street_ids_from_partial_street_name(street1);
    streetID2 = find_street_ids_from_partial_street_name(street2);
     
    // Clear current highlighted intersections
    store.highlightedIntersections.clear();
      
    for (auto it = streetID1.begin(); it != streetID1.end(); it++) {
        for (auto it2 = streetID2.begin(); it2 != streetID2.end(); it2++) {
            commonIntersections = find_intersection_ids_from_street_ids(*it, *it2);
            if (!commonIntersections.empty()) {
                store.highlightedIntersections.insert(store.highlightedIntersections.begin(), commonIntersections.begin(), commonIntersections.end());
            }
        }
    }
    
    if (store.highlightedIntersections.empty()) {
        errorHandler(application, "Intersection not found");
        return;
    }
    
    // Builds and create intersection cards
    for (auto it = store.highlightedIntersections.begin(); it != store.highlightedIntersections.end(); it++) {
        LatLon intersectionPos = getIntersectionPosition(*it);
        
        std::string coords = "<" + std::to_string(intersectionPos.lat()) + ", " + std::to_string(intersectionPos.lon()) + ">";
        std::vector<unsigned> nearby = store.INTERSECTIONS[*it]->getNearbyPOIs();
        application->createIntersectionCard(getIntersectionName(*it), coords, nearby);
    }
   
    // Zoom fit routine to visualize all the highlights
    std::string main_canvas_id = application->get_main_canvas_id();
    auto canvas = application->get_canvas(main_canvas_id);
    store.zoomLevel = 1;
    ezgl::zoom_fit(canvas, canvas->get_camera().get_initial_world());
    ezgl::zoom_in(canvas, DEFAULT_ZOOM_SCALE);
}

bool intersectionHandler(ezgl::application *application){
    std::string prefix = store.searchString;
    if(prefix.length() == 0) return true;

    std::vector<unsigned> ids = find_intersection_ids_from_partial_intersection_name(prefix);
    
    if(ids.size() == 0) return false;
    
    store.highlightedIntersections.clear();
    
    for (auto it = ids.begin(); it != ids.end(); it++) {
        
        store.highlightedIntersections.push_back(*it);
        LatLon intersectionPos = getIntersectionPosition(*it);
        
        std::string coords = "<" + std::to_string(intersectionPos.lat()) + ", " + std::to_string(intersectionPos.lon()) + ">";
        std::vector<unsigned> nearby = store.INTERSECTIONS[*it]->getNearbyPOIs();
        application->createIntersectionCard(getIntersectionName(*it), coords, nearby);
    }
    
    // Zoom fit routine to visualize all the highlights
    std::string main_canvas_id = application->get_main_canvas_id();
    auto canvas = application->get_canvas(main_canvas_id);
    store.zoomLevel = 1;
    ezgl::zoom_fit(canvas, canvas->get_camera().get_initial_world());
    ezgl::zoom_in(canvas, DEFAULT_ZOOM_SCALE);
    
    return true;
}

void mapChangeCountryHandler() {
    std::string mapName = ""; 
    std::vector<std::string> countryNameSplit;
    boost::split(countryNameSplit, store.searchString, boost::is_any_of(" "));
        
    // Build map path for load map
    for (unsigned wordIndex = 0; wordIndex < countryNameSplit.size() - 1; wordIndex++) {
        mapName.append(countryNameSplit[wordIndex]);
        mapName.append("-");
    }
    mapName.append(countryNameSplit[countryNameSplit.size() - 1]); 

    // Prepare store for restart
    store.prevMap = store.mapName;
    store.mapName = mapName;
    store.reloadFlag = true;
    store.resetStore();
}

/* Handlers for poi, return false if the poi is not found
 * Return is to be used for search handling
 */
bool poiHandler(ezgl::application *application){
    std::string poiName = store.searchString;
    store.highlightedPOIs.clear();
    
    if (poiName.length() == 0) { return true; }
    
    std::transform(poiName.begin(), poiName.end(), poiName.begin(), ::tolower);
    
    auto pointsOfInterests = store.POI_DICTIONARY.equal_range(poiName);
    if (pointsOfInterests.first == pointsOfInterests.second) {
        errorHandler(application, "Did not find POI");
        return false;
    }
    
    for (auto it = pointsOfInterests.first; it != pointsOfInterests.second; it++) {
        store.highlightedPOIs.push_back(it->second);
    }
    
    // Zoom fit routine to visualize all the highlights
    std::string main_canvas_id = application->get_main_canvas_id();
    auto canvas = application->get_canvas(main_canvas_id);
    store.zoomLevel = 1;
    ezgl::zoom_fit(canvas, canvas->get_camera().get_initial_world());
    ezgl::zoom_in(canvas, DEFAULT_ZOOM_SCALE);
    
    return true;
}

/* Click handler for the clicks on map, "clicks" on closest POI/intersection/bus stop to clicked position
 * Controls Store values, and refresh/restarts the canvas as necessary
 * @params ezgl::application pointer, location of the click
 * @returns void
 */
void clickHandler(ezgl::application *application, double &x, double &y) {
    // Get the coordinate position of the click
    LatLon clickedLocation(yToLat(y),xToLon(x));
  
    // Find the closest intersection, poi, and stop to that click
    unsigned intersectionID = find_closest_intersection(clickedLocation);
    unsigned poiID = find_closest_point_of_interest(clickedLocation);
    std::pair<LatLon, int> stop = findClosestBusStop(clickedLocation);
  
    // Get their position to compare their distances to the click
    LatLon intersectionPos = getIntersectionPosition(intersectionID);
    LatLon pointOfInterestPos = getPointOfInterestPosition(poiID);
    LatLon stopPos = stop.first;
  
    // Get their distance from the clicks
    double i = find_distance_between_two_points(clickedLocation, intersectionPos);
    double p = find_distance_between_two_points(clickedLocation, pointOfInterestPos);
    double b = find_distance_between_two_points(clickedLocation, stopPos);
    
    // Clear currently clicked points
    store.highlightedIntersections.clear();
    store.highlightedPOIs.clear();
    
    if(i < p && i < b){
        store.highlightedIntersections.push_back(intersectionID);
        std::string coords = "<" + std::to_string(intersectionPos.lat()) + ", " + std::to_string(intersectionPos.lon()) + ">";
        std::vector<unsigned> nearby = store.INTERSECTIONS[intersectionID]->getNearbyPOIs();
        application->createIntersectionCard(getIntersectionName(intersectionID), coords, nearby);
        application->refresh_drawing();
    } else  if (p < i && p < b) {
        store.highlightedPOIs.push_back(poiID);
        std::string coords = "<" + std::to_string(pointOfInterestPos.lat()) + ", " + std::to_string(pointOfInterestPos.lon()) + ">";
        application->createPOICard(getPointOfInterestName(poiID), getPointOfInterestType(poiID), coords);
        application->refresh_drawing();
    } else {
        store.focusedBusStop = stop.second;
        busPredictionHandler(application);
    }
}
   

void mapChangeCityHandler(std::string delimiter) {
    // Get city and country from search string
    std::string city = store.searchString.substr(0, store.searchString.find(delimiter));
    std::string country = store.searchString.substr(store.searchString.find(delimiter) + delimiter.size(), store.searchString.length());
    std::string mapName = ""; 
    std::vector<std::string> cityNameSplit;
    
    // Split city name if contains multiple words
    boost::split(cityNameSplit, city, boost::is_any_of(" "));
        
    // Builds the map path required for load map
    for (unsigned wordIndex = 0; wordIndex < cityNameSplit.size() - 1; wordIndex++) {
        mapName.append(cityNameSplit[wordIndex]);
        mapName.append("-");
    }
    
    mapName.append(cityNameSplit[cityNameSplit.size() - 1]); 
    mapName.append("_");
    mapName.append(country);
    
    // Prepare store for map reload
    store.prevMap = store.mapName;
    store.mapName = mapName;
    store.reloadFlag = true;
    store.resetStore();
}

void weatherHandler(ezgl::application *application, std::string cityName) {
    using namespace boost::property_tree;
    ptree propertyTree;
    application->resetCards();
    std::string temperature, weather, lastupdate, visibility;

    CURL *curlHandle = curl_easy_init();
    store.responseBuffer = "";
    
    // Construct headers
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Accept: application/xml");
    headers = curl_slist_append(headers, "charset: utf-8");

    std::string url = "http://api.openweathermap.org/data/2.5/weather/?q=" + cityName + "&mode=xml&units=metric&appid=5fd5ca6dd1c3bf49577dc13781a96b43";
    
    curl_easy_setopt(curlHandle, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curlHandle, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curlHandle, CURLOPT_WRITEFUNCTION, curl_write);  
    
    curl_easy_perform(curlHandle);
    curl_slist_free_all(headers);
    
    std::istringstream inputStream(store.responseBuffer);
    read_xml(inputStream, propertyTree);
    
    // Loop through each children of "current" parent
    BOOST_FOREACH(ptree::value_type const &value, propertyTree.get_child("current")) {
        if (value.first == "temperature") {
            temperature = value.second.get("<xmlattr>.value", "Error: Did not get temp");
        } else if (value.first == "weather") {
            weather = value.second.get("<xmlattr>.value", "Error: Did not get weather");
        } else if (value.first == "lastupdate") {
            lastupdate = value.second.get("<xmlattr>.value", "Error: Did not get last update stamp");
        } else if (value.first == "visibility") {
            int rawVisibility = std::stoi(value.second.get("<xmlattr>.value", "Error: Did not get visibility"));
            visibility = std::to_string(rawVisibility / 1000);
        }
    }
  
    curl_easy_cleanup(curlHandle);
    
    // Create weather card
    application->createWeatherCard(cityName, weather, temperature, visibility);
}

void busPredictionHandler(ezgl::application *application) {
    using namespace boost::property_tree;
    ptree propertyTree;
    
    std::string text = "";
    application->resetCards();

    CURL *curlHandle = curl_easy_init();
    store.responseBuffer = "";

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Accept: application/xml");
    headers = curl_slist_append(headers, "charset: utf-8");

    std::string url = "http://webservices.nextbus.com/service/publicXMLFeed?command=predictions&a=ttc&stopId=" + std::to_string(store.focusedBusStop) + "&terse";

    curl_easy_setopt(curlHandle, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curlHandle, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curlHandle, CURLOPT_WRITEFUNCTION, curl_write);  

    curl_easy_perform(curlHandle);
    curl_slist_free_all(headers);

    std::istringstream inputStream(store.responseBuffer);
    read_xml(inputStream, propertyTree);

    BOOST_FOREACH(ptree::value_type const &direction, propertyTree.get_child("body").get_child("predictions")) {
        if (direction.first == "direction") {
            std::string routeName = direction.second.get("<xmlattr>.title", "<unknown>");
            BOOST_FOREACH(ptree::value_type const &prediction, direction.second) {
                if (prediction.first == "prediction") {
                    std::string minutes = prediction.second.get("<xmlattr>.minutes", "-1");
                    std::string branch = prediction.second.get("<xmlattr>.branch", "-1");
                    application->createPredictionCard(minutes, routeName, branch);
                }
            }
        }
    }
   
    application->refresh_drawing();
}

/* Path handler for creating driving instructions concerning the path
 * Path is assumed to be legal
 * @params ezgl::application pointer, vector of segment ids which form the path
 * @returns void
 */
void pathHandler(ezgl::application *application, std::vector<unsigned> &path){
    
    // No path found
    if(path.size() == 0) { 
        application->createErrorCard("Path not found.");
        return;
    }
    
    // Declaring and initializing variables to be used later on
    int distance = store.SEGMENTS[path[0]]->getLength();
    int travelTime = 0;
    std::string distanceString;
    std::string travelTimeString;
    std::string detail;
    std::vector<std::string> pre_details;
    std::vector<std::string> details;
    
    // Providing starting instruction 
    std::string name = store.STREETS[store.SEGMENTS[path[0]]->getStreetID()]->getStreetName();
    if(name == "<unknown>") name = "unnamed road";
    detail = "Continue onto " + name + " (" + std::to_string(distance) + ")";
    pre_details.push_back(detail);
    
    // Handling the case when path consists of only one segment
    if(path.size() == 1){
        travelTime = store.SEGMENTS[path[0]]->getTravelTime();
        if(distance < 1000) distanceString = std::to_string(distance) + "m";
        else distanceString = std::to_string(distance/1000) + "km";
        pre_details.push_back("Arrive at destination.");
        application->createPathCard(distanceString, std::to_string(travelTime), pre_details);
        return;
    }
    
    // Looping over the path to add the rest
    for(auto it = path.begin(); it != path.end()-1; it++){
        int segmentLength = store.SEGMENTS[*(it+1)]->getLength();
        
        std::string direction;
        TurnType turn_type = find_turn_type(*it, *(it+1));

        switch (turn_type) {
            case TurnType::STRAIGHT: direction = "Continue onto "; break;
            case TurnType::LEFT: direction = "Turn left onto "; break;
            case TurnType::RIGHT: direction = "Turn right onto "; break;
            default: direction = "Continue onto "; break;
        }
        
        if(turn_type==TurnType::STRAIGHT){
            std::string last_detail = pre_details.back();
            std::string subString = last_detail.substr(0, last_detail.find("("));
            std::string len = stringSplit(last_detail, "(", ")");
            int last_length = std::stoi(len);
            pre_details[pre_details.size()-1] = subString + "(" + std::to_string(last_length+segmentLength) + ")";
        } else {
            std::string streetName = store.STREETS[store.SEGMENTS[*(it+1)]->getStreetID()]->getStreetName();
            if (streetName == "<unknown>") streetName = "unnamed road";
            detail = direction + streetName + '\n' + "Continue onto " + streetName + " (" + std::to_string(distance) + ")";
            pre_details.push_back(detail);
        }
        
        // Calculating total distance to be travelled
        distance += segmentLength;
    }
    
    // Second loop for conversion to appropriate units of distance
    for(auto str = pre_details.begin(); str != pre_details.end(); str++){
        std::string subString = str->substr(0, str->find("("));
        std::string len = stringSplit(*str, "(", ")");
        int length = std::stoi(len);
        if(length < 1000) details.push_back(subString + "(" + len + "m)");
        else details.push_back(subString + "(" + std::to_string(length/1000) + "km)");
    }
    
    // Final instruction
    details.push_back("Arrive at destination.");
    
    // Calculating total travel time
    travelTime = compute_path_travel_time(path, store.RIGHT_TURN_PENALTY, store.LEFT_TURN_PENALTY);
    
    // Conversions to appropriate units of distance and time
    if(distance < 1000) distanceString = std::to_string(distance) + "m";
    else distanceString = std::to_string(distance/1000) + "km";
    
    if(travelTime < 60) travelTimeString = std::to_string(travelTime) + "s";
    else if(travelTime < 3600) travelTimeString = std::to_string(travelTime/60) + "min";
    else travelTimeString = std::to_string(travelTime/3600) + "h";
    
    // Create path card
    application->createPathCard(distanceString, travelTimeString, details);
}

/* Handler for printing the help menu
 * @params ezgl::application pointer
 * @returns void
 */
void helpHandler(ezgl::application *application){
    application->createHelpCard();
}

/* Handler for printing various errors
 * @params ezgl::application pointer
 * @returns void
 */
void errorHandler(ezgl::application *application, std::string message){
    application->createErrorCard(message);
}

void searchPathHandler(ezgl::application *application, std::string delimiter){
    
    if (store.searchString.find(" & ") == std::string::npos || store.destinationSearchString.find(" & ") == std::string::npos) {
        errorHandler(application, "Invalid street names");
        return;
    }
    
    // Get streets from search string
    std::string sourceStreet1 = store.searchString.substr(0, store.searchString.find(delimiter));
    std::string sourceStreet2 = store.searchString.substr(store.searchString.find(delimiter) + delimiter.size(), store.searchString.length());
    
    std::string destStreet1 = store.destinationSearchString.substr(0, store.destinationSearchString.find(delimiter));
    std::string destStreet2 = store.destinationSearchString.substr(store.destinationSearchString.find(delimiter) + delimiter.size(), store.destinationSearchString.length());
        
    // Get streetIDs between intersections
    std::vector<unsigned> sourceStreetID1, sourceStreetID2, destStreetID1, destStreetID2, commonIntersectionsSource, commonIntersectionsDest;
    sourceStreetID1 = find_street_ids_from_partial_street_name(sourceStreet1);
    sourceStreetID2 = find_street_ids_from_partial_street_name(sourceStreet2);
    
    destStreetID1 = find_street_ids_from_partial_street_name(destStreet1);
    destStreetID2 = find_street_ids_from_partial_street_name(destStreet2);
    
    unsigned startID, endID;
    bool foundCommon =  false;
    
    if (sourceStreetID1.size() == 0 || sourceStreetID2.size() == 0 || destStreetID1.size() == 0 || destStreetID2.size() == 0) {
        errorHandler(application, "Streets do not exist");
        return;
    }
     
    // Clear current highlighted intersections
    store.highlightedIntersections.clear();
    
    for (auto it = sourceStreetID1.begin(); it != sourceStreetID1.end(); it++) {
        for (auto it2 = sourceStreetID2.begin(); it2 != sourceStreetID2.end(); it2++) {
            commonIntersectionsSource = find_intersection_ids_from_street_ids(*it, *it2);
            if (!commonIntersectionsSource.empty()) {
                startID = commonIntersectionsSource.front();
                foundCommon = true;
                break;
            }
        }
    }
    
    if (foundCommon) {
        foundCommon = false;
        for (auto it = destStreetID1.begin(); it != destStreetID1.end(); it++) {
            for (auto it2 = destStreetID2.begin(); it2 != destStreetID2.end(); it2++) {
                commonIntersectionsDest = find_intersection_ids_from_street_ids(*it, *it2);
                if (!commonIntersectionsDest.empty()) {
                    endID = commonIntersectionsDest.front();
                    foundCommon = true;
                    break;
                }
            }
        }
    }
    

    if (foundCommon) { 
        store.path = find_path_between_intersections(startID, endID, store.RIGHT_TURN_PENALTY, store.LEFT_TURN_PENALTY);
        store.drawPath = true;
        pathHandler(application, store.path);
        application->refresh_drawing();
    } else {
        errorHandler(application, "Streets do not intersect, can not find path");
    }
}