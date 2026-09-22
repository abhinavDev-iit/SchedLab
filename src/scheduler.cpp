#include "scheduler.hpp"
#include "metrics.hpp"
#include <algorithm>
#include <deque>
#include <limits>
#include <stdexcept>
#include <utility>
using namespace std;

namespace{
enum class Policy{FCFS,SJF,SRTF,Priority};

SimulationResult prepare(vector<Process> processes,const string &name,Time cost){
    if(cost<0)throw invalid_argument("context switch cost must be nonnegative");
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

void switchProcess(SimulationResult &ans,int prev,int ind,Time &time,Time cost){
    if(prev==-1||prev==ind)return;
    ans.contextSwitches++;
    addSlice(ans,-2,time,cost);
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

vector<int> arrivalOrder(const vector<Process> &processes){
    vector<int>order;
    for(int i=0;i<static_cast<int>(processes.size());i++)order.push_back(i);
    sort(order.begin(),order.end(),[&](int a,int b){
        return better(processes[a],processes[b],Policy::FCFS);
    });
    return order;
}

void enqueueArrivals(const vector<Process> &processes,const vector<int> &order,
                     size_t &pos,Time time,deque<int> &q){
    while(pos<order.size()&&processes[order[pos]].arrivalTime<=time){
        q.push_back(order[pos++]);
    }
}

int selectReady(const vector<Process> &processes,Time time,Policy policy,Time &next){
    int ind=-1;
    next=numeric_limits<Time>::max();
    for(int i=0;i<static_cast<int>(processes.size());i++){
        auto &p=processes[i];
        if(p.remainingTime==0)continue;
        if(p.arrivalTime>time){
            next=min(next,p.arrivalTime);
        }else if(ind==-1||better(p,processes[ind],policy)){
            ind=i;
        }
    }
    return ind;
}

SimulationResult runSelected(vector<Process> processes,Policy policy,const string &name,Time cost,bool preemptive=false){
    auto ans=prepare(move(processes),name,cost);
    Time time=0;
    int prev=-1;
    while(ans.completionOrder.size()<ans.processes.size()){
        Time next;
        int ind=selectReady(ans.processes,time,policy,next);
        if(ind==-1){
            addSlice(ans,-1,time,next-time);
            prev=-1;
            continue;
        }
        switchProcess(ans,prev,ind,time,cost);
        // Arrivals during overhead participate in the final dispatch decision.
        ind=selectReady(ans.processes,time,policy,next);
        Time duration=ans.processes[ind].remainingTime;
        if(preemptive&&next>time)duration=min(duration,next-time);
        execute(ans,ind,time,duration);
        prev=ind;
    }
    calculateMetrics(ans);
    return ans;
}
}

SimulationResult runFCFS(vector<Process> processes,Time contextSwitchCost){
    return runSelected(move(processes),Policy::FCFS,"FCFS",contextSwitchCost);
}

SimulationResult runSJF(vector<Process> processes,Time contextSwitchCost){
    return runSelected(move(processes),Policy::SJF,"SJF",contextSwitchCost);
}

SimulationResult runSRTF(vector<Process> processes,Time contextSwitchCost){
    return runSelected(move(processes),Policy::SRTF,"SRTF",contextSwitchCost,true);
}

SimulationResult runPriority(vector<Process> processes,bool preemptive,Time contextSwitchCost){
    return runSelected(move(processes),Policy::Priority,preemptive?"Priority-P":"Priority-NP",contextSwitchCost,preemptive);
}

namespace{
SimulationResult runQueued(vector<Process> processes,Time quantum,bool mlfq,Time cost){
    if(quantum<=0)throw invalid_argument("quantum must be positive");
    auto ans=prepare(move(processes),mlfq?"MLFQ":"RR",cost);
    auto order=arrivalOrder(ans.processes);
    deque<int>q[3];
    vector<Time>budget(ans.processes.size(),quantum);
    size_t pos=0;
    Time time=0;
    int prev=-1;
    while(ans.completionOrder.size()<ans.processes.size()){
        enqueueArrivals(ans.processes,order,pos,time,q[0]);
        int level=0;
        while(level<3&&q[level].empty())level++;
        if(level==3){
            addSlice(ans,-1,time,ans.processes[order[pos]].arrivalTime-time);
            prev=-1;
            continue;
        }
        int ind=q[level].front();
        switchProcess(ans,prev,ind,time,cost);
        enqueueArrivals(ans.processes,order,pos,time,q[0]);
        level=0;
        while(q[level].empty())level++;
        ind=q[level].front();
        q[level].pop_front();
        Time duration=ans.processes[ind].remainingTime;
        if(level<2)duration=min(duration,budget[ind]);
        if(mlfq&&level>0&&pos<order.size()){
            duration=min(duration,ans.processes[order[pos]].arrivalTime-time);
        }
        execute(ans,ind,time,duration);
        if(level<2)budget[ind]-=duration;
        enqueueArrivals(ans.processes,order,pos,time,q[0]);
        if(ans.processes[ind].remainingTime>0){
            if(level<2&&budget[ind]==0){
                int next=mlfq?level+1:0;
                budget[ind]=mlfq?4:quantum;
                q[next].push_back(ind);
            }else{
                q[level].push_front(ind);
            }
        }
        prev=ind;
    }
    calculateMetrics(ans);
    return ans;
}
}

SimulationResult runRoundRobin(vector<Process> processes,Time quantum,Time contextSwitchCost){
    return runQueued(move(processes),quantum,false,contextSwitchCost);
}

SimulationResult runMLFQ(vector<Process> processes,Time contextSwitchCost){
    return runQueued(move(processes),2,true,contextSwitchCost);
}

vector<SimulationResult> compareSchedulers(const vector<Process> &processes,Time quantum,Time contextSwitchCost){
    if(quantum<=0)throw invalid_argument("quantum must be positive");
    return {runFCFS(processes,contextSwitchCost),runSJF(processes,contextSwitchCost),
        runSRTF(processes,contextSwitchCost),runPriority(processes,false,contextSwitchCost),
        runPriority(processes,true,contextSwitchCost),runRoundRobin(processes,quantum,contextSwitchCost),
        runMLFQ(processes,contextSwitchCost)};
}
