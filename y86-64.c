#include <stdio.h>
#include <stdlib.h>

#include "utils.h"

const int MAX_MEM_SIZE  = (1 << 13);

void fetchStage(int *icode, int *ifun, int *rA, int *rB, wordType *valC, wordType *valP) {
/* starting fetch*/
wordType pc = getPC();
  byteType byte = getByteFromMemory(pc);

  *icode = (byte >> 4) & 0xf;
  *ifun = byte & 0xf;

  if (*icode == HALT) {
    *valP = pc + 1;
    setStatus(STAT_HLT);
  }
  if (*icode == NOP || *icode == RET) {
    *valP = pc + 1;
  }
  if (*icode == IRMOVQ){
    byte = getByteFromMemory(pc + 1);
    *rB = byte & 0xf;
    *valC = getWordFromMemory(pc + 2);
    *valP = pc + 10;
  }
  if (*icode == PUSHQ || *icode == POPQ){
    byte = getByteFromMemory(pc + 1);
    *rA = (byte >> 4) & 0xf;
    *valP = pc + 2;

  }
  if (*icode == RMMOVQ || *icode == MRMOVQ ) {
    byte = getByteFromMemory(pc + 1);
    *rA = (byte >> 4) & 0xf;
    *rB = byte & 0xf;
    *valC = getWordFromMemory(pc + 2);
    *valP = pc + 10;
  }
  if (*icode == RRMOVQ || *icode == OPQ) {
    byte = getByteFromMemory(pc + 1);
    *rA = (byte >> 4) & 0xf;
    *rB = byte & 0xf;
    *valP = pc + 2;
  }

  if (*icode == CALL || *icode == JXX) {
    byte = getByteFromMemory(pc);
    *valC = getWordFromMemory(pc + 1);
    *valP = pc + 9;
  } 
}

void decodeStage(int icode, int rA, int rB, wordType *valA, wordType *valB) {
/*decoding this stuff, make sure registers are correct on final testing */ 
if (icode == RMMOVQ || OPQ) {
    *valA = getRegister(rA);
    *valB = getRegister(rB);
  }
  if (icode == MRMOVQ) {
    *valB = getRegister(rB);
  }
  if (icode == RRMOVQ) {
    *valA = getRegister(rA);
  }
  if (icode == POPQ || icode == RET) {
    *valA = getRegister(RSP);
    *valB = getRegister(RSP);
  }
  if (icode == PUSHQ) {
    *valA = getRegister(rA);
    *valB = getRegister(RSP);
  }
  if (icode == CALL) {
    *valB = getRegister(RSP);
  }
}

void executeStage(int icode, int ifun, wordType valA, wordType valB, wordType valC, wordType *valE, bool *Cnd) {
  if (icode == IRMOVQ) {
    *valE = 0 + valC;
  }
  if (icode == RRMOVQ) {
    *valE = valA + 0;
  }
  if (icode == RMMOVQ || icode == MRMOVQ) {
    *valE = valB + valC;
  }
  if (icode == OPQ) {
/*setting some flags*/
    bool of = 0;
    bool zf = 0;
    bool sf = 0;
    if (ifun == ADD) {
      *valE = valB + valA;
      if((valA > 0 && valB > 0) && (*valE < 0)) {
        of = 1;
      }else if ((valA < 0 && valB < 0) && (*valE > 0)) {
        of = 1;
      }
    }
    if (ifun == SUB) {
      *valE = valB - valA;
      if((valA < 0 && valB < 0) && (*valE > 0)) {
        of = 1;
      }
    }
    if (ifun == AND) {
      *valE = valB & valA;
    }
    if (ifun == XOR) {
      *valE = valB ^ valA;
    }
    if (*valE < 0) {
      sf = 1;
    }
    if (*valE == 0) {
      zf = 1;
    }
    setFlags(sf, zf, of);
  }
  if (icode == JXX) {
    *Cnd = Cond(ifun);
  }
  if (icode == POPQ || icode == RET) {
    *valE = valB + 8;
  }
  if (icode == CALL || icode == PUSHQ) {
    *valE = valB - 8;
  }
}

void memoryStage(int icode, wordType valA, wordType valP, wordType valE, wordType *valM) {
 if (icode == RMMOVQ || icode == PUSHQ) {
    setWordInMemory(valE, valA);
  }
  if (icode == MRMOVQ) {
    *valM = getWordFromMemory(valE);
  }
  if (icode == POPQ || icode == RET) {
    *valM = getWordFromMemory(valA);
  }
  if (icode == CALL) {
    setWordInMemory(valE, valP);
  }
}

void writebackStage(int icode, int rA, int rB, wordType valE, wordType valM) {
 if (icode == MRMOVQ) {
    setRegister(rA, valM);
  }
  if (icode == IRMOVQ || icode == RRMOVQ || icode == OPQ) {
    setRegister(rB, valE);
  }
  if (icode == POPQ) {
    setRegister(RSP, valE);
    setRegister(rA, valM);
  }
  if (icode == CALL || icode == RET || icode == PUSHQ){
    setRegister(RSP, valE);
  }
}

void pcUpdateStage(int icode, wordType valC, wordType valP, bool Cnd, wordType valM) {
  if (icode == HALT || icode == NOP || icode == IRMOVQ || icode == RRMOVQ || icode == RMMOVQ || icode == MRMOVQ || icode == POPQ || icode == PUSHQ || icode == OPQ) {
    setPC(valP);
  }
  if (icode == CALL) {
    setPC(valC);
  }
  if (icode == RET) {
    setPC(valM);
  }
  if (icode == JXX) {
    if (Cnd == 0) {
      setPC(valP);
    }else{
      setPC(valC);
    }
  }
}

void stepMachine(int stepMode) {
  /* FETCH STAGE */
  int icode = 0, ifun = 0;
  int rA = 0, rB = 0;
  wordType valC = 0;
  wordType valP = 0;
 
  /* DECODE STAGE */
  wordType valA = 0;
  wordType valB = 0;

  /* EXECUTE STAGE */
  wordType valE = 0;
  bool Cnd = 0;

  /* MEMORY STAGE */
  wordType valM = 0;

  fetchStage(&icode, &ifun, &rA, &rB, &valC, &valP);
  applyStageStepMode(stepMode, "Fetch", icode, ifun, rA, rB, valC, valP, valA, valB, valE, Cnd, valM);

  decodeStage(icode, rA, rB, &valA, &valB);
  applyStageStepMode(stepMode, "Decode", icode, ifun, rA, rB, valC, valP, valA, valB, valE, Cnd, valM);
  
  executeStage(icode, ifun, valA, valB, valC, &valE, &Cnd);
  applyStageStepMode(stepMode, "Execute", icode, ifun, rA, rB, valC, valP, valA, valB, valE, Cnd, valM);
  
  memoryStage(icode, valA, valP, valE, &valM);
  applyStageStepMode(stepMode, "Memory", icode, ifun, rA, rB, valC, valP, valA, valB, valE, Cnd, valM);
  
  writebackStage(icode, rA, rB, valE, valM);
  applyStageStepMode(stepMode, "Writeback", icode, ifun, rA, rB, valC, valP, valA, valB, valE, Cnd, valM);
  
  pcUpdateStage(icode, valC, valP, Cnd, valM);
  applyStageStepMode(stepMode, "PC update", icode, ifun, rA, rB, valC, valP, valA, valB, valE, Cnd, valM);

  incrementCycleCounter();
}

/** 
 * main
 * */
int main(int argc, char **argv) {
  int stepMode = 0;
  FILE *input = parseCommandLine(argc, argv, &stepMode);

  initializeMemory(MAX_MEM_SIZE);
  initializeRegisters();
  loadMemory(input);

  applyStepMode(stepMode);
  while (getStatus() != STAT_HLT) {
    stepMachine(stepMode);
    applyStepMode(stepMode);
#ifdef DEBUG
    printMachineState();
    printf("\n");
#endif
  }
  printMachineState();
  return 0;
}
