#ifndef INTERNALFEATURE_H
#define INTERNALFEATURE_H

#include <ezgl/color.hpp>

class InternalFeature {
public:
    InternalFeature(int id, ezgl::color colour, int level);
    
    // Getters
    int getID();
    ezgl::color getColour();
    int getZoomLevel();
    
private:
    int featureID, zoomLevel = 1;
    ezgl::color featureColour = ezgl::BLACK;

};

#endif /* INTERNALFEATURE_H */

