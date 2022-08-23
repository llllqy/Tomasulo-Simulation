#include <iostream>
#include <iomanip>
#include <cmath>
#include <fstream>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <vector>
using namespace std;

const int addRS = 3;
const int mulRS = 2;
const int loadRS = 3;
const int storeRS = 2;

class instruction
{
public:
    char type[100];
    char dest_reg[100];
    char Q_j[100];
    char Q_k[100];
    double num;
    double rs = 0;
    double issue = -1;
    double completion = -1;
    double written = -1;
    int load = 0;
};

const int maxInst = 16;//maximum number of the instrutions
const int Reg_Num = 6;  //the numvers of registers

float data_Reg[Reg_Num] = { 3, 5, 7, 9, 11, 16 };

typedef struct dataRegister
{
    char name[2];
    double data;
    int tag=0;
    bool busy=false;
} dataRegister;

class reservation_station
{public:
    char name[100]="NULL";
    double num;
    double V_j=NULL;
    double V_k=NULL;
    int Q_j=0;
    int Q_k=0;
    int instr=-1;
    int cycle_count=0;
    int cycles_required=-1;
    bool executing=false;
    bool busy = false;
};

class load_reservation_station
{
public:
    char name_load[100]="NULL";
    double num;
    double address=NULL;
    int instr=-1;
    int tag=0;
    int cycle_count=0;
    int cycles_required=-1;
    bool executing=false;
    bool busy=false;
} ;

class cycles_assumption
{
public:
    int MULTI = 10;
    int DIVIDE = 40;
};

bool issueSuccessful = false;

int main() 
{
    //Here is where to change the input txt
	//const char* filename = "LoadExample.txt";
    //const char* filename = "RAWExample.txt";
    //const char* filename = "WARExample.txt";
	//const char* filename = "WAWExample.txt";
	//const char* filename = "STALLExample.txt";
    //const char* filename = "LongExample.txt";
    //const char* filename = "LoadExample.txt";

    cycles_assumption Cycles;
  
    dataRegister  data_registers[Reg_Num];
    
    char rs_num[3];
    for (int i=0; i < Reg_Num; i++){
        strcpy(data_registers[i].name, "F");
        sprintf(rs_num, "%i", i*2);
        strcat(data_registers[i].name, rs_num);
        data_registers[i].data = data_Reg[i];
    }
    
    vector<reservation_station> add_reservation_station;
    add_reservation_station.resize(addRS);
    for (int j=0; j < addRS; j++) 
    {
        add_reservation_station[j].num = j + 1;
    }

    vector<reservation_station> mul_reservation_station;
    mul_reservation_station.resize(mulRS);
    for (int j=0; j < mulRS; j++)
    {
        mul_reservation_station[j].num = addRS + j +1;
    }

    vector<load_reservation_station> load_reserv_stat;
    load_reserv_stat.resize(loadRS);
    for (int i=0; i < loadRS; i++) 
    {
        load_reserv_stat[i].num = addRS + mulRS + i + 1;
    }

    vector<load_reservation_station> store_reserv_stat;
    store_reserv_stat.resize(storeRS);
    for (int i=0; i < storeRS; i++) 
    {
        store_reserv_stat[i].num = addRS + mulRS + loadRS + i + 1;
    }
    
   //初始化一些变量
    double cdb_data = 0;
    
    int completedInstr = 0;
    int MUL_output = 1;
    int ADD_output = 1;
    int writtenInstr = 0;
   
    // start reading in these new instructions
    FILE *inFile;
    char mystring[1000];

    inFile = fopen(filename, "r");
    if (inFile == NULL)
    {
        cout<<"Could not open file %s"<<filename;
        return 1;
    }
    int Instr_issued = 0;
    int line = 0; //count the lines
    int fini_rs = -1;//record if rs is finished
    instruction list_inst[maxInst];
    bool cdb_available = false;
    fgets(mystring, 1000, inFile);
    do{
        const char nextline[] = "\t";
        const char space[] = " ";
        //const char m[] = "\n";
        char *str_token;
        
        str_token = strtok(mystring, nextline);
        list_inst[line].num = atof(str_token);
        strcpy(list_inst[line].type, strtok(str_token, space)); 
        int inst_Ld = strcmp(list_inst[line].type, "LD");


        str_token = strtok(NULL, space);
        strcpy(list_inst[line].dest_reg, str_token);        
        str_token = strtok(NULL, space);
       if (inst_Ld == 0)
        {
            list_inst[line].load = atoi(str_token);
        }
        else 
        {
            strcpy(list_inst[line].Q_j, str_token);
        }
        
        /*在不是load or store时读入最后一段 */
        int is_ld = strcmp(list_inst[line].type, "LD");
        int is_sd = strcmp(list_inst[line].type, "SD");

        if (is_ld * is_sd != 0)
        {
            str_token = strtok(NULL, space);
            //str_token = strtok(NULL, m);
            strcpy(list_inst[line].Q_k, str_token);
        }

        if (is_ld * is_sd == 0)
        {
            strcpy(list_inst[line].Q_k, " ");
        }
        //start next instruction
        line ++;
    } while (fgets(mystring, 1000, inFile) != NULL);
    fclose(inFile);
    
    int cycle_main = 0;  //record the number of cycle
    while (writtenInstr < line)
    {
        cout << endl;
        cout << "Cycle " << cycle_main+1 << endl << endl;

        //read in instruction
        if (fini_rs != -1) 
        {
            // broadcast_data
            for (int i=0; i < addRS; i++) 
            {
                if (add_reservation_station[i].Q_j == fini_rs) 
                {
                    add_reservation_station[i].V_j = cdb_data;
                    add_reservation_station[i].Q_j = 0;
                    if (add_reservation_station[i].Q_j == 0 and add_reservation_station[i].Q_k == 0) 
                    {
                        add_reservation_station[i].executing = true;
                    }
                }
                if (add_reservation_station[i].Q_k == fini_rs)
                {
                    add_reservation_station[i].V_k = cdb_data;
                    add_reservation_station[i].Q_k = 0;
                    if (add_reservation_station[i].Q_j == 0 and add_reservation_station[i].Q_k == 0)
                    {
                        add_reservation_station[i].executing = true;
                    }
                }
                
                // remove data from reservation station
                if (add_reservation_station[i].num == fini_rs) 
                {
                    add_reservation_station[i].busy = false;
                    add_reservation_station[i].cycle_count = 0;
                    add_reservation_station[i].cycles_required = -1;
                    add_reservation_station[i].executing = false;
                }
            }
            for (int i=0; i < mulRS; i++) 
            {
                if (mul_reservation_station[i].Q_j == fini_rs) 
                {
                    mul_reservation_station[i].V_j = cdb_data;
                    mul_reservation_station[i].Q_j = 0;
                    if (mul_reservation_station[i].Q_j == 0 and mul_reservation_station[i].Q_k == 0) 
                    {
                        mul_reservation_station[i].executing = true;
                    }
                }
                if (mul_reservation_station[i].Q_k == fini_rs)
                {
                    mul_reservation_station[i].V_k = cdb_data;
                    mul_reservation_station[i].Q_k = 0;
                    if (mul_reservation_station[i].Q_j == 0 and mul_reservation_station[i].Q_k == 0) 
                    {
                        mul_reservation_station[i].executing = true;
                    }
                }
                    
                // clear reservation station
                if (mul_reservation_station[i].num == fini_rs) 
                {
                    mul_reservation_station[i].busy = false;
                    mul_reservation_station[i].cycle_count = 0;
                    mul_reservation_station[i].cycles_required = -1;
                    mul_reservation_station[i].executing = false;
                }
            }
            for (int i=0; i < loadRS; i++) 
            {
                if (load_reserv_stat[i].tag == fini_rs) 
                {
                    load_reserv_stat[i].address = cdb_data;
                    load_reserv_stat[i].tag = 0;
                    load_reserv_stat[i].executing = true;
                }
                // clear reservation station
                if (load_reserv_stat[i].num == fini_rs)
                {
                    load_reserv_stat[i].busy = false;
                    load_reserv_stat[i].cycle_count = 1;
                    load_reserv_stat[i].cycles_required = -1;
                    load_reserv_stat[i].executing = false;
                }
            }
            for (int i=0; i < storeRS; i++) 
            {
                if (store_reserv_stat[i].tag == fini_rs)
                {
                    store_reserv_stat[i].address = cdb_data;
                    store_reserv_stat[i].tag = 0;
                    store_reserv_stat[i].executing = true;
                }
                // clear reservation station
                if (store_reserv_stat[i].num == fini_rs) 
                {
                    store_reserv_stat[i].busy = false;
                    store_reserv_stat[i].cycle_count = 1;
                    store_reserv_stat[i].cycles_required = -1;
                    store_reserv_stat[i].executing = false;
                }
            }
            // write to dataRegister
            for (int i=0; i < Reg_Num; i++) 
            {
                if (data_registers[i].tag == fini_rs) 
                {
                    data_registers[i].data = cdb_data;
                    data_registers[i].busy = false;
                    data_registers[i].tag = 0;
                }
            }

            // just for bookkeeping - instruction written cycle number
            for (int j=0; j < Instr_issued; j++)
            {
                if (list_inst[j].rs == fini_rs && list_inst[j].written == -1) list_inst[j].written = cycle_main;
            }

            writtenInstr ++;
            // reset
            fini_rs = -1;
            cdb_data = 0;
        }
    
        
//start issue
        if (Instr_issued < line) 
        {
            int isAdd = strcmp(list_inst[Instr_issued].type, "ADDD");
            int isSub = strcmp(list_inst[Instr_issued].type, "SUBD");
            int isLoad = strcmp(list_inst[Instr_issued].type, "LD");
            int isStore = strcmp(list_inst[Instr_issued].type, "SD");
           
            // issue these instructions
            if (isAdd == 0 || isSub == 0) 
            {
                int addcycle = 2;
                int subcycle = 2;
                // adding to reservation station
                for (int l=0; l < addRS; l++) 
                {
                    if (issueSuccessful == false && add_reservation_station[l].busy == false)
                    {
                        for (int i=0; i < Reg_Num; i++)
                        {
                            // j register
                            if (strcmp(list_inst[Instr_issued].Q_j, data_registers[i].name) == 0)
                            {
                                if (data_registers[i].tag == 0)
                                {
                                    add_reservation_station[l].V_j = data_registers[i].data;
                                }
                                else
                                {
                                    add_reservation_station[l].Q_j = data_registers[i].tag;
                                }
                            }
                            // k register
                            if (strncmp(list_inst[Instr_issued].Q_k, data_registers[i].name, 2) == 0)
                            {
                                if (data_registers[i].tag == 0)
                                {
                                    add_reservation_station[l].V_k = data_registers[i].data;
                                }
                                else
                                {
                                    add_reservation_station[l].Q_k = data_registers[i].tag;
                                }
                            }
                            // destination register
                            if (strcmp(list_inst[Instr_issued].dest_reg, data_registers[i].name) == 0)
                            {
                                data_registers[i].tag = add_reservation_station[l].num;
                            }
                        }
                        add_reservation_station[l].busy = true;
                        list_inst[Instr_issued].rs = add_reservation_station[l].num;
                        add_reservation_station[l].cycle_count = 0;
                        list_inst[Instr_issued].issue = cycle_main + 1;
                        issueSuccessful = true;
                        if (add_reservation_station[l].Q_j == 0 and add_reservation_station[l].Q_k == 0)
                        {
                            add_reservation_station[l].executing = true;
                        }
                        if (strcmp(list_inst[Instr_issued].type, "ADDD") == 0)
                        {
                            strcpy(add_reservation_station[l].name, "ADDD");
                            add_reservation_station[l].cycles_required = addcycle;
                        }
                        else
                        {
                            add_reservation_station[l].cycles_required = subcycle;
                            strcpy(add_reservation_station[l].name, "SUBD");
                        }
                    }
                }
            }
            else if ( isLoad == 0) 
            {
                for (int l=0; l < loadRS; l++) 
                {
                    if (issueSuccessful == false && load_reserv_stat[l].busy == false)
                    {
                        for (int i = 0; i < Reg_Num; i++)
                        {
                            // load value
                            load_reserv_stat[l].address = list_inst[Instr_issued].load;
                            // destination register
                            if (strncmp(list_inst[Instr_issued].dest_reg, data_registers[i].name, 2) == 0)
                            {
                                data_registers[i].tag = load_reserv_stat[l].num;
                            }
                        }
                        load_reserv_stat[l].busy = true;
                        int loadcycle = 3;
                        load_reserv_stat[l].cycles_required = loadcycle; 
                        list_inst[Instr_issued].rs = load_reserv_stat[l].num;
                        load_reserv_stat[l].cycle_count = 1;
                        list_inst[Instr_issued].issue = cycle_main + 1;
                        issueSuccessful = true;
                        load_reserv_stat[l].executing = true;                           
                    }
                }
            }
            else if (isStore == 0) 
            {
                int storecycles = 3;
                for (int l=0; l < storeRS; l++) 
                {
                    if (issueSuccessful == false && store_reserv_stat[l].busy == false)
                    {
                        for (int i=0; i < Reg_Num; i++) 
                        {
                            if (strcmp(list_inst[Instr_issued].dest_reg, data_registers[i].name) == 0)
                            {
                                if (data_registers[i].tag == 0)
                                {
                                    store_reserv_stat[l].address = data_registers[i].data;
                                }
                                else
                                {
                                    store_reserv_stat[l].tag = data_registers[i].tag;
                                }
                            }
                            // destination register
                            if (strncmp(list_inst[Instr_issued].Q_j, data_registers[i].name, 2) == 0)
                            {
                                data_registers[i].tag = store_reserv_stat[l].num;
                            }
                        }
                        store_reserv_stat[l].busy = true;
                        store_reserv_stat[l].cycles_required = storecycles;
                        list_inst[Instr_issued].rs = store_reserv_stat[l].num;
                        store_reserv_stat[l].cycle_count = 1;
                        list_inst[Instr_issued].issue = cycle_main + 1;
                        issueSuccessful = true;
                        if (store_reserv_stat[l].tag == 0)
                        {
                            store_reserv_stat[l].executing = true;
                        }
                    }
                }
            }
            else 
            {
                // adding to reservation station
                for (int l=0; l < mulRS; l++) 
                {
                    if (issueSuccessful == false && mul_reservation_station[l].busy == false)
                    {
                        for (int i=0; i < Reg_Num; i++)
                        {
                            // destination register
                            if (strcmp(list_inst[Instr_issued].dest_reg, data_registers[i].name) == 0)
                            {
                                data_registers[i].tag = mul_reservation_station[l].num;
                            }
                            // j register
                            if (strcmp(list_inst[Instr_issued].Q_j, data_registers[i].name) == 0)
                            {
                                if (data_registers[i].tag == 0)
                                {
                                    mul_reservation_station[l].V_j = data_registers[i].data;
                                }
                                else {
                                    mul_reservation_station[l].Q_j = data_registers[i].tag;
                                }
                            }
                            // k register
                            if (strncmp(list_inst[Instr_issued].Q_k, data_registers[i].name, 2) == 0)
                            {
                                if (data_registers[i].tag == 0)
                                {
                                    mul_reservation_station[l].V_k = data_registers[i].data;
                                }
                                else
                                {
                                    mul_reservation_station[l].Q_k = data_registers[i].tag;
                                }
                            }
                        }
                        mul_reservation_station[l].busy = true;
                        list_inst[Instr_issued].rs = mul_reservation_station[l].num;
                        mul_reservation_station[l].cycle_count = 0;
                        list_inst[Instr_issued].issue = cycle_main + 1;
                        issueSuccessful = true;
                        if (mul_reservation_station[l].Q_j == 0 and mul_reservation_station[l].Q_k == 0)
                        {
                            mul_reservation_station[l].executing = true;
                        }
                        if (strcmp(list_inst[Instr_issued].type, "MULTD") == 0)
                        {
                            strcpy(mul_reservation_station[l].name, "MULTD");
                            mul_reservation_station[l].cycles_required = Cycles.MULTI;

                        }
                        else {
                            mul_reservation_station[l].cycles_required = Cycles.DIVIDE;
                            strcpy(mul_reservation_station[l].name, "DIVD");
                        }
                    }
                }
            }
            
        }
        
        // ordered by increasing reservation station number
        cdb_available = false;
        for (int i=0; i < addRS; i++)
        {
            // completing instructions
            if (add_reservation_station[i].cycle_count == add_reservation_station[i].cycles_required)
            {
                for (int j=0; j < Instr_issued; j++) 
                {
                    if (list_inst[j].rs == add_reservation_station[i].num) 
                    {
                        if (list_inst[j].completion == -1) 
                        {
                            list_inst[j].completion = cycle_main+1;
                            completedInstr += 1;
                        }
                    }
                }
                if (!cdb_available) 
                {
                    if (strcmp(add_reservation_station[i].name, "ADDD") == 0) 
                    {
                        cdb_data = add_reservation_station[i].V_j + add_reservation_station[i].V_k;
                    }
                    else 
                    {
                        cdb_data = add_reservation_station[i].V_j - add_reservation_station[i].V_k;
                    }
                    cdb_available = true;
                    fini_rs = add_reservation_station[i].num;
                }
            }
            // executing instructions
            if (add_reservation_station[i].busy and add_reservation_station[i].executing) 
            {
                if (add_reservation_station[i].cycle_count < add_reservation_station[i].cycles_required) 
                {
                    add_reservation_station[i].cycle_count += 1;
                }
            }
        }
        for (int i=0; i < mulRS; i++) 
        {
            // completing instructions
            if (mul_reservation_station[i].cycle_count == mul_reservation_station[i].cycles_required) 
            {
                for (int j=0; j < Instr_issued; j++)
                {
                    //tracking completion cycle
                    if (list_inst[j].rs == mul_reservation_station[i].num)
                    {
                        if (list_inst[j].completion == -1) 
                        {
                            list_inst[j].completion = cycle_main+1;
                            completedInstr ++;
                        }
                    }
                }
                if (!cdb_available) 
                {
                    if (strcmp(mul_reservation_station[i].name, "MULTD") == 0) 
                    {
                        cdb_data = mul_reservation_station[i].V_j * mul_reservation_station[i].V_k;
                    }
                    else
                    {
                        cdb_data = mul_reservation_station[i].V_j / mul_reservation_station[i].V_k;
                    }
                    cdb_available = true;
                    fini_rs = mul_reservation_station[i].num;
                }
            }
            // executing instructions
            if (mul_reservation_station[i].busy and mul_reservation_station[i].executing) 
            {
                // only increment if not yet reached
                if (mul_reservation_station[i].cycle_count < mul_reservation_station[i].cycles_required) 
                {
                    mul_reservation_station[i].cycle_count ++;
                }
            }
        }
        for (int i=0; i < loadRS; i++) 
        {
            // completing instructions
            if (load_reserv_stat[i].cycle_count == load_reserv_stat[i].cycles_required) 
            {
                for (int j=0; j < Instr_issued; j++) 
                {
                    //tracking completion cycle
                    if (list_inst[j].rs == load_reserv_stat[i].num) 
                    {
                        if (list_inst[j].completion == -1) 
                        {
                            list_inst[j].completion = cycle_main+1;
                            completedInstr += 1;
                        }
                    }
                }
                if (!cdb_available) 
                {
                    cdb_data = load_reserv_stat[i].address;
                    cdb_available = true;
                    fini_rs = load_reserv_stat[i].num;
                }
            }
            // executing instructions
            if (load_reserv_stat[i].busy and load_reserv_stat[i].executing)
            {
                if (load_reserv_stat[i].cycle_count < load_reserv_stat[i].cycles_required) 
                {
                    load_reserv_stat[i].cycle_count ++;
                }
            }
        }
        for (int i=0; i < storeRS; i++) 
        {
            // completing instructions
            if (store_reserv_stat[i].cycle_count == store_reserv_stat[i].cycles_required)
            {
                for (int j=0; j < Instr_issued; j++) 
                {
                    //tracking completion cycle
                    if (list_inst[j].rs == store_reserv_stat[i].num) 
                    {
                        if (list_inst[j].completion == -1) 
                        {
                            list_inst[j].completion = cycle_main+1;
                            completedInstr += 1;
                        }
                    }
                }
                if (!cdb_available) 
                {
                    cdb_data = store_reserv_stat[i].address;
                    cdb_available = true;
                    fini_rs = store_reserv_stat[i].num;
                }
            }
        }
    
//print the results
        //print instruction status
        cout << "Instruction Status:" << endl;
        cout << left << setw(15) << setfill(' ') << "Instruction";
        cout << left << setw(6) << setfill(' ') << "j";
        cout << left << setw(6) << setfill(' ') << "k";
        cout << left << setw(6) << setfill(' ') << "l";
        cout << left << setw(8) << setfill(' ') << "Issue";
        cout << left << setw(12)<< setfill(' ') << "Completion";
        cout << left << setw(0) << setfill(' ') << "Written";
        cout << endl;

        for (int j=0; j < line; j++) 
        {
            cout << left << setw(15) << setfill(' ') << list_inst[j].type;
            if (strcmp(list_inst[j].type, "LD") == 0)
            {
                cout << left << setw(6) << setfill(' ') << list_inst[j].dest_reg;
                cout << left << setw(6) << setfill(' ') << list_inst[j].load;
                cout << left << setw(6) << setfill(' ') << "";
            }
            else 
            {
                cout << left << setw(6) << setfill(' ') << list_inst[j].dest_reg;
                cout << left << setw(6) << setfill(' ') << list_inst[j].Q_j;
                cout << left << setw(0) << setfill(' ') << list_inst[j].Q_k[0];
                cout << left << setw(5) << setfill(' ') << list_inst[j].Q_k[1];
            }
            if (list_inst[j].issue == -1)
            {
                cout << left << setw(8) << setfill(' ') << " ";
            }
            else 
                cout << left << setw(8) << setfill(' ') << list_inst[j].issue;

            if (list_inst[j].completion == -1)
            {
                cout << left << setw(12) << setfill(' ') << " ";
            }
            else  cout << left << setw(12) << setfill(' ') << list_inst[j].completion;

            if (list_inst[j].written == -1) 
            {
                cout << left << setw(0) << setfill(' ') << " ";
            }
            else 
                cout << left << setw(0) << setfill(' ') << list_inst[j].written + 1;

            cout << endl;
        }
        //print reservation stations' state
        cout << endl << "Reservation Stations:" << endl;
        cout << left << setw(8) << setfill(' ') << "Time";
        cout << left << setw(8) << setfill(' ') << "Name";
        cout << left << setw(8) << setfill(' ') << "Busy";
        cout << left << setw(8) << setfill(' ') << "Op";
        cout << left << setw(8) << setfill(' ') << "Qj";
        cout << left << setw(8) << setfill(' ') << "Vj";
        cout << left << setw(8) << setfill(' ') << "Qk";
        cout << left << setw(8) << setfill(' ') << "Vk" << endl;

        for (int i=0; i < addRS; i++) 
        {
            if (add_reservation_station[i].cycles_required == -1)
            {
                cout << left << setw(8) << setfill(' ') << "";
            }
            else if (add_reservation_station[i].cycle_count == 0)
                cout << left << setw(8) << setfill(' ') << add_reservation_station[i].cycles_required;
            else if ((add_reservation_station[i].cycles_required - add_reservation_station[i].cycle_count) > -1 && ADD_output == 1)
            {
                cout << left << setw(8) << setfill(' ') << add_reservation_station[i].cycles_required - add_reservation_station[i].cycle_count + 1;
                if (add_reservation_station[i].cycles_required - add_reservation_station[i].cycle_count == 0)
                    ADD_output = 0;
            }
            else
            {
                cout << left << setw(8) << setfill(' ') << "0";
                ADD_output = 1;
            }                
            cout << left << setw(0) << setfill(' ') << "[";
            cout << left << setw(0) << setfill(' ') << add_reservation_station[i].num;
            cout << left << setw(6) << setfill(' ') << "]";
            if (add_reservation_station[i].busy == 0) 
            {
                cout << left << setw(8) << setfill(' ') << " ";
                cout << left << setw(8) << setfill(' ') << " ";
                cout << left << setw(8) << setfill(' ') << " ";
                cout << left << setw(8) << setfill(' ') << " ";
            }
            else 
            {
                cout << left << setw(8) << setfill(' ') << add_reservation_station[i].busy;
                cout << left << setw(8) << setfill(' ') << add_reservation_station[i].name;
                if (add_reservation_station[i].Q_j == 1)
                {
                    cout << left << setw(6) << setfill(' ') << "add1";
                    cout << left << setw(2) << setfill(' ') << "";
                }
                else if (add_reservation_station[i].Q_j == 2)
                {
                    cout << left << setw(6) << setfill(' ') << "add2";
                    cout << left << setw(2) << setfill(' ') << "";
                }
                else if (add_reservation_station[i].Q_j == 3)
                {
                    cout << left << setw(6) << setfill(' ') << "add3";
                    cout << left << setw(2) << setfill(' ') << "";
                }
                else if (add_reservation_station[i].Q_j == 4)
                {
                    cout << left << setw(6) << setfill(' ') << "multi1";
                    cout << left << setw(2) << setfill(' ') << "";
                }
                else if (add_reservation_station[i].Q_j == 5)
                {
                    cout << left << setw(6) << setfill(' ') << "multi2";
                    cout << left << setw(2) << setfill(' ') << "";
                }
                else if (add_reservation_station[i].Q_j == 6)
                {
                    cout << left << setw(6) << setfill(' ') << "load1";
                    cout << left << setw(2) << setfill(' ') << "";
                }
                else if (add_reservation_station[i].Q_j == 7)
                {
                    cout << left << setw(6) << setfill(' ') << "load2";
                    cout << left << setw(2) << setfill(' ') << "";
                }
                else if (add_reservation_station[i].Q_j == 8)
                {
                    cout << left << setw(6) << setfill(' ') << "load3";
                    cout << left << setw(2) << setfill(' ') << "";
                }
                else
                {
                    cout << left << setw(8) << setfill(' ') << add_reservation_station[i].Q_j;
                }
                cout << left << setw(8) << setfill(' ') << add_reservation_station[i].V_j;

                if (add_reservation_station[i].Q_k == 1)
                {
                    cout << left << setw(6) << setfill(' ') << "add1";
                    cout << left << setw(2) << setfill(' ') << "";
                }
                else if (add_reservation_station[i].Q_k == 2)
                {
                    cout << left << setw(6) << setfill(' ') << "add2";
                    cout << left << setw(2) << setfill(' ') << "";
                }
                else if (add_reservation_station[i].Q_k == 3)
                {
                    cout << left << setw(6) << setfill(' ') << "add3";
                    cout << left << setw(2) << setfill(' ') << "";
                }
                else if (add_reservation_station[i].Q_k == 4)
                {
                    cout << left << setw(6) << setfill(' ') << "multi1";
                    cout << left << setw(2) << setfill(' ') << "";
                }
                else if (add_reservation_station[i].Q_k == 5)
                {
                    cout << left << setw(6) << setfill(' ') << "multi2";
                    cout << left << setw(2) << setfill(' ') << "";
                }
                else if (add_reservation_station[i].Q_k == 6)
                {
                    cout << left << setw(6) << setfill(' ') << "load1";
                    cout << left << setw(2) << setfill(' ') << "";
                }
                else if (add_reservation_station[i].Q_k == 7)
                {
                    cout << left << setw(6) << setfill(' ') << "load2";
                    cout << left << setw(2) << setfill(' ') << "";
                }
                else if (add_reservation_station[i].Q_k == 8)
                {
                    cout << left << setw(6) << setfill(' ') << "load3";
                    cout << left << setw(2) << setfill(' ') << "";
                }
                else
                {
                    cout << left << setw(8) << setfill(' ') << add_reservation_station[i].Q_k;
                }
                cout << left << setw(8) << setfill(' ') << add_reservation_station[i].V_k;
            }
            cout << endl;
        }
        for (int i=0; i < mulRS; i++) 
        {
            if (mul_reservation_station[i].cycles_required == -1)
            {
                cout << left << setw(8) << setfill(' ') << "";
            }
            else if (mul_reservation_station[i].cycle_count == 0)
                cout << left << setw(8) << setfill(' ') << mul_reservation_station[i].cycles_required;
            else if((mul_reservation_station[i].cycles_required - mul_reservation_station[i].cycle_count) > -1 && MUL_output == 1)
            {
                    cout << left << setw(8) << setfill(' ') << mul_reservation_station[i].cycles_required - mul_reservation_station[i].cycle_count + 1;
                    if(mul_reservation_station[i].cycles_required - mul_reservation_station[i].cycle_count == 0)
                    MUL_output = 0;
            }
            else
            {
                cout << left << setw(8) << setfill(' ') << "0";
                MUL_output = 1;
            }
            
            cout << left << setw(0) << setfill(' ') << "[";
            cout << left << setw(0) << setfill(' ') << mul_reservation_station[i].num;
            cout << left << setw(6) << setfill(' ') << "]";

            if (mul_reservation_station[i].busy == 0) 
            {
                cout << left << setw(8) << setfill(' ') << " ";
                cout << left << setw(8) << setfill(' ') << " ";
                cout << left << setw(8) << setfill(' ') << " ";
                cout << left << setw(8) << setfill(' ') << " ";
            }
            else 
            {
                cout << left << setw(8) << setfill(' ') << mul_reservation_station[i].busy;
                cout << left << setw(8) << setfill(' ') << mul_reservation_station[i].name;
                if (mul_reservation_station[i].Q_j == 1)
                {
                    cout << left << setw(6) << setfill(' ') << "add1";
                    cout << left << setw(2) << setfill(' ') << "";
                }
                else if (mul_reservation_station[i].Q_j == 2)
                {
                    cout << left << setw(6) << setfill(' ') << "add2";
                    cout << left << setw(2) << setfill(' ') << "";
                }
                else if (mul_reservation_station[i].Q_j == 3)
                {
                    cout << left << setw(6) << setfill(' ') << "add3";
                    cout << left << setw(2) << setfill(' ') << "";
                }
                else if (mul_reservation_station[i].Q_j == 4)
                {
                    cout << left << setw(6) << setfill(' ') << "multi1";
                    cout << left << setw(2) << setfill(' ') << "";
                }
                else if (mul_reservation_station[i].Q_j == 5)
                {
                    cout << left << setw(6) << setfill(' ') << "multi2";
                    cout << left << setw(2) << setfill(' ') << "";
                }
                else if (mul_reservation_station[i].Q_j == 6)
                {
                    cout << left << setw(6) << setfill(' ') << "load1";
                    cout << left << setw(2) << setfill(' ') << "";
                }
                else if (mul_reservation_station[i].Q_j == 7)
                {
                    cout << left << setw(6) << setfill(' ') << "load2";
                    cout << left << setw(2) << setfill(' ') << "";
                }
                else if (mul_reservation_station[i].Q_j == 8)
                {
                    cout << left << setw(6) << setfill(' ') << "load3";
                    cout << left << setw(2) << setfill(' ') << "";
                }
                else
                {
                    cout << left << setw(8) << setfill(' ') << mul_reservation_station[i].Q_j;
                }
                cout << left << setw(8) << setfill(' ') << mul_reservation_station[i].V_j;

                if (mul_reservation_station[i].Q_k == 1)
                {
                    cout << left << setw(6) << setfill(' ') << "add1";
                    cout << left << setw(2) << setfill(' ') << "";
                }
                else if (mul_reservation_station[i].Q_k == 2)
                {
                    cout << left << setw(6) << setfill(' ') << "add2";
                    cout << left << setw(2) << setfill(' ') << "";
                }
                else if (mul_reservation_station[i].Q_k == 3)
                {
                    cout << left << setw(6) << setfill(' ') << "add3";
                    cout << left << setw(2) << setfill(' ') << "";
                }
                else if (mul_reservation_station[i].Q_k == 4)
                {
                    cout << left << setw(6) << setfill(' ') << "multi1";
                    cout << left << setw(2) << setfill(' ') << "";
                }
                else if (mul_reservation_station[i].Q_k == 5)
                {
                    cout << left << setw(6) << setfill(' ') << "multi2";
                    cout << left << setw(2) << setfill(' ') << "";
                }
                else if (mul_reservation_station[i].Q_k == 6)
                {
                    cout << left << setw(6) << setfill(' ') << "load1";
                    cout << left << setw(2) << setfill(' ') << "";
                }
                else if (mul_reservation_station[i].Q_k == 7)
                {
                    cout << left << setw(6) << setfill(' ') << "load2";
                    cout << left << setw(2) << setfill(' ') << "";
                }
                else if (mul_reservation_station[i].Q_k == 8)
                {
                    cout << left << setw(6) << setfill(' ') << "load3";
                    cout << left << setw(2) << setfill(' ') << "";
                }
                else
                {
                    cout << left << setw(8) << setfill(' ') << mul_reservation_station[i].Q_k;
                }
                cout << left << setw(8) << setfill(' ') << mul_reservation_station[i].V_k;
            }
            cout << endl;
        }
        //print load status
        cout << endl << "Load Status:" << endl;
        cout << left << setw(8) << setfill(' ') << "";
        cout << left << setw(8) << setfill(' ') << "Name";
        cout << left << setw(10) << setfill(' ') << "Busy";
        cout << left << setw(0) << setfill(' ') << "Address";
        cout << endl;

        for (int i=0; i < loadRS; i++) 
        {
            cout << left << setw(8) << setfill(' ') << "";
            cout << left << setw(0) << setfill(' ') << "[";
            cout << left << setw(0) << setfill(' ') << load_reserv_stat[i].num;
            cout << left << setw(8) << setfill(' ') << "]";
            cout << left << setw(10) << setfill(' ') << load_reserv_stat[i].busy;

            if (load_reserv_stat[i].busy == true) 
            {
                cout << left << setw(8) << setfill(' ') << load_reserv_stat[i].address;
            }
            cout << endl;
        }

        //print register result status
        cout << endl << "Register Result Status:" << endl;
        cout << left << setw(8) << setfill(' ') << "F0";
        cout << left << setw(8) << setfill(' ') << "F2";
        cout << left << setw(8) << setfill(' ') << "F4";
        cout << left << setw(8) << setfill(' ') << "F6";
        cout << left << setw(8) << setfill(' ') << "F8";
        cout << left << setw(8) << setfill(' ') << "F10" << endl;

        for (int i=0; i < Reg_Num; i++) 
        {
            if (data_registers[i].tag == 0) 
            {
                cout << left << setw(8) << setfill(' ') << data_registers[i].data;
            }
            else
            {
                if (data_registers[i].tag == 1)
                {
                    cout << left << setw(6) << setfill(' ') << "add1";
                    cout << left << setw(2) << setfill(' ') << "";
                }
                if (data_registers[i].tag == 2)
                { 
                    cout << left << setw(6) << setfill(' ') << "add2";
                    cout << left << setw(2) << setfill(' ') << "";
                }
                if (data_registers[i].tag == 3)
                {
                    cout << left << setw(6) << setfill(' ') << "add3";
                    cout << left << setw(2) << setfill(' ') << "";
                }
                if (data_registers[i].tag == 4)
                {
                    cout << left << setw(6) << setfill(' ') << "multi1";
                    cout << left << setw(2) << setfill(' ') << "";
                }
                if (data_registers[i].tag == 5)
                {
                    cout << left << setw(6) << setfill(' ') << "multi2";
                    cout << left << setw(2) << setfill(' ') << "";
                }
                if (data_registers[i].tag == 6)
                {
                    cout << left << setw(6) << setfill(' ') << "load1";
                    cout << left << setw(2) << setfill(' ') << "";
                }
                if (data_registers[i].tag == 7)
                {
                    cout << left << setw(6) << setfill(' ') << "load2";
                    cout << left << setw(2) << setfill(' ') << "";
                }
                if (data_registers[i].tag == 8)
                {
                    cout << left << setw(6) << setfill(' ') << "load3";
                    cout << left << setw(2) << setfill(' ') << "";
                }
            }
        }
        cout << endl << endl;
        
        //cycle_main+1
        if (issueSuccessful) 
        {
            Instr_issued += 1;
            issueSuccessful = false;
        }
        cycle_main += 1;
    }
    
    return 0;
}
