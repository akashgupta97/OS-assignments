// Homework - 1
// Submitted by - Akash Gupta
// Net ID: ag7637

#include<iostream>
#include<iomanip>
#include<string.h>
#include<fstream>
#include<vector>
#include<tuple>
#include<ctype.h>
using namespace std;


// Symbol class

class Symbol{
    public:
    string s;
    int val;
    int absAddr;
    bool multipleInst = false;
    bool inuseList = false;
    int mod_No;
    int count=0;
};

// class for elements of uselist
class UseSymbol{
    public:
    string s;
    bool used = false;
};

//parse error function
void __parseerror(int errcode, int line_no, int line_offset) 
{
    //Rule-1 checking
	string errstr[] = {
		"NUM_EXPECTED",
		"SYM_EXPECTED",
		"ADDR_EXPECTED",
		"SYM_TOO_LONG",
		"TOO_MANY_DEF_IN_MODULE",
		"TOO_MANY_USE_IN_MODULE",
		"TOO_MANY_INSTR",
	};
    cout<<"Parse Error line "<<line_no<<" offset "<<line_offset<<": "<<errstr[errcode]<<endl; 
	exit(0);
}

//getToken function returns a string containing tokens as required

string getToken(tuple<string, int, int> tup){
    string s1 = get<0>(tup);
    return s1;
}


//readInt
int readInt(tuple<string, int, int>  tup){
    string tok = getToken(tup);
    int line_no = get<1>(tup);
    int line_offset = get<2>(tup);
    if(tok.length()==0)
        __parseerror(0, line_no, line_offset);
    for(int i=0;i<tok.length();i++){
        if(isdigit(tok[i])!=1){
            __parseerror(0, line_no, line_offset);
        }
    }
    //cout<<tok<<endl;
    int i = stoi(tok);
    if(i > 1073741823){
        __parseerror(0, line_no, line_offset);
    }
    return i;
}

//readSymbol
string readSymbol(tuple<string, int, int> tup){
    string tok = getToken(tup);
    int line_no = get<1>(tup);
    int line_offset = get<2>(tup);
    
    if(tok.length()>16){
        __parseerror(3, line_no, line_offset);
    }
    if(isalpha(tok[0])==0){
        __parseerror(1, line_no, line_offset);
    }

    for(int i=1;i<tok.length();i++){
        if(isalnum(tok[i])==1){
            __parseerror(1, line_no, line_offset);
        }
    }
    return tok;
}


//readIAER
string readIAER(tuple<string, int, int> tup){
    string tok = getToken(tup);
    int line_no = get<1>(tup);
    int line_offset = get<2>(tup);

    if(tok=="I" || tok=="A" || tok=="E" || tok=="R"){
        return tok;
    }
    else{
        __parseerror(2,line_no,line_offset);
    }
    return "";
}

//helper function to read contents from the input file 
vector< tuple<string, int, int> > readFileContents(string fname){

    ifstream input_file(fname.c_str());
    string line;
    int line_no=1;
    int line_offset;
    int tok_length;
    char *line_start;
    int end_line_off=0;
    char* tok;
    //string last;
    int end_indic = 0;
    tuple <string, int, int> tok_set;
    vector < tuple<string, int, int> > token_list;
    while(getline(input_file, line)){
        line_start = &line[0];
        end_indic = input_file.eof();        
        end_line_off = strlen(line_start);
        if(strlen(line_start) == 0){
            line_no+=1;
            continue;
        }
        
        tok = strtok(line_start," \t");
                
        char* rem;
        if(tok!=NULL){
            line_offset=strlen(line_start) - strlen(strstr(line_start,tok)) + 1;
        }

        while(tok!=NULL){
            tok_length = strlen(tok);
            
            //cout<<"Token: "<<line_no<<":"<<line_offset<<" : "<<tok<<endl;
            tok_set = make_tuple(tok, line_no, line_offset);
            token_list.push_back(tok_set);
            rem = strtok(NULL,"");
            tok = strtok(rem, " \t");
            
            if(tok!=NULL){
                //last = tok;
                //cout<<tok<<endl;
                line_offset = line_offset + tok_length + strlen(rem) - strlen(strstr(rem,tok)) + 1;
                //cout<<tok<<endl;
            }
        }
        line_no+=1;    
        }
        if(end_indic == 0)
            end_line_off+=1;
                
        if(line_no==1)
            tok_set = make_tuple("", line_no, end_line_off);
            //cout<<"Final Spot in File : line="<<line_no<<" offset="<<end_line_off+1<<endl;
        else
            tok_set = make_tuple("", line_no-1, end_line_off);
            //cout<<"Final Spot in File : line="<<line_no-1<<" offset="<<end_line_off+1<<endl;
        
        
        token_list.push_back(tok_set);
        //cout<<tok_length<<endl;
        input_file.close();
        return token_list;
}


//Starting pass-1 function
vector<Symbol> Pass1(string fname){


    vector<Symbol> sym_table;
    int counter=0;
    int temp_mod_No = 0;
    int Tot_Inst = 0;
   
    vector < tuple<string, int, int> > tokens;
    tokens = readFileContents(fname);
        
    vector< tuple<string, int, int> >:: iterator it = tokens.begin();
    while(it!=tokens.end()-1){
        temp_mod_No++;

        string sym_str;
        // Definition list
        int DefCount = readInt(*it);
        
        if(DefCount > 16){
            __parseerror(4, get<1>(*it), get<2>(*it));
        }

        vector<string> def_list;
        int counter = 0;
        while(counter<DefCount){
            //Getting symbol string
            ++it;
            sym_str = readSymbol(*it);
            
            //Getting symbol value
            
            ++it;
            
            int val =  readInt(*it);
            //cout<<"wdqwdq"<<endl;
            
            int indic = 0;
            for(int k=0;k<sym_table.size();k++){
                if(sym_str == sym_table[k].s){
                    indic=1;
                    sym_table[k].multipleInst = true;
                    sym_table[k].count++;
                    break;
                }
            }
            if(indic==0){
                Symbol sym1;
                sym1.s = sym_str;
                sym1.val = val;
                sym1.absAddr = Tot_Inst + val;
                sym1.mod_No = temp_mod_No;
                sym1.count = 1;
                sym_table.push_back(sym1);
            }

            counter++;
        }
        
        //Use list
        ++it;        
        int UseCount = readInt(*it);
        
        if(UseCount > 16){
            __parseerror(5, get<1>(*it), get<2>(*it));
        }
        counter = 0;
        
        while(counter<UseCount){
            ++it;
            sym_str = readSymbol(*it);
            counter++;
        }

        //Read Instructions
        ++it;
        int InstCount = readInt(*it);
        Tot_Inst += InstCount;

        if(Tot_Inst>512){
            __parseerror(6, get<1>(*it), get<2>(*it));
        }
    

        counter=0;
        while(counter<InstCount){
            ++it;
            string AddrType = readIAER(*it);

            ++it;
            int addr = readInt(*it);
            counter++;
        } 
        
        // Rule-5 checking
        for(int i=0;i<sym_table.size();i++){
                        
            for(int p=0;p<sym_table[i].count;p++){
                if(sym_table[i].absAddr >= Tot_Inst){
                cout<<"Warning: Module "<<temp_mod_No<<": " + sym_table[i].s 
            + " too big " + to_string(sym_table[i].val) + " (max=" + to_string(InstCount-1) + ") assume zero relative"<<endl;
                sym_table[i].val = 0;
                sym_table[i].absAddr = (Tot_Inst - InstCount) + sym_table[i].val;
            }
            }
            sym_table[i].count=0;
       
        }
        ++it;
    }
    
    return sym_table;
}
//End of pass-1 function

//Starting Pass-2 function
void Pass2(string fname, vector<Symbol> &symbol_table){

    int Module_Base = 0;
    int mem_no = 0;
    int temp_mod_No = 0;

    vector < tuple<string, int, int> > tokens;
    tokens = readFileContents(fname);
        
    vector< tuple<string, int, int> >:: iterator it = tokens.begin();
    while(it!=tokens.end()-1){
      
        temp_mod_No++;
        string sym_str;
      
        // Definition list
        int DefCount = readInt(*it);
        
        it+=(2*DefCount + 1);        
        //Use list
        
        int UseCount = readInt(*it);
        vector<UseSymbol> useList;

        int counter =0;

        while(counter<UseCount){
            ++it;
            UseSymbol sym_use;
            sym_str = readSymbol(*it);
            sym_use.s = sym_str;
            useList.push_back(sym_use);

            for(int k=0;k<symbol_table.size();k++){
                if(symbol_table[k].s == sym_str){
                    symbol_table[k].inuseList = true;
                }
            }

            counter++;
        }

        //Read Instructions
        ++it;
        int InstCount = readInt(*it);
            
        counter=0;
        while(counter<InstCount){
            ++it;
            string AddrType = readIAER(*it);

            ++it;
            int addr = readInt(*it);
            
            int opcode = addr/1000;
            int operand = addr%1000;

            int mem_addr = 0;
            string error="";
            
            //Rule-10 checking
            if(AddrType == "I"){
                if(addr >= 10000){
                    mem_addr = 9999;
                    error = "Error: Illegal immediate value; treated as 9999";
                }
                else{
                    mem_addr = addr;
                }  
            }
            else{
            if(opcode>=10){
                //Rule-11 checking
                error = "Error: Illegal opcode; treated as 9999";
                mem_addr = 9999;
            }
            else if(AddrType == "A"){
                if(operand>=512){
                    //Rule-8 checking
                    error = "Error: Absolute address exceeds machine size; zero used";
                    mem_addr = opcode*1000;
                }
                else{
                    mem_addr = addr;
                }
            }
            else if(AddrType == "E"){
                
                if(operand>=useList.size()){
                    //Rule-6 checking
                    error = "Error: External address exceeds length of uselist; treated as immediate";
                    mem_addr = addr;
                }
                else{
                int indic=0;
                useList[operand].used = true;
                
                for(int k=0;k<symbol_table.size();k++){
                                        
                    if(useList[operand].s == symbol_table[k].s){
                        mem_addr = opcode*1000 + symbol_table[k].absAddr;
                        indic=1;
                        break;
                    }
                }
                if(indic==0){
                    //Rule-3 checking
                    mem_addr = opcode*1000;
                    error = "Error: " + useList[operand].s + " is not defined; zero used";
                }

            }
            }
            else{
                if(operand>=InstCount){
                    //Rule-9 checking
                    mem_addr = opcode*1000 + Module_Base;
                    error = "Error: Relative address exceeds module size; zero used";
                }
                else{
                    mem_addr = Module_Base + addr;
                }
            }
            }
            counter++;
            cout<<setfill('0')<<setw(3)<<mem_no<<": ";
            mem_no++;
            cout<<setfill('0')<<setw(4)<<mem_addr;
            if(error!="")
                cout<<" "<<error;
            cout<<endl;
        } 
        //Rule-7 checking
        for(int i=0;i<useList.size();i++){
            if(useList[i].used == false){
                cout<<"Warning: Module "<<temp_mod_No<<": "<<useList[i].s<<" appeared in the uselist but was not actually used"<<endl;
            }
        }
        Module_Base += InstCount;
        ++it;
    }
}
//End of Pass-2 function

//Main function to drive the linker and call out the above pass functions
int main(int argc, char** argv){

    string fname = argv[1];
    
    vector<Symbol> s1 = Pass1(fname);

    vector<Symbol>:: iterator it1 = s1.begin();
    cout<<"Symbol Table"<<endl;
    for(;it1!=s1.end();it1++){
        cout<<(*it1).s<<"="<<(*it1).absAddr;
        //Rule-2 checking
        if((*it1).multipleInst){
            cout<<" Error: This variable is multiple times defined; first value used";
        }
        cout<<endl;
    }
    cout<<"\n";
    cout<<"\n";
    cout<<"Memory Map"<<endl;
    
    Pass2(fname, s1);
    cout<<"\n";
    cout<<"\n";
    cout<<"\n";
    //Rule-4 checking
    for(int i=0;i<s1.size();i++){
        if(s1[i].inuseList==false){
            cout<<"Warning: Module "<<s1[i].mod_No<<": "<<s1[i].s<<" was defined but never used"<<endl;
        }
    }

    return 0;
}