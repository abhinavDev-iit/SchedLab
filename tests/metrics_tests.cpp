#include "metrics.hpp"
#include "scheduler.hpp"
#include <gtest/gtest.h>
#include <stdexcept>

TEST(Validation,RejectsInvalidWorkloads){
    EXPECT_THROW(validateWorkload({}),std::invalid_argument);
    EXPECT_THROW(validateWorkload({{1,-1,2,0}}),std::invalid_argument);
    EXPECT_THROW(validateWorkload({{1,0,0,0}}),std::invalid_argument);
    EXPECT_THROW(validateWorkload({{1,0,-2,0}}),std::invalid_argument);
    EXPECT_THROW(validateWorkload({{-1,0,2,0}}),std::invalid_argument);
    EXPECT_THROW(validateWorkload({{1,0,2,0},{1,2,3,0}}),std::invalid_argument);
    EXPECT_NO_THROW(validateWorkload({{0,0,2,-5},{1,2,3,0}}));
}

TEST(Metrics,HandCalculatedWithIdle){
    SimulationResult r;
    r.processes={{1,2,3,0},{2,3,2,0}};
    r.processes[0].firstRunTime=2;
    r.processes[0].completionTime=5;
    r.processes[1].firstRunTime=5;
    r.processes[1].completionTime=7;
    r.timeline={{-1,0,2},{1,2,5},{2,5,7}};
    calculateMetrics(r);
    EXPECT_EQ(r.processes[1].turnaroundTime,4);
    EXPECT_EQ(r.processes[1].waitingTime,2);
    EXPECT_EQ(r.processes[1].responseTime,2);
    EXPECT_DOUBLE_EQ(r.averageWaitingTime,1);
    EXPECT_DOUBLE_EQ(r.averageTurnaroundTime,3.5);
    EXPECT_DOUBLE_EQ(r.averageResponseTime,1);
    EXPECT_DOUBLE_EQ(r.throughput,2.0/7);
    EXPECT_DOUBLE_EQ(r.cpuUtilization,5.0/7*100);
    calculateMetrics(r);
    EXPECT_DOUBLE_EQ(r.averageWaitingTime,1);
}

TEST(Metrics,ContextOverheadAffectsAllMetrics){
    auto r=runFCFS({{1,2,3,0},{2,3,2,0}},1);
    EXPECT_EQ(r.timeline,(std::vector<ExecutionSlice>{{-1,0,2},{1,2,5},{-2,5,6},{2,6,8}}));
    EXPECT_EQ(r.processes[1].completionTime,8);
    EXPECT_EQ(r.processes[1].turnaroundTime,5);
    EXPECT_EQ(r.processes[1].waitingTime,3);
    EXPECT_EQ(r.processes[1].responseTime,3);
    EXPECT_DOUBLE_EQ(r.averageWaitingTime,1.5);
    EXPECT_DOUBLE_EQ(r.averageTurnaroundTime,4);
    EXPECT_DOUBLE_EQ(r.averageResponseTime,1.5);
    EXPECT_DOUBLE_EQ(r.throughput,0.25);
    EXPECT_DOUBLE_EQ(r.cpuUtilization,62.5);
    EXPECT_EQ(r.contextSwitches,1);
}
