#include "scheduler.hpp"
#include "metrics.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>
#include <utility>
using namespace std;

namespace{
enum class Policy{FCFS,SJF,SRTF,Priority};

SimulationResult prepare(vector<Process> processes,const string &name){
    validateWorkload(processes);
    for(auto &p:processes){
        p.remainingTime=p.burstTime;
        p.firstRunTime=-1;
        p.completionTime=0;
        p.waitingTime=0;
        p.turnaroundTime=0;
        p.responseTime=0;
    }
    SimulationResult ans;
    ans.algorithmName=name;
    ans.processes=move(processes);
    return ans;
}

void addSlice(SimulationResult &ans,int pid,Time &time,Time duration){
    if(duration<=0)return;
    if(time>numeric_limits<Time>::max()-duration){
        throw overflow_error("simulated time exceeds 64-bit range");
    }
    Time end=time+duration;
    if(!ans.timeline.empty()&&ans.timeline.back().pid==pid){
        ans.timeline.back().endTime=end;
    }else{
        ans.timeline.push_back({pid,time,end});
    }
    time=end;
}

void execute(SimulationResult &ans,int ind,Time &time,Time duration){
    auto &p=ans.processes[ind];
    if(p.firstRunTime==-1)p.firstRunTime=time;
    addSlice(ans,p.pid,time,duration);
    p.remainingTime-=duration;
    if(p.remainingTime==0){
        p.completionTime=time;
        ans.completionOrder.push_back(p.pid);
    }
}

bool better(const Process &a,const Process &b,Policy policy){
    if(policy==Policy::SRTF&&a.remainingTime!=b.remainingTime){
        return a.remainingTime<b.remainingTime;
    }
    if(policy==Policy::Priority&&a.priority!=b.priority){
        return a.priority<b.priority;
    }
    if(policy==Policy::SJF&&a.burstTime!=b.burstTime){
        return a.burstTime<b.burstTime;
    }
    if(a.arrivalTime!=b.arrivalTime)return a.arrivalTime<b.arrivalTime;
    return a.pid<b.pid;
}

SimulationResult runSelected(vector<Process> processes,Policy policy,const string &name,bool preemptive=false){
    auto ans=prepare(move(processes),name);
    Time time=0;
    int prev=-1;
    while(ans.completionOrder.size()<ans.processes.size()){
        int ind=-1;
        Time next=numeric_limits<Time>::max();
        for(int i=0;i<static_cast<int>(ans.processes.size());i++){
            auto &p=ans.processes[i];
            if(p.remainingTime==0)continue;
            if(p.arrivalTime>time){
                next=min(next,p.arrivalTime);
            }else if(ind==-1||better(p,ans.processes[ind],policy)){
                ind=i;
            }
        }
        if(ind==-1){
            addSlice(ans,-1,time,next-time);
            prev=-1;
            continue;
        }
        if(prev!=-1&&prev!=ind)ans.contextSwitches++;
        Time duration=ans.processes[ind].remainingTime;
        if(preemptive&&next>time)duration=min(duration,next-time);
        execute(ans,ind,time,duration);
        prev=ind;
    }
    calculateMetrics(ans);
    return ans;
}
}

SimulationResult runFCFS(vector<Process> processes){
    return runSelected(move(processes),Policy::FCFS,"FCFS");
}

SimulationResult runSJF(vector<Process> processes){
    return runSelected(move(processes),Policy::SJF,"SJF");
}

SimulationResult runSRTF(vector<Process> processes){
    return runSelected(move(processes),Policy::SRTF,"SRTF",true);
}

SimulationResult runPriority(vector<Process> processes,bool preemptive){
    return runSelected(move(processes),Policy::Priority,preemptive?"Priority-P":"Priority-NP",preemptive);
}
