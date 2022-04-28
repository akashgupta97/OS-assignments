#include<iostream>
#include<fstream>
#include<unistd.h>
#include<stdlib.h>
#include<sstream>
#include<string.h>
#include<tuple>
#include<vector>
#include<deque>


using namespace std;

#define N_PTEs 64
int ofs=0;
vector<int> random_arr;
int random_total;
unsigned long global_inst_count=0;
unsigned long ctxt_switches = 0;
unsigned long proc_exits = 0;
unsigned long long TOTAL_COST=0;
int nru_inst_count=0;
int wst_inst_count=0;

struct pte_t{
    unsigned int PRESENT:1;
    unsigned int REFERENCED:1;
    unsigned int MODIFIED:1;
    unsigned int WRITE_PROTECT:1;
    unsigned int PAGEDOUT:1;
    unsigned int FRAME_NO:7;
    unsigned int FILEMAPPED:1;
    unsigned int ACCESSIBLE:1;

    pte_t() { memset(this, 0, sizeof *this); };
};


class Process{
    public:
        int n_vmas;
        int pid;
        unsigned long maps =0;
        unsigned long unmaps =0;
        unsigned long ins =0;
        unsigned long outs =0;
        unsigned long fins =0;
        unsigned long fouts =0;
        unsigned long zeros =0;
        unsigned long segv =0;
        unsigned long segprot =0;

        Process(int n){
            n_vmas = n;
        }
        vector< tuple<int, int , int, int> > VMAS;
        //Insert page table
        pte_t page_table[N_PTEs];
};


typedef struct{
    int frame_no;
    Process* p=NULL;
    int vpage=-1;
    uint32_t age=0;
    int tau = 0;
}frame_t;


int myrandom(int burst)
{   
    if(ofs==random_total) 
        ofs=0;
    
    int c = (random_arr[ofs] % burst);
    ofs++;

    return c; 
}

void PrintPT(Process* p){
    cout<<"PT["<<p->pid<<"]: ";
    
    for(int i=0;i<N_PTEs;i++){
        if(p->page_table[i].PRESENT == 1){
            cout<<i<<":";
            if(p->page_table[i].REFERENCED == 1)
                cout<<"R";
            else
                cout<<"-";
            
            if(p->page_table[i].MODIFIED == 1)
                cout<<"M";
            else
                cout<<"-";

            if(p->page_table[i].PAGEDOUT == 1)
                cout<<"S";
            else
                cout<<"-";
        }
        else{
            if(p->page_table[i].PAGEDOUT == 1)
                cout<<"#";
            else
                cout<<"*";
        }
        if(i!=N_PTEs-1)
            cout<<" ";
    }
    cout<<endl;
}

void PrintFT(frame_t *FT, int num_frames){
    cout<<"FT: ";
    for(int i=0;i<num_frames;i++){
        if(FT[i].vpage!=-1)
            cout<<FT[i].p->pid<<":"<<FT[i].vpage;
        else
            cout<<"*";
        if(i!=num_frames-1)
            cout<<" ";
    }
    cout<<endl;
}

class Pager{
    public:
        virtual frame_t* select_victim_frame(frame_t *FT, int n_frames) = 0;
};

class FIFO: public Pager{
    public:
        int HAND = 0;
        frame_t* select_victim_frame(frame_t *FT, int n_frames){
            if(HAND == n_frames)
                HAND=0;
            
            frame_t* f = &FT[HAND];
            HAND++;
            return f;
        }
};

class CLOCK: public Pager{
    public:
        int HAND=0;
        frame_t* select_victim_frame(frame_t *FT, int n_frames){
            
            for(;;){
                if(HAND == n_frames)
                    HAND=0;

                if(FT[HAND].p->page_table[FT[HAND].vpage].REFERENCED == 0){
                    frame_t* f = &FT[HAND];
                    HAND++;
                    return f;
                }
                else{
                    FT[HAND].p->page_table[FT[HAND].vpage].REFERENCED = 0;
                    HAND++;
                }
            }
        }
};

class RANDOM: public Pager{
    public:
    int HAND=0;
    frame_t* select_victim_frame(frame_t* FT, int n_frames){
        HAND = myrandom(n_frames);
        frame_t* f = &FT[HAND];
        return f;
    }
};

class NRU: public Pager{
    public:
        int HAND=0;
        
        frame_t* select_victim_frame(frame_t* FT, int n_frames){
            int temp_frame_class = 0;
            frame_t* victim_frame=NULL;
            int victim_frame_class=-1;
            int victim_position = 0;
            for(int i=0;i<n_frames;i++){
                temp_frame_class = 2*(FT[HAND].p->page_table[FT[HAND].vpage].REFERENCED) + FT[HAND].p->page_table[FT[HAND].vpage].MODIFIED;
                //cout<<temp_frame_class<<endl;
                if(victim_frame == NULL || temp_frame_class < victim_frame_class){
                    victim_frame = &FT[HAND];
                    victim_frame_class = temp_frame_class;
                    victim_position = HAND;
                }
                
                if(temp_frame_class == 0 && nru_inst_count < 50){
                    break;
                }
                if(nru_inst_count >= 50){
                    FT[HAND].p->page_table[FT[HAND].vpage].REFERENCED = 0;
                }

                if(HAND == n_frames-1)
                    HAND=0;
                else{
                    HAND++;
                }
            }
            if(nru_inst_count >= 50)
                nru_inst_count = 0;
            
            if(victim_position == n_frames-1)
                HAND=0;
            else 
                HAND = victim_position+1;

            return victim_frame;
        }
};

class AGING: public Pager{
    public:
        int HAND=0;
        
        frame_t* select_victim_frame(frame_t* FT, int n_frames){
            frame_t* victim_frame=NULL;
            int victim_position =0;
            for(int i=0;i<n_frames;i++){
                if(victim_frame == NULL){
                    victim_frame = &FT[HAND];
                    victim_position = HAND;
                }
                
                FT[HAND].age = FT[HAND].age >> 1;
                if(FT[HAND].p->page_table[FT[HAND].vpage].REFERENCED == 1){
                    FT[HAND].age = (FT[HAND].age | 0x80000000);
                    FT[HAND].p->page_table[FT[HAND].vpage].REFERENCED = 0;
                }

                if(FT[HAND].age < victim_frame->age){
                    victim_frame = &FT[HAND];
                    victim_position = HAND;
                }

                if(HAND == n_frames-1)
                    HAND=0;
                else
                    HAND++;

            }
            if(victim_position == n_frames-1)
                HAND=0;
            else 
                HAND = victim_position+1;

            return victim_frame;

        }

        
};

class WORKING_SET: public Pager{
    public:
        int HAND=0;
        
        frame_t* select_victim_frame(frame_t *FT, int n_frames){
            frame_t* victim_frame = NULL;
            frame_t* oldest_frame = NULL;
            int victim_position = 0;
            int oldest_position=0;
            bool break_ind = false;
            int oldest=0;

            for(int i=0;i<n_frames;i++){
                
                if(victim_frame == NULL){
                    victim_frame = &FT[HAND];
                    victim_position = HAND;
                }

                if(FT[HAND].p->page_table[FT[HAND].vpage].REFERENCED == 1){
                    FT[HAND].tau = wst_inst_count;
                    FT[HAND].p->page_table[FT[HAND].vpage].REFERENCED = 0;
                }

                else if(FT[HAND].p->page_table[FT[HAND].vpage].REFERENCED == 0){
                    int temp_age = wst_inst_count - FT[HAND].tau;
                    victim_frame = &FT[HAND];
                    victim_position = HAND;
                    if(temp_age > 49){   
                        break_ind = true;                     
                        break;
                    }
                    if(temp_age>oldest){
                        oldest = temp_age;
                        oldest_frame = victim_frame;
                        oldest_position = victim_position;
                    }
                }
               
                if(HAND == n_frames-1)
                    HAND=0;
                else
                    HAND++;
            }
            if(!break_ind && oldest_frame!=NULL){
                victim_frame = oldest_frame;
                victim_position = oldest_position;
            }

            if(victim_position == n_frames-1)
                HAND=0;
            else 
                HAND = victim_position+1;
            //cout<<HAND<<endl;
            //cout<<victim_frame->vpage<<endl;
            return victim_frame;
        }
};

frame_t* allocate_from_free_list(deque<frame_t* > &Free_list, int n_frames){
    //cout<<Free_list.size()<<endl;
    if(Free_list.size() == 0){
        //cout<<"addwd"<<endl;
        return NULL;
    }
    frame_t* f_frame = Free_list.front();
    Free_list.pop_front();
    //cout<<Free_list.size()<<endl;

    return f_frame;
}

frame_t* getframe(Pager* THE_PAGER, deque<frame_t* > &Free_list, frame_t *FT, int n_frames){
    frame_t *frame = allocate_from_free_list(Free_list, n_frames);
    
    if(frame == NULL){
        frame = THE_PAGER->select_victim_frame(FT,n_frames);
    }
    return frame;
}

void Simulation(vector<Process*> &PL, vector< tuple<char, int> > &IL, frame_t *FT, int n_frames, vector<bool> opt_vect, deque<frame_t* > &Free_list, Pager* THE_PAGER){
    //PL -> Process list
    //IL -> Instruction list
    //FT -> Frame table
    Process* curr_proc = NULL;
    vector< tuple<char, int> >::iterator it = IL.begin();
    int inst_count = 0;
    for(;it!=IL.end();it++){

        if(opt_vect[3])
            global_inst_count+=1;

        if(opt_vect[0])
            cout<<inst_count<<": ==> "<<get<0>(*it)<<" "<<get<1>(*it)<<endl;
        
        inst_count++;
        nru_inst_count++;
        wst_inst_count++;

        if(get<0>(*it) == 'c'){
            curr_proc = NULL;
            
            for(int i=0;i<PL.size();i++){
                if(PL[i]->pid == get<1>(*it)){
                    curr_proc = PL[i];
                    break;
                }
            }
            if(curr_proc == NULL){
                cout<<"Invalid Process"<<endl;
                exit(0);
            }
            if(opt_vect[3]){
                ctxt_switches+=1;
                TOTAL_COST+=130;
            }
            continue;
        }
        else if(get<0>(*it) == 'e'){
            if(opt_vect[0])
                cout<<"EXIT current process "<<get<1>(*it)<<endl;
            
            Process *temp_proc = NULL;
            for(int i=0;i<PL.size();i++){
                if(PL[i]->pid == get<1>(*it)){
                    temp_proc = PL[i];
                    break;
                }
            }

            for(int i=0; i<N_PTEs;i++){
                if(temp_proc->page_table[i].PRESENT == 1){
                    //UNMAP
                    if(opt_vect[0])
                        cout<<" UNMAP "<<get<1>(*it)<<":"<<i<<endl;
                    
                    if(opt_vect[3]){
                        temp_proc->unmaps+=1;
                        TOTAL_COST+=400;
                    }
                    FT[temp_proc->page_table[i].FRAME_NO].p = NULL;
                    FT[temp_proc->page_table[i].FRAME_NO].vpage = -1;
                    Free_list.push_back(&FT[temp_proc->page_table[i].FRAME_NO]);
                    
                    temp_proc->page_table[i].FRAME_NO = 0;

                    if(temp_proc->page_table[i].FILEMAPPED==1 && temp_proc->page_table[i].MODIFIED == 1){
                        if(opt_vect[0])
                            cout<<" FOUT"<<endl;
                        if(opt_vect[3]){
                            temp_proc->fouts+=1;
                            TOTAL_COST+=2400;
                        }
                    }
                    
                    //Resetting
                    temp_proc->page_table[i].PRESENT = 0;
                        
                }
                temp_proc->page_table[i].REFERENCED = 0;
                temp_proc->page_table[i].MODIFIED = 0;
                temp_proc->page_table[i].PAGEDOUT = 0;

            }
            if(opt_vect[3]){
                proc_exits+=1;
                TOTAL_COST+=1250;
            }
            continue;
        }

        else{          
            
            if(get<1>(*it) >= N_PTEs){
                cout<<"address <" << get<1>(*it) <<"> exceeds max virtual address <" <<N_PTEs<<">"<<endl;
                exit(0);
            }
            
            if(opt_vect[3]){
                    TOTAL_COST+=1;
            }
            if(get<0>(*it) == 'r'){

                if(curr_proc->page_table[get<1>(*it)].PRESENT == 0){
                //pagefault handler
                    bool temp_access = false;
                    if(curr_proc->page_table[get<1>(*it)].ACCESSIBLE == 0){                
                        for(int i =0;i<curr_proc->n_vmas;i++){
                            if(get<0>(curr_proc->VMAS[i]) <= get<1>(*it) && get<1>(*it) <= get<1>(curr_proc->VMAS[i]))
                                {  
                                    curr_proc->page_table[get<1>(*it)].ACCESSIBLE = 1;            
                                    temp_access = true;
                                    
                                    if(get<2>(curr_proc->VMAS[i]) == 1)
                                        curr_proc->page_table[get<1>(*it)].WRITE_PROTECT = 1;

                                    if(get<3>(curr_proc->VMAS[i]) == 1)
                                        curr_proc->page_table[get<1>(*it)].FILEMAPPED = 1;           
                                    
                                    break;
                                }
                        }
                        if(!temp_access){
                            if(opt_vect[3]){
                                curr_proc->segv+=1;
                                TOTAL_COST+=340;
                            }
                            if(opt_vect[0])
                                cout<<" SEGV"<<endl;
                                continue;
                        }
                    }

                    
                    frame_t *newframe = getframe(THE_PAGER, Free_list, FT, n_frames);
                    
                    //cout<<Free_list.size()<<endl;

                    if(newframe->vpage != -1){
                        if(opt_vect[0]){
                            cout<<" UNMAP "<<newframe->p->pid<<":"<<newframe->vpage<<endl;
                        }
                        if(opt_vect[3]){
                            newframe->p->unmaps+=1;
                            TOTAL_COST+=400;
                        }
                        if(newframe->p->page_table[newframe->vpage].MODIFIED == 1){
                            if(newframe->p->page_table[newframe->vpage].FILEMAPPED == 1){
                                if(opt_vect[0])
                                    cout<<" FOUT"<<endl;
                                
                                if(opt_vect[3]){
                                    newframe->p->fouts+=1;
                                    TOTAL_COST+=2400;
                                }
                            }
                            else{
                                if(opt_vect[0])
                                    cout<<" OUT"<<endl;
                                if(opt_vect[3]){
                                    newframe->p->outs+=1;
                                    TOTAL_COST+=2700;
                                }    
                                newframe->p->page_table[newframe->vpage].PAGEDOUT = 1;
                            }
                                                        
                        }
                        newframe->p->page_table[newframe->vpage].FRAME_NO = 0;
                        newframe->p->page_table[newframe->vpage].PRESENT = 0;
                    }

                    //RESETTING
                    curr_proc->page_table[get<1>(*it)].PRESENT = 1;
                    curr_proc->page_table[get<1>(*it)].REFERENCED = 0;
                    curr_proc->page_table[get<1>(*it)].MODIFIED = 0;
                
                    if(curr_proc->page_table[get<1>(*it)].PAGEDOUT == 1){
                        
                            if(opt_vect[0])
                                cout<<" IN"<<endl;
                            
                            if(opt_vect[3]){
                                curr_proc->ins+=1;
                                TOTAL_COST+=3100;
                            }
                        
                    }
                    else{
                        if(curr_proc->page_table[get<1>(*it)].FILEMAPPED == 1){
                        
                        if(opt_vect[0])
                            cout<<" FIN"<<endl;
                        
                        if(opt_vect[3]){
                            curr_proc->fins+=1;
                            TOTAL_COST+=2800;
                        }
                        }
                        else{
                        if(opt_vect[0])
                            cout<<" ZERO"<<endl;
                        
                        if(opt_vect[3]){
                            curr_proc->zeros+=1;
                            TOTAL_COST+=140;
                        }
                        }
                    }

                    curr_proc->page_table[get<1>(*it)].FRAME_NO = newframe->frame_no;
                    newframe->p = curr_proc;
                    newframe->vpage = get<1>(*it);
                    newframe->age = 0;
                    cout<<" MAP "<< newframe->frame_no<<endl;

                    if(opt_vect[3]){
                        curr_proc->maps+=1;
                        TOTAL_COST+=300;
                    }
                }
                
                curr_proc->page_table[get<1>(*it)].REFERENCED = 1;    
            }
            else if(get<0>(*it) == 'w'){
                if(curr_proc->page_table[get<1>(*it)].PRESENT == 0){
                //pagefault handler
                    bool temp_access = false;
                    if(curr_proc->page_table[get<1>(*it)].ACCESSIBLE == 0){                
                        for(int i =0;i<curr_proc->n_vmas;i++){
                            if(get<0>(curr_proc->VMAS[i]) <= get<1>(*it) && get<1>(*it) <= get<1>(curr_proc->VMAS[i]))
                                {  
                                    curr_proc->page_table[get<1>(*it)].ACCESSIBLE = 1;            
                                    temp_access = true;
                                    
                                    if(get<2>(curr_proc->VMAS[i]) == 1)
                                        curr_proc->page_table[get<1>(*it)].WRITE_PROTECT = 1;

                                    if(get<3>(curr_proc->VMAS[i]) == 1)
                                        curr_proc->page_table[get<1>(*it)].FILEMAPPED = 1;           
                                    
                                    break;
                                }
                        }
                        if(!temp_access){
                            if(opt_vect[3]){
                                curr_proc->segv+=1;
                                TOTAL_COST+=340;
                            }
                            if(opt_vect[0])
                                cout<<" SEGV"<<endl;
                                continue;
                        }
                    }
                    
                    //cout<<"fdiwehffew"<<endl;

                    frame_t *newframe = getframe(THE_PAGER, Free_list, FT, n_frames);
                    
                    if(newframe->vpage != -1){
                        if(opt_vect[0]){
                            cout<<" UNMAP "<<newframe->p->pid<<":"<<newframe->vpage<<endl;
                        }
                        if(opt_vect[3]){
                            newframe->p->unmaps+=1;
                            TOTAL_COST+=400;
                        }

                        if(newframe->p->page_table[newframe->vpage].MODIFIED == 1){
                            if(newframe->p->page_table[newframe->vpage].FILEMAPPED == 1){
                                if(opt_vect[0])
                                    cout<<" FOUT"<<endl;
                                
                                if(opt_vect[3]){
                                    newframe->p->fouts+=1;
                                    TOTAL_COST+=2400;
                                }
                            }
                            else{
                                if(opt_vect[0])
                                    cout<<" OUT"<<endl;

                                if(opt_vect[3]){
                                    newframe->p->outs+=1;
                                    TOTAL_COST+=2700;
                                }    
                                newframe->p->page_table[newframe->vpage].PAGEDOUT = 1;
                            }
                                                        
                        }
                        newframe->p->page_table[newframe->vpage].FRAME_NO = 0;
                        newframe->p->page_table[newframe->vpage].PRESENT = 0;
                    }

                    //RESETTING
                    curr_proc->page_table[get<1>(*it)].PRESENT = 1;
                    curr_proc->page_table[get<1>(*it)].REFERENCED = 0;
                    curr_proc->page_table[get<1>(*it)].MODIFIED = 0;
                    
                    if(curr_proc->page_table[get<1>(*it)].PAGEDOUT == 1){
                        
                            if(opt_vect[0])
                                cout<<" IN"<<endl;
                            
                            if(opt_vect[3]){
                                curr_proc->ins+=1;
                                TOTAL_COST+=3100;
                            }
                        
                    }
                    else{
                        if(curr_proc->page_table[get<1>(*it)].FILEMAPPED == 1){
                        
                        if(opt_vect[0])
                            cout<<" FIN"<<endl;
                        
                        if(opt_vect[3]){
                            curr_proc->fins+=1;
                            TOTAL_COST+=2800;
                        }
                        }
                        else{
                        if(opt_vect[0])
                            cout<<" ZERO"<<endl;
                        
                        if(opt_vect[3]){
                            curr_proc->zeros+=1;
                            TOTAL_COST+=140;
                        }
                        }
                    }

                    curr_proc->page_table[get<1>(*it)].FRAME_NO = newframe->frame_no;
                    newframe->p = curr_proc;
                    newframe->vpage = get<1>(*it);
                    newframe->age = 0;
                    cout<<" MAP "<< newframe->frame_no<<endl;
                    if(opt_vect[3]){
                        curr_proc->maps+=1;
                        TOTAL_COST+=300;
                    }
                }

                curr_proc->page_table[get<1>(*it)].REFERENCED = 1;

                if(curr_proc->page_table[get<1>(*it)].WRITE_PROTECT == 1){
                    if(opt_vect[3]){
                        curr_proc->segprot+=1;
                        TOTAL_COST+=420;
                    }
                    if(opt_vect[0])
                        cout<<" SEGPROT"<<endl;
                    continue;
                }
            
                curr_proc->page_table[get<1>(*it)].MODIFIED = 1;
            }
            else{
                cout<<"Invalid instruction"<<endl;
            }
        
        }   

        if(opt_vect[5]){
            for(int i=0;i<PL.size();i++){
                PrintPT(PL[i]);
            }
        }
        else if(opt_vect[4]){
            PrintPT(curr_proc);
        }

        if(opt_vect[6]){
            PrintFT(FT, n_frames);
        }
    }

    if(opt_vect[1]){
        for(int i=0;i<PL.size();i++){
            PrintPT(PL[i]);
        }
    }
    if(opt_vect[2]){
        PrintFT(FT, n_frames);
    }
}


int main(int argc, char** argv){
    
    int opt;
    int num_frames;
    char algo;
    int n_processes=-1;
    vector<Process*> Proc_list;
    vector< tuple<char, int> > Instructions;
    deque<frame_t* > Free_list;

    bool ohno;
    bool oP_opt;
    bool oF_opt;
    bool stats;
    bool ox_opt;
    bool oy_opt;
    bool of_opt;
    
    while((opt = getopt(argc, argv, "f:a:o:")) != -1){
        int i=0;
        string o_args;
        switch(opt){
            case 'f':
                if(optarg!=NULL)
                    num_frames = stoi(optarg);
                break;
            case 'a':
                if(optarg!=NULL)
                    algo = optarg[0];
                break;
            case 'o':
                if(optarg!=NULL){
                    o_args = optarg;
                    while(i<o_args.size()){
                        //cout<<"sss"<<endl;
                        if(o_args[i] == 'O'){
                            ohno=true;         
                        }
                        else if(o_args[i] == 'P'){
                            oP_opt=true;
                        }
                        else if(o_args[i] == 'F'){
                            oF_opt = true;
                        }
                        else if(o_args[i] == 'S'){
                            stats=true;
                        }
                        else if(optarg[i] == 'x'){
                            ox_opt=true;
                        }
                        else if(optarg[i] == 'y'){
                            oy_opt=true;
                        }
                        else if(optarg[i] == 'f'){
                            of_opt=true;
                        }
                        // else if(optarg[i] == 'a'){

                        // }
                        else{
                            cout<<"Invalid option"<<endl;
                        }
                        i++;
                    }
                }
                break;
            default:
                exit(0);
        }
    }
    //cout<<"Reading"<<endl;
    //cout<<endl;
    //Reading input file
    vector<bool> opt_vect;
    opt_vect.push_back(ohno);
    opt_vect.push_back(oP_opt);
    opt_vect.push_back(oF_opt);
    opt_vect.push_back(stats);
    opt_vect.push_back(ox_opt);
    opt_vect.push_back(oy_opt);
    opt_vect.push_back(of_opt);


    ifstream inputfile;
    inputfile.open(argv[optind]);

    ifstream rfile;
    rfile.open(argv[optind+1]);

    string line1;
    int proc_counter = 0;

    //Reading Process specifications
    if(inputfile.is_open()){
        while(getline(inputfile, line1)){
            if(line1[0] == '#')
                continue;
                        
            if(n_processes == -1){
                n_processes = stoi(line1);       
            }
            else if(proc_counter!=n_processes){
                Process *p = new Process(stoi(line1));
                
                p->pid = proc_counter;
                proc_counter++;

                int temp_count = p->n_vmas;
                
                while(temp_count!=0){
                    
                    int start_vpage, end_vpage, write_protected, file_protected;

                    getline(inputfile, line1);
                    string temp;
                    istringstream iss(line1);
                    
                    iss >> temp;
                    start_vpage = stoi(temp);
                    
                    iss >> temp;
                    end_vpage = stoi(temp);

                    iss >> temp;
                    write_protected = stoi(temp);

                    iss >> temp;
                    file_protected = stoi(temp);

                    p->VMAS.push_back(make_tuple(start_vpage, end_vpage, write_protected, file_protected));
                    temp_count--;
                }
                Proc_list.push_back(p);
            }
            else{

                string inst_temp;
                char inst_type;
                int proc_num;
                istringstream iss1(line1);
                
                iss1 >> inst_temp;
                inst_type = inst_temp[0];

                iss1 >> inst_temp;
                proc_num = stoi(inst_temp);

                Instructions.push_back(make_tuple(inst_type, proc_num));
            }

        }
    }

    //Reading Random file
    line1="";
    random_total = 0;
    if(rfile.is_open()){
        while(getline(rfile, line1)){
            if(random_total==0){
                random_total = stoi(line1);
                continue;
            }
                
            random_arr.push_back(stoi(line1));
        }
    }

    frame_t frame_table[num_frames];
    for(int i=0;i<num_frames;i++){
        frame_t* nf = &frame_table[i];
        Free_list.push_back(nf);
    }

    for(int i=0;i<num_frames;i++){
        frame_table[i].frame_no = i;
    }
    
    Pager* THE_PAGER;
    if(algo == 'f')
        THE_PAGER = new FIFO();
    else if(algo == 'c')
        THE_PAGER = new CLOCK();
    else if(algo == 'r')
        THE_PAGER = new RANDOM();
    else if(algo == 'e')
        THE_PAGER = new NRU();
    else if(algo == 'a')
        THE_PAGER = new AGING();
    else if(algo == 'w')
        THE_PAGER = new WORKING_SET();
    else{
        cout<<"Invalid algo"<<endl;
        exit(0);
    }
    //Execution of instructions

    //Printing
    // vector<Process*>::iterator iter = Proc_list.begin();
    // cout<<"No. of Processes: "<<n_processes<<endl;
    // cout<<endl;
    // for(;iter!=Proc_list.end();iter++){
    //     cout<<"Process: " << (*iter)->pid<<endl;
    //     cout<<"No. of VMAs: "<<(*iter)->n_vmas<<endl;

    //     vector< tuple<int, int, int, int> >::iterator iter2;
    //     int vma_counter=0;
    //     for(iter2 = (*iter)->VMAS.begin();iter2!=(*iter)->VMAS.end();iter2++){
    //         vma_counter++;
    //         cout<<"VMA: "<<vma_counter<<endl;
    //         cout<<"Starting_Vpage: "<<get<0>(*iter2)<<endl;
    //         cout<<"Ending_Vpage: "<<get<1>(*iter2)<<endl;
    //         cout<<"File_protected: "<<get<2>(*iter2)<<endl;
    //         cout<<"Write_protected: "<<get<3>(*iter2)<<endl;
    //         cout<<endl;
    //     }
    //     cout<<endl;
    // }

    Simulation(Proc_list, Instructions, frame_table, num_frames, opt_vect, Free_list, THE_PAGER);

    if(opt_vect[3]){
        for(int i=0;i<Proc_list.size();i++){
            printf("PROC[%d]: U=%lu M=%lu I=%lu O=%lu FI=%lu FO=%lu Z=%lu SV=%lu SP=%lu\n",
                    Proc_list[i]->pid,
                    Proc_list[i]->unmaps, Proc_list[i]->maps, Proc_list[i]->ins, Proc_list[i]->outs,
                    Proc_list[i]->fins, Proc_list[i]->fouts, Proc_list[i]->zeros,
                    Proc_list[i]->segv, Proc_list[i]->segprot);
        }
        printf("TOTALCOST %lu %lu %lu %llu %lu\n",
                global_inst_count, ctxt_switches, proc_exits, TOTAL_COST, sizeof(pte_t));
    }

    // if(page_table_show){
    //     for(int i=0;i<n_processes;i++)
    //         Print_PT(Proc_list[i]);
    // }

    // if(frame_table_show){
    //     Print_FT(frame_table, num_frames);
    // }

    
    return 0;
}