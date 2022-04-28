#include<iostream>
#include<vector>
#include<tuple>
#include<fstream>
#include<sstream>
#include<string>
#include<stdlib.h>
#include<unistd.h>
#include<queue>
#include<climits>
#include<list>

using namespace std;

bool verbose;
bool oq_opt;
bool of_opt;
long int sim_time = 0;
long int track_pos = 0;
long int tot_movement =0;
double avg_turnaround;
double avg_waittime;
long int max_waittime = -1;
long int total_time = 0;
enum Direction {F, B};
Direction dir = F;
char sched;

class IORequest{
    public:
        int opid=-1;
        int arr_time=-1;
        int track=-1;
        int ds_starttime = -1;
        int ds_endtime = -1;
};
list< IORequest* > IOQueue;
list< IORequest* > active_queue;
list< IORequest* > add_queue;

class IOScheduler{
    public:
        virtual IORequest* strategy() = 0;
};

IOScheduler* THE_SCHEDULER;

class FIFO: public IOScheduler{
    public:
        IORequest* strategy(){

            if(IOQueue.front()->track > track_pos)
                dir = F;
            else if(IOQueue.front()->track < track_pos)
                dir = B;
            IORequest* temp_IO = IOQueue.front();
            IOQueue.pop_front();

            return temp_IO;
        } 
};

class SSTF: public IOScheduler{
    public:
        IORequest* strategy(){
            int short_seek = INT_MAX;
            int temp_seek;
            list <IORequest* >::iterator temp_IO;
            list< IORequest* >::iterator it = IOQueue.begin();
            
            while(it!=IOQueue.end()){
                temp_seek = abs((*it)->track - track_pos);
                if(temp_seek < short_seek){
                    short_seek = temp_seek;
                    temp_IO = it;
                }
                it++;
            }
            if((*temp_IO)->track > track_pos)
                dir = F;
            else if((*temp_IO)->track < track_pos)
                dir = B;
            
            IOQueue.erase(temp_IO);
            return (*temp_IO);
        }
};

class LOOK: public IOScheduler{
    public:
        IORequest* strategy(){
            //determin direction
            
            int short_seek = INT_MAX;
            int temp_seek;
            list <IORequest* >::iterator temp_IO;
            list< IORequest* >::iterator it = IOQueue.begin();
            if(dir == F){
                while(it!=IOQueue.end()){
                    if((*it)->track >= track_pos){
                        temp_seek = (*it)->track - track_pos;
                        if(temp_seek < short_seek){
                            short_seek = temp_seek;
                            temp_IO = it;
                        }
                    }
                    it++;
                }
                if(short_seek == INT_MAX){
                    dir = B;
                    it = IOQueue.begin();
                    while(it!=IOQueue.end()){
                    if((*it)->track <= track_pos){
                        temp_seek = track_pos - (*it)->track;
                        if(temp_seek < short_seek){
                            short_seek = temp_seek;
                            temp_IO = it;
                        }
                    }
                    it++;
                }
                }
            }
            else{
                while(it!=IOQueue.end()){
                    if((*it)->track <= track_pos){
                        temp_seek = track_pos - (*it)->track;
                        if(temp_seek < short_seek){
                            short_seek = temp_seek;
                            temp_IO = it;
                        }
                    }
                    it++;
                }
                if(short_seek == INT_MAX){
                    dir = F;
                    it = IOQueue.begin();
                    while(it!=IOQueue.end()){
                    if((*it)->track >= track_pos){
                        temp_seek = (*it)->track - track_pos;
                        if(temp_seek < short_seek){
                            short_seek = temp_seek;
                            temp_IO = it;
                        }
                    }
                    it++;
                }
                }
            }
        IOQueue.erase(temp_IO);
        return (*temp_IO);
        }
};

class CLOOK: public IOScheduler{
    public:
        IORequest* strategy(){
            int mintrack = INT_MAX;
            int short_seek = INT_MAX;
            int temp_seek;
            list <IORequest* >::iterator temp_IO;
            list< IORequest* >::iterator it = IOQueue.begin();
            if(dir == F){
                
                while(it!=IOQueue.end()){
                    if((*it)->track >= track_pos){
                        temp_seek = (*it)->track - track_pos;
                        if(temp_seek < short_seek){
                            short_seek = temp_seek;
                            temp_IO = it;
                        }
                    }
                    it++;
                }
                if(short_seek == INT_MAX){
                    dir = B;
                    it = IOQueue.begin();
                    while(it!=IOQueue.end()){
                    if((*it)->track <= track_pos){
                        if((*it)->track < mintrack){
                            mintrack = (*it)->track;
                            temp_IO = it;
                        }
                    }
                    it++;
                }
                }
            }
            else{
                dir=F;
                while(it!=IOQueue.end()){
                    if((*it)->track >= track_pos){
                        temp_seek = (*it)->track - track_pos;
                        if(temp_seek < short_seek){
                            short_seek = temp_seek;
                            temp_IO = it;
                        }
                    }
                    it++;
                }
                if(short_seek == INT_MAX){
                    dir = B;
                    it = IOQueue.begin();
                    while(it!=IOQueue.end()){
                    if((*it)->track <= track_pos){
                        if((*it)->track < mintrack){
                            mintrack = (*it)->track;
                            temp_IO = it;
                        }
                    }
                    it++;
                }
                }
            }
            IOQueue.erase(temp_IO);
            return (*temp_IO);
        }
};

class FLOOK: public IOScheduler{
    public:
        IORequest* strategy(){
            int short_seek = INT_MAX;
            int temp_seek;
            list <IORequest* >::iterator temp_IO;
            list< IORequest* >::iterator it = active_queue.begin();
            if(dir == F){
                while(it!=active_queue.end()){
                    if((*it)->track >= track_pos){
                        temp_seek = (*it)->track - track_pos;
                        if(temp_seek < short_seek){
                            short_seek = temp_seek;
                            temp_IO = it;
                        }
                    }
                    it++;
                }
                if(short_seek == INT_MAX){
                    dir = B;
                    it = active_queue.begin();
                    while(it!=active_queue.end()){
                    if((*it)->track <= track_pos){
                        temp_seek = track_pos - (*it)->track;
                        if(temp_seek < short_seek){
                            short_seek = temp_seek;
                            temp_IO = it;
                        }
                    }
                    it++;
                }
                }
            }
            else{
                while(it!=active_queue.end()){
                    if((*it)->track <= track_pos){
                        temp_seek = track_pos - (*it)->track;
                        if(temp_seek < short_seek){
                            short_seek = temp_seek;
                            temp_IO = it;
                        }
                    }
                    it++;
                }
                if(short_seek == INT_MAX){
                    dir = F;
                    it = active_queue.begin();
                    while(it!=active_queue.end()){
                    if((*it)->track >= track_pos){
                        temp_seek = (*it)->track - track_pos;
                        if(temp_seek < short_seek){
                            short_seek = temp_seek;
                            temp_IO = it;
                        }
                    }
                    it++;
                }
                }
            }
        active_queue.erase(temp_IO);

        return (*temp_IO); 
        }
};

void Simulation(vector< IORequest* > &IOReq_list){
    int io_idx=0;
    IORequest* curr_IO = NULL;
    long int p_waittime=0;

    long int sum_turnaround = 0;
    long int sum_waittime = 0;
    if(verbose){
        cout<<"TRACE:"<<endl;
    }
    while(true){
        if(io_idx != IOReq_list.size() && sim_time == IOReq_list[io_idx]->arr_time){
            IOQueue.push_back(IOReq_list[io_idx]);

            if(verbose){
                cout<<sim_time<<":"<<"  "<<IOReq_list[io_idx]->opid<<" add "<<IOReq_list[io_idx]->track<<endl;
            }

            io_idx++;       
        }
        
        if(curr_IO != NULL && track_pos == curr_IO->track){
            //compute relevant info
            curr_IO->ds_endtime = sim_time;
            
            sum_turnaround += (curr_IO->ds_endtime - curr_IO->arr_time);
            p_waittime = curr_IO->ds_starttime - curr_IO->arr_time;
            sum_waittime += p_waittime;

            if(verbose){
                cout<<sim_time<<":"<<" "<<curr_IO->opid<<" finish "<<(curr_IO->ds_endtime - curr_IO->arr_time)<<endl;
            }

            if(p_waittime > max_waittime)
                max_waittime = p_waittime;
            

            curr_IO = NULL;
        }
        if(curr_IO == NULL){
            if(IOQueue.size() != 0){
                if(sched=='f' && active_queue.empty())
                    active_queue.swap(IOQueue);

                curr_IO = THE_SCHEDULER->strategy();
                curr_IO->ds_starttime = sim_time;
                
                if(verbose){
                    cout<<sim_time<<":"<<" "<<curr_IO->opid<<" issue "<<curr_IO->track<<" "<<track_pos<<endl;
                }
                if(curr_IO->track == track_pos)
                    continue;
            }
            else if(sched == 'f' && !active_queue.empty()){
                curr_IO = THE_SCHEDULER->strategy();
                curr_IO->ds_starttime = sim_time;
                
                if(verbose){
                    cout<<sim_time<<":"<<"  "<<curr_IO->opid<<" issue "<<curr_IO->track<<" "<<track_pos<<endl;
                }
                if(curr_IO->track == track_pos)
                    continue;
            }
            else if(io_idx == IOReq_list.size()){
                total_time = sim_time;
                
                double size1 = IOReq_list.size();
                avg_turnaround = sum_turnaround/size1;
                avg_waittime = sum_waittime/size1;

                vector< IORequest* >::iterator iter = IOReq_list.begin();

                while(iter!=IOReq_list.end()){
                    printf("%5d: %5d %5d %5d\n",(*iter)->opid, (*iter)->arr_time, (*iter)->ds_starttime, (*iter)->ds_endtime);
                    iter++;
                }
                
                printf("SUM: %d %d %.2lf %.2lf %d\n",
                    total_time, tot_movement, avg_turnaround, avg_waittime, max_waittime);
                exit(0);
            }
        }
        if(curr_IO != NULL){
            if(dir == F)
                track_pos++;
            else
                track_pos--;
            tot_movement++;
        }        
        sim_time++;
    }
}

int main(int argc, char** argv){
    int opt;
    
    vector< IORequest* > IOReq_list;


    while((opt = getopt(argc, argv, "s:vqf")) != -1){
        switch(opt){
            case 's':
                if(optarg!=NULL){
                    sched = optarg[0];
                    if(sched == 'i')
                        THE_SCHEDULER = new FIFO();
                    else if(sched == 'j')
                        THE_SCHEDULER = new SSTF();
                    else if(sched == 's')
                        THE_SCHEDULER = new LOOK();
                    else if(sched == 'c')
                        THE_SCHEDULER = new CLOOK();
                    else if(sched == 'f')
                        THE_SCHEDULER = new FLOOK();
                    else{
                        cout<<"Invalid option";
                        exit(0);
                    }
                }
                break;
            case 'v':
                verbose = true;
                break;
            case 'q':
                if(optarg != NULL)
                    oq_opt = true;
                break;
            case 'f':
                if(optarg != NULL)
                    of_opt = true;
                break;
            default:
                exit(0);     
        }
    }
    ifstream inputfile;
    inputfile.open(argv[optind]);

    string line1;
    int request_time = 0;
    int track = -1;
    int k=0;

    if(inputfile.is_open()){
        while(getline(inputfile, line1)){
            if(line1[0] == '#')
                continue;
            string temp;
            istringstream iss(line1);

            iss >> temp;
            request_time = stoi(temp);
            
            iss >> temp;
            track = stoi(temp);   

            IORequest *io_req = new IORequest();
            io_req->opid = k;
            io_req->arr_time = request_time;
            io_req->track = track;
            IOReq_list.push_back(io_req);
            k++;
        }
    }
    if(IOReq_list.size()>10000){
        cout<<"to many io requests: max allowed=10000)"<<endl;
        exit(0);
    }
    if(IOReq_list.size()==0){
        cout<<"Error input file format line 0"<<endl;
        exit(0);
    }
 
    Simulation(IOReq_list);


    return 0;
}