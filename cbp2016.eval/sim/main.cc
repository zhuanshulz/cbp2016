///////////////////////////////////////////////////////////////////////
//  Copyright 2015 Samsung Austin Semiconductor, LLC.                //
///////////////////////////////////////////////////////////////////////

//Description : Main file for CBP2016 

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <map>
using namespace std;

#include "utils.h"
//#include "bt9.h"
#include "bt9_reader.h"
//#include "predictor.cc"
#include "predictor.h"


#define COUNTER     unsigned long long
#define parallel_num USE_PARALLEL
// trace 都是32位的。
// 此版本的思路在于对每一条单独的指令进行预测。
// 根据trace的PC值找对应的预测器，然后看其预测是否准确，同周期的其他指令都当作是不跳转的指令。
// 更新的时候的话，首先先更新当前trace对应的分支结果，当前周期其他的指令的更新就当作是没有跳转时的情况进行更新。
// 这个版本的思路应该是第一个版本的，version1的仿真更接近与实际情况，因为其考虑到了多条分支指令存在于同一个周期中的情况。
// 而这个版本的话思路简单了许多，就只考虑一条指令的情况。

void CheckHeartBeat(UINT64 numIter, UINT64 numMispred)
{
   // 这个函数是输出每经过1k 10k ... 条branch时的分支预测的 MPKI
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
    printf("  MPKBr_1K         \t : %10.4f \n",   1000.0*(double)(numMispred)/(double)(numIter));   
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

int main(int argc, char* argv[]){
  
  if (argc != 2) {
    printf("usage: %s <trace>\n", argv[0]);
    exit(-1);
  }
  
  ///////////////////////////////////////////////
  // Init variables
  ///////////////////////////////////////////////
    
    PREDICTOR  *brpred = new PREDICTOR();  // this instantiates the predictor code
  ///////////////////////////////////////////////
  // read each trace recrod, simulate until done
  ///////////////////////////////////////////////

    std::string trace_path;
    trace_path = argv[1];
    bt9::BT9Reader bt9_reader(trace_path);

    std::string key = "total_instruction_count:";
    std::string value;
    bt9_reader.header.getFieldValueStr(key, value);
    UINT64     total_instruction_counter = std::stoull(value, nullptr, 0);
    UINT64 current_instruction_counter = 0;
    key = "branch_instruction_count:";
    bt9_reader.header.getFieldValueStr(key, value);
    UINT64     branch_instruction_counter = std::stoull(value, nullptr, 0);
    UINT64     numMispred =0;  
//ver2    UINT64     numMispred_btbMISS =0;  
//ver2    UINT64     numMispred_btbANSF =0;  
//ver2    UINT64     numMispred_btbATSF =0;  
//ver2    UINT64     numMispred_btbDYN =0;  

    UINT64 cond_branch_instruction_counter=0;
    UINT64 uncond_branch_instruction_counter=0;
    
    UINT64 cond_return_instruction_counter=0;
    UINT64 uncond_return_instruction_counter=0;

    UINT64 cond_directJump_instruction_counter=0;
    UINT64 uncond_directJump_instruction_counter=0;
    
    UINT64 cond_indirectJump_instruction_counter=0;
    UINT64 uncond_indirectJump_instruction_counter=0;
    
    UINT64 cond_directCall_instruction_counter=0;
    UINT64 uncond_directCall_instruction_counter=0;
    
    UINT64 cond_indirectCall_instruction_counter=0;
    UINT64 uncond_indirectCall_instruction_counter=0;

    UINT64 error_instruction_conter=0;


//ver2     UINT64 btb_ansf_cond_branch_instruction_counter=0;
//ver2     UINT64 btb_atsf_cond_branch_instruction_counter=0;
//ver2     UINT64 btb_dyn_cond_branch_instruction_counter=0;
//ver2     UINT64 btb_miss_cond_branch_instruction_counter=0;

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

      OpType opType;
      UINT64 PC;
      UINT64 PC_aligned;
      uint index;
      bool branchTaken;
      UINT64 branchTarget;
      UINT64 numIter = 0;

      for (auto it = bt9_reader.begin(); it != bt9_reader.end(); ++it) {
        CheckHeartBeat(++numIter, numMispred); //Here numIter will be equal to number of branches read

        try {
          bt9::BrClass br_class = it->getSrcNode()->brClass();

//          bool dirDynamic = (it->getSrcNode()->brObservedTakenCnt() > 0) && (it->getSrcNode()->brObservedNotTakenCnt() > 0); //JD2_2_2016
//          bool dirNeverTkn = (it->getSrcNode()->brObservedTakenCnt() == 0) && (it->getSrcNode()->brObservedNotTakenCnt() > 0); //JD2_2_2016

//JD2_2_2016 break down branch instructions into all possible types
          opType = OPTYPE_ERROR; 

          if ((br_class.type == bt9::BrClass::Type::UNKNOWN) && (it->getSrcNode()->brNodeIndex())) { //only fault if it isn't the first node in the graph (fake branch)
            opType = OPTYPE_ERROR; //sanity check
            error_instruction_conter++;
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
              {
                cond_return_instruction_counter++;
                opType = OPTYPE_RET_COND;
              }
            else if (br_class.conditionality == bt9::BrClass::Conditionality::UNCONDITIONAL)
              {
                uncond_return_instruction_counter++;
                opType = OPTYPE_RET_UNCOND;
              }
            else {
              opType = OPTYPE_ERROR;
              error_instruction_conter++;
            }
          }
          else if (br_class.directness == bt9::BrClass::Directness::INDIRECT) {
            if (br_class.type == bt9::BrClass::Type::CALL) {
              if (br_class.conditionality == bt9::BrClass::Conditionality::CONDITIONAL)
                {
                  cond_indirectCall_instruction_counter++;
                  opType = OPTYPE_CALL_INDIRECT_COND;
                }
              else if (br_class.conditionality == bt9::BrClass::Conditionality::UNCONDITIONAL)
                {
                  uncond_indirectCall_instruction_counter++;
                  opType = OPTYPE_CALL_INDIRECT_UNCOND;
                }
              else {
                opType = OPTYPE_ERROR;
                error_instruction_conter++;
              }
            }
            else if (br_class.type == bt9::BrClass::Type::JMP) {
              if (br_class.conditionality == bt9::BrClass::Conditionality::CONDITIONAL)
                {
                  cond_indirectJump_instruction_counter++;
                  opType = OPTYPE_JMP_INDIRECT_COND;
                }
              else if (br_class.conditionality == bt9::BrClass::Conditionality::UNCONDITIONAL)
                {
                  uncond_indirectJump_instruction_counter++;
                  opType = OPTYPE_JMP_INDIRECT_UNCOND;
                }
              else {
                opType = OPTYPE_ERROR;
                error_instruction_conter++;
              }
            }
            else {
              opType = OPTYPE_ERROR;
              error_instruction_conter++;
            }
          }
          else if (br_class.directness == bt9::BrClass::Directness::DIRECT) {
            if (br_class.type == bt9::BrClass::Type::CALL) {
              if (br_class.conditionality == bt9::BrClass::Conditionality::CONDITIONAL) {
                cond_directCall_instruction_counter++;
                opType = OPTYPE_CALL_DIRECT_COND;
              }
              else if (br_class.conditionality == bt9::BrClass::Conditionality::UNCONDITIONAL) {
                uncond_directCall_instruction_counter++;
                opType = OPTYPE_CALL_DIRECT_UNCOND;
              }
              else {
                opType = OPTYPE_ERROR;
                error_instruction_conter++;
              }
            }
            else if (br_class.type == bt9::BrClass::Type::JMP) {
              if (br_class.conditionality == bt9::BrClass::Conditionality::CONDITIONAL) {
                {
                  cond_directJump_instruction_counter++;
                  opType = OPTYPE_JMP_DIRECT_COND;
                }
              }
              else if (br_class.conditionality == bt9::BrClass::Conditionality::UNCONDITIONAL) {
                {
                  uncond_directJump_instruction_counter++;
                  opType = OPTYPE_JMP_DIRECT_UNCOND;
                }
              }
              else {
                opType = OPTYPE_ERROR;
                error_instruction_conter++;
              }
            }
            else {
              opType = OPTYPE_ERROR;
              error_instruction_conter++;
            }
          }
          else {
            opType = OPTYPE_ERROR;
            error_instruction_conter++;
          }

          PC = it->getSrcNode()->brVirtualAddr();
          PC_aligned = PC - (PC % ((UINT64)(parallel_num * 4)));
          // index = 1 2 3 4 5 6 7 8... 说明是第几个
          index = ((PC % ((UINT64)(parallel_num * 4)))/4) + 1;
          branchTaken = it->getEdge()->isTakenPath();
          branchTarget = it->getEdge()->brVirtualTarget();

          // printf("PC: %llx type: %x T %d N %d outcome: %d \n", PC, (UINT32)opType, it->getSrcNode()->brObservedTakenCnt(), it->getSrcNode()->brObservedNotTakenCnt(), branchTaken);
          // printf("PC_aligned: %llx type: %x T %d N %d outcome: %d \n", PC_aligned, (UINT32)opType, it->getSrcNode()->brObservedTakenCnt(), it->getSrcNode()->brObservedNotTakenCnt(), branchTaken);
          // printf("index = %d \n", index);
/************************************************************************************************************/

          if (opType == OPTYPE_ERROR) { 
            if (it->getSrcNode()->brNodeIndex()) { //only fault if it isn't the first node in the graph (fake branch)
              fprintf(stderr, "OPTYPE_ERROR\n");
              printf("OPTYPE_ERROR\n");
              exit(-1); //this should never happen, if it does please email CBP org chair.
            }
          }
          else if (br_class.conditionality == bt9::BrClass::Conditionality::CONDITIONAL) { //JD2_17_2016 call UpdatePredictor() for all branches that decode as conditional

            bool* predDir ;
            predDir = brpred->GetPrediction(PC_aligned);
            brpred->UpdatePredictor(PC_aligned, opType, branchTaken, predDir, branchTarget, index); 

            if(*(predDir + index -1) != branchTaken){
              numMispred++; // update mispred stats
            }
            cond_branch_instruction_counter++;

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

      printf("  TRACE \t : %s \n" , trace_path.c_str()); 
      printf("  NUM_INSTRUCTIONS            \t : %10llu \n",   total_instruction_counter);
      printf("  NUM_BR                      \t : %10llu \n",   branch_instruction_counter-1); //JD2_2_2016 NOTE there is a dummy branch at the beginning of the trace...
      printf("  NUM_UNCOND_BR               \t : %10llu \n",   uncond_branch_instruction_counter);
      printf("  NUM_CONDITIONAL_BR          \t : %10llu \n",   cond_branch_instruction_counter);
//ver2      printf("  NUM_CONDITIONAL_BR_BTB_MISS \t : %10llu",   btb_miss_cond_branch_instruction_counter);
//ver2      printf("  NUM_CONDITIONAL_BR_BTB_ANSF \t : %10llu",   btb_ansf_cond_branch_instruction_counter);
//ver2      printf("  NUM_CONDITIONAL_BR_BTB_ATSF \t : %10llu",   btb_atsf_cond_branch_instruction_counter);
//ver2      printf("  NUM_CONDITIONAL_BR_BTB_DYN  \t : %10llu",   btb_dyn_cond_branch_instruction_counter);
      printf("  NUM_MISPREDICTIONS          \t : %10llu \n",   numMispred);
//ver2      printf("  NUM_MISPREDICTIONS_BTB_MISS \t : %10llu",   numMispred_btbMISS);
//ver2      printf("  NUM_MISPREDICTIONS_BTB_ANSF \t : %10llu",   numMispred_btbANSF);
//ver2      printf("  NUM_MISPREDICTIONS_BTB_ATSF \t : %10llu",   numMispred_btbATSF);
//ver2      printf("  NUM_MISPREDICTIONS_BTB_DYN  \t : %10llu",   numMispred_btbDYN);
      printf("  MISPRED_PER_1K_INST         \t : %10.4f \n",   1000.0*(double)(numMispred)/(double)(total_instruction_counter));
//ver2      printf("  MISPRED_PER_1K_INST_BTB_MISS\t : %10.4f",   1000.0*(double)(numMispred_btbMISS)/(double)(total_instruction_counter));
//ver2      printf("  MISPRED_PER_1K_INST_BTB_ANSF\t : %10.4f",   1000.0*(double)(numMispred_btbANSF)/(double)(total_instruction_counter));
//ver2      printf("  MISPRED_PER_1K_INST_BTB_ATSF\t : %10.4f",   1000.0*(double)(numMispred_btbATSF)/(double)(total_instruction_counter));
//ver2      printf("  MISPRED_PER_1K_INST_BTB_DYN \t : %10.4f",   1000.0*(double)(numMispred_btbDYN)/(double)(total_instruction_counter));
      printf("\n");
}



