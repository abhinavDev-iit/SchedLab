#include "scheduler.hpp"
#include <gtest/gtest.h>
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
