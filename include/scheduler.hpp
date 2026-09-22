#pragma once
#include "process.hpp"

SimulationResult runFCFS(std::vector<Process> processes);
SimulationResult runSJF(std::vector<Process> processes);
SimulationResult runSRTF(std::vector<Process> processes);
SimulationResult runPriority(std::vector<Process> processes,bool preemptive=false);
