#pragma once
#include "process.hpp"

SimulationResult runFCFS(std::vector<Process> processes,Time contextSwitchCost=0);
SimulationResult runSJF(std::vector<Process> processes,Time contextSwitchCost=0);
SimulationResult runSRTF(std::vector<Process> processes,Time contextSwitchCost=0);
SimulationResult runPriority(std::vector<Process> processes,bool preemptive=false,Time contextSwitchCost=0);
SimulationResult runRoundRobin(std::vector<Process> processes,Time quantum=2,Time contextSwitchCost=0);
SimulationResult runMLFQ(std::vector<Process> processes,Time contextSwitchCost=0);
