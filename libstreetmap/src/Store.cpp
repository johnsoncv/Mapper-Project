#include "Store.h"

// Reset user store for new map load
void Store::resetStore() {
    searchString = "";
    zoomLevel = 1;
    focusedRoute = -1;
    focusedBusStop = -1;
    highlightedIntersections.clear();
    highlightedPOIs.clear();
    clicked.clear();
    drawPath = false;
}
