#pragma once
#include <string>
#include <vector>

using Time=long long;

struct Process{
    int pid;
    Time arrivalTime;
    Time burstTime;
    int priority;
    Time remainingTime=0;
    Time firstRunTime=-1;
    Time completionTime=0;
    Time waitingTime=0;
    Time turnaroundTime=0;
    Time responseTime=0;
};

struct ExecutionSlice{
    int pid;
    Time startTime;
    Time endTime;
    bool operator==(const ExecutionSlice&) const=default;
};

struct SimulationResult{
    std::string algorithmName;
    std::vector<Process> processes;
    std::vector<ExecutionSlice> timeline;
    std::vector<int> completionOrder;
    double averageWaitingTime=0;
    double averageTurnaroundTime=0;
    double averageResponseTime=0;
    double throughput=0;
    double cpuUtilization=0;
    Time contextSwitches=0;
};

void validateWorkload(const std::vector<Process> &processes);
