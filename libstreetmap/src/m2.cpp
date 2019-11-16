#include "m1.h"
#include "m2.h"
#include "m3.h"

#include "util.h"
#include "RefreshCallbacks.h"
#include "Build.h"
#include "Store.h"
#include "StreetsDatabaseAPI.h"
#include "Handlers.h"

#include <iostream>
#include <string>
#include <ezgl/application.hpp>
#include <ezgl/graphics.hpp>

void act_on_mouse_press(ezgl::application *application, GdkEventButton *event, double x, double y);
void act_on_key_press(ezgl::application *application, GdkEventKey *event, char *key_name);
void initial_setup(ezgl::application *application);

// Unused callback
void act_on_mouse_move(ezgl::application *application, GdkEventButton *event, double x, double y);

// Refresh Callback
void refresh_main_canvas(ezgl::renderer &g);

void draw_map() {
    
    //used to determine initial bounds of the world
    double lat_min = getIntersectionPosition(1).lat();
    double lat_max = lat_min;
    double lon_min = getIntersectionPosition(1).lon();
    double lon_max = lon_min;
    
    for (int i = 0; i < getNumIntersections(); i++) {
        LatLon coords = getIntersectionPosition(i);
        
        if (lat_min > coords.lat()) lat_min = coords.lat();
        if (lat_max < coords.lat()) lat_max = coords.lat();
        if (lon_min > coords.lon()) lon_min = coords.lon();
        if (lon_max < coords.lon()) lon_max = coords.lon();
    }
    
    store.LAT_AVG = (lat_min + lat_max)/2 * DEG_TO_RAD;
    ezgl::rectangle initial_world{{lonToX(lon_min), latToY(lat_min)}, {lonToX(lon_max), latToY(lat_max)}};
    
    ezgl::application::settings settings;
    settings.main_ui_resource = "./libstreetmap/resources/main.ui";
    // Note: the "main.ui" file has a GtkWindow called "MainWindow".
    settings.window_identifier = "MainWindow";
    // Note: the "main.ui" file has a GtkDrawingArea called "MainCanvas".
    settings.canvas_identifier = "MainCanvas";
    
    ezgl::application application(settings);
    application.add_canvas("MainCanvas", refresh_main_canvas, initial_world);
    
    
    application.run(initial_setup, act_on_mouse_press, act_on_mouse_move, act_on_key_press);
}
void refresh_main_canvas(ezgl::renderer &g) {
    
    // Ran once per map load
    if (store.newMapLoadFlag) { 
        buildPNG(g); 
        store.newMapLoadFlag = false;
    }
    
    // Sets map backdrop
    g.set_color(ezgl::LIGHT_GRAY);
    g.fill_rectangle(g.get_visible_world());
    
    drawFeatures(g);
    drawStreets(g);
    
    // Called every refresh but if no focused route (store.FOCUSED_ROUTE == -1), then will never draw any stops
    drawBusStops(g);
    
    // To be called when close enough to see details
    if (store.zoomLevel >= 8) { 
        drawPointsOfInterest(g);
    }
    
    // Drawn on top of all, as they are highlights to indicate the users
    drawHighlighedIntersections(g);
    drawHighlightedPointsOfInterests(g);
    drawUserLoc(g);
    
    // Drawn only once a path is selected
    if(store.drawPath){
        drawPath(g);
    }
}

void initial_setup(ezgl::application *application)
{
  std::string main_canvas_id = application->get_main_canvas_id();
  auto canvas = application->get_canvas(main_canvas_id);
  
  std::string status_message = "Successfully loaded map";
          
  // Initial zoom to prettify map
  ezgl::zoom_in(canvas, 5.0 / 3.0);
  
  // Start with appropriate card message
  if(!store.loadSuccessFlag) errorHandler(application, "Did not find map");
  else helpHandler(application);
  
  // Only ran if networking in enabled to reduce API loads
  if (store.networkingFlag) {
    if (store.mapName == "toronto_canada") buildBusRoutes();
    weatherHandler(application, "toronto");
    status_message += " with networking";
  }
  
  // Init auto complete
  application->initializeAutoComplete();
  
  // Update status of map load
  application->update_status(status_message);  
}

/**
 * Function to handle mouse press event
 * The current mouse position in the main canvas' world coordinate system is returned
 * A pointer to the application and the entire GDK event are also returned
 */
void act_on_mouse_press(ezgl::application *application, GdkEventButton *event, double x, double y)
{
    if (event->state & GDK_CONTROL_MASK) {
        
        LatLon clickedLocation(yToLat(y),xToLon(x));
        unsigned intersectionID = find_closest_intersection(clickedLocation);
        
        store.clicked.push_back(intersectionID);
        std::cout << getIntersectionName(intersectionID) << std::endl;
         
        if (store.clicked.size() == 2) {
            store.path = find_path_between_intersections(store.clicked[0], store.clicked[1], store.RIGHT_TURN_PENALTY, store.LEFT_TURN_PENALTY);
            store.drawPath = true;
            
            store.clicked.clear();
            
            pathHandler(application, store.path);
            application->refresh_drawing();
        }
             
        
    } else if (event->type == GDK_DOUBLE_BUTTON_PRESS) {
        clickHandler(application, x, y);
    }
}

/**
 * Function to handle keyboard press event
 * The name of the key pressed is returned (0-9, a-z, A-Z, Up, Down, Left, Right, Shift_R, Control_L, space, Tab, ...)
 * A pointer to the application and the entire GDK event are also returned
 */
void act_on_key_press(ezgl::application *application, GdkEventKey *event, char *key_name)
{ 
    (void) event;
    std::string key(key_name);
    if (key == "Return") searchHandler(application);
  
} 

/**
 * Function to handle mouse move event
 * The current mouse position in the main canvas' world coordinate system is returned
 * A pointer to the application and the entire GDK event are also returned
 */
void act_on_mouse_move(ezgl::application *application, GdkEventButton *event, double x, double y)
{
    (void) x; (void) y; (void) event; (void) application;
    //std::cout << "Mouse move at coordinates (" << x << "," << y << ") "<< std::endl;
}

