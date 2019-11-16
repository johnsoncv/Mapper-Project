#include "ezgl/application.hpp"
#include <set>

// A flag to disable event loop (default is false)
bool disable_event_loop = false;

namespace ezgl {

void application::startup(GtkApplication *, gpointer user_data)
{
  auto ezgl_app = static_cast<application *>(user_data);
  g_return_if_fail(ezgl_app != nullptr);

  char const *main_ui_resource = ezgl_app->m_main_ui.c_str();

  // Build the main user interface from the XML resource.
  GError *error = nullptr;
  if(gtk_builder_add_from_file(ezgl_app->m_builder, main_ui_resource, &error) == 0) {
    g_error("%s.", error->message);
  }

  for(auto &c_pair : ezgl_app->m_canvases) {
    GObject *drawing_area = ezgl_app->get_object(c_pair.second->id());
    c_pair.second->initialize(GTK_WIDGET(drawing_area));
  }

  g_info("application::startup successful.");
}

void application::activate(GtkApplication *, gpointer user_data)
{
  auto ezgl_app = static_cast<application *>(user_data);
  g_return_if_fail(ezgl_app != nullptr);

  // The main parent window needs to be explicitly added to our GTK application.
  GObject *window = ezgl_app->get_object(ezgl_app->m_window_id.c_str());
  gtk_application_add_window(ezgl_app->m_application, GTK_WINDOW(window));
  
  GtkCssProvider *cssProvider = gtk_css_provider_new();
  if (gtk_css_provider_load_from_path(cssProvider, "./libstreetmap/resources/custom.css", NULL)) {
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(), GTK_STYLE_PROVIDER(cssProvider), GTK_STYLE_PROVIDER_PRIORITY_USER);
  }
  
  
  if(ezgl_app->m_register_callbacks != nullptr) {
    ezgl_app->m_register_callbacks(ezgl_app);
  } else {
    register_default_buttons_callbacks(ezgl_app);
    register_default_events_callbacks(ezgl_app);
  }

  if(ezgl_app->initial_setup_callback != nullptr)
    ezgl_app->initial_setup_callback(ezgl_app);
  
  g_info("application::activate successful.");
}

application::application(application::settings s)
    : m_main_ui(s.main_ui_resource)
    , m_window_id(s.window_identifier)
    , m_canvas_id(s.canvas_identifier)
    , m_application(gtk_application_new("edu.toronto.eecg.ezgl.app", G_APPLICATION_FLAGS_NONE))
    , m_builder(gtk_builder_new())
    , m_register_callbacks(s.setup_callbacks)
{
  // Connect our static functions application::{startup, activate} to their callbacks. We pass 'this' as the userdata
  // so that we can use it in our static functions.
  g_signal_connect(m_application, "startup", G_CALLBACK(startup), this);
  g_signal_connect(m_application, "activate", G_CALLBACK(activate), this);

  first_run = true;
}

application::~application()
{
  // GTK uses reference counting to track object lifetime. Since we called *_new() for our application and builder, we
  // need to unreference them. This should set their reference count to 0, letting GTK know that they should be cleaned
  // up in memory.
  g_object_unref(m_builder);
  g_object_unref(m_application);
}

canvas *application::get_canvas(const std::string &canvas_id) const
{
  auto it = m_canvases.find(canvas_id);
  if(it != m_canvases.end()) {
    return it->second.get();
  }

  g_warning("Could not find canvas with name %s.", canvas_id.c_str());
  return nullptr;
}

canvas *application::add_canvas(std::string const &canvas_id,
    draw_canvas_fn draw_callback,
    rectangle coordinate_system)
{
  if(draw_callback == nullptr) {
    // A NULL draw callback means the canvas will never render anything to the screen.
    g_warning("Canvas %s's draw callback is NULL.", canvas_id.c_str());
  }

  // Can't use make_unique with protected constructor without fancy code that will confuse students, so we use new
  // instead.
  std::unique_ptr<canvas> canvas_ptr(new canvas(canvas_id, draw_callback, coordinate_system));
  auto it = m_canvases.emplace(canvas_id, std::move(canvas_ptr));

  if(!it.second) {
    // std::map's emplace does not insert the value when the key is already present.
    g_warning("Duplicate key (%s) ignored in application::add_canvas.", canvas_id.c_str());
  } else {
    g_info("The %s canvas has been added to the application.", canvas_id.c_str());
  }

  return it.first->second.get();
}

GObject *application::get_object(gchar const *name) const
{
  // Getting an object from the GTK builder does not increase its reference count.
  GObject *object = gtk_builder_get_object(m_builder, name);
  g_return_val_if_fail(object != nullptr, nullptr);

  return object;
}

int application::run(setup_callback_fn initial_setup_user_callback,
    mouse_callback_fn mouse_press_user_callback,
    mouse_callback_fn mouse_move_user_callback,
    key_callback_fn key_press_user_callback)
{
  if(disable_event_loop)
    return 0;

  initial_setup_callback = initial_setup_user_callback;
  mouse_press_callback = mouse_press_user_callback;
  mouse_move_callback = mouse_move_user_callback;
  key_press_callback = key_press_user_callback;

  // The result of calling g_application_run() again after it returns is unspecified.
  // So we have to destruct and reconstruct the GTKApplication
  if(!first_run) {
    // Destroy the GTK application
    g_object_unref(m_application);
    g_object_unref(m_builder);

    // Reconstruct the GTK application
    m_application = (gtk_application_new("edu.toronto.eecg.ezgl.app", G_APPLICATION_FLAGS_NONE));
    m_builder = (gtk_builder_new());
    g_signal_connect(m_application, "startup", G_CALLBACK(startup), this);
    g_signal_connect(m_application, "activate", G_CALLBACK(activate), this);
  }

  // set the first_run flag to false
  first_run = false;

  g_info("The event loop is now starting.");

  // see: https://developer.gnome.org/gio/unstable/GApplication.html#g-application-run
  return g_application_run(G_APPLICATION(m_application), 0, 0);
}

void application::quit()
{
  // Close the current window
  GObject *window = get_object(m_window_id.c_str());
  gtk_window_close(GTK_WINDOW(window));

  // Quit the GTK application
  g_application_quit(G_APPLICATION(m_application));
}

void application::register_default_events_callbacks(ezgl::application *application)
{
  // Get a pointer to the main window GUI object by using its name.
  std::string main_window_id = application->get_main_window_id();
  GObject *window = application->get_object(main_window_id.c_str());

  // Get a pointer to the main canvas GUI object by using its name.
  std::string main_canvas_id = application->get_main_canvas_id();
  GObject *main_canvas = application->get_object(main_canvas_id.c_str());

  // Connect press_key function to keyboard presses in the MainWindow.
  g_signal_connect(window, "key_press_event", G_CALLBACK(press_key), application);

  // Connect press_mouse function to mouse presses and releases in the MainWindow.
  g_signal_connect(main_canvas, "button_press_event", G_CALLBACK(press_mouse), application);

  // Connect release_mouse function to mouse presses and releases in the MainWindow.
  g_signal_connect(main_canvas, "button_release_event", G_CALLBACK(release_mouse), application);

  // Connect release_mouse function to mouse presses and releases in the MainWindow.
  g_signal_connect(main_canvas, "motion_notify_event", G_CALLBACK(move_mouse), application);

  // Connect scroll_mouse function to the mouse scroll event (up, down, left and right)
  g_signal_connect(main_canvas, "scroll_event", G_CALLBACK(scroll_mouse), application);
}

void application::register_default_buttons_callbacks(ezgl::application *application)
{
    (void) application;
//  // Signal connection skeleton
//  GObject *zoom_fit_button = application->get_object("ZoomFitButton");
//  g_signal_connect(zoom_fit_button, "clicked", G_CALLBACK(press_zoom_fit), application);
}

void application::initializeAutoComplete() {
    // Populate the completion model that was init during load_map
    for (auto entry = store.completionDictionary.begin(); entry != store.completionDictionary.end(); entry++) {
        //first search bar
        GtkTreeIter iterator1;
        gtk_list_store_append(GTK_LIST_STORE(get_object("completionModel1")), &iterator1);
        gtk_list_store_set(GTK_LIST_STORE(get_object("completionModel1")), &iterator1, 0, (*entry).c_str(), -1);

        //second search bar
        GtkTreeIter iterator2;
        gtk_list_store_append(GTK_LIST_STORE(get_object("completionModel2")), &iterator2);
        gtk_list_store_set(GTK_LIST_STORE(get_object("completionModel2")), &iterator2, 0, (*entry).c_str(), -1);
    }
  
}

    void application::update_status(std::string const &message)
    {
      // Get the StatusBar Object
      GtkStatusbar *status_bar = (GtkStatusbar *)get_object("StatusBar");

      // Remove all previous messages from the message stack
      gtk_statusbar_remove_all(status_bar, 0);

      // Push user message to the message stack
      gtk_statusbar_push(status_bar, 0, message.c_str());
    }

    void application::get_input() {
        // Init GtkEntry 
        GtkEntry *input_box = (GtkEntry *)get_object("SearchBox");
        GtkEntryBuffer *input_buffer = gtk_entry_get_buffer(input_box);

        // Get buffer
        const gchar *input_string = gtk_entry_buffer_get_text(input_buffer);

        // Buffer to C++ string
        std::string string(input_string);

        store.searchString = string;
    }
    
    void application::get_destination() {
        // Init GtkEntry 
        GtkEntry *input_box = (GtkEntry *)get_object("Destination");
        GtkEntryBuffer *input_buffer = gtk_entry_get_buffer(input_box);

        // Get buffer
        const gchar *input_string = gtk_entry_buffer_get_text(input_buffer);

        // Buffer to C++ string
        std::string string(input_string);

        store.destinationSearchString = string;
    }


    void application::resetCards() {
        GtkBox *detailBox = (GtkBox *)get_object("DetailBox");
        GList *children, *iter;
        children = gtk_container_get_children(GTK_CONTAINER(detailBox));
        for (iter = children; iter != NULL; iter = g_list_next(iter)) {
            gtk_widget_destroy(GTK_WIDGET(iter->data));
        }

        g_list_free(children);
    }

    GtkWidget* application::createCard(std::string const &title, std::string const &subtitle, std::string const &detailHeader, std::vector<std::string> const &details) {
        // Construct default card styling / text positions
        GtkWidget *card = gtk_grid_new();
        gtk_widget_set_size_request((GtkWidget *)card, 400, 300);
        gtk_grid_insert_row((GtkGrid *)card, 0);
        gtk_grid_insert_row((GtkGrid *)card, 1);
        gtk_grid_insert_row((GtkGrid *)card, 2);
        gtk_grid_insert_column((GtkGrid *)card, 0);

        GtkStyleContext * context = gtk_widget_get_style_context((GtkWidget *)card);
        gtk_style_context_add_class(context, "Card");

        GtkTextBuffer *titleBuffer = gtk_text_buffer_new(NULL);
        GtkTextBuffer *subTitleBuffer = gtk_text_buffer_new(NULL);

        gtk_text_buffer_set_text(titleBuffer, title.c_str(), title.length());
        gtk_text_buffer_set_text(subTitleBuffer, subtitle.c_str(), subtitle.length());

        GtkWidget *titleWidget = gtk_text_view_new_with_buffer(titleBuffer);    
        GtkWidget *subTitleWidget = gtk_text_view_new_with_buffer(subTitleBuffer);

        context = gtk_widget_get_style_context(titleWidget);
        gtk_style_context_add_class(context, "title");
        context = gtk_widget_get_style_context(subTitleWidget);
        gtk_style_context_add_class(context, "subtitle");

        gtk_grid_attach((GtkGrid *) card, titleWidget, 0, 0, 1, 1);
        gtk_grid_attach((GtkGrid *) card, subTitleWidget, 0, 1, 1, 1);

        if (detailHeader.length() != 0) {
            GtkTextBuffer *headerBuffer = gtk_text_buffer_new(NULL);
            gtk_text_buffer_set_text(headerBuffer, detailHeader.c_str(), detailHeader.length());
            GtkWidget *headerWidget = gtk_text_view_new_with_buffer(headerBuffer);

            context = gtk_widget_get_style_context(headerWidget);
            gtk_style_context_add_class(context, "header");
            gtk_grid_attach((GtkGrid *) card, headerWidget, 0, 2, 1, 1);
        }

        gtk_text_view_set_wrap_mode((GtkTextView *)titleWidget, GTK_WRAP_WORD);

        // Push each detail into the card
        int rowIndex = 3;
        for (auto detail = details.begin(); detail != details.end(); detail++, rowIndex++) {
            GtkTextBuffer *detailBuffer = gtk_text_buffer_new(NULL);
            gtk_text_buffer_set_text(detailBuffer, detail->c_str(), detail->length());

            GtkWidget *detailWidget = gtk_text_view_new_with_buffer(detailBuffer);

            context = gtk_widget_get_style_context(detailWidget);
            gtk_style_context_add_class(context, "detail");

            gtk_grid_insert_row((GtkGrid *)card, rowIndex);
            gtk_grid_attach((GtkGrid *) card, detailWidget, 0, rowIndex, 1, 1);
        }

        return card;
    }

    void application::createPredictionCard(std::string const &estimatedTime, std::string const &routeName, std::string const &routeNumber) {
        // Does not reset cards, as it is called multiple times for each estimated time

        GtkBox *detailBox = (GtkBox *)get_object("DetailBox");
        std::vector<std::string> details;

        details.push_back(routeName);

        GtkWidget* card = createCard("Next arrival: " + estimatedTime + " minute(s)", routeNumber, "Route Description: ", details);
        gtk_box_pack_start(detailBox, (GtkWidget *)card, true, true, 5);

        gtk_widget_show_all((GtkWidget *)card);
    }

    void application::createIntersectionCard(std::string const &name, std::string const &coords, std::vector<unsigned> nearbyPOIs) {
        resetCards();

        GtkBox *detailBox = (GtkBox *)get_object("DetailBox");
        std::vector<std::string> details;

        for (auto it = nearbyPOIs.begin(); it != nearbyPOIs.end(); it++) {
            details.push_back(getPointOfInterestName(*it) + " (" + getPointOfInterestType(*it) + ")");
        }


        GtkWidget* card = createCard(name, coords, "Nearby (50m): ", details);
        gtk_box_pack_start(detailBox, (GtkWidget *)card, true, true, 10);

        gtk_widget_show_all((GtkWidget *)card);
   }
 
    void application::createPOICard(std::string const &name, std::string const &coords, std::string const &type) {
        resetCards();

        GtkBox *detailBox = (GtkBox *)get_object("DetailBox");
        std::vector<std::string> details;

        details.push_back(type);

        GtkWidget* card = createCard(name, coords, "Coords: ", details);
        gtk_box_pack_start(detailBox, (GtkWidget *)card, true, true, 10);

        gtk_widget_show_all((GtkWidget *)card);
    }
  
    void application::createWeatherCard(std::string const &name, std::string const &weather, std::string const &currentTemp, std::string const &visibility) {
        resetCards();

        GtkBox *detailBox = (GtkBox *)get_object("DetailBox");
        std::vector<std::string> details;

        details.push_back(visibility);

        GtkWidget* card = createCard("In " + name + " " + currentTemp + "\u2103", weather, "Visibility (km)", details);
        gtk_box_pack_start(detailBox, (GtkWidget *)card, true, true, 10);

        gtk_widget_show_all((GtkWidget *)card);
    }
    
    void application::createPathCard(std::string const &distance, std::string const &time, std::vector<std::string> const &details){
        resetCards();
        
        GtkBox *detailBox = (GtkBox *)get_object("DetailBox");
        
        GtkWidget* card = createCard("Directions", " ", "Distance: " + distance + ".  Time: " + time, details);
        gtk_box_pack_start(detailBox, (GtkWidget *)card, true, true, 10);

        gtk_widget_show_all((GtkWidget *)card);
    }
    
    void application::createHelpCard(){
        resetCards();
        
        GtkBox *detailBox = (GtkBox *)get_object("DetailBox");
        
        GtkWidget* card = createCard("Welcome to City Mapper \n\nHelp Menu", " ", "List of Commands", store.commands);
        gtk_box_pack_start(detailBox, (GtkWidget *)card, true, true, 10);

        gtk_widget_show_all((GtkWidget *)card);
    }
    
    void application::createErrorCard(std::string message){
        resetCards();
        
        GtkBox *detailBox = (GtkBox *)get_object("DetailBox");
        
        std::vector<std::string> details;
        details.push_back("Please refer to the help menu. Type /help.");
        GtkWidget* card = createCard("Error", "", message, details);
        gtk_box_pack_start(detailBox, (GtkWidget *)card, true, true, 10);

        gtk_widget_show_all((GtkWidget *)card);
    }
    
    void application::refresh_drawing()
    {
      // get the main canvas
      canvas *cnv = get_canvas(m_canvas_id);

      // force redrawing
      cnv->redraw();
    }
}

void set_disable_event_loop(bool new_setting)
{
  disable_event_loop = new_setting;
}
