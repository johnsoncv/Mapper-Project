// StreetSegment Object holds the length and travel time of each segment
#include "StreetsDatabaseAPI.h"
#include "./ezgl/color.hpp"

#ifndef STREETSEGMENT_H
#define STREETSEGMENT_H

class StreetSegment {
    private:
        int streetID;
        double length;
        double travelTime;
        std::vector<LatLon> segmentPoints; // from -> to
        double speedLimit;
        bool oneWay;
        
        // Map drawing data
        std::vector<double> segmentAngles;
        std::string segmentType;
        ezgl::color segmentColour = ezgl::RED;
        int drawLevel = 1;
        
    public:
        StreetSegment(int id, double length, double travelTime, std::vector<double> angles, std::vector<LatLon> points, double speedLimit, bool oneWay);
        ~StreetSegment();

        // setters
        void setSegmentType(std::string type);
        void setSegmentColour(ezgl::color colour);
        void setDrawLevel(int zoomLevel);
        
        // getters
        double getLength();
        double getTravelTime();
        std::vector<double> getSegmentAngles();
        double getSpeedLimit();
        bool getOneWay();
        int getStreetID();
        
        // Getters for map draw data
        int getDrawLevel();
        std::vector<LatLon> getSegmentPoints();
        std::string getSegmentType();
        ezgl::color getSegmentColour();    
};

#endif 

