#include "cli.hpp"
#include "scheduler.hpp"
#include <charconv>
#include <fstream>
#include <iomanip>
#include <limits>
#include <set>
#include <sstream>
#include <stdexcept>
using namespace std;

namespace{
Time number(const string &s,const string &name){
    Time value=0;
    auto [end,error]=from_chars(s.data(),s.data()+s.size(),value);
    if(error!=errc{}||end!=s.data()+s.size()){
        throw invalid_argument("invalid integer for "+name+": "+s);
    }
    return value;
}

void printResult(const SimulationResult &r,ostream &out){
    out<<"Algorithm: "<<r.algorithmName<<"\n\nTimeline:\n";
    for(auto &s:r.timeline){
        string label=s.pid==-1?"IDLE":s.pid==-2?"SWITCH":"P"+to_string(s.pid);
        out<<"["<<s.startTime<<","<<s.endTime<<") "<<label<<"\n";
    }
    out<<"Completion order:";
    for(int pid:r.completionOrder)out<<" P"<<pid;
    out<<"\n\nProcess AT BT Priority CT TAT WT RT\n";
    for(auto &p:r.processes){
        out<<"P"<<p.pid<<" "<<p.arrivalTime<<" "<<p.burstTime<<" "<<p.priority<<" "
           <<p.completionTime<<" "<<p.turnaroundTime<<" "<<p.waitingTime<<" "<<p.responseTime<<"\n";
    }
    out<<fixed<<setprecision(3)
       <<"\nAverage waiting time: "<<r.averageWaitingTime
       <<"\nAverage turnaround time: "<<r.averageTurnaroundTime
       <<"\nAverage response time: "<<r.averageResponseTime
       <<"\nCPU utilization: "<<r.cpuUtilization<<"%"
       <<"\nThroughput: "<<r.throughput<<" processes/time unit"
       <<"\nContext switches: "<<r.contextSwitches<<"\n";
}

void printHelp(ostream &out){
    out<<"SchedLab - C++20 CPU scheduling simulator\n"
       <<"Usage: schedlab --algorithm NAME --input FILE [options]\n"
       <<"       schedlab --compare --input FILE [--quantum N] [--context-switch N]\n"
       <<"Algorithms: fcfs, sjf, srtf, priority, rr, mlfq\n"
       <<"Options:\n"
       <<"  --preemptive       Preemptive priority scheduling\n"
       <<"  --quantum N        Round Robin quantum (default 2)\n"
       <<"  --context-switch N Switch overhead (default 0)\n"
       <<"  --help             Show this help\n"
       <<"Workload rows: pid arrival burst priority; # begins a comment.\n";
}
}

vector<Process> readWorkload(istream &input){
    vector<Process>processes;
    string line;
    int lineNo=0;
    while(getline(input,line)){
        lineNo++;
        auto pos=line.find('#');
        if(pos!=string::npos)line.erase(pos);
        istringstream row(line);
        string a,b,c,d,extra;
        if(!(row>>a))continue;
        try{
            if(!(row>>b>>c>>d)||(row>>extra)){
                throw invalid_argument("expected four integers: pid arrival burst priority");
            }
            Time pid=number(a,"PID"),priority=number(d,"priority");
            if(pid<0||pid>numeric_limits<int>::max()){
                throw invalid_argument("PID must be a nonnegative 32-bit integer");
            }
            if(priority<numeric_limits<int>::min()||priority>numeric_limits<int>::max()){
                throw invalid_argument("priority must be a 32-bit integer");
            }
            processes.push_back({static_cast<int>(pid),number(b,"arrival"),number(c,"burst"),static_cast<int>(priority)});
        }catch(const exception &e){
            throw invalid_argument("line "+to_string(lineNo)+": "+e.what());
        }
    }
    if(input.bad())throw runtime_error("failed to read workload");
    validateWorkload(processes);
    return processes;
}

int runCLI(const vector<string> &args,ostream &out,ostream &err){
    try{
        if(args.empty()||(args.size()==1&&args[0]=="--help")){
            printHelp(out);
            return 0;
        }
        string algorithm,file;
        Time quantum=2,cost=0;
        bool preemptive=false,compare=false;
        set<string>seen;
        for(size_t i=0;i<args.size();i++){
            const string &arg=args[i];
            if(!seen.insert(arg).second)throw invalid_argument("duplicate option: "+arg);
            if(arg=="--preemptive"){
                preemptive=true;
            }else if(arg=="--compare"){
                compare=true;
            }else if(arg=="--algorithm"||arg=="--input"||arg=="--quantum"||arg=="--context-switch"){
                if(i+1==args.size())throw invalid_argument("missing value for "+arg);
                const string &value=args[++i];
                if(arg=="--algorithm")algorithm=value;
                else if(arg=="--input")file=value;
                else if(arg=="--quantum")quantum=number(value,arg);
                else cost=number(value,arg);
            }else{
                throw invalid_argument("unknown option: "+arg);
            }
        }
        if(compare&&seen.count("--algorithm"))throw invalid_argument("choose either --compare or --algorithm");
        if(!compare&&algorithm.empty())throw invalid_argument("--algorithm or --compare is required");
        if(!compare&&algorithm!="fcfs"&&algorithm!="sjf"&&algorithm!="srtf"&&algorithm!="priority"&&algorithm!="rr"&&algorithm!="mlfq"){
            throw invalid_argument("unknown algorithm: "+algorithm);
        }
        if(file.empty())throw invalid_argument("--input is required");
        if(preemptive&&algorithm!="priority")throw invalid_argument("--preemptive requires --algorithm priority");
        if(seen.count("--quantum")&&algorithm!="rr"&&!compare)throw invalid_argument("--quantum requires --algorithm rr or --compare");
        if(quantum<=0)throw invalid_argument("quantum must be positive");
        if(cost<0)throw invalid_argument("context switch cost must be nonnegative");
        ifstream input(file);
        if(!input)throw runtime_error("cannot open workload: "+file);
        auto processes=readWorkload(input);
        if(compare){
            auto results=compareSchedulers(processes,quantum,cost);
            out<<left<<setw(15)<<"Algorithm"<<right<<setw(12)<<"Avg WT"<<setw(12)<<"Avg TAT"
               <<setw(12)<<"Avg RT"<<setw(12)<<"CPU %"<<setw(14)<<"Throughput"<<setw(12)<<"Switches"<<"\n";
            for(auto &r:results){
                out<<left<<setw(15)<<r.algorithmName<<right<<fixed<<setprecision(3)
                   <<setw(12)<<r.averageWaitingTime<<setw(12)<<r.averageTurnaroundTime
                   <<setw(12)<<r.averageResponseTime<<setw(12)<<r.cpuUtilization
                   <<setw(14)<<r.throughput<<setw(12)<<r.contextSwitches<<"\n";
            }
            return 0;
        }
        SimulationResult r;
        if(algorithm=="fcfs")r=runFCFS(processes,cost);
        else if(algorithm=="sjf")r=runSJF(processes,cost);
        else if(algorithm=="srtf")r=runSRTF(processes,cost);
        else if(algorithm=="priority")r=runPriority(processes,preemptive,cost);
        else if(algorithm=="rr")r=runRoundRobin(processes,quantum,cost);
        else r=runMLFQ(processes,cost);
        printResult(r,out);
        return 0;
    }catch(const exception &e){
        err<<"Error: "<<e.what()<<"\n";
        return 1;
    }
}
