#include "cli.hpp"
#include <gtest/gtest.h>
#include <sstream>
#include <stdexcept>
using namespace std;

TEST(Workload,ReadsCommentsAndBlankLines){
    istringstream input("# header\n\n1 0 8 2 # long job\n2 1 4 -1\r\n");
    auto p=readWorkload(input);
    ASSERT_EQ(p.size(),2u);
    EXPECT_EQ(p[0].burstTime,8);
    EXPECT_EQ(p[1].priority,-1);
}

TEST(Workload,RejectsMalformedRows){
    for(auto text:{"1 0 2","1 0 2 3 extra","1 0 x 3","1 0 2 3.5",
                   "2147483648 0 1 0","1 0 9223372036854775808 0","# empty"}){
        istringstream input(text);
        EXPECT_THROW(readWorkload(input),invalid_argument)<<text;
    }
}

TEST(CLI,ReportsErrors){
    vector<vector<string>>cases={
        {"--algorithm","unknown","--input","unused"},
        {"--algorithm","fcfs","--input","missing-file-for-test.txt"},
        {"--algorithm","rr","--quantum","0","--input","unused"},
        {"--algorithm","rr","--quantum","2x","--input","unused"},
        {"--algorithm","fcfs","--preemptive","--input","unused"},
        {"--algorithm","fcfs","--context-switch","-1","--input","unused"},
        {"--algorithm"},{"--input","unused"},{"--wat"}
    };
    for(auto &args:cases){
        ostringstream out,err;
        EXPECT_EQ(runCLI(args,out,err),1);
        EXPECT_NE(err.str().find("Error:"),string::npos);
        EXPECT_TRUE(out.str().empty());
    }
}

TEST(CLI,Help){
    ostringstream out,err;
    EXPECT_EQ(runCLI({"--help"},out,err),0);
    EXPECT_NE(out.str().find("--algorithm"),string::npos);
    EXPECT_TRUE(err.str().empty());
}
