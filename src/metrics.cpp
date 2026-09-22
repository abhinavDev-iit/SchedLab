#include "metrics.hpp"
#include <set>
#include <stdexcept>
using namespace std;

void validateWorkload(const vector<Process> &processes){
    if(processes.empty())throw invalid_argument("workload is empty");
    set<int>seen;
    for(auto &p:processes){
        if(p.pid<0)throw invalid_argument("PID must be nonnegative");
        if(!seen.insert(p.pid).second)throw invalid_argument("duplicate PID");
        if(p.arrivalTime<0)throw invalid_argument("negative arrival time");
        if(p.burstTime<=0)throw invalid_argument("burst time must be positive");
    }
}

void calculateMetrics(SimulationResult &result){
    result.averageWaitingTime=0;
    result.averageTurnaroundTime=0;
    result.averageResponseTime=0;
    result.throughput=0;
    result.cpuUtilization=0;
    double busy=0;
    for(auto &p:result.processes){
        p.turnaroundTime=p.completionTime-p.arrivalTime;
        p.waitingTime=p.turnaroundTime-p.burstTime;
        p.responseTime=p.firstRunTime-p.arrivalTime;
        result.averageWaitingTime+=p.waitingTime;
        result.averageTurnaroundTime+=p.turnaroundTime;
        result.averageResponseTime+=p.responseTime;
        busy+=p.burstTime;
    }
    if(result.processes.empty())return;
    double n=static_cast<double>(result.processes.size());
    result.averageWaitingTime/=n;
    result.averageTurnaroundTime/=n;
    result.averageResponseTime/=n;
    if(!result.timeline.empty()&&result.timeline.back().endTime>0){
        double elapsed=static_cast<double>(result.timeline.back().endTime);
        result.throughput=n/elapsed;
        result.cpuUtilization=busy/elapsed*100;
    }
}
