// Contains all the handler routines for GUI, and individual functions

#ifndef HANDLERS_H
#define HANDLERS_H

#include "Store.h"
#include "Build.h"
#include "util.h"
#include "m3.h"

#include <string>
#include <cctype>
#include <ezgl/application.hpp>
#include <boost/algorithm/string.hpp>

#include <boost/date_time.hpp>

// GUI handler
void searchHandler(ezgl::application *application);
void clickHandler(ezgl::application *application, double &x, double &y);

// Live weather data handler
void weatherHandler(ezgl::application *application, std::string cityName);

// Path handler
void pathHandler(ezgl::application *application, std::vector<unsigned> &path);

// Handlers for search
void routeHandler(std::string delimiter);
void streetIntersectionHandler(std::string delimiter, ezgl::application *application);
bool intersectionHandler (ezgl::application *application);
void mapChangeCityHandler(std::string delimiter);
void mapChangeCountryHandler();
bool poiHandler(ezgl::application *application);
void busPredictionHandler(ezgl::application *application);
void helpHandler(ezgl::application *application);
void errorHandler(ezgl::application *application, std::string errorMessage);
void searchPathHandler(ezgl::application *application, std::string delimiter);

#endif /* HANDLERS_H */

