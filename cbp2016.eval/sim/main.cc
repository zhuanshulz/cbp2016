///////////////////////////////////////////////////////////////////////
//  Copyright 2015 Samsung Austin Semiconductor, LLC.                //
///////////////////////////////////////////////////////////////////////

//Description : Main file for CBP2016 

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <map>
#include <cmath>
using namespace std;

#include "utils.h"
//#include "bt9.h"
#include "bt9_reader.h"
//#include "predictor.cc"
#include "predictor.h"

// simutanously predict first needs to read out n instructions
// which means there is a n entries buffer contains n instructions
// and then judge the 
#define parallel_num USE_PARALLEL
// if there are several instructions making prediction simutanously,
// then we will use future information to update the current status if there are more than two instructions in a same perieod.
// this is a unsynthesis algorithm.


UINT64 PC_entries[parallel_num];
bool * mask_true_bool;
OpType * optype_pointer;
UINT64 * target_pointer;
UINT64     numMispred =0;  
PREDICTOR  *brpred = new PREDICTOR();  // this instantiates the predictor code



void CheckHeartBeat(UINT64 numIter, UINT64 numMispred)
{
  UINT64 dotInterval=1000000;
  UINT64 lineInterval=30*dotInterval;

 UINT64 d1K   =1000;
 UINT64 d10K  =10000;
 UINT64 d100K =100000;
 UINT64 d1M   =1000000; 
 UINT64 d10M  =10000000;
 UINT64 d30M  =30000000;
 UINT64 d60M  =60000000;
 UINT64 d100M =100000000;
 UINT64 d300M =300000000;
 UINT64 d600M =600000000;
 UINT64 d1B   =1000000000;
 UINT64 d10B  =10000000000;


//  if(numIter % lineInterval == 0){ //prints line every 30 million branches
//    printf("\n");
//    fflush(stdout);
//  }
  if(numIter == d1K){ //prints MPKI after 100K branches
    printf("  MPKBr_1K         \t : %10.4f",   1000.0*(double)(numMispred)/(double)(numIter));   
    fflush(stdout);
  }

  if(numIter == d10K){ //prints MPKI after 100K branches
    printf("  MPKBr_10K         \t : %10.4f",   1000.0*(double)(numMispred)/(double)(numIter));   
    fflush(stdout);
  }
  
  if(numIter == d100K){ //prints MPKI after 100K branches
    printf("  MPKBr_100K         \t : %10.4f",   1000.0*(double)(numMispred)/(double)(numIter));   
    fflush(stdout);
  }
  if(numIter == d1M){
    printf("  MPKBr_1M         \t : %10.4f",   1000.0*(double)(numMispred)/(double)(numIter)); 
    fflush(stdout);
  }

  if(numIter == d10M){ //prints MPKI after 100K branches
    printf("  MPKBr_10M         \t : %10.4f",   1000.0*(double)(numMispred)/(double)(numIter));   
    fflush(stdout);
  }

  if(numIter == d30M){ //prints MPKI after 100K branches
    printf("  MPKBr_30M         \t : %10.4f",   1000.0*(double)(numMispred)/(double)(numIter));   
    fflush(stdout);
  }

  if(numIter == d60M){ //prints MPKI after 100K branches
    printf("  MPKBr_60M         \t : %10.4f",   1000.0*(double)(numMispred)/(double)(numIter));   
    fflush(stdout);
  }

  if(numIter == d100M){ //prints MPKI after 100K branches
    printf("  MPKBr_100M         \t : %10.4f",   1000.0*(double)(numMispred)/(double)(numIter));   
    fflush(stdout);
  }
  
  if(numIter == d300M){ //prints MPKI after 100K branches
    printf("  MPKBr_300M         \t : %10.4f",   1000.0*(double)(numMispred)/(double)(numIter));   
    fflush(stdout);
  }

  if(numIter == d600M){ //prints MPKI after 100K branches
    printf("  MPKBr_600M         \t : %10.4f",   1000.0*(double)(numMispred)/(double)(numIter));   
    fflush(stdout);
  }

  if(numIter == d1B){ //prints MPKI after 100K branches
    printf("  MPKBr_1B         \t : %10.4f",   1000.0*(double)(numMispred)/(double)(numIter));   
    fflush(stdout);
  }
  
  if(numIter == d10B){ //prints MPKI after 100K branches
    printf("  MPKBr_10B         \t : %10.4f",   1000.0*(double)(numMispred)/(double)(numIter));   
    fflush(stdout);
  }
 
}//void CheckHeartBeat

// usage: predictor <trace>



















// parallel_num
// predict need PC
// update  nedd (PC, opType, branchTaken, predDir, branchTarget); 
class parallel_unit{
  public:
    UINT64 PC;
    OpType opType;
    bool branchTaken;
    bool predicTaken = false;
    bool valid = false;
    UINT64 branchTarget;
    parallel_unit()
    {
      PC = 0;
      opType = OPTYPE_ERROR;
      branchTaken = false;
      predicTaken = true;
      branchTarget = 0;
    }
    void set_valid()
    {
      valid = true;
    }
    void reset_valid()
    {
      valid = false;
    }
    void set_entry(UINT64 pc_addr, OpType op,  bool btaken,UINT64 btarget)
    {
      PC = pc_addr;
      opType = op;
      branchTaken = btaken;
      branchTarget = btarget;
    }
    parallel_unit(UINT64 pc_addr, OpType op,  bool btaken, bool pretaken, UINT64 btarget)
    {
      PC = pc_addr;
      opType = op;
      branchTaken = btaken;
      predicTaken = pretaken;
      branchTarget = btarget;
    }
    void set_pretaken(bool pretaken)
    {
      predicTaken = pretaken;
    }
    bool read_preresult()
    {
      return branchTaken == predicTaken;
    }
};
struct pre_unit
      //(PC, opType, branchTaken, branchTarget);
{
  UINT64 PC;
  OpType opType;
  bool branchTaken;
  UINT64 branchTarget;
};
struct update_unit
    //(PC, opType, branchTaken, predDir, branchTarget);
{
  UINT64 PC;
  OpType opType;
  bool branchTaken;
  bool predDir;
  UINT64 branchTarget;
};
class parallel_units{
  public:
    UINT64 write_entry_pointer = 0;
    UINT64 write_pretaken_pointer = 0;
    UINT64 read_entry_pointer = 0;
    UINT64 pc_addr_need_predict = 0;
    UINT64 aligned_first_PC = 0;
    UINT64 aligned_last_PC = 0;
    bool continuing_read = false;
    uint8_t mask = 0;
    parallel_unit parallel_m[ parallel_num+2 ];
    parallel_unit *first_entry = parallel_m;
    parallel_unit *parallel_write_entry = parallel_m;
    parallel_unit *parallel_write_pretaken = parallel_write_entry;
    parallel_unit *parallel_read = parallel_write_entry;

    OpType optype[parallel_num];
    UINT64 branch_targets[parallel_num];
    bool branchTaken_pointer[parallel_num];

    void write_new_entry(UINT64 pc_addr, OpType op,  bool btaken,UINT64 btarget)
    {
      // if (write_entry_pointer == parallel_num + 1)
      //   {
      //     parallel_write_entry = parallel_m;
      //     write_entry_pointer = 0;
      //   }
      parallel_write_entry->set_entry(pc_addr, op, btaken, btarget);
      parallel_write_entry->set_valid();
      write_entry_pointer++;
      parallel_write_entry++;
    }
    void write_new_pretaken(bool pretaken)
    {
      if (write_pretaken_pointer == parallel_num + 1)
      {
          parallel_write_pretaken = parallel_m;
          write_pretaken_pointer = 0;
      }
      parallel_write_pretaken->set_pretaken(pretaken);
      write_pretaken_pointer++;
      parallel_write_pretaken++;
    }
    bool read_full()
    {
      if (write_entry_pointer == parallel_num + 1)
        return true;
      else
        return false;
      // return write_entry_pointer == parallel_num;
    }
    // bool judge_running_condition()
    // {
    //   // pc_addr_need_predict = first_entry
    // }
    OpType* get_optype(UINT64 shift_num)
    {
      for(UINT64 i = 0; i<parallel_num; i++)
        optype[i] = OPTYPE_NULL;
      for(UINT64 i = 0; i<=shift_num - 1; i++)
      {
        if(parallel_m[i].valid)
        {
          optype[(parallel_m[i].PC-aligned_first_PC)/4] = parallel_m[i].opType;
        }
      }
      return optype;
    }
    UINT64* get_branchTarget(UINT64 shift_num)
    {
      for(UINT64 i = 0; i<=shift_num - 1; i++)
        branch_targets[i] = 0;
      for(UINT64 i = 0; i<=shift_num - 1; i++)
      {
        if(parallel_m[i].valid)
        {
          branch_targets[(parallel_m[i].PC-aligned_first_PC)/4] = parallel_m[i].branchTarget;
        }
      }
      return branch_targets;
    }
    void logic_shift(UINT64 shift_num)
    {
    //   UINT64 PC;
    // OpType opType;
    // bool branchTaken;
    // bool predicTaken = false;
    // bool valid = false;
    // UINT64 branchTarget;
      //for(int i =0; i<parallel_num;i++)
      //  printf("before finish shift : %d: %llx \n", i,parallel_m[i].PC);
      for(UINT64 i = 0;i <= parallel_num + 1 - shift_num; i++)
      {
        parallel_m[i].PC = parallel_m[i + shift_num].PC;
        parallel_m[i].opType = parallel_m[i + shift_num].opType;
        parallel_m[i].branchTaken = parallel_m[i + shift_num].branchTaken;
        parallel_m[i].predicTaken = parallel_m[i + shift_num].predicTaken;
        parallel_m[i].valid = parallel_m[i + shift_num].valid;
        parallel_m[i].branchTarget = parallel_m[i + shift_num].branchTarget;
      }
      for(UINT64 j = 0; j <=shift_num - 1; j++)
      {
        parallel_m[parallel_num + 1 - j].valid = false;

        write_entry_pointer = write_entry_pointer - 1;
        parallel_write_entry = parallel_write_entry - 1;

        write_pretaken_pointer = write_pretaken_pointer - 1;
        parallel_write_pretaken = parallel_write_pretaken - 1;
      }
    }
    // UINT64 get_repeat_num(uint8_t shift_num)
    // {
    //   UINT64 first_pc = first_entry->PC;
    //   UINT64 aligned_first_PC = first_pc - (UINT64)(first_pc % (UINT64)(parallel_num * 4));
    //   UINT64 aligned_next_PC = aligned_first_PC + (UINT64)(parallel_num * 4);
    //   UINT64 compared_pc = parallel_m[shift_num].PC;
    //   UINT64 aliged_compared_pc = compared_pc - (UINT64)(compared_pc % (UINT64)(parallel_num * 4));
      
    // }
    uint8_t get_shift_num()
    {
      //check 
      //from first instruction to the last n instructions, check if they are in a same period to be predicted.
      UINT64 first_pc = first_entry->PC;
      bool first_branch = first_entry->branchTaken;
      aligned_first_PC = first_pc - (UINT64)(first_pc % (UINT64)(parallel_num * 4));
      UINT64 aligned_next_PC = aligned_first_PC + (UINT64)(parallel_num * 4);
      UINT64 compared_pc = 0;
      uint8_t temp_mask = first_branch?((uint8_t)pow(2.0,(first_pc - aligned_first_PC)/4)):0;//检查第一条分支指令是否发生了跳转。
      for (uint8_t i = 1;i<=parallel_num;)
      {
        compared_pc = parallel_m[i].PC;
        for(uint8_t j=0; j<i; j++)
        {  
          //if there is a loop, then split them.
          if(compared_pc==parallel_m[j].PC)
          {
            mask = temp_mask;
            aligned_last_PC = compared_pc - (UINT64)(compared_pc % (UINT64)(parallel_num * 4));
            return i;
          }
        }
        if (i==parallel_num)
        {
          //如果n条指令全是跳转指令，且在同一个n指令地址范围内连续排列。
          aligned_last_PC = compared_pc - (UINT64)(compared_pc % (UINT64)(parallel_num * 4));
          mask = temp_mask;
          return parallel_num;
        }
        if ((compared_pc<aligned_next_PC) && (compared_pc > aligned_first_PC))
          //如果在第一条指令对齐的地址后的n条指令范围内，则继续检查下一条指令。
          {
            i++;
            temp_mask = temp_mask + (parallel_m[i].branchTaken)?((uint8_t)pow(2.0,(compared_pc - aligned_first_PC)/4)):0;
            continue;
          }
        
        else
        {
          mask = temp_mask;
          aligned_last_PC = compared_pc - (UINT64)(compared_pc % (UINT64)(parallel_num * 4));
          return i;
        }
      }
      return 0;
    }

    bool * get_predict_mask()
    {
      for (int i = 0; i<parallel_num; i++)
      {
        branchTaken_pointer[i] = ((mask/(uint8_t(pow(2.0,i))))%2)?true:false;
      }
      return branchTaken_pointer;
    }

    void get_mask_seq(bool * bool_mask, int seq_num_branch, uint* get_index_seq)
    {
      int temp_seq_num_branch = 0;
      for (int i = 0; i<parallel_num; i++)
      {
        bool_mask[i] = false;
      }
      for (int i = 0; i<parallel_num; i++)
      {
        if (parallel_m[i].branchTaken)
            temp_seq_num_branch = temp_seq_num_branch + 1;
        if (temp_seq_num_branch == seq_num_branch)
        {
            *get_index_seq = (UINT64)((parallel_m[i].PC % (UINT64)(parallel_num * 4))/4);
            bool_mask[*get_index_seq] = true;
            break;
        }
      }
    }


    // pre_unit get_predict_data()
    // {
    // }
    pre_unit read_entry_Sequentially()
    {
      if (read_entry_pointer == parallel_num)
      {
          parallel_read = parallel_m;
          read_entry_pointer = 0;
      }
      //(PC, opType, branchTaken, branchTarget);
      pre_unit temp = {parallel_read->PC, parallel_read->opType, parallel_read->branchTaken, parallel_read->branchTarget};
      read_entry_pointer++;
      parallel_read++;
      return temp;
    }
    update_unit read_entry_reSequentially()
    {
      if (read_entry_pointer == parallel_num)
      {
          parallel_read = parallel_m;
          read_entry_pointer = 0;
      }
      //(PC, opType, branchTaken, predDir, branchTarget);
      update_unit temp = {parallel_read->PC, parallel_read->opType, parallel_read->branchTaken,parallel_read->predicTaken, parallel_read->branchTarget};
      read_entry_pointer++;
      parallel_read++;
      return temp;
    }
};

void predict_and_update(UINT64 * PC_pointer, 
                          OpType * opType_pointer,
                           bool * branchTaken_pointer ,
                            UINT64 * branchTarget_pointer,
                             UINT64 * numMispred_pointer,
                              PREDICTOR  * brpred_pointer,
                                uint shift_num,
                                  parallel_units * parallel_store_units,
                                    bool check_time_enabled, 
                                      uint check_time)
{
  // bool tage_pred = false;		// prediction

  // bool * prediction_pointer = brpred_pointer->GetPrediction(PC_pointer);
  // // printf("prediction end");
  // brpred_pointer->UpdatePredictor(PC_pointer, opType_pointer, branchTaken_pointer, prediction_pointer, branchTarget_pointer); 
  // {
  //   if (*(prediction_pointer+i) != *(branchTaken_pointer+i))
  //     (*numMispred_pointer) = (*numMispred_pointer) +1;
  // }

  // UINT64 temp_numMispred;
  // temp_numMispred = *numMispred_pointer;
  bool * tage_pred = brpred_pointer->GetPrediction(*PC_pointer);
  for(int i=0;i<shift_num;i++)
  {
    if (check_time_enabled)
    {
      if (*(tage_pred+check_time) != (*branchTaken_pointer+check_time))
              *numMispred_pointer = *numMispred_pointer +1;
    }
    else if(parallel_store_units->parallel_m[i].opType == (OPTYPE_RET_COND) ||   // = 8
        parallel_store_units->parallel_m[i].opType == OPTYPE_JMP_DIRECT_COND ||   // = 9
          parallel_store_units->parallel_m[i].opType == OPTYPE_JMP_INDIRECT_COND ||  // = 10
            parallel_store_units->parallel_m[i].opType == OPTYPE_CALL_DIRECT_COND ||  // = 11
              parallel_store_units->parallel_m[i].opType == OPTYPE_CALL_INDIRECT_COND)  // = 12))
      //只判断条件分支。
      {
        // printf("\n test for conditional \n");
        if (*(tage_pred+i) != (*branchTaken_pointer+i))
          *numMispred_pointer = *numMispred_pointer +1;
      }
  }
  brpred_pointer->UpdatePredictor(PC_pointer, opType_pointer, branchTaken_pointer, tage_pred, branchTarget_pointer); 
};
//   for(int i =0;i<parallel_num;i++)
//   {
//   // if (*(tage_pred+i) != (*branchTaken_pointer+i))
//   //   // *numMispred_pointer = *numMispred_pointer +1;
//   //   // printf("\npredict_taken?: %d\n",*(tage_pred+i) ?1:0);

//   //   //这里只统计条件分支指令的预测准确率。!!

//   // }

//   brpred_pointer->UpdatePredictor(PC_pointer, opType_pointer, branchTaken_pointer, tage_pred, branchTarget_pointer); 
// // printf("\ntesting\n");
// }


int main(int argc, char* argv[]){
  
  if (argc != 2) {
    printf("usage: %s <trace>\n", argv[0]);
    exit(-1);
  }
  // printf("%d \n",OPTYPE_OP);
  // printf("%d \n",OPTYPE_RET_UNCOND);
  // printf("%d \n",OPTYPE_JMP_DIRECT_UNCOND);
  // printf("%d \n",OPTYPE_JMP_INDIRECT_UNCOND);
  // printf("%d \n",OPTYPE_CALL_DIRECT_UNCOND);
  // printf("%d \n",OPTYPE_CALL_INDIRECT_UNCOND);
  // printf("%d \n",OPTYPE_RET_COND);
  // printf("%d \n",OPTYPE_JMP_DIRECT_COND);
  // printf("%d \n",OPTYPE_JMP_INDIRECT_COND);
  // printf("%d \n",OPTYPE_CALL_DIRECT_COND);
  // printf("%d \n",OPTYPE_CALL_INDIRECT_COND);
  // printf("%d \n",OPTYPE_ERROR);
  // printf("%d \n",OPTYPE_MAX);
  // printf("%d \n",OPTYPE_NULL);
  ///////////////////////////////////////////////
  // Init variables
  ///////////////////////////////////////////////
    
  ///////////////////////////////////////////////
  // read each trace recrod, simulate until done
  ///////////////////////////////////////////////


    // this is the path of trace file /traces/??.bt9.trace.gz
    std::string trace_path;
    trace_path = argv[1];     
    bt9::BT9Reader bt9_reader(trace_path);  // Branch Trace version 9 (BT9) format.



    std::string key = "total_instruction_count:";
    std::string value;
    bt9_reader.header.getFieldValueStr(key, value);
    UINT64     total_instruction_counter = std::stoull(value, nullptr, 0);
    // UINT64 current_instruction_counter = 0;
    key = "branch_instruction_count:";
    bt9_reader.header.getFieldValueStr(key, value);
    UINT64     branch_instruction_counter = std::stoull(value, nullptr, 0);
//ver2    UINT64     numMispred_btbMISS =0;  
//ver2    UINT64     numMispred_btbANSF =0;  
//ver2    UINT64     numMispred_btbATSF =0;  
//ver2    UINT64     numMispred_btbDYN =0;  

    UINT64 cond_branch_instruction_counter=0;
//ver2     UINT64 btb_ansf_cond_branch_instruction_counter=0;
//ver2     UINT64 btb_atsf_cond_branch_instruction_counter=0;
//ver2     UINT64 btb_dyn_cond_branch_instruction_counter=0;
//ver2     UINT64 btb_miss_cond_branch_instruction_counter=0;
           UINT64 uncond_branch_instruction_counter=0;

//ver2    ///////////////////////////////////////////////
//ver2    // model simple branch marking structure
//ver2    ///////////////////////////////////////////////
//ver2    std::map<UINT64, UINT32> myBtb; 
//ver2    map<UINT64, UINT32>::iterator myBtbIterator;
//ver2
//ver2    myBtb.clear();
   

  ///////////////////////////////////////////////
  // read each trace record, simulate until done
  ///////////////////////////////////////////////

      // OpType opType_temp;
      // UINT64 PC_temp;
      // bool branchTaken_temp;
      // bool preDir_temp;
      // UINT64 branchTarget_temp;

      OpType opType;
      UINT64 PC;
      bool branchTaken;
      UINT64 branchTarget;

      UINT64 numIter = 0;


      UINT64 num_total[parallel_num];
      for(int i = 0; i<parallel_num; i++)
        num_total[i] = 0;

      parallel_units parallel_store_unit;
      // pre_unit temp1_unit[parallel_num];
      // update_unit temp2_unit[parallel_num];



              // UINT64 *PC_entries_pointer[parallel_num];
              // for(int i=0;i<parallel_num;i++)
              // {
              //   PC_entries_pointer[i] = &PC_entries[i];
              // }
              UINT64 next_aligned_pc;
              uint8_t mask_true;

      for (auto it = bt9_reader.begin(); it != bt9_reader.end(); ++it) {
        CheckHeartBeat(++numIter, numMispred); //Here numIter will be equal to number of branches read
        if (numIter == 1)
          continue;
        try {
          bt9::BrClass br_class = it->getSrcNode()->brClass();

//          bool dirDynamic = (it->getSrcNode()->brObservedTakenCnt() > 0) && (it->getSrcNode()->brObservedNotTakenCnt() > 0); //JD2_2_2016
//          bool dirNeverTkn = (it->getSrcNode()->brObservedTakenCnt() == 0) && (it->getSrcNode()->brObservedNotTakenCnt() > 0); //JD2_2_2016

//JD2_2_2016 break down branch instructions into all possible types
          opType = OPTYPE_ERROR; 

          if ((br_class.type == bt9::BrClass::Type::UNKNOWN) && (it->getSrcNode()->brNodeIndex())) { //only fault if it isn't the first node in the graph (fake branch)
            opType = OPTYPE_ERROR; //sanity check
          }
//NOTE unconditional could be part of an IT block that is resolved not-taken
//          else if (dirNeverTkn && (br_class.conditionality == bt9::BrClass::Conditionality::UNCONDITIONAL)) {
//            opType = OPTYPE_ERROR; //sanity check
//          }
//JD_2_22 There is a bug in the instruction decoder used to generate the traces
//          else if (dirDynamic && (br_class.conditionality == bt9::BrClass::Conditionality::UNCONDITIONAL)) {
//            opType = OPTYPE_ERROR; //sanity check
//          }
          else if (br_class.type == bt9::BrClass::Type::RET) {
            if (br_class.conditionality == bt9::BrClass::Conditionality::CONDITIONAL)
              opType = OPTYPE_RET_COND;
            else if (br_class.conditionality == bt9::BrClass::Conditionality::UNCONDITIONAL)
              opType = OPTYPE_RET_UNCOND;
            else {
              opType = OPTYPE_ERROR;
            }
          }
          else if (br_class.directness == bt9::BrClass::Directness::INDIRECT) {
            if (br_class.type == bt9::BrClass::Type::CALL) {
              if (br_class.conditionality == bt9::BrClass::Conditionality::CONDITIONAL)
                opType = OPTYPE_CALL_INDIRECT_COND;
              else if (br_class.conditionality == bt9::BrClass::Conditionality::UNCONDITIONAL)
                opType = OPTYPE_CALL_INDIRECT_UNCOND;
              else {
                opType = OPTYPE_ERROR;
              }
            }
            else if (br_class.type == bt9::BrClass::Type::JMP) {
              if (br_class.conditionality == bt9::BrClass::Conditionality::CONDITIONAL)
                opType = OPTYPE_JMP_INDIRECT_COND;
              else if (br_class.conditionality == bt9::BrClass::Conditionality::UNCONDITIONAL)
                opType = OPTYPE_JMP_INDIRECT_UNCOND;
              else {
                opType = OPTYPE_ERROR;
              }
            }
            else {
              opType = OPTYPE_ERROR;
            }
          }
          else if (br_class.directness == bt9::BrClass::Directness::DIRECT) {
            if (br_class.type == bt9::BrClass::Type::CALL) {
              if (br_class.conditionality == bt9::BrClass::Conditionality::CONDITIONAL) {
                opType = OPTYPE_CALL_DIRECT_COND;
              }
              else if (br_class.conditionality == bt9::BrClass::Conditionality::UNCONDITIONAL) {
                opType = OPTYPE_CALL_DIRECT_UNCOND;
              }
              else {
                opType = OPTYPE_ERROR;
              }
            }
            else if (br_class.type == bt9::BrClass::Type::JMP) {
              if (br_class.conditionality == bt9::BrClass::Conditionality::CONDITIONAL) {
                opType = OPTYPE_JMP_DIRECT_COND;
              }
              else if (br_class.conditionality == bt9::BrClass::Conditionality::UNCONDITIONAL) {
                opType = OPTYPE_JMP_DIRECT_UNCOND;
              }
              else {
                opType = OPTYPE_ERROR;
              }
            }
            else {
              opType = OPTYPE_ERROR;
            }
          }
          else {
            opType = OPTYPE_ERROR;
          }
  
          PC = it->getSrcNode()->brVirtualAddr();

          branchTaken = it->getEdge()->isTakenPath();
          branchTarget = it->getEdge()->brVirtualTarget();
          // if (numIter == 3000)
          //   {
          //     printf("\n num_mispred: %d \n", numMispred);
          //     break;
          //   }
          // printf("PC:%llx; Target: %llx; Type: %x; branchTaken: %u;", PC,branchTarget,((UINT32)opType),branchTaken?((uint32_t)1):((uint32_t)0));
          // printf("PC: ", PC, "\t", "Target:", branchTarget, '\n');
          //printf("PC: %llx type: %x T %d N %d outcome: %d \n", PC, (UINT32)opType, it->getSrcNode()->brObservedTakenCnt(), it->getSrcNode()->brObservedNotTakenCnt(), branchTaken);

/************************************************************************************************************/

          if (opType == OPTYPE_ERROR) { 
            if (it->getSrcNode()->brNodeIndex()) { //only fault if it isn't the first node in the graph (fake branch)
              fprintf(stderr, "OPTYPE_ERROR\n");
              printf("OPTYPE_ERROR\n");
              exit(-1); //this should never happen, if it does please email CBP org chair.
            }
          }
          else if (br_class.conditionality == bt9::BrClass::Conditionality::CONDITIONAL) { //JD2_17_2016 call UpdatePredictor() for all branches that decode as conditional
            cond_branch_instruction_counter++;
            //printf("COND "); only predict conditional branches!!!

//NOTE: contestants are NOT allowed to use the btb* information from ver2 of the infrastructure below:
//ver2             myBtbIterator = myBtb.find(PC); //check BTB for a hit
//ver2            bool btbATSF = false;
//ver2            bool btbANSF = false;
//ver2            bool btbDYN = false;
//ver2
//ver2            if (myBtbIterator == myBtb.end()) { //miss -> we have no history for the branch in the marking structure
//ver2              //printf("BTB miss ");
//ver2              myBtb.insert(pair<UINT64, UINT32>(PC, (UINT32)branchTaken)); //on a miss insert with outcome (N->btbANSF, T->btbATSF)
//ver2              predDir = brpred->GetPrediction(PC, btbANSF, btbATSF, btbDYN);
//ver2              brpred->UpdatePredictor(PC, opType, branchTaken, predDir, branchTarget, btbANSF, btbATSF, btbDYN); 
//ver2            }
//ver2            else {
//ver2              btbANSF = (myBtbIterator->second == 0);
//ver2              btbATSF = (myBtbIterator->second == 1);
//ver2              btbDYN = (myBtbIterator->second == 2);
//ver2              //printf("BTB hit ANSF: %d ATSF: %d DYN: %d ", btbANSF, btbATSF, btbDYN);
//ver2
//ver2              predDir = brpred->GetPrediction(PC, btbANSF, btbATSF, btbDYN);
//ver2              brpred->UpdatePredictor(PC, opType, branchTaken, predDir, branchTarget, btbANSF, btbATSF, btbDYN); 
//ver2
//ver2              if (  (btbANSF && branchTaken)   // only exhibited N until now and we just got a T -> upgrade to dynamic conditional
//ver2                 || (btbATSF && !branchTaken)  // only exhibited T until now and we just got a N -> upgrade to dynamic conditional
//ver2                 ) {
//ver2                myBtbIterator->second = 2; //2-> dynamic conditional (has exhibited both taken and not-taken in the past)
//ver2              }
//ver2            }
//ver2            //puts("");















//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

            

              /*             
                

                      {
                      }
                      
                    }
                    {
                        
                    }
                  }
                }
              }

                  {
                  }


              
              
              // for(int i = 0; i < parallel_num; i++)
              // {
              //   //(PC, opType, branchTaken, branchTarget);
              //   temp1_unit[i] = parallel_store_unit.read_entry_Sequentially();
              //   // PC_temp = temp_unit[i].PC;
              //   // opType_temp = temp_unit[i].opType;
              //   // branchTaken_temp = temp_unit[i].branchTaken;
              //   // branchTarget_temp = temp_unit[i].branchTarget;

              //   preDir_temp = brpred->GetPrediction(temp1_unit[i].PC);
              //   parallel_store_unit.write_new_pretaken(preDir_temp);
              // }
              // for(int i = 0; i < parallel_num; i++)
              // {
              //   //(PC, opType, branchTaken, branchTarget);
              //   temp2_unit[i] = parallel_store_unit.read_entry_reSequentially();
              //   PC_temp = temp2_unit[i].PC;
              //   opType_temp = temp2_unit[i].opType;
              //   branchTaken_temp = temp2_unit[i].branchTaken;
              //   preDir_temp = temp2_unit[i].predDir;
              //   branchTarget_temp = temp2_unit[i].branchTarget;
              //   brpred->UpdatePredictor(PC_temp, opType_temp, branchTaken_temp, preDir_temp, branchTarget_temp); 
              //   if(preDir_temp != branchTaken_temp){
                  // numMispred++; // update mispred stats
              //   }
                cond_branch_instruction_counter++;
              }
            }
              */

            // predDir = brpred->GetPrediction(PC);
            // brpred->UpdatePredictor(PC, opType, branchTaken, predDir, branchTarget); 

            // if(predDir != branchTaken){
              // numMispred++; // update mispred stats
//ver2              if(btbATSF)
//ver2                numMispred_btbATSF++; // update mispred stats
//ver2              else if(btbANSF)
//ver2                numMispred_btbANSF++; // update mispred stats
//ver2              else if(btbDYN)
//ver2                numMispred_btbDYN++; // update mispred stats
//ver2              else
//ver2                numMispred_btbMISS++; // update mispred stats
            // }
            cond_branch_instruction_counter++;

//ver2            if (btbDYN)
//ver2              btb_dyn_cond_branch_instruction_counter++; //number of branches that have been N at least once after being T at least once
//ver2            else if (btbATSF)
//ver2              btb_atsf_cond_branch_instruction_counter++; //number of branches that have been T at least once, but have not yet seen a N after the first T
//ver2            else if (btbANSF)
//ver2              btb_ansf_cond_branch_instruction_counter++; //number of cond branches that have not yet been observed T
//ver2            else
//ver2              btb_miss_cond_branch_instruction_counter++; //number of cond branches that have not yet been observed T
          }
          else if (br_class.conditionality == bt9::BrClass::Conditionality::UNCONDITIONAL) { // for predictors that want to track unconditional branches
            uncond_branch_instruction_counter++;
            brpred->TrackOtherInst(PC, opType, branchTaken, branchTarget);
          }
          else {
            fprintf(stderr, "CONDITIONALITY ERROR\n");
            printf("CONDITIONALITY ERROR\n");
            exit(-1); //this should never happen, if it does please email CBP org chair.
          }













          // bool predDir = false;

          // printf("input: %d:   PC:%llx; Target: %llx; Type: %x; branchTaken: %u;\n",numIter - 1, PC,branchTarget,((UINT32)opType),branchTaken?((uint32_t)1):((uint32_t)0));
            // 保存n条分支指令，如果存满了就开始进行下一步。
            parallel_store_unit.write_new_entry(PC, opType, branchTaken, branchTarget);
            if(parallel_store_unit.read_full())
            {
              // 获得n条指令中可以同时进行预测的指令数。
              uint shift_num = parallel_store_unit.get_shift_num();
              // printf("shift_num: %d  \n",shift_num);
              // 这个num_total仅仅做分析之用。
              num_total[shift_num] = num_total[shift_num] + 1;  //analyse different width of shift number

              // mask =[0:7]
              // 1 2 4 8 16 32 64 128
              
              // first time read out the information of branches, return the pointer of them.
              // 这个mask就是表明在这n条同时执行的指令中，哪些是分支指令。
              // mask_true_bool是布尔类型的
              // mask_true是八位二进制的
              mask_true_bool = parallel_store_unit.get_predict_mask();
              mask_true = parallel_store_unit.mask;
              //optype的指针，说明这n条指令是什么类型的指令，普通执行指令就是NULL类型
              optype_pointer = parallel_store_unit.get_optype(shift_num);
              //target的指针,如果没有跳转，则其值为零
              target_pointer = parallel_store_unit.get_branchTarget(shift_num);
              //第一个对齐后的PC
              PC_entries[0] = parallel_store_unit.aligned_first_PC;
              //第一个对齐后的PC - 4， 即上一次预测的地址
              PC_entries[parallel_num - 1] = PC_entries[0] - 4;
              //下一次预测时的对齐地址
              next_aligned_pc = parallel_store_unit.aligned_last_PC;
              // parallel_store_unit.logic_shift(shift_num);
              
              // printf("numMispred: %d \n shift_num: %d \n",numMispred, shift_num);
              uint check_time = 0;
              uint check_to_index=0;
              bool check_time_enabled=false;//更新时判断是否需要使用checktime的变量。
              // mask代表了发生跳转的指令。

              
              if(parallel_store_unit.mask!=0)
              {
                // 仅仅利用mask来进行控制，实际进行预测时还是用mask_true来进行，相当于mask_true来正向进行控制，mask_true_bool反向进行预测和更新。

                // 当预测的指令中有发生跳转的指令时
                // 可能会跳往很远的地址，可能跳回循环，可能跳到同一个n地址范围内的其他地址。
                // there is branches taken
                for(int i = 1; i<parallel_num; i++)
                    PC_entries[i] = PC_entries[i-1] + 4;
                // first time prediction
                ///////////////////////////////
                // 首先看第一个跳转指令，这里要把第一条指令所在的mask_true设置为yes，其余的设置成false.
                check_time = check_time+1;
                // 这一个函数的功能就是把第check_time个branch指令所在的地址对应的mask设置为true，其余的设置成为false.
                parallel_store_unit.get_mask_seq(mask_true_bool, check_time, &check_to_index);
                check_time_enabled = true;
                predict_and_update(PC_entries, optype_pointer, mask_true_bool ,target_pointer, &numMispred, brpred ,shift_num, &parallel_store_unit,check_time_enabled, check_to_index);
                      // printf("PC_aligned: %llx \n", PC_entries[0]);
                      // printf("mask: %X\n",mask_true);

                // after first prediction, there is a branch taken.
                  uint8_t temp_time = 0;
                
                // for j = 1 ~ parallel_num-1 branches
                // 对每一条指令进行处理。
                // 如果在同一个n指令范围内存在多条分支指令，且通过了前面的get_shift_num阶段，那么已知的就有两种情况，其一是向后跳转，其二是向前跳转。
                // 1
                // 2
                // 3
                // 4
                // 5
                // 例如5跳1， 4跳1，就会出现 5、4的序列。
                // 又或者1跳2，2跳4，就会出现1，2的序列。

                // 这是一个二维的过程，在一维的角度，是n条进行预测的指令，在另一个维度，是随时间变化的跳转值。

                // 所以解决办法就是对每一个发生跳转的位置进行处理，当预测后将该位置的跳转信息设置为不跳转。
                for(int j = 1; j<parallel_num; j++)
                {
                  if((mask_true/(uint8_t)(pow(2.0, j-1))) % 2 == 1)
                  // 第j条指令为分支 跳转 指令，则进行处理，否则跳过。继续判断下一条指令。
                  {
                    temp_time++; // 用于保存这是第几条分支 跳转 指令。
                    check_time = check_time+1;
                    // here the j th instruction in parallel_num instructions taken branch
                    uint8_t unit_num = 0; //用于第几条指令发生了跳转。
                    uint8_t k = 0;
                    for(k=0;k<shift_num;k++)
                    {
                      if(parallel_store_unit.parallel_m[k].branchTaken)
                        unit_num++;
                        //unit_num代表了这是第几个发生跳转的指令。
                      if(unit_num == temp_time)
                      {
                        k = k +1;
                        break;
                      }
                      // 找出第j个发生跳转的分支指令。下一条是第k条指令。
                    }

                    if(k == shift_num)
                      //last one branch taken
                    {
                      UINT64 temp_last_branche_target = parallel_store_unit.parallel_m[k -1].branchTarget;
                      UINT64 temp_last_branche_target_aligned = temp_last_branche_target - (temp_last_branche_target % (UINT64)(parallel_num * 4));
                      UINT64 temp_next_branch_addr = parallel_store_unit.parallel_m[shift_num].PC;
                      UINT64 temp_next_branch_addr_aligned = temp_next_branch_addr - (temp_next_branch_addr % (UINT64)(parallel_num * 4));
                      while(temp_last_branche_target_aligned != temp_next_branch_addr_aligned)
                      {
                        PC_entries[0] = temp_last_branche_target_aligned;
                        for(int i = 1; i<parallel_num; i++)
                          PC_entries[i] = PC_entries[i-1] + 4;
                        temp_last_branche_target_aligned = temp_last_branche_target_aligned + 4*(parallel_num);
                        for(int i = 0; i<parallel_num; i++)
                        {
                          *(optype_pointer + i) = OPTYPE_NULL;
                          *(mask_true_bool + i) = false;
                          *(target_pointer + i) = 0;
                        }
                        check_time_enabled = false;
                        predict_and_update(PC_entries, optype_pointer, mask_true_bool ,target_pointer, &numMispred, brpred ,shift_num, &parallel_store_unit,check_time_enabled, check_to_index);
                              // printf("PC_aligned: %llx \n", PC_entries[0]);
                              // printf("mask: 00000000\n");
                      }
                    }
                    else
                    {
                      //internal branches taken
                      // uint8_t temp_mask = 0;
                      UINT64 temp_previous_PC = parallel_store_unit.parallel_m[k-1].branchTarget;   // the j th branch target
                      UINT64 temp_previous_PC_aliged = temp_previous_PC - (temp_previous_PC % (UINT64)(parallel_num * 4));
                      UINT64 temp_current_PC_aligned = PC_entries[0];
                      if ( temp_current_PC_aligned != temp_previous_PC_aliged )
                        {
                          while ( temp_current_PC_aligned != temp_previous_PC_aliged )
                          {
                            //here are normal instructions, no branches
                            PC_entries[0] = temp_previous_PC_aliged;
                            for(int i = 1; i<parallel_num; i++)
                              PC_entries[i] = PC_entries[i-1] + 4;
                            temp_previous_PC_aliged = temp_previous_PC_aliged + 4*(parallel_num);
                            for(int i = 0; i<parallel_num; i++)
                            {
                              *(optype_pointer + i) = OPTYPE_NULL;
                              *(mask_true_bool + i) = false;
                              *(target_pointer + i) = 0;
                            }
                            // printf("PC_aligned: %llx \n", PC_entries[0]);
                            // printf("mask: 00000000 \n");
                            check_time_enabled = false;
                            predict_and_update(PC_entries, optype_pointer, mask_true_bool ,target_pointer, &numMispred, brpred ,shift_num, &parallel_store_unit,check_time_enabled, check_to_index);
                          }
                          // processed the normal instructions between the branch  instruction and the next branch instruction
                          // 然后就将处理完的这个位置的mast_true设置为0.
                          mask_true = mask_true - ((uint8_t)(pow(2.0, j-1)));


                          // 这里的mask可能还存在一些问题，因为会有先后顺序的问题。
                          mask_true_bool = parallel_store_unit.get_predict_mask();
                          parallel_store_unit.get_mask_seq(mask_true_bool, check_time, &check_to_index);
                          check_time_enabled = true;
                          // // 将第一个设置为false.
                          // for(int i = 0; i<parallel_num; i++)
                          //   if (*(mask_true_bool+i))
                          //   {
                          //     *(mask_true_bool+i) = false;
                          //     break;
                          //   }
                          // // 这里不应该是把第一个设置成false，因为第一条分支跳转指令的地址不一定在第一个，所以删除，改用get_mask_seq。
                            //complicated the normal instructions, here process the current period.
                          optype_pointer = parallel_store_unit.get_optype(shift_num);
                          target_pointer = parallel_store_unit.get_branchTarget(shift_num);
                          PC_entries[0] = parallel_store_unit.aligned_first_PC;
                          for(int i = 1; i<parallel_num; i++)
                            PC_entries[i] = PC_entries[i-1] + 4;
                          predict_and_update(PC_entries, optype_pointer, mask_true_bool ,target_pointer, &numMispred, brpred ,shift_num, &parallel_store_unit,check_time_enabled, check_to_index);
                              // printf("PC_aligned: %llx \n", PC_entries[0]);
                              // printf("mask: %X\n",mask_true);

                          // here finished one branch taken instuction
                        }
                    }
                    
                  }
                }
                parallel_store_unit.logic_shift(shift_num);

                  
              }
              else
              {
                //there is no branch taken in a period
                // usually it won't taken place
                // 这些预测的指令中没有发生跳转的指令，不代表没有分支指令。
                // 所以首先对这些指令先进行一次预测，然后直到下一条指令的对齐地址前，应该都是普通的指令。
                  // for(int i = 1; i <parallel_num; i++)
                  //   PC_entries[i] = PC_entries[i - 1] + 4;
                  // predict_and_update(PC_entries, optype_pointer, mask_true_bool ,target_pointer, &numMispred, brpred ,shift_num, &parallel_store_unit);
                    // printf("PC_aligned: %llx \n", PC_entries[0]);
                    // printf("mask: %X\n",mask_true);

                  while (1)
                  {
                    PC_entries[0] = PC_entries[parallel_num - 1] + 4;
                    if(PC_entries[0] == next_aligned_pc)
                      break;
                    else
                    {
                      for(int i = 1; i <parallel_num; i++)
                        PC_entries[i] = PC_entries[i - 1] + 4;
                    }
                    for(int i = 0; i<parallel_num; i++)
                      {
                        //普通的指令的类型为NULL，mask为false，target为0。
                        *(optype_pointer+i) = OPTYPE_NULL;
                        *(mask_true_bool+i) = false;
                        *(target_pointer+i) = 0;
                      }
                      check_time_enabled = false;
                      predict_and_update(PC_entries, optype_pointer, mask_true_bool ,target_pointer, &numMispred, brpred ,shift_num, &parallel_store_unit,check_time_enabled, check_to_index);
                      //    printf("PC_aligned: %llx \n", PC_entries[0]);
                          // printf("mask: 00000000\n");

                  }
                  //将存储的指令移动shift_num个位置，将已经预测过的指令移出去
                  parallel_store_unit.logic_shift(shift_num);
              }
              
            }

/************************************************************************************************************/
        }
        catch (const std::out_of_range & ex) {
          std::cout << ex.what() << '\n';
          break;
        }
      } //for (auto it = bt9_reader.begin(); it != bt9_reader.end(); ++it)


    ///////////////////////////////////////////
    //print_stats
    ///////////////////////////////////////////

    //NOTE: competitors are judged solely on MISPRED_PER_1K_INST. The additional stats are just for tuning your predictors.
      for(int i =0;i<parallel_num;i++)
      {
        printf("\n NUM %d at a same period is %d  \n", i, num_total[i]);
      }
      printf("  TRACE \t : %s" , trace_path.c_str()); 
      printf("  NUM_INSTRUCTIONS            \t : %10llu",   total_instruction_counter);
      printf("  NUM_BR                      \t : %10llu",   branch_instruction_counter-1); //JD2_2_2016 NOTE there is a dummy branch at the beginning of the trace...
      printf("  NUM_UNCOND_BR               \t : %10llu",   uncond_branch_instruction_counter);
      printf("  NUM_CONDITIONAL_BR          \t : %10llu",   cond_branch_instruction_counter);
//ver2      printf("  NUM_CONDITIONAL_BR_BTB_MISS \t : %10llu",   btb_miss_cond_branch_instruction_counter);
//ver2      printf("  NUM_CONDITIONAL_BR_BTB_ANSF \t : %10llu",   btb_ansf_cond_branch_instruction_counter);
//ver2      printf("  NUM_CONDITIONAL_BR_BTB_ATSF \t : %10llu",   btb_atsf_cond_branch_instruction_counter);
//ver2      printf("  NUM_CONDITIONAL_BR_BTB_DYN  \t : %10llu",   btb_dyn_cond_branch_instruction_counter);
      printf("  NUM_MISPREDICTIONS          \t : %10llu",   numMispred);
//ver2      printf("  NUM_MISPREDICTIONS_BTB_MISS \t : %10llu",   numMispred_btbMISS);
//ver2      printf("  NUM_MISPREDICTIONS_BTB_ANSF \t : %10llu",   numMispred_btbANSF);
//ver2      printf("  NUM_MISPREDICTIONS_BTB_ATSF \t : %10llu",   numMispred_btbATSF);
//ver2      printf("  NUM_MISPREDICTIONS_BTB_DYN  \t : %10llu",   numMispred_btbDYN);
      printf("  MISPRED_PER_1K_INST         \t : %10.4f",   1000.0*(double)(numMispred)/(double)(total_instruction_counter));
//ver2      printf("  MISPRED_PER_1K_INST_BTB_MISS\t : %10.4f",   1000.0*(double)(numMispred_btbMISS)/(double)(total_instruction_counter));
//ver2      printf("  MISPRED_PER_1K_INST_BTB_ANSF\t : %10.4f",   1000.0*(double)(numMispred_btbANSF)/(double)(total_instruction_counter));
//ver2      printf("  MISPRED_PER_1K_INST_BTB_ATSF\t : %10.4f",   1000.0*(double)(numMispred_btbATSF)/(double)(total_instruction_counter));
//ver2      printf("  MISPRED_PER_1K_INST_BTB_DYN \t : %10.4f",   1000.0*(double)(numMispred_btbDYN)/(double)(total_instruction_counter));
      printf("\n");
}



