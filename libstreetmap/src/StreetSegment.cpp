#include "StreetSegment.h"
#include "Intersection.h"

StreetSegment::StreetSegment(int id, double segLength, double time, std::vector<double> angles, std::vector<LatLon> points, double segSpeedLimit, bool isOneWay) {
    streetID = id;
    length = segLength;
    travelTime = time;
    segmentAngles = angles;
    segmentPoints = points;
    speedLimit = segSpeedLimit;
    oneWay = isOneWay;
}

StreetSegment::~StreetSegment() {}

// Setters
void StreetSegment::setSegmentType(std::string type) {
    segmentType = type;
}

void StreetSegment::setSegmentColour(ezgl::color colour) {
    segmentColour = colour;
}

void StreetSegment::setDrawLevel(int zoomLevel) {
    drawLevel = zoomLevel;
}

// Getters
double StreetSegment::getLength(){
    return length;
}

double StreetSegment::getTravelTime(){
    return travelTime;
}
double StreetSegment::getSpeedLimit(){
    return speedLimit;
}

bool StreetSegment::getOneWay(){
    return oneWay;
}

int StreetSegment::getStreetID(){
    return streetID;
}

int StreetSegment::getDrawLevel() {
    return drawLevel;
}

std::string StreetSegment::getSegmentType() {
    return segmentType;
}

ezgl::color StreetSegment::getSegmentColour() {
    return segmentColour;
}

std::vector<LatLon> StreetSegment::getSegmentPoints() {
    return segmentPoints;
}

std::vector<double> StreetSegment::getSegmentAngles(){
    return segmentAngles;
}
