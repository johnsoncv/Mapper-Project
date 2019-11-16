#include "Build.h"

/* Builds OSMWays for StreetSegments
 * @params void
 * @returns void
 */
void buildOSMWays() {
    for (unsigned i = 0; i < getNumberOfWays(); i++) {
        const OSMWay* way = getWayByIndex(i);
        
        // Get ID range with the same OSMID, as multiple segments have the same OSMID
        auto segmentIDRange = store.OSMID_SEGMENTID_MAP.equal_range(way->id());
    
        // For each segment get its draw properties, depending on the type
        for (auto segmentID = segmentIDRange.first; segmentID != segmentIDRange.second; segmentID++) {
            
            for(unsigned tagIndex = 0; tagIndex < getTagCount(way); tagIndex++) {
		std::string key, type;
		std::tie(key,type) = getTagPair(way, tagIndex);
               
                // Only use the "highway" property
		if (key == "highway") {
                    store.SEGMENTS[segmentID->second]->setDrawLevel(getStreetDrawLevel(type));
                    store.SEGMENTS[segmentID->second]->setSegmentColour(getStreetColour(type));
                    store.SEGMENTS[segmentID->second]->setSegmentType(type);
                    break;
                }
            }  
        }
    }
}

/* Initial m1 build functions, to init map load */
void buildIntersectionsVector() {
    for (int intersection_id = 0; intersection_id < getNumIntersections(); intersection_id++) {
      int segmentCount = getIntersectionStreetSegmentCount(intersection_id);
      
      std::vector<std::string> streetNames;
      std::vector<unsigned> segmentIDs;
      std::vector<unsigned> adjIntersections;
      
      std::string intersectionName = getIntersectionName(intersection_id);
      // transform to lower case for insertion to search dictionary
      std::transform(intersectionName.begin(), intersectionName.end(), intersectionName.begin(), ::tolower);

      // Loop through each segment of the intersection
      for (int i = 0; i < segmentCount; i++) {
        StreetSegmentIndex segmentID = getIntersectionStreetSegment(i, intersection_id);
        InfoStreetSegment segmentInfo = getInfoStreetSegment(segmentID);
        segmentIDs.push_back(segmentID);
        streetNames.push_back(getStreetName(segmentInfo.streetID));
        
        // Get adjacent intersections
        if (segmentInfo.to != intersection_id) adjIntersections.push_back(segmentInfo.to);
        if (!segmentInfo.oneWay && segmentInfo.from != intersection_id) adjIntersections.push_back(segmentInfo.from);
        
        // Intersections can be connected to itself, only if to == from
        if (segmentInfo.to == segmentInfo.from) adjIntersections.push_back(segmentInfo.from);
        
      }

      std::sort(segmentIDs.begin(), segmentIDs.end());
      segmentIDs.erase(std::unique(segmentIDs.begin(), segmentIDs.end()), segmentIDs.end());  
      
      // Delete Duplicate Intersection Entries
      std::sort(adjIntersections.begin(), adjIntersections.end());
      adjIntersections.erase(std::unique(adjIntersections.begin(), adjIntersections.end()), adjIntersections.end());     

      // Build intersection segments separately from object to improve performance
      store.intersectionSearchNodes[intersection_id] = new IntersectionSearchNode(intersection_id, segmentIDs);
      store.SEGMENTS_IDS[intersection_id] = segmentIDs;
      
      // Build intersection
      store.INTERSECTIONS[intersection_id] = new Intersection(
              intersection_id, getIntersectionName(intersection_id), 
              streetNames, adjIntersections, getIntersectionPosition(intersection_id));
      
      // Push back ids into exisiting intersection names
      if(store.INTERSECTION_DICTIONARY.find(intersectionName) != store.INTERSECTION_DICTIONARY.end()){
          store.INTERSECTION_DICTIONARY.find(intersectionName)->second.push_back(intersection_id);
      }else {
          std::vector<unsigned> ids;
          ids.push_back(intersection_id);
          store.INTERSECTION_DICTIONARY.insert(std::make_pair(intersectionName, ids));
      }
      
    }
}

// Populate the dictionary with street names, POIs, and intersections
void buildCompletionDictionary() {
    for (int street_id = 0; street_id < getNumStreets(); street_id++) {
        store.completionDictionary.insert(getStreetName(street_id));
    }
    for (int intersection_id = 0; intersection_id < getNumIntersections(); intersection_id++) {
        store.completionDictionary.insert(getIntersectionName(intersection_id));
    }
    for (int poi_id = 0; poi_id < getNumPointsOfInterest(); poi_id++) {
        store.completionDictionary.insert(getPointOfInterestName(poi_id));
    }
}

void buildStreetSegments() {
    for (int street_seg_id = 0; street_seg_id < getNumStreetSegments(); street_seg_id++) {
        
        double segment_length = 0, travel_time, speedLimit;
        std::vector<LatLon> segment_points;
        std::vector<double> segment_angles;
        InfoStreetSegment segmentInfo = getInfoStreetSegment(street_seg_id);
        
        // Build segments curve point vector
        segment_points.push_back(getIntersectionPosition(segmentInfo.from));
        for(int i=0; i< segmentInfo.curvePointCount; i++){
            segment_points.push_back(getStreetSegmentCurvePoint(i, street_seg_id)); 
        }
        segment_points.push_back(getIntersectionPosition(segmentInfo.to));
        
        // Traverse through segment_points to calculate angles
        for(auto current=segment_points.begin(); current != segment_points.end()-1; current++){
            LatLon temp = *(current + 1);
            segment_angles.push_back(segmentRotationAngle(*current, temp));
        }

        // Traverse through points vector and add up distance
        for(unsigned i = 0; i < segment_points.size()-1; i++){
            segment_length += find_distance_between_two_points(segment_points[i], segment_points[i+1]);
        }
      
        travel_time = segment_length / (segmentInfo.speedLimit / 3.6);
        
        speedLimit = segmentInfo.speedLimit;
        
        if (store.topSpeedLimit < speedLimit) store.topSpeedLimit = speedLimit;
      
        store.SEGMENTS[street_seg_id] = new StreetSegment(segmentInfo.streetID, segment_length, travel_time, segment_angles, segment_points, speedLimit, segmentInfo.oneWay);
        store.OSMID_SEGMENTID_MAP.insert(std::make_pair(segmentInfo.wayOSMID, street_seg_id));
    }
}

void buildStreets() { 
    for (int street_id = 0; street_id < getNumStreets(); street_id++) {        
        store.STREETS[street_id] = new Street(street_id, getStreetName(street_id));
    }
    
    // Adds all the segments to a street
    for (int street_seg_id = 0; street_seg_id < getNumStreetSegments(); street_seg_id++) {
        InfoStreetSegment segmentInfo = getInfoStreetSegment(street_seg_id);
        store.STREETS[segmentInfo.streetID]->addSegment(street_seg_id);
    }
    
    // Completing street creation
    for (int street_id = 0; street_id < getNumStreets(); street_id++) {
        std::string nameOfStreet = getStreetName(street_id);  
        std::transform(nameOfStreet.begin(), nameOfStreet.end(), nameOfStreet.begin(), ::tolower);
        store.STREETS_DICTIONARY.insert(std::make_pair(nameOfStreet, street_id));
        
        store.STREETS[street_id]->clearSegmentDuplicates();
        store.STREETS[street_id]->generateIntersectionsList();
        store.STREETS[street_id]->calculateStreetLength();
    }
}

/*
 * Builds POI Dictionary by looping through all the POIS and inserting into POI_DICTIONARY
 * Manipulates store.POI_DICTIONARY
 */
void buildPOIDictionary(){
    for(int poi_id = 0; poi_id < getNumPointsOfInterest(); poi_id++){
        std::string name = getPointOfInterestName(poi_id);
        std::transform(name.begin(), name.end(), name.begin(), ::tolower);
        store.POI_DICTIONARY.insert(std::make_pair(name, poi_id));
    }
}

/* Builds Feature objects with their corresponding draw properties 
 * @params void
 * @returns void
 */
void buildFeatureMap() {  
    // Must initialize colour
    ezgl::color colour = ezgl::BLACK;
   
    for (int featureIndex = 0; featureIndex < getNumFeatures(); featureIndex++) {
        int drawZoomLevel = 1; // default zoom level
        FeatureType type = getFeatureType(featureIndex);

        switch (type) {
            case River:
                colour = ezgl::WATER_BLUE;
                drawZoomLevel = 5;
                break;
            case Lake: 
                colour = ezgl::WATER_BLUE;
                break;
            case Stream: 
                colour = ezgl::WATER_BLUE;
                drawZoomLevel = 5;
                break;

            case Greenspace: 
                colour = ezgl::PARK_GREEN;
                drawZoomLevel = 5;
                break;
            case Golfcourse: 
                colour = ezgl::PARK_GREEN;
                break;
            case Park: 
                colour = ezgl::PARK_GREEN;
                break;

            case Building: 
                colour = ezgl::DARK_GRAY;
                drawZoomLevel = 5;
                break;

            case Island:
                colour = ezgl::LIGHT_GRAY;
                break;
            case Beach: 
                colour= ezgl::SAND;
                break;
            default: break;

        }
        
        // Insert data into feature object, and store into global store
        InternalFeature* feature = new InternalFeature(featureIndex, colour, drawZoomLevel);
        store.FEATURES_TYPE_MAP.insert(std::make_pair(getFeatureType(featureIndex), feature));
    }
}

/* Builds initial bus routes vector with GET request to nextBus API with command routeList 
 * To be used only with toronto_canada map 
 * @params void
 * @returns void
 */
void buildBusRoutes() {
    using namespace boost::property_tree;
    ptree propertyTree;
    
    CURL *curlHandle = curl_easy_init();
    store.responseBuffer = "";
    
    // Initialize request headers
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Accept: application/xml");
    headers = curl_slist_append(headers, "charset: utf-8");
    
    std::string url = "http://webservices.nextbus.com/service/publicXMLFeed?command=routeList&a=ttc";
    
    curl_easy_setopt(curlHandle, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curlHandle, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curlHandle, CURLOPT_WRITEFUNCTION, curl_write);  
    
    // Get response from API
    curl_easy_perform(curlHandle);
    curl_slist_free_all(headers);
    
    // Transform responseBuffer to istream, and parse into property tree
    std::istringstream inputStream(store.responseBuffer);
    read_xml(inputStream, propertyTree);
    
    // Loop through the children of body attribute to get all the routes
    BOOST_FOREACH(ptree::value_type const &value, propertyTree.get_child("body")) {
        // Guard to check only "route" xml attributes
        if (value.first == "route") {
            // Create route object and insert into store.ROUTES
            int routeTag = std::stoi(value.second.get("<xmlattr>.tag", "Error: did not get bus route"));
            std::string routeName = value.second.get("<xmlattr>.title", "Error: did not get bus title");
            
            Route* route = new Route(routeTag, routeName);
            store.routes.insert(std::make_pair(routeTag, route));
        }
    }
    
    curl_easy_cleanup(curlHandle);
}

/* Builds bus stops given the route tag, is to be called only when bus is searched for the first time
 * Result is then built and cached into Route object
 * @params routeNumber (int)
 * @returns void
 */
void buildBusStops(int routeTag) {
    using namespace boost::property_tree;
    ptree propertyTree;
    
    std::vector<std::pair<LatLon, int>> stopPoints;

    CURL *curlHandle = curl_easy_init();
    store.responseBuffer = "";
    
    // Initialize headers
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Accept: application/xml");
    headers = curl_slist_append(headers, "charset: utf-8");

    std::string url = "http://webservices.nextbus.com/service/publicXMLFeed?command=routeConfig&a=ttc&r=" + std::to_string(routeTag) + "&terse";
    
    curl_easy_setopt(curlHandle, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curlHandle, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curlHandle, CURLOPT_WRITEFUNCTION, curl_write);  
    
    // Get response
    curl_easy_perform(curlHandle);
    curl_slist_free_all(headers);
    
    // Parse into property trees
    std::istringstream inputStream(store.responseBuffer);
    read_xml(inputStream, propertyTree);
    
    // For each children of the route, get the position and build into stopPoints
    BOOST_FOREACH(ptree::value_type const &value, propertyTree.get_child("body").get_child("route")) {
        if (value.first == "stop") {
            float lat = std::stof(value.second.get("<xmlattr>.lat", "-1"));
            float lon = std::stof(value.second.get("<xmlattr>.lon", "-1"));
            int id = std::stoi(value.second.get("<xmlattr>.stopId", "-1"));
            
            LatLon stop(lat, lon);
            stopPoints.push_back(std::make_pair(stop, id));
        }
    }
   
    // Find the corresponding route, and cache the stop data
    store.routes.find(routeTag)->second->setRouteStops(stopPoints);
    store.routes.find(routeTag)->second->alreadyBuilt = true;
    
    curl_easy_cleanup(curlHandle);
}

/* Builds PNG_MAP to reduce file I/O request when using pngs
 * To be called only once during initial refresh of main canvas
 * @params Renderer object
 * @returns void
 */
void buildPNG(ezgl::renderer &g) {
    store.PNG_MAP.insert(std::make_pair("bank", g.load_png("./libstreetmap/resources/poi_icons/bank.png")));
    store.PNG_MAP.insert(std::make_pair("marker", g.load_png("./libstreetmap/resources/poi_icons/marker.png")));
    store.PNG_MAP.insert(std::make_pair("user_marker", g.load_png("./libstreetmap/resources/poi_icons/user_marker.png")));
    store.PNG_MAP.insert(std::make_pair("dentist", g.load_png("./libstreetmap/resources/poi_icons/dentist.png")));
    store.PNG_MAP.insert(std::make_pair("hospital", g.load_png("./libstreetmap/resources/poi_icons/hospital.png")));
    store.PNG_MAP.insert(std::make_pair("university", g.load_png("./libstreetmap/resources/poi_icons/university.png")));
    store.PNG_MAP.insert(std::make_pair("default", g.load_png("./libstreetmap/resources/poi_icons/default.png")));
    store.PNG_MAP.insert(std::make_pair("atm", g.load_png("./libstreetmap/resources/poi_icons/atm.png")));
    store.PNG_MAP.insert(std::make_pair("fast_food", g.load_png("./libstreetmap/resources/poi_icons/fast_food.png")));
    store.PNG_MAP.insert(std::make_pair("bus_stop", g.load_png("./libstreetmap/resources/poi_icons/bus_stop.png")));
}

/* Builds a vector of help commands to be displayed on the cards
 * To be called only once during during load_map
 * @params void
 * @returns void
 */
void buildCommandList(){
    std::string help = "/help          --       Brings up this help menu";
    std::string network = "/network    --       Enables networking";
    std::string weather = "/weather    --       Displays weather data of current map";
    std::string refresh = "/refresh      --       Refreshes the bus routes";
    std::string search = "You can search for any Point of Interest, Street, \nIntersection, or city/country in the search bar\n"
            "Map changes can be specified with either a valid \ncountry name search, or a 'city, country' pair";
    std::string busSearch = "To search for bus/streetcars use route ### - \nand click on the desired stop";
    std::string directions = "To get directions from one intersection to another, \nenter the source in the 1st search bar,"
            "\nand the destination in the destination search bar.";
    std::string directionsClick = "Directions can also be given by \nCTRL-clicking two intersections";

    
    store.commands.push_back(help);
    store.commands.push_back(network);
    store.commands.push_back(weather);
    store.commands.push_back(refresh);
    store.commands.push_back(search);
    store.commands.push_back(busSearch);
    store.commands.push_back(directions);
    store.commands.push_back(directionsClick);
}
