#include "scheduler.hpp"
#include <algorithm>
#include <gtest/gtest.h>
#include <limits>
#include <map>
#include <random>
#include <stdexcept>
using namespace std;

TEST(FCFS,ArrivalOrderAndMetrics){
    auto r=runFCFS({{1,0,5,0},{2,1,3,0},{3,2,1,0}});
    EXPECT_EQ(r.timeline,(vector<ExecutionSlice>{{1,0,5},{2,5,8},{3,8,9}}));
    EXPECT_EQ(r.completionOrder,(vector<int>{1,2,3}));
    EXPECT_EQ(r.processes[1].waitingTime,4);
    EXPECT_EQ(r.processes[2].turnaroundTime,7);
    EXPECT_DOUBLE_EQ(r.averageWaitingTime,10.0/3);
    EXPECT_DOUBLE_EQ(r.averageTurnaroundTime,19.0/3);
    EXPECT_DOUBLE_EQ(r.averageResponseTime,10.0/3);
}

TEST(FCFS,IdleAndSameArrivalTie){
    auto r=runFCFS({{3,7,1,0},{2,2,1,0},{1,2,1,0}});
    EXPECT_EQ(r.timeline,(vector<ExecutionSlice>{{-1,0,2},{1,2,3},{2,3,4},{-1,4,7},{3,7,8}}));
    EXPECT_EQ(r.contextSwitches,1);
    EXPECT_DOUBLE_EQ(r.cpuUtilization,37.5);
    EXPECT_DOUBLE_EQ(r.throughput,3.0/8);
}

TEST(SJF,OnlyReadyProcesses){
    auto r=runSJF({{1,0,8,0},{2,1,4,0},{3,2,2,0}});
    EXPECT_EQ(r.timeline,(vector<ExecutionSlice>{{1,0,8},{3,8,10},{2,10,14}}));
    EXPECT_EQ(r.processes[1].completionTime,14);
    EXPECT_EQ(r.processes[1].waitingTime,9);
    EXPECT_EQ(r.processes[2].responseTime,6);
    EXPECT_DOUBLE_EQ(r.averageWaitingTime,5);
    EXPECT_DOUBLE_EQ(r.averageTurnaroundTime,29.0/3);
}

TEST(SJF,EqualBurstTie){
    auto r=runSJF({{4,0,5,0},{3,2,2,0},{2,1,2,0},{1,1,2,0}});
    EXPECT_EQ(r.completionOrder,(vector<int>{4,1,2,3}));
}

TEST(SRTF,MultiplePreemptionsKeepFirstResponse){
    auto r=runSRTF({{1,0,8,0},{2,1,4,0},{3,2,1,0}});
    EXPECT_EQ(r.timeline,(vector<ExecutionSlice>{{1,0,1},{2,1,2},{3,2,3},{2,3,6},{1,6,13}}));
    EXPECT_EQ(r.processes[0].responseTime,0);
    EXPECT_EQ(r.processes[0].waitingTime,5);
    EXPECT_EQ(r.processes[1].waitingTime,1);
    EXPECT_DOUBLE_EQ(r.averageWaitingTime,2);
    EXPECT_EQ(r.contextSwitches,4);
}

TEST(SRTF,LongerAndEqualRemainingDoNotPreemptEarlierArrival){
    auto r=runSRTF({{3,0,4,0},{1,1,3,0},{2,2,9,0}});
    EXPECT_EQ(r.timeline,(vector<ExecutionSlice>{{3,0,4},{1,4,7},{2,7,16}}));
    EXPECT_EQ(runSRTF({{2,0,2,0},{1,0,2,0}}).completionOrder,(vector<int>{1,2}));
}

TEST(Priority,NonPreemptive){
    auto r=runPriority({{1,0,5,3},{2,1,2,2},{3,2,1,1}});
    EXPECT_EQ(r.timeline,(vector<ExecutionSlice>{{1,0,5},{3,5,6},{2,6,8}}));
}

TEST(Priority,PreemptiveAndEqualPriority){
    auto r=runPriority({{1,0,5,3},{2,1,3,1},{3,2,1,1}},true);
    EXPECT_EQ(r.timeline,(vector<ExecutionSlice>{{1,0,1},{2,1,4},{3,4,5},{1,5,9}}));
    EXPECT_EQ(r.processes[0].responseTime,0);
    EXPECT_EQ(r.processes[0].waitingTime,4);
    EXPECT_EQ(runPriority({{2,0,2,1},{1,0,2,1}},true).completionOrder,(vector<int>{1,2}));
}

TEST(RoundRobin,ArrivalsDuringAndAtQuantumBoundary){
    auto r=runRoundRobin({{1,0,5,0},{2,1,2,0},{3,2,1,0}},2);
    EXPECT_EQ(r.timeline,(vector<ExecutionSlice>{{1,0,2},{2,2,4},{3,4,5},{1,5,8}}));
    EXPECT_EQ(r.completionOrder,(vector<int>{2,3,1}));
    EXPECT_EQ(r.processes[0].waitingTime,3);
    EXPECT_EQ(r.processes[1].responseTime,1);
    EXPECT_EQ(r.contextSwitches,3);
}

TEST(RoundRobin,QuantumOneAndSameArrivalTie){
    auto r=runRoundRobin({{2,0,2,0},{1,0,2,0}},1);
    EXPECT_EQ(r.timeline,(vector<ExecutionSlice>{{1,0,1},{2,1,2},{1,2,3},{2,3,4}}));
}

TEST(RoundRobin,LargeQuantum){
    auto r=runRoundRobin({{1,0,3,0},{2,1,2,0}},20);
    EXPECT_EQ(r.timeline,(vector<ExecutionSlice>{{1,0,3},{2,3,5}}));
}

TEST(RoundRobin,SingleProcessAndIdle){
    auto r=runRoundRobin({{1,3,7,0}},2);
    EXPECT_EQ(r.timeline,(vector<ExecutionSlice>{{-1,0,3},{1,3,10}}));
    EXPECT_EQ(r.contextSwitches,0);
    EXPECT_EQ(r.processes[0].waitingTime,0);
    EXPECT_THROW(runRoundRobin({{1,0,2,0}},0),invalid_argument);
    EXPECT_THROW(runRoundRobin({{1,0,2,0}},-1),invalid_argument);
}

TEST(MLFQ,DemotionsAndFCFSBottomQueue){
    auto r=runMLFQ({{1,0,9,0},{2,0,8,0},{3,0,1,0}});
    EXPECT_EQ(r.timeline,(vector<ExecutionSlice>{{1,0,2},{2,2,4},{3,4,5},
        {1,5,9},{2,9,13},{1,13,16},{2,16,18}}));
    EXPECT_EQ(r.completionOrder,(vector<int>{3,1,2}));
    EXPECT_EQ(r.processes[0].waitingTime,7);
    EXPECT_EQ(r.processes[1].responseTime,2);
}

TEST(MLFQ,PreemptsQ1AndPreservesBudget){
    auto r=runMLFQ({{1,0,10,0},{2,3,1,0},{3,4,3,0}});
    EXPECT_EQ(r.timeline,(vector<ExecutionSlice>{{1,0,3},{2,3,4},{3,4,6},
        {1,6,9},{3,9,10},{1,10,14}}));
    EXPECT_EQ(r.processes[0].responseTime,0);
}

TEST(MLFQ,PreemptsQ2AndKeepsItsFCFSPosition){
    auto r=runMLFQ({{1,0,10,0},{2,0,8,0},{3,13,1,0}});
    EXPECT_EQ(r.timeline,(vector<ExecutionSlice>{{1,0,2},{2,2,4},{1,4,8},
        {2,8,12},{1,12,13},{3,13,14},{1,14,17},{2,17,19}}));
}

TEST(MLFQ,ShortJobAndInitialIdle){
    auto r=runMLFQ({{1,4,1,0},{2,4,2,0}});
    EXPECT_EQ(r.timeline,(vector<ExecutionSlice>{{-1,0,4},{1,4,5},{2,5,7}}));
    EXPECT_EQ(r.completionOrder,(vector<int>{1,2}));
}

TEST(ContextSwitch,IdleAndSameProcessAreFree){
    auto r=runFCFS({{1,2,1,0},{2,7,1,0}},5);
    EXPECT_EQ(r.timeline,(vector<ExecutionSlice>{{-1,0,2},{1,2,3},{-1,3,7},{2,7,8}}));
    EXPECT_EQ(r.contextSwitches,0);
    auto rr=runRoundRobin({{1,0,5,0}},1,5);
    EXPECT_EQ(rr.timeline,(vector<ExecutionSlice>{{1,0,5}}));
    EXPECT_EQ(rr.contextSwitches,0);
    EXPECT_EQ(runMLFQ({{1,0,10,0}},5).contextSwitches,0);
    EXPECT_THROW(runFCFS({{1,0,1,0}},-1),invalid_argument);
}

TEST(ContextSwitch,ReselectsAfterOverhead){
    auto r=runSRTF({{1,0,8,0},{2,1,4,0},{3,2,1,0}},2);
    EXPECT_EQ(r.timeline,(vector<ExecutionSlice>{{1,0,1},{-2,1,3},{3,3,4},
        {-2,4,6},{2,6,10},{-2,10,12},{1,12,19}}));
    EXPECT_EQ(r.contextSwitches,3);
    EXPECT_EQ(r.processes[1].firstRunTime,6);
    EXPECT_EQ(r.processes[0].responseTime,0);
}

TEST(ContextSwitch,RoundRobinEnqueuesDuringOverhead){
    auto r=runRoundRobin({{1,0,3,0},{2,1,1,0},{3,3,1,0}},2,2);
    EXPECT_EQ(r.timeline,(vector<ExecutionSlice>{{1,0,2},{-2,2,4},{2,4,5},
        {-2,5,7},{1,7,8},{-2,8,10},{3,10,11}}));
}

TEST(ContextSwitch,MLFQArrivalDuringSwitchToLowerQueue){
    auto r=runMLFQ({{1,0,8,0},{2,0,1,0},{3,6,1,0}},2);
    EXPECT_EQ(r.timeline,(vector<ExecutionSlice>{{1,0,2},{-2,2,4},{2,4,5},
        {-2,5,7},{3,7,8},{-2,8,10},{1,10,16}}));
    EXPECT_EQ(r.contextSwitches,3);
}

TEST(Comparison,FreshWorkloadsAndConsistentResults){
    vector<Process>p={{1,0,8,3},{2,1,4,1},{3,2,1,2}};
    auto results=compareSchedulers(p,3,1);
    ASSERT_EQ(results.size(),7u);
    EXPECT_EQ(results[0].timeline,runFCFS(p,1).timeline);
    EXPECT_EQ(results[1].timeline,runSJF(p,1).timeline);
    EXPECT_EQ(results[2].timeline,runSRTF(p,1).timeline);
    EXPECT_EQ(results[3].timeline,runPriority(p,false,1).timeline);
    EXPECT_EQ(results[4].timeline,runPriority(p,true,1).timeline);
    EXPECT_EQ(results[5].timeline,runRoundRobin(p,3,1).timeline);
    EXPECT_EQ(results[6].timeline,runMLFQ(p,1).timeline);
    for(auto &it:p){
        EXPECT_EQ(it.firstRunTime,-1);
        EXPECT_EQ(it.remainingTime,0);
    }
    auto again=compareSchedulers(results[0].processes,3,1);
    for(size_t i=0;i<results.size();i++){
        EXPECT_EQ(results[i].timeline,again[i].timeline);
        EXPECT_DOUBLE_EQ(results[i].averageResponseTime,again[i].averageResponseTime);
    }
}

TEST(AllSchedulers,SingleProcessAndIdleGap){
    for(auto &r:compareSchedulers({{7,3,7,0},{2,20,1,0}},1,9)){
        SCOPED_TRACE(r.algorithmName);
        EXPECT_EQ(r.timeline,(vector<ExecutionSlice>{{-1,0,3},{7,3,10},{-1,10,20},{2,20,21}}));
        EXPECT_EQ(r.contextSwitches,0);
        EXPECT_DOUBLE_EQ(r.averageWaitingTime,0);
        EXPECT_DOUBLE_EQ(r.averageResponseTime,0);
        EXPECT_DOUBLE_EQ(r.averageTurnaroundTime,4);
        EXPECT_DOUBLE_EQ(r.cpuUtilization,8.0/21*100);
    }
}

TEST(AllSchedulers,ZeroCostStillCountsProcessChanges){
    for(auto &r:compareSchedulers({{2,0,1,0},{1,0,1,0}})){
        SCOPED_TRACE(r.algorithmName);
        EXPECT_EQ(r.timeline,(vector<ExecutionSlice>{{1,0,1},{2,1,2}}));
        EXPECT_EQ(r.contextSwitches,1);
        EXPECT_DOUBLE_EQ(r.cpuUtilization,100);
    }
}

TEST(ContextSwitch,NonPreemptiveDispatchIncludesOverheadArrivals){
    auto r=runSJF({{1,0,2,0},{2,0,5,0},{3,3,1,0}},2);
    EXPECT_EQ(r.timeline,(vector<ExecutionSlice>{{1,0,2},{-2,2,4},{3,4,5},{-2,5,7},{2,7,12}}));
    auto p=runPriority({{1,0,2,0},{2,0,5,2},{3,3,1,1}},false,2);
    EXPECT_EQ(p.timeline,r.timeline);
}

TEST(ContextSwitch,PreemptivePriorityArrivalDuringOverhead){
    auto r=runPriority({{1,0,4,3},{2,1,2,2},{3,2,1,1}},true,2);
    EXPECT_EQ(r.timeline,(vector<ExecutionSlice>{{1,0,1},{-2,1,3},{3,3,4},
        {-2,4,6},{2,6,8},{-2,8,10},{1,10,13}}));
    EXPECT_EQ(r.processes[1].responseTime,5);
    EXPECT_EQ(r.processes[0].waitingTime,9);
}

TEST(MLFQ,ArrivalsExactlyAtDemotionBoundaries){
    auto q0=runMLFQ({{1,0,5,0},{2,2,1,0}});
    EXPECT_EQ(q0.timeline,(vector<ExecutionSlice>{{1,0,2},{2,2,3},{1,3,6}}));
    auto q1=runMLFQ({{1,0,8,0},{2,6,1,0}});
    EXPECT_EQ(q1.timeline,(vector<ExecutionSlice>{{1,0,6},{2,6,7},{1,7,9}}));
}

TEST(Validation,TimeOverflowThrowsInsteadOfWrapping){
    Time limit=numeric_limits<Time>::max();
    vector<Process>p={{1,limit,1,0}};
    EXPECT_THROW(runFCFS(p),overflow_error);
    EXPECT_THROW(runSJF(p),overflow_error);
    EXPECT_THROW(runSRTF(p),overflow_error);
    EXPECT_THROW(runPriority(p),overflow_error);
    EXPECT_THROW(runPriority(p,true),overflow_error);
    EXPECT_THROW(runRoundRobin(p),overflow_error);
    EXPECT_THROW(runMLFQ(p),overflow_error);
    EXPECT_THROW(runFCFS({{1,0,2,0},{2,0,1,0}},limit),overflow_error);
    EXPECT_EQ(runFCFS({{1,limit-1,1,0}}).processes[0].completionTime,limit);
}

TEST(AllSchedulers,RandomWorkloadsConserveCPUTimeAndMetrics){
    mt19937 gen(2026);
    for(int test=0;test<100;test++){
        vector<Process>p;
        int n=1+gen()%8;
        Time cost=gen()%4,quantum=1+gen()%5;
        for(int i=0;i<n;i++){
            p.push_back({i,static_cast<Time>(gen()%20),1+static_cast<Time>(gen()%12),static_cast<int>(gen()%7)-3});
        }
        auto results=compareSchedulers(p,quantum,cost);
        shuffle(p.begin(),p.end(),gen);
        auto shuffled=compareSchedulers(p,quantum,cost);
        for(size_t i=0;i<results.size();i++){
            auto &r=results[i];
            SCOPED_TRACE(to_string(test)+" "+r.algorithmName);
            EXPECT_EQ(r.timeline,shuffled[i].timeline);
            map<int,Time>ran,first,last;
            Time time=0,busy=0,overhead=0,switches=0;
            int prev=-1;
            for(auto &s:r.timeline){
                ASSERT_EQ(s.startTime,time);
                ASSERT_GT(s.endTime,s.startTime);
                Time duration=s.endTime-s.startTime;
                time=s.endTime;
                if(s.pid==-1){
                    for(auto &it:r.processes){
                        EXPECT_TRUE(ran[it.pid]==it.burstTime||it.arrivalTime>=s.endTime);
                    }
                    prev=-1;
                }else if(s.pid==-2){
                    EXPECT_EQ(duration,cost);
                    EXPECT_NE(prev,-1);
                    overhead+=duration;
                }else{
                    ASSERT_GE(s.pid,0);
                    ASSERT_LT(s.pid,n);
                    if(prev!=-1&&prev!=s.pid)switches++;
                    prev=s.pid;
                    if(!first.count(s.pid))first[s.pid]=s.startTime;
                    last[s.pid]=s.endTime;
                    ran[s.pid]+=duration;
                    busy+=duration;
                }
            }
            EXPECT_EQ(switches,r.contextSwitches);
            EXPECT_EQ(overhead,cost*switches);
            double wt=0,tat=0,rt=0;
            vector<pair<Time,int>>finished;
            for(auto &it:r.processes){
                EXPECT_EQ(ran[it.pid],it.burstTime);
                EXPECT_GE(first[it.pid],it.arrivalTime);
                EXPECT_EQ(it.firstRunTime,first[it.pid]);
                EXPECT_EQ(it.completionTime,last[it.pid]);
                EXPECT_EQ(it.remainingTime,0);
                EXPECT_EQ(it.turnaroundTime,last[it.pid]-it.arrivalTime);
                EXPECT_EQ(it.waitingTime,last[it.pid]-it.arrivalTime-ran[it.pid]);
                EXPECT_EQ(it.responseTime,first[it.pid]-it.arrivalTime);
                EXPECT_GE(it.waitingTime,it.responseTime);
                wt+=it.waitingTime;
                tat+=it.turnaroundTime;
                rt+=it.responseTime;
                finished.push_back({last[it.pid],it.pid});
            }
            sort(finished.begin(),finished.end());
            vector<int>order;
            for(auto &it:finished)order.push_back(it.second);
            EXPECT_EQ(r.completionOrder,order);
            EXPECT_DOUBLE_EQ(r.averageWaitingTime,wt/n);
            EXPECT_DOUBLE_EQ(r.averageTurnaroundTime,tat/n);
            EXPECT_DOUBLE_EQ(r.averageResponseTime,rt/n);
            EXPECT_DOUBLE_EQ(r.throughput,static_cast<double>(n)/time);
            EXPECT_DOUBLE_EQ(r.cpuUtilization,static_cast<double>(busy)/time*100);
        }
    }
}
