#include "InternalFeature.h"

InternalFeature::InternalFeature(int id, ezgl::color colour, int level) {
    featureID = id;
    featureColour = colour;
    zoomLevel = level;
}

// Getters
int InternalFeature::getID() {
    return featureID;
}

ezgl::color InternalFeature::getColour() {
    return featureColour;
}

int InternalFeature::getZoomLevel() {
    return zoomLevel;
}


