/* 
 * Copyright 2018 University of Toronto
 *
 * Permission is hereby granted, to use this software and associated 
 * documentation files (the "Software") in course work at the University 
 * of Toronto, or for personal use. Other uses are prohibited, in 
 * particular the distribution of the Software either publicly or to third 
 * parties.
 *
 * The above copyright notice and this permission notice shall be included in 
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include <iostream>
#include <string>
#include <curl/curl.h>

#include "m1.h"
#include "m2.h"
#include "m3.h"
#include "util.h"
#include "Store.h"

//Program exit codes
constexpr int SUCCESS_EXIT_CODE = 0;        //Everything went OK
constexpr int ERROR_EXIT_CODE = 1;          //An error occurred
constexpr int BAD_ARGUMENTS_EXIT_CODE = 2;  //Invalid command-line usage

int main() {

    do {
            
        curl_global_init(CURL_GLOBAL_ALL);
        store.reloadFlag = false;
        std::string map_path = "/cad2/ece297s/public/maps/" + store.mapName + ".streets.bin";

        // Load the map and related data structures
        bool load_success = load_map(map_path);
        if(!load_success) {
            store.mapName = store.prevMap;
            map_path = "/cad2/ece297s/public/maps/" + store.mapName + ".streets.bin";
            std::cerr << "Failed to load map '" << map_path << "'\n";
            std::cout << "Restarting with previous map " << store.mapName << "\n";
            load_map(map_path);
        }

        std::cout << "Successfully loaded map '" << map_path << "'\n";

        store.loadSuccessFlag = load_success;
        
        // Draw map
        draw_map();
                
        //Clean-up the map data and related data structures
        std::cout << "Closing map: " + map_path + "\n";
        close_map();
        curl_global_cleanup();
        
    } while (store.reloadFlag);
    
    std::cout << "Quitting Map\n";

    return SUCCESS_EXIT_CODE;
}
