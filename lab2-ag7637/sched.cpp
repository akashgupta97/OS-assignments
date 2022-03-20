#include<iostream>
#include<string.h>
#include<stdlib.h>
#include<queue>
#include<getopt.h>
#include<fstream>
#include<list>
#include<vector>
using namespace std;

//Defining variables to be used
bool verbose;
bool trace;
bool rec_eventQ;
int quantum = 10000;
int max_prio = 4;
int cpu_util_final =0;
int io_util_final = 0;
int finish_last_evt;
int total_TAT;
int total_cpu_wait;
int io_init = 0;
int count_io = 0;
int throughput = 0;
bool preprio_indic = false;
int random_total;
vector<int> random_arr;

//representing states as enum variables
enum State {CREATED,READY,RUNNING, PREEMPT, BLOCK, Done};
enum Transition {TRANS_TO_READY, TRANS_TO_RUN, TRANS_TO_BLOCK, TRANS_TO_PREEMPT};

//random fucntion
int ofs=0;

int myrandom(int burst)
{
    if(ofs==random_total) 
        ofs=0;
    
    return 1 + (random_arr[ofs++] % burst); 
}

//Process class
class Process{
    public:
        int AT;
        int TC;
        int CB;
        int IO;
        int static_prio;
        int dynamic_prio;
        int time_curr_state;
        int rem_cpu_burst;
        State state;
        int cpu_time;
        int io_time;
        int cpu_wait;
        int finishing_time;
        int TAT;
        int pid;

        Process(int id, int at, int tc, int cb, int io){
            pid=id;
            AT = at;
            TC = tc;
            CB = cb;
            IO = io;
            state = CREATED;
            cpu_wait =0;
            io_time = 0;
            rem_cpu_burst=0;
            cpu_time = 0;
        }   

        void getProcessSummary()
        {
            printf("%04d: %4d %4d %4d %4d %1d | %5d %5d %5d %5d\n",pid, AT, TC, CB, IO, static_prio, finishing_time, TAT, io_time, cpu_wait);
        }

};

//Event class
class Event{
    public:
        
        static int n_events;
        int evtTimestamp;
        Process* process; 
        State Proc_OldState;
        Transition Proc_Trans;
        int ins_time = n_events;

        Event(int timestamp, Process* p1, Transition t){
            evtTimestamp = timestamp;
            process = p1;
            Proc_Trans = t;
            n_events++;
        }
       
};


int Event::n_events=0;

struct Compare1{
    bool operator() (Event* p, Event* q){
        if (p->evtTimestamp == q->evtTimestamp)
            return p->ins_time > q->ins_time;
        return p->evtTimestamp > q->evtTimestamp;
    }
};

//Scheduler class
class Scheduler{
    public:
        list<Process*> runQueue;
        virtual void add_process(Process * p){};
        virtual Process* get_next_process() {};
        bool test_preempt(Process *p, int curtime){};        

};

//FCFS
class FCFS: public Scheduler{
    public:
        
        void add_process(Process* p){
            runQueue.push_back(p);
        }
        Process* get_next_process(){
            
            
            Process* temp_proc;

            if(!runQueue.empty()){
                temp_proc = runQueue.front();
                runQueue.pop_front();       
            }
            else{
                return NULL;
            }
            
            return temp_proc;
        }

};

string sched_name = "FCFS";
Scheduler* scheduler = new FCFS();

//LCFS
class LCFS: public Scheduler{
    public:
        void add_process(Process* p){
            runQueue.push_back(p);
        }
        Process* get_next_process(){
            
            
            Process* temp_proc;

            if(!runQueue.empty()){
                temp_proc = runQueue.back();
                runQueue.pop_back();       
            }
            else{
                return NULL;
            }
            
            return temp_proc;
        }

};

//SRTF
class SRTF: public Scheduler{
    public:
        void add_process(Process* p){
            int indic = 0;
            int ins_pos = 0;
            for(list<Process* >:: iterator it = runQueue.begin(); it != runQueue.end(); it++){
                Process* temp_proc  = *it;
                ins_pos++;
                if((p->TC - p->cpu_time) < (temp_proc->TC - temp_proc->cpu_time)){
                    indic=1;
                    break;
                }
                
            }
            if(indic==0){
                runQueue.push_back(p);
            }
            else{
                list<Process* >:: iterator it = runQueue.begin();
                advance(it,ins_pos-1);
                runQueue.insert(it, p);
            }
        }
        
        Process* get_next_process(){

            Process* temp_proc;

            if(!runQueue.empty()){
                temp_proc = runQueue.front();
                runQueue.pop_front();       
            }
            else{
                return NULL;
            }
            
            return temp_proc;

        }
};


//Round-Robin
class RR: public Scheduler{
        public:
        
        void add_process(Process* p){
            runQueue.push_back(p);
        }
        Process* get_next_process(){
            
            
            Process* temp_proc;

            if(!runQueue.empty()){
                temp_proc = runQueue.front();
                runQueue.pop_front();       
            }
            else{
                return NULL;
            }
            
            return temp_proc;
        }
};


//Priority Scheduling
class PRIO: public Scheduler{

    int active_c;
    int expired_c;
    vector<queue<Process* > > q1, q2;
    vector<queue<Process* > > *activeQ, *expiredQ;
    
    public:
        void init_prio(){
            for(int i=0;i<max_prio;i++){
                q1.push_back(queue<Process * >());
                q2.push_back(queue<Process * >());
            }
            activeQ = &q1;
            expiredQ = &q2;
            active_c = 0;
            expired_c = 0;
        }

        void add_process(Process* p){

            if(activeQ == NULL){
                init_prio();
            }          
            if(p->state==PREEMPT)
            {
                p->dynamic_prio--;
            }



            p->state = READY;

            if(p->dynamic_prio>=0)
            {
                activeQ->at(p->dynamic_prio).push(p);
                active_c++;
            }
            else
            {
                p->dynamic_prio = p->static_prio-1;
                expiredQ->at(p->dynamic_prio).push(p);
                expired_c++;
            }              
        }

        void Exchange(){
            
            vector<queue<Process *> > *temp = activeQ;
            activeQ = expiredQ;
            expiredQ = temp;

            active_c = expired_c;
            expired_c = 0;        
        }

        Process * get_next_process()
        {
            if(active_c==0) 
                Exchange();

            Process *p = NULL;
            int qnum = -1;

            for(int i=0; i<activeQ->size(); i++)
            {
                if(!activeQ->at(i).empty())
                {
                    qnum=i;
                }
            }

            if(qnum>=0)
            {
                p = activeQ->at(qnum).front();
                activeQ->at(qnum).pop(); 
                active_c--;
            }

            return p;
        }

};


//Preemptive Priority Scheduling
class PREPRIO: public Scheduler{

    int active_c;
    int expired_c;
    vector<queue<Process* > > q1, q2;
    vector<queue<Process* > > *activeQ, *expiredQ;
    
    public:
        void init_prio(){
            for(int i=0;i<max_prio;i++){
                q1.push_back(queue<Process * >());
                q2.push_back(queue<Process * >());
            }
            activeQ = &q1;
            expiredQ = &q2;
            active_c = 0;
            expired_c = 0;
        }

        void add_process(Process* p){

            if(activeQ == NULL){
                init_prio();
            }          
            if(p->state==PREEMPT)
            {
                p->dynamic_prio--;
            }



            p->state = READY;

            if(p->dynamic_prio>=0)
            {
                activeQ->at(p->dynamic_prio).push(p);
                active_c++;
            }
            else
            {
                p->dynamic_prio = p->static_prio-1;
                expiredQ->at(p->dynamic_prio).push(p);
                expired_c++;
            }              
        }

        void Exchange(){
            
            vector<queue<Process *> > *temp = activeQ;
            activeQ = expiredQ;
            expiredQ = temp;

            active_c = expired_c;
            expired_c = 0;        
        }

        Process * get_next_process()
        {
            if(active_c==0) 
                Exchange();

            Process *p = NULL;
            int qnum = -1;

            for(int i=0; i<activeQ->size(); i++)
            {
                if(!activeQ->at(i).empty())
                {
                    qnum=i;
                }
            }

            if(qnum>=0)
            {
                p = activeQ->at(qnum).front();
                activeQ->at(qnum).pop(); 
                active_c--;
            }

            return p;
        }

};


//class to mimic discrete event simulation as required
class Discrete_Event_Simulator{
    public:
        list<Event> evtQueue;
        
        void put_event(Event evt){
            
            if(rec_eventQ){
                string trans;
                if(evt.Proc_Trans==TRANS_TO_READY)
                    trans = "READY";

                else if(evt.Proc_Trans==TRANS_TO_RUN)
                    trans = "RUNNG";

                else if(evt.Proc_Trans==TRANS_TO_BLOCK)
                    trans = "BLOCK";

                else if(evt.Proc_Trans==TRANS_TO_PREEMPT)
                    trans = "PREEMPT";
                
                cout<< "AddEvent(" << evt.evtTimestamp << ":" << evt.process << ":" << trans << ") ";
                cout<<endl;
            }            

            int indic=0;
            int ins_pos=0;
            for(list<Event>::iterator it = evtQueue.begin() ; it!= evtQueue.end(); it++)
            {
                Event e = *it;
                ins_pos++;
                //cout<<e->evtTimestamp<<endl;
                if(e.evtTimestamp>evt.evtTimestamp)
                {
                    indic=1;
                    break;
                }
                
            }

            if(indic==0)
            {
                evtQueue.push_back(evt);
                //cout<<evt.evtTimestamp<<endl;

            }
            else
            {
                list<Event>::iterator it = evtQueue.begin();
                advance(it,ins_pos-1);
                evtQueue.insert(it,evt);
            }

            if(rec_eventQ)
            {
                Display_Queue();
                cout << endl;
            }
        }

        Event* get_event(){
            if(evtQueue.empty())
                return NULL;
            
            return &evtQueue.front();
        }

        void rm_event(){
            evtQueue.pop_front();
        }

        void RemoveFutureEvents(Process *p){
            int indic = 0;
            int ins_pos = 0;

            for(list<Event>:: iterator it = evtQueue.begin(); it!=evtQueue.end();it++){
                Event e = *it;
                ins_pos++;
                if(e.process == p){
                    indic=1;
                    break;
                }
                
            }
            if(indic==1){
                list<Event>::iterator it = evtQueue.begin();
                advance(it,ins_pos-1);
                evtQueue.erase(it);
            }

        }

        int get_next()
        {
            if(evtQueue.empty())
            {
                return -1;
            }
            
            return evtQueue.front().evtTimestamp;
        }
        
        void Display_Queue(){
           
            for(list<Event>:: iterator it = evtQueue.begin(); it!=evtQueue.end(); it++) {
                    Event event  = *it;
                    cout << "(" << event.evtTimestamp << ":" << event.process->pid << ":" << event.Proc_Trans << ":" << event.ins_time << ")";        
                }
            cout<<endl;
}
};


//Main Simulation
void Simulation(Discrete_Event_Simulator *des)
{
    Event *e;
    int current_time;
    bool call_scheduler;
    Process* p;
    Process* running_p = NULL;
    int temp_cpu_burst;

    while((e = des->get_event())!=NULL){
        //cout<<e->process->AT<<endl;
        p = e->process;
        current_time = e->evtTimestamp;
        int transition = e->Proc_Trans;
        int timeInPrevState = current_time - p->time_curr_state;

        p->time_curr_state = current_time;
        
        switch(transition){
            case TRANS_TO_READY:{
                string prev_state;

                if(e->Proc_OldState == RUNNING)
                    prev_state = "RUNNG";
                else if(e->Proc_OldState == BLOCK)
                    prev_state = "BLOCK";
                else if(e->Proc_OldState == CREATED)
                    prev_state = "CREATED";

                if(verbose)
                    cout << current_time << " " << p->pid << " " << timeInPrevState << ": " << prev_state << " -> " << "READY" << endl;
                
                if(e->Proc_OldState == RUNNING){
                    p->cpu_time = p->cpu_time + timeInPrevState;
                    cpu_util_final = cpu_util_final + timeInPrevState;
                }
                else if(e->Proc_OldState == BLOCK){
                    p->io_time = p->io_time + timeInPrevState;
                    p->dynamic_prio = p->static_prio - 1;
                    count_io--;
                    if(count_io == 0){
                        io_util_final = io_util_final + (current_time - io_init);
                    }
                }

                
                if(preprio_indic && running_p!=NULL)
                {
                    if(p->state==BLOCK || p->state==CREATED)
                    {
                        int ct;
                        

                        for(list<Event>::iterator it = des->evtQueue.begin();it!=des->evtQueue.end();it++)
                        {
                            Event e = *it;
                            if(e.process==running_p)
                            {
                                ct = e.evtTimestamp;
                            }
                        }

                        if(current_time < ct && p->dynamic_prio > running_p->dynamic_prio)
                        {
                            des->RemoveFutureEvents(running_p);
                            Event e(current_time, running_p, TRANS_TO_PREEMPT);
                            e.Proc_OldState = RUNNING;
                            des->put_event(e);
                        }
                    }
                }                
                call_scheduler= true;
                scheduler->add_process(p);
                break;
            }
            case TRANS_TO_BLOCK:
                {
                    p->cpu_time = p->cpu_time + p->rem_cpu_burst;
                    p->rem_cpu_burst = p->rem_cpu_burst - timeInPrevState;
                    cpu_util_final = cpu_util_final + timeInPrevState;
                    running_p=NULL;

                    if((p->cpu_time - p->TC)!=0){
                        p->state = BLOCK;

                        if(count_io==0)
                            io_init = current_time;
                        
                        count_io++;

                        int io_b = myrandom(p->IO);
                        Event e(current_time + io_b, p, TRANS_TO_READY);
                        e.Proc_OldState = BLOCK;

                        des->put_event(e);
                        if(verbose)
                            printf("%d %d %d: %s -> %s ib=%2d rem=%d \n",current_time, p->pid, timeInPrevState,  "RUNNG", "BLOCK", io_b, p->TC-p->cpu_time);
                        }

                        
                    
                    else{
                        
                        p->TAT = current_time - p->AT;
                        p->finishing_time = current_time;
                        p->state = Done;

                        finish_last_evt = current_time;
                        total_TAT = total_TAT + p->TAT;

                        if(verbose)
                            printf("%d %d %d: Done \n", current_time, p->pid, timeInPrevState);                                          

                    }

                    call_scheduler = true;
                    break;
                }

                case TRANS_TO_RUN:{
                    running_p=p;
                    p->state = RUNNING;

                    if(p->rem_cpu_burst <=0){
                        temp_cpu_burst = myrandom(p->CB);
                        if(temp_cpu_burst > (p->TC - p->cpu_time))
                            temp_cpu_burst = p->TC - p->cpu_time;
                    }
                    else{
                        temp_cpu_burst = p->rem_cpu_burst;
                    }
                    p->rem_cpu_burst = temp_cpu_burst;
                    
                    total_cpu_wait = total_cpu_wait+ timeInPrevState;
                    
                    p->cpu_wait = p->cpu_wait + timeInPrevState;
                    
                    if(verbose)
                        printf("%d %d %d: %s -> %s cb=%2d rem=%d prio=%d\n",current_time, p->pid, timeInPrevState, "READY", "RUNNG" ,temp_cpu_burst, p->TC-p->cpu_time, p->dynamic_prio);

                    if(temp_cpu_burst > quantum){
                        Event e(current_time + quantum, p, TRANS_TO_PREEMPT);    
                        e.Proc_OldState = RUNNING;
                        des->put_event(e);
                    }
                    else{
                        Event e(current_time + temp_cpu_burst, p, TRANS_TO_BLOCK);
                        e.Proc_OldState = RUNNING;
                        des->put_event(e);
                    }
                    break;
                }
                case TRANS_TO_PREEMPT:{
                    p->cpu_time = p->cpu_time + timeInPrevState;
                    p->rem_cpu_burst = p->rem_cpu_burst - timeInPrevState;

                    if(verbose)
                        printf("%d %d %d: %s -> %s cb=%2d rem=%d prio=%d\n",current_time, p->pid, timeInPrevState, "RUNNG", "READY", p->rem_cpu_burst, p->TC-p->cpu_time, p->dynamic_prio);
                    running_p = NULL;
                    p->state = PREEMPT;
                    cpu_util_final = cpu_util_final + timeInPrevState;
                    scheduler->add_process(p);
                    call_scheduler = true;
                    break;

                }
        
        }

        des->rm_event();

        if(call_scheduler)
        {
            if(des->get_next() == current_time)
            {
                continue;
            }

            call_scheduler = false;

            if(running_p==NULL)
            {   
                running_p = scheduler->get_next_process();
                
                if(running_p==NULL)
                    continue;

                Event e(current_time, running_p, TRANS_TO_RUN);
                e.Proc_OldState = READY;
                des->put_event(e);
            }
        }       
    }

}



//Printing stats here
void print_stats(int proc_list_size, int cpu_util_final, int io_util_final, int finish_last_evt, int total_TAT, int total_cpu_wait){
    
    double throughput = (proc_list_size*100.0)/finish_last_evt;
    double io_util = 100*((double)io_util_final/finish_last_evt);
    double cpu_util = 100*((double)cpu_util_final/finish_last_evt);
    double avg_TAT = ((double)total_TAT/proc_list_size);
    double avg_cpu_wait = ((double)total_cpu_wait/proc_list_size);
   
    printf("SUM: %d %.2lf %.2lf %.2lf %.2lf %.3lf\n", finish_last_evt, cpu_util, io_util, avg_TAT, avg_cpu_wait, throughput);
}


//Main function
int main(int argc, char *argv[]){
    
    
    bool quantum_exists;
    int opt;
    int q=0, m=0;
    char a;

    while((opt = getopt (argc, argv, "vtes:")) != -1)
    {
        switch(opt)
        {
            case 'v': 
                verbose = true;
                break;
            case 't': 
                trace = true;
                break;
            case 'e': 
                rec_eventQ = true;
                break;
            case 's':
                if(optarg!=NULL){
                    if(optarg[0] == 'F'){
                        sched_name = "FCFS";
                        scheduler = new FCFS();                       
                    }
                    else if(optarg[0] == 'L'){
                        sched_name = "LCFS";
                        scheduler = new LCFS();
                    }
                    else if(optarg[0] == 'S'){
                        sched_name = "SRTF";
                        scheduler = new SRTF();
                    }
                    else if(optarg[0] == 'R'){
                        sched_name = "RR";
                        scheduler = new RR();
                        sscanf(optarg, "%c%d",&a,&quantum);
                        quantum_exists = true;
                    }
                    else if(optarg[0] == 'P'){
                        sched_name = "PRIO";
                        scheduler = new PRIO();
                        sscanf(optarg, "%c%d:%d",&a,&quantum, &max_prio);
                        quantum_exists = true;
                    }
                    else if(optarg[0] == 'E'){
                        sched_name = "PREPRIO";
                        scheduler = new PREPRIO();
                        sscanf(optarg, "%c%d:%d",&a,&quantum, &max_prio);
                        quantum_exists = true;
                        preprio_indic = true;
                    }
                    else{
                        cout<<"Unknown Scheduler spec: -v {FLSPRE}"<<endl;
                        exit(0);
                    }
            case 0:
                break;
            default:
                exit(0);
        }
    }
    }
    
    // if(a == 'E' || a == 'R' || a == 'P'){
    //     if(q <=0 || m <=0){
    //         cout<<"Invalid scheduler param <" << a << q <<":"<< m << ">"<< endl;
    //         exit(0);
    //     }
    // }
    // quantum = q;
    // max_prio = m;

    string rfile_name = argv[optind+1];
       

    //Reading random file
    ifstream rfile;
    rfile.open(rfile_name);
    
    string line1;
    
    bool tok1 = true;

    if(rfile.is_open())
    {
        while(rfile>>line1)
        {
            if(tok1)
            {
                random_total = stoi(line1);
                tok1 = false;
            }
            else
            {
                random_arr.push_back(stoi(line1));
            }
        }
        rfile.close();
    }   

    //Reading input file
    vector <Process> Proc_list;

    string input_file_name  = argv[optind];

    ifstream input_file;
    input_file.open(input_file_name);
    string line2;
    if(input_file.is_open())
    {
        int k=0;
        while(input_file>>line2)
        {   
            int arr_time, tot_cpu, cpu_burst, io_burst;
            
            arr_time = stoi(line2);
            input_file>>line2;
            
            tot_cpu = stoi(line2);
            input_file>>line2;
            
            cpu_burst = stoi(line2);
            input_file>>line2;     
            
            io_burst = stoi(line2);

            Process p1(k,arr_time, tot_cpu, cpu_burst , io_burst);
            
            p1.static_prio = myrandom(max_prio);
            p1.dynamic_prio = p1.static_prio-1;
            p1.time_curr_state = arr_time;
            Proc_list.push_back(p1);

            k++;
        }

        input_file.close();
    }

    //Defines a DES
    Discrete_Event_Simulator des;

    for(int i=0;i<Proc_list.size();i++){
        Event evt(Proc_list[i].AT,&Proc_list[i],TRANS_TO_READY);
        //cout<<Proc_list[i].AT<<endl;
        evt.Proc_OldState = CREATED;
        des.put_event(evt);
    }
    
    //Starting simulation   
    Simulation(&des);

    cout << sched_name;

    if(quantum_exists)
    {
        cout << " " << quantum;
    }    
    cout << endl;
    

    for(int i=0;i<Proc_list.size();i++){
        Proc_list[i].getProcessSummary();
    }
   
    print_stats(Proc_list.size(), cpu_util_final, io_util_final, finish_last_evt, total_TAT, total_cpu_wait);


    return 0;

}