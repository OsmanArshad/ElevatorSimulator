/*
    Title:  ElevatorSimulator.cpp
    Author: Osman Arshad
    Email:  osmanaarshad@gmail.com
    Description: elevatorSimulator.cpp is a CSIM/C++ program that simulates the usage of 2 
    elevators in a building of 8 floors (these values can be changed but certain values can 
    cause nonsensical output).
    The user can specifiy the starting floors of the two elevators, and from which 
    floors passengers are generated on. Default values start the two elevators with
    one on the ground floor, and the other on the top floor.

    Notes:  Command to compile (requires CSIM installed):
g++ -o ElevatorSimulator --std=c++11 -DCPP -DGPP -I/usr/csshare/pkgs/csim_cpp-19.0/lib -m32 /usr/csshare/pkgs/csim_cpp-19.0/lib/csim.cpp.a -lm ElevatorSimulator.cpp
*/

#include <iostream>
#include "cpp.h"
#include <string.h>
#include <algorithm> 
#include <string>
#include <vector>
using namespace std;

// Global variables
#define TINY 1.e-20     // A small time period
vector<long> floors_with_passengers; 
vector<long> passenger_group_sizes;
long num_of_floors = 8;
long num_of_elevators = 2;
long elev_0_start_floor = 0;
long elev_1_start_floor = 0;
long passenger_floor;
long passenger_group_size;
bool more_floors_to_add = true;
string user_menu_input;
string another_floor;

// Facilities
facility_set  *update_workload;

// Events
event wakeup ("wakeup");
event_set *heading_up;
event_set *heading_down;
event_set *here_is_floor; 
event_set *get_off_now;
event_set *boarded;
event_set *unloaded;

// Mailboxes
mailbox_set *mb_up;
mailbox_set *mb_dn;

// Logical Arrays
bool *want_up;
bool *want_dn;
bool** want_off;

// Functions
void generate_passengers(long which_floor);
void passenger_actions(long current_floor, long destination_floor); 
void elevator_actions(long which_elevator, long start_floor, bool using_default_loc);
void unload_passengers(long which_elevator, long current_elev_location, long &on_board);
void load_passengers(long which_elevator, long current_elev_location, long &on_board, long direction);

qtable elevator_occ("elevator occupancy");

// main process
extern "C" void sim()
{
    create("sim");

    // Creating all of the CSIM constructs
    mb_up = new mailbox_set("mb_up", num_of_floors);
    mb_dn = new mailbox_set("mb_dn", num_of_floors);
    boarded = new event_set("boarded", num_of_elevators);
    unloaded = new event_set("unloaded", num_of_elevators);
    heading_up = new event_set("heading_up", num_of_elevators);
    heading_down = new event_set("heading_down", num_of_elevators);
    get_off_now = new event_set("get_off_now", num_of_elevators);
    here_is_floor = new event_set("here_is_floor", num_of_elevators);
    update_workload = new facility_set("update_workload", num_of_elevators);

    // Dynamically creating and initializing boolean arrays
    want_up = new bool[num_of_floors];
    want_dn = new bool[num_of_floors];
    std::fill_n(want_up, num_of_floors, false);
    std::fill_n(want_dn, num_of_floors, false);

    want_off = new bool*[num_of_elevators];
    for(int i = 0; i < num_of_elevators; ++i)
        want_off[i] = new bool[num_of_floors];

    for(int i = 0; i < num_of_elevators; ++i)
        for(int j = 0; j < num_of_floors; ++j)
            want_off[i][j] = false;

    // Ask user if he wants to specify his own values for which floors 
    // elevators start on, and which floors generate a stream of passengers
    cout << "Do you want to specify specific elevator start locations.? (y/n) \n";
    cin >> user_menu_input;

    if (user_menu_input == "y")
    {
        cout << "Enter starting location for elevator 0.\n";
        cin >> elev_0_start_floor;

        cout << "Enter starting location for elevator 1.\n";
        cin >> elev_1_start_floor;

        while (more_floors_to_add)
        {
            cout << "Which floors do you want to generate passengers in? (0 to 7)\n";
            cin >> passenger_floor;
            floors_with_passengers.push_back(passenger_floor);

            cout << "How many passengers do you want to generate on this floor?\n";
            cin >> passenger_group_size;
            passenger_group_sizes.push_back(passenger_group_size);

            cout << "Do you want to add another floor?. (y/n)\n";
            cin >> another_floor;

            if (another_floor == "y")
                more_floors_to_add = true;
            else if (another_floor == "n")
                more_floors_to_add = false;
            else
                cout << "Incorrect input\n";
        }

        // Give passengers their destination floors and simulate their actions
        for (long v = 0; v < floors_with_passengers.size(); v++)
        { 
            long current_floor = floors_with_passengers[v];
            long group_size = passenger_group_sizes[v];
        
            // if current floor is the ground floor
            if (current_floor == 0)
            {
                for (long s = 0; s < group_size; s++) 
                {
                    long start_floor = 0;
                    long end_floor = uniform(1,7);
                    passenger_actions(start_floor, end_floor);
                }
            }
        
            else if (current_floor != 0)
            {
                for (long s = 0; s < group_size; s++) 
                {
                    long start_floor = current_floor;
                    long end_floor = uniform(1,7);
                    double y = prob();
                    
                    // 0.54 was the percentage of people I observed in a science
                    // building, who used the elevator to go to the ground floor
                    if (y < .54)
                    {
                        end_floor = start_floor;
                        // This while loop makes sure the start and end floors are different
                        while (start_floor == end_floor)
                        {
                            end_floor = uniform(1,7);
                        }
                    }
                    else
                    {
                        end_floor = 0;
                    }
                    passenger_actions(start_floor, end_floor);
                }
            }
        }

        elevator_actions(0, elev_0_start_floor, false);
        elevator_actions(1, elev_1_start_floor, false);
    }

    // If user does not want to specify his own values, use default values.
    else if (user_menu_input == "n")
    {
        for (int i = 0; i < num_of_floors; i++)
            generate_passengers(i);

        for (int j = 0; j < num_of_elevators; j++)
            elevator_actions(j, 0, true);  
    }

    // Hold for 1440 minutes (1 day), simulation ends after this
    hold(1440);

    // Freeing the dynamically allocated arrays
    delete [] want_up;
    delete [] want_dn;

    for(int i = 0; i < num_of_elevators; ++i)
        delete [] want_off[i];
    delete [] want_off;

    // Generating the CSIM output
    report_mailboxes();
    report();
}

void generate_passengers(long which_floor)
{
    create("passenger_generator"); 

    while (clock < 1440.)
    {
        hold(5);  // time between generated passengers at each floor

        long destination_floor = uniform_int(0, num_of_floors - 1);
        if (destination_floor != which_floor)  
        {
            passenger_actions(which_floor, destination_floor);
        }
    }
}

void passenger_actions(long current_floor, long destination_floor) 
{
  create("passenger_actions"); 
  long an_elevator;

    // passenger_actions going up
    if(current_floor < destination_floor)
    {
        want_up[current_floor] = true;
        wakeup.set();
        (*mb_up)[current_floor].receive(&an_elevator);
        (*update_workload)[an_elevator].reserve();
        (*heading_up)[an_elevator].queue();
        want_off[an_elevator][destination_floor] = true;
        (*boarded)[an_elevator].set(); 
        (*update_workload)[an_elevator].release();
        (*here_is_floor)[an_elevator].wait();
        (*get_off_now)[an_elevator].queue();
        (*unloaded)[an_elevator].set();
    }

    // passenger_actions going down
    if(current_floor > destination_floor)
    {
        want_dn[current_floor] = true;
        wakeup.set();
        (*mb_dn)[current_floor].receive(&an_elevator);
        (*heading_down)[an_elevator].queue(); 
        want_off[an_elevator][destination_floor] = true;
        (*boarded)[an_elevator].set(); 
        (*here_is_floor)[an_elevator].wait();
        (*get_off_now)[an_elevator].queue();
        (*unloaded)[an_elevator].set();
    }
}

void elevator_actions(long which_elevator, long start_floor, bool using_default_loc) 
{
    create ("elevator");

    long on_board = 0;
    long current_elev_location;
    long which_direction;   // 1 means going up, 0 means going down
    bool moving_up;
    bool moving_down;
    long travel_time;

    if (which_elevator % 2 == 0) // even numbered elevator; 0, 2, ...
    {
        if (using_default_loc == false)
            current_elev_location = start_floor;
        else if (using_default_loc == true)
            current_elev_location = 0;
    
        which_direction = 1;
        moving_up = true;
        moving_down = false;
    }
    
    else if (which_elevator % 2 != 0)  // odd numbered elevator; 1, 3, ...
    {
        if (using_default_loc == false)
            current_elev_location = start_floor;
        else if (using_default_loc == true)
            current_elev_location = num_of_floors - 1; 
        
        which_direction = 0;
        moving_up = false;
        moving_down = true;
    }

    // starts the elevator's behavior here
    while (clock < 1440.)
    {
        wakeup.wait();
        
        if (moving_up) 
        {
            // iterates thru each floor and first unloads and then 
            // loads the appropriate passengers at that floor
            for (int f = 0; f < num_of_floors; f++)
            {
                if (want_off[which_elevator][f])
                {
                    travel_time = 5 * sqrt(abs(f - current_elev_location));
                    hold(travel_time); 
                    current_elev_location = f; 
                    unload_passengers(which_elevator, current_elev_location, on_board);
                }

                if (want_up[f])
                {
                    if (current_elev_location != f) 
                    {
                        travel_time = 5 * sqrt(abs(f - current_elev_location));
                        hold(travel_time);    // time to reach that floor
                        current_elev_location = f;
                    }
                    load_passengers(which_elevator, current_elev_location, on_board, which_direction);
                }
            }
            moving_up = false;
            moving_down = true;
            which_direction = 0;
        }

        if (moving_down) 
        {
            for (int f = num_of_floors - 1; f >= 0; f--)
            {
                if (want_off[which_elevator][f])
                {
                    travel_time = 5 * sqrt(abs(f - current_elev_location));
                    hold(travel_time); 
                    current_elev_location = f;  
                    unload_passengers(which_elevator, current_elev_location, on_board);
                }

                if (want_dn[f])
                {
                    if (current_elev_location != f) 
                    {
                        travel_time = 5 * sqrt(abs(f - current_elev_location));
                        hold(travel_time); 
                        current_elev_location = f;
                    }
                    load_passengers(which_elevator, current_elev_location, on_board, which_direction);
                }
            }
            moving_up = true;
            moving_down = false;  
            which_direction = 1;   
        }
    }
}

void unload_passengers(long which_elevator, long current_elev_location, long &on_board)
{
    (*here_is_floor)[which_elevator].set();
    hold(TINY);
    while ((*get_off_now)[which_elevator].queue_cnt() > 0) 
    {
        (*get_off_now)[which_elevator].set();
        (*unloaded)[which_elevator].wait();
        on_board--;
    }
    want_off[which_elevator][current_elev_location] = false;
}

void load_passengers(long which_elevator, long current_elev_location, long &on_board, long direction)
{
    if (direction == 1) // elevator is moving up
    {
        while ((*mb_up)[current_elev_location].queue_cnt() > 0 ) 
        {
            (*mb_up)[current_elev_location].send(which_elevator);
            (*heading_up)[which_elevator].set();
            (*boarded)[which_elevator].wait();
            on_board++;
        }
        want_up[current_elev_location] = false;
    }

    else if (direction == 0) // elevator is moving down
    {
        while ((*mb_dn)[current_elev_location].queue_cnt() > 0 ) 
        {
            (*mb_dn)[current_elev_location].send(which_elevator);
            (*heading_down)[which_elevator].set();
            (*boarded)[which_elevator].wait();
            on_board++;
        }
        want_dn[current_elev_location] = false;   
    }
}