#include "m1.h"
#include "m3.h"
#include "m4.h"
#include "util.h"
#include <vector>
#include <iostream>
#include <chrono>

struct Delivery {
    unsigned id;
    unsigned dropOff;
    float itemWeight;
} typedef Delivery;

// 2d hash of (src, dest) -> travelTime as value
std::vector<std::unordered_map<unsigned, std::pair<double, std::vector<unsigned>>>> pathTimes;
std::unordered_map<unsigned, unsigned> pathMap;

std::pair<std::vector<CourierSubpath>, double> startingDepotPath(const std::vector<DeliveryInfo>& deliveries,
        const std::vector<unsigned>& depots,
        const float right_turn_penalty,
        const float left_turn_penalty,
        const float truck_capacity,
        int startingDepot);

std::vector<CourierSubpath> traveling_courier(
        const std::vector<DeliveryInfo>& deliveries,
        const std::vector<unsigned>& depots,
        const float right_turn_penalty,
        const float left_turn_penalty,
        const float truck_capacity) {
    
    pathMap.clear();
    pathTimes.clear();

    std::unordered_set<unsigned> intersections;
    std::vector<unsigned> intersectionsVector;

    for (auto it = deliveries.begin(); it != deliveries.end(); it++) {
        intersections.insert(it->dropOff);
        intersections.insert(it->pickUp);
    }

    for (auto it = depots.begin(); it != depots.end(); it++) {
        intersections.insert(*it);
    }

    for (auto it = intersections.begin(); it != intersections.end(); it++) {
        intersectionsVector.push_back(*it);
    }
    
    pathTimes.resize(intersectionsVector.size());

    #pragma omp parallel for
    for (unsigned i = 0; i < intersectionsVector.size(); i++) {
        pathTimes[i] = find_paths_to_destinations(intersectionsVector[i], intersections, right_turn_penalty, left_turn_penalty);
    }
    
    for (unsigned i = 0; i < intersectionsVector.size(); i++) {
        pathMap.insert(std::make_pair(intersectionsVector[i], i));
    }

    std::vector<CourierSubpath> idealPath;
    double smallestTravelTime = DBL_MAX;

    if (deliveries.empty() || depots.empty()) return idealPath;

    std::pair<std::vector<CourierSubpath>, double> depotPath;

    for (auto depot = depots.begin(); depot != depots.end(); depot++) {
        depotPath = startingDepotPath(deliveries, depots, right_turn_penalty, left_turn_penalty, truck_capacity, *depot);
        if (depotPath.second < smallestTravelTime) {
            smallestTravelTime = depotPath.second;
            idealPath = depotPath.first;
        }
    }

    return idealPath;
}

std::pair<std::vector<CourierSubpath>, double> startingDepotPath(const std::vector<DeliveryInfo>& deliveries,
        const std::vector<unsigned>& depots,
        const float right_turn_penalty,
        const float left_turn_penalty,
        const float truck_capacity,
        int startingDepot) {

    std::vector<CourierSubpath> optimalRoute;
    // pending => <pickUpIDs, Delivery>
    std::multimap<unsigned, Delivery> pendingDeliveries;
    // on truck => <dropOffIDs, weight for packages at that dropoff>
    std::unordered_map<unsigned, double> deliveriesInTruck;
    double totalWeight = 0, travelTime = 0;
    int startID = startingDepot, endID = UNDEFINED;
    std::vector<unsigned> empty;

    int index = 0;
    for (auto delivery = deliveries.begin(); delivery != deliveries.end(); delivery++) {
        Delivery newDelivery;

        newDelivery.id = index;
        newDelivery.dropOff = delivery->dropOff;
        newDelivery.itemWeight = delivery->itemWeight;

        pendingDeliveries.insert(std::make_pair(delivery->pickUp, newDelivery));

        index++;
    }

    //smallest distance between starting depot and a delivery pickup
    double smallestDistance = DBL_MAX;
    auto sourceNodeDestinations = pathTimes[pathMap.find(startID)->second];
    for (auto delivery = pendingDeliveries.begin(); delivery != pendingDeliveries.end(); delivery++) {
        if (sourceNodeDestinations.find(delivery->first) != sourceNodeDestinations.end()) {
            double distance = sourceNodeDestinations.find(delivery->first)->second.first;
            if (distance < smallestDistance) {
                smallestDistance = distance;
                endID = delivery->first;
            }
        }
    }

    //subpath between depot and first pickup
    CourierSubpath subpath;
    subpath.start_intersection = startID;
    subpath.end_intersection = endID;

    if (sourceNodeDestinations.find(endID) != sourceNodeDestinations.end()) subpath.subpath = sourceNodeDestinations.find(endID)->second.second;
    else subpath.subpath = empty;

    if (subpath.subpath.empty()) {
        return std::make_pair(optimalRoute, DBL_MAX);
    }

    travelTime += compute_path_travel_time(subpath.subpath, right_turn_penalty, left_turn_penalty);

    optimalRoute.push_back(subpath);

    while (!pendingDeliveries.empty() || !deliveriesInTruck.empty()) {
        // End of prev iteration is the start of the next subpath
        startID = endID;
        subpath.start_intersection = startID;
        std::vector<unsigned> pickUp_indices;

        auto pickUpAvailable = pendingDeliveries.find(startID);
        auto dropOffAvailable = deliveriesInTruck.find(startID);

        // Valid pickup
        if (pickUpAvailable != pendingDeliveries.end()) {
            for (auto i = pendingDeliveries.lower_bound(startID); i != pendingDeliveries.upper_bound(startID) && i != pendingDeliveries.end();) {
                if (totalWeight + i->second.itemWeight <= truck_capacity) {
                    totalWeight += i->second.itemWeight;
                    pickUp_indices.push_back(i->second.id);

                    // Add to truck
                    // Current drop off not in list of places to go
                    auto item = deliveriesInTruck.find(i->second.dropOff);
                    if (item == deliveriesInTruck.end()) {
                        deliveriesInTruck.insert(std::make_pair(i->second.dropOff, i->second.itemWeight));
                    } else { // we are already going to that drop off, just need to add to the weight of things for that loc
                        item->second += i->second.itemWeight;
                    }

                    //erase from pending
                    i = pendingDeliveries.erase(i);
                } else {
                    i++;
                }
            }
        }

        if (dropOffAvailable != deliveriesInTruck.end()) { // at dropoff
            // Remove all packages they are in the truck at this dropoff location
            totalWeight -= dropOffAvailable->second; // remove weight of packages from truck
            deliveriesInTruck.erase(startID); // delete from truck list
        }

        subpath.pickUp_indices = pickUp_indices;

        // Checked we are at the last drop off and done everything then go to depot
        if (deliveriesInTruck.empty() && pendingDeliveries.empty()) {
            smallestDistance = DBL_MAX;
            startID = endID;
            endID = UNDEFINED;
            sourceNodeDestinations = pathTimes[pathMap.find(startID)->second];
            for (auto depot = depots.begin(); depot != depots.end(); depot++) {
                if (sourceNodeDestinations.find(*depot) != sourceNodeDestinations.end()) {
                    double distance = sourceNodeDestinations.find(*depot)->second.first;
                    if (distance < smallestDistance) {
                        smallestDistance = distance;
                        endID = *depot;
                    }
                }
            }

            subpath.start_intersection = startID;
            subpath.end_intersection = endID;


            if (sourceNodeDestinations.find(endID) != sourceNodeDestinations.end()) subpath.subpath = sourceNodeDestinations.find(endID)->second.second;
            else subpath.subpath = empty;

            if (subpath.subpath.empty()) {
                return std::make_pair(optimalRoute, DBL_MAX);
            }

            travelTime += compute_path_travel_time(subpath.subpath, right_turn_penalty, left_turn_penalty);
            subpath.pickUp_indices = pickUp_indices;

            optimalRoute.push_back(subpath);

            return std::make_pair(optimalRoute, travelTime);
        }

        // Determine next intersection to go to
        // Finds nearest drop off
        double shortestDropOff = DBL_MAX;
        unsigned closestDropOff = 0;
        sourceNodeDestinations = pathTimes[pathMap.find(startID)->second];
        for (auto dropOff = deliveriesInTruck.begin(); dropOff != deliveriesInTruck.end(); dropOff++) {
            // distance b/w current and the drop off
            double distance = sourceNodeDestinations.find(dropOff->first)->second.first;
            // if less than current shortest distance to drop off, then update
            if (distance < shortestDropOff && (int)dropOff->first != startID) {
                shortestDropOff = distance;
                closestDropOff = dropOff->first;
            }
        }

        // Finds nearest pickup
        double shortestPickUp = DBL_MAX;
        if (pendingDeliveries.size() > 0) {
            unsigned closestPickUp = 0;
            for (auto pickUp = pendingDeliveries.begin(); pickUp != pendingDeliveries.end(); pickUp++) {
                double distance = sourceNodeDestinations.find(pickUp->first)->second.first;
                // if less than current shortest distance to pick up, then update
                if (distance < shortestPickUp && (int)pickUp->first != startID) {
                    shortestPickUp = distance;
                    closestPickUp = pickUp->first;
                }
            }

            double smallestWeightAtPickUp = DBL_MAX;
            int numPickUps = 0;
            for (auto i = pendingDeliveries.lower_bound(closestPickUp); i != pendingDeliveries.upper_bound(closestPickUp); i++) {
                    smallestWeightAtPickUp = i->second.itemWeight;
                    numPickUps++;
            }
            
            smallestWeightAtPickUp = smallestWeightAtPickUp/numPickUps;

            // Too much stuff at that nearest pickup 
            if (totalWeight + smallestWeightAtPickUp > truck_capacity) {
                shortestPickUp = DBL_MAX;
            }

            endID = shortestPickUp < shortestDropOff ? closestPickUp : closestDropOff;
        } else {
            endID = closestDropOff;
        }



        subpath.end_intersection = endID;
        subpath.subpath = pathTimes[pathMap.find(startID)->second].find(endID)->second.second;

        if (subpath.subpath.empty()) {
            return std::make_pair(optimalRoute, DBL_MAX);
        }

        travelTime += compute_path_travel_time(subpath.subpath, right_turn_penalty, left_turn_penalty);

        optimalRoute.push_back(subpath);
    }

    return std::make_pair(optimalRoute, travelTime);
}