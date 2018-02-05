# ElevatorSimulator

## Description
This is a CSIM program that models the usage of multiple elevators in a multi-story building. Statistics are generated regarding the usage of these elevators.
I used the rate of people using the elevator in Wingston Chung Hall at UC Riverside over the course of 3 hours to come up with a rate to spawn elevator passengers.

## Output
![elevatorsimulatoroutput](https://user-images.githubusercontent.com/35906533/35829265-2c51c8f8-0a77-11e8-8b59-9d85c27a2efe.PNG)

## Notes
Requires CSIM installed for compilation, the command for compile then is:
g++ -o ElevatorSimulator --std=c++11 -DCPP -DGPP -I/usr/csshare/pkgs/csim_cpp-19.0/lib -m32 /usr/csshare/pkgs/csim_cpp-19.0/lib/csim.cpp.a -l
