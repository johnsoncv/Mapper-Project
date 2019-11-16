#include "RefreshCallbacks.h"

void drawStreets(ezgl::renderer &g) {
    bool shouldDraw;
    
    g.set_line_cap(ezgl::line_cap::round);
    for (auto segmentIndex = store.SEGMENTS.begin(); segmentIndex != store.SEGMENTS.end(); segmentIndex++) {
        
        StreetSegment& segmentObject = *(*segmentIndex);
        const std::vector<LatLon>&  points = segmentObject.getSegmentPoints();
        const std::vector<double>& angles = segmentObject.getSegmentAngles();
        std::string name = getStreetName(segmentObject.getStreetID());
        std::string type = segmentObject.getSegmentType();
        bool oneWay = segmentObject.getOneWay();
        
        //sets line width with respect to segment type
        g.set_line_width(getStreetWidth(type));
        
        //to determine which segments to draw, depending on the zoom level
        shouldDraw = segmentObject.getDrawLevel() <= store.zoomLevel;
        
        //iterates through all the points of the segment
        for (auto currentPoint = points.begin(); currentPoint != points.end() -1 && shouldDraw; currentPoint++) {
            const LatLon& nextPoint = *(currentPoint + 1);
            
            g.set_color(segmentObject.getSegmentColour());
            g.draw_line({lonToX(currentPoint->lon()), latToY(currentPoint->lat())}, {lonToX(nextPoint.lon()), latToY(nextPoint.lat())});

            if (name != "<unknown>" && store.zoomLevel >= 8) {
                 g.set_color(38, 50, 56);
                 LatLon avg = averageLatLon(*currentPoint, nextPoint);
                 double x1 = lonToX(currentPoint->lon());
                 double x2 = lonToX(nextPoint.lon());
                 double y1 = lonToX(currentPoint->lat());
                 double y2 = lonToX(nextPoint.lat());

                 int index = currentPoint - points.begin();         

                 //creates the text rectangle
                 ezgl::rectangle text_rec({lonToX(avg.lon()), latToY(avg.lat())}, sqrt(pow(y2-y1,2)+pow(x2-x1,2)), 10);
                 g.set_text_rotation(angles[index]);

                 //arrows for one way streets
                 if(oneWay){
                     if(x1 < x2){
                         g.draw_text(text_rec.bottom_left(), name + "(>>)", text_rec.width(), text_rec.height());
                     }else{
                         g.draw_text(text_rec.bottom_left(), name + "(<<)", text_rec.width(), text_rec.height());
                     }
                 }else{
                     g.draw_text(text_rec.bottom_left(), name, text_rec.width(), text_rec.height());
                 }
            }

        }
    }
}

void drawPath(ezgl::renderer &g){
    if(store.path.size() == 0) return;
    
    for(auto it = store.path.begin(); it != store.path.end(); it++){
        StreetSegment& segmentObject = *(store.SEGMENTS[*it]);
        bool oneWay = segmentObject.getOneWay();
        std::string name = getStreetName(segmentObject.getStreetID());
        const std::vector<LatLon>&  points = segmentObject.getSegmentPoints();
        const std::vector<double>& angles = segmentObject.getSegmentAngles();
       
        //iterates through all the points of the segment
        for (auto currentPoint = points.begin(); currentPoint != points.end() -1; currentPoint++) {
            const LatLon& nextPoint = *(currentPoint + 1);

            g.set_color(ezgl::LIGHT_BLUE);
            g.draw_line({lonToX(currentPoint->lon()), latToY(currentPoint->lat())}, {lonToX(nextPoint.lon()), latToY(nextPoint.lat())});

            if (name != "<unknown>" && store.zoomLevel >= 8) {
                 g.set_color(38, 50, 56);
                 LatLon avg = averageLatLon(*currentPoint, nextPoint);
                 double x1 = lonToX(currentPoint->lon());
                 double x2 = lonToX(nextPoint.lon());
                 double y1 = lonToX(currentPoint->lat());
                 double y2 = lonToX(nextPoint.lat());

                 int index = currentPoint - points.begin();         

                 //creates the text rectangle
                 ezgl::rectangle text_rec({lonToX(avg.lon()), latToY(avg.lat())}, sqrt(pow(y2-y1,2)+pow(x2-x1,2)), 10);
                 g.set_text_rotation(angles[index]);

                 //arrows for one way streets
                 if(oneWay){
                     if(x1 < x2){
                         g.draw_text(text_rec.bottom_left(), name + "(>>)", text_rec.width(), text_rec.height());
                     }else{
                         g.draw_text(text_rec.bottom_left(), name + "(<<)", text_rec.width(), text_rec.height());
                     }
                 }else{
                     g.draw_text(text_rec.bottom_left(), name, text_rec.width(), text_rec.height());
                 }
            }

        }
    }
    
}

void drawFeatures(ezgl::renderer &g) {
    const int featureTypeCount = 9;
    
    //different priorities for different types of maps
    FeatureType defaultFeaturePriorityArray[featureTypeCount] = {Park, Lake, Island, Beach, River, Stream, Greenspace, Golfcourse, Building};
    FeatureType islandFeaturePriorityArray[featureTypeCount] = {Lake, Island, Beach, River, Stream, Greenspace, Golfcourse, Park, Building};
    
    g.set_line_width(1);
    
    for (int featureType = 0; featureType < featureTypeCount; featureType++) {
        
        std::pair<std::multimap<FeatureType, InternalFeature*>::iterator, std::multimap<FeatureType, InternalFeature*>::iterator> bounds;
        
        // Select feature priority depending on map type
        if (store.mapName == "saint-helena" || store.mapName == "new-york_usa") {
           bounds = store.FEATURES_TYPE_MAP.equal_range(islandFeaturePriorityArray[featureType]);
        } 
        else {
           bounds = store.FEATURES_TYPE_MAP.equal_range(defaultFeaturePriorityArray[featureType]);
        }
        
        g.set_color(bounds.first->second->getColour());
        
        for (auto featureIterator = bounds.first; featureIterator != bounds.second; featureIterator++) {
            int i = featureIterator->second->getID();
            int count = getFeaturePointCount(i);

            if (getFeaturePoint(0, i).lon() == getFeaturePoint(count - 1, i).lon() && getFeaturePoint(0, i).lat() == getFeaturePoint(count - 1, i).lat()) {
                // Draw closed features
                
                std::vector<ezgl::point2d> point;
                for (int j = 0; j < count; j++) {
                    point.push_back(ezgl::point2d(lonToX(getFeaturePoint(j, i).lon()), latToY(getFeaturePoint(j, i).lat())));
                }

                if(store.zoomLevel >= featureIterator->second->getZoomLevel() && point.size() > 1) g.fill_poly(point);
                
            } 
            else {
                // Draw open features 
                for (int j = 0; j < count - 1; j++) {
                    if (store.zoomLevel >= featureIterator->second->getZoomLevel()) g.draw_line({lonToX(getFeaturePoint(j, i).lon()), latToY(getFeaturePoint(j, i).lat())}, 
                        {lonToX(getFeaturePoint(j+1, i).lon()), latToY(getFeaturePoint(j+1, i).lat())});
                }
            }
            
        }
    
    }
}

void drawPointsOfInterest(ezgl::renderer &g) {
    bool shouldDraw = store.zoomLevel <= 7 ? false: true;

    for (int k = 0; k < getNumPointsOfInterest(); k++) {
        LatLon center = getPointOfInterestPosition(k);

        if (shouldDraw) {
            std::string type = getPointOfInterestType(k);
            std::string name = getPointOfInterestName(k);
            
            //draw depending on the zoom level and POI types
            if (type == "hospital" || type == "bank" ||  type == "dentist" || type == "atm" || type == "university") {
                g.draw_surface(store.PNG_MAP.find(type)->second, {lonToX(center.lon()), latToY(center.lat())});
                g.draw_text({lonToX(center.lon()), latToY(center.lat())}, name);
            } 
            else if (type == "fast_food" && store.zoomLevel >= 10) {
                g.draw_surface(store.PNG_MAP.find(type)->second, {lonToX(center.lon()), latToY(center.lat())});
                g.draw_text({lonToX(center.lon()), latToY(center.lat())}, name);
            } 
            else if (store.zoomLevel >= 10) {
                g.draw_surface(store.PNG_MAP.find("default")->second, {lonToX(center.lon()), latToY(center.lat())});
            }
        } 
    }
}

//draws a marker for highlighted intersections
void drawHighlighedIntersections(ezgl::renderer &g) {
    g.set_color(ezgl::PINK);
    for (auto it = store.highlightedIntersections.begin(); it != store.highlightedIntersections.end(); it++) {
        LatLon coords = getIntersectionPosition(*it);
        g.draw_surface(store.PNG_MAP.find("marker")->second, {lonToX(coords.lon()), latToY(coords.lat())});
    }
}

//draws a marker for highlighted POIs
void drawHighlightedPointsOfInterests(ezgl::renderer &g) {
    g.set_color(ezgl::PINK);
    for (auto it = store.highlightedPOIs.begin(); it != store.highlightedPOIs.end(); it++) {
        LatLon coords = getPointOfInterestPosition(*it);
        g.draw_surface(store.PNG_MAP.find("marker")->second, {lonToX(coords.lon()), latToY(coords.lat())});
    }
}

//draws the icon for bus stops
void drawBusStops(ezgl::renderer &g) {
    g.set_color(ezgl::BLUE);
    if (store.routes.find(store.focusedRoute) != store.routes.end()) {
        const std::vector<std::pair<LatLon, int>>& points = store.routes.find(store.focusedRoute)->second->getStopPoints();
        for (auto i = points.begin(); i != points.end(); i++) {
            g.draw_surface(store.PNG_MAP.find("bus_stop")->second, {lonToX(i->first.lon()), latToY(i->first.lat())});
        }
    }   
}

//draws the icon for the user location
void drawUserLoc(ezgl::renderer &g){
    g.draw_surface(store.PNG_MAP.find("user_marker")->second, {lonToX(store.userLocation.lon()), latToY(store.userLocation.lat())});
}

