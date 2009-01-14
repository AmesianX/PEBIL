#ifndef _BasicBlock_h_
#define _BasicBlock_h_

#include <Base.h>
#include <Vector.h>
#include <BitSet.h>
#include <FlowGraph.h>

class Instruction;
class Function;

class BasicBlock : public Base {
private:
    const static uint32_t PaddingMask      = 0x1;
    const static uint32_t EntryMask        = 0x2;
    const static uint32_t ExitMask         = 0x4;
    const static uint32_t OnlyCtrlMask     = 0x8;
    const static uint32_t NoPathMask       = 0x10;
    const static uint32_t NoInstMask       = 0x20;

protected:
    uint32_t index;

    FlowGraph* flowGraph;
    
    Vector<Instruction*> instructions;
    Vector<BasicBlock*> sourceBlocks;
    Vector<BasicBlock*> targetBlocks;
    BasicBlock* immDominatedBy;

    HashCode hashCode;

    //    MemoryOperation** memoryOps;
    uint32_t numberOfMemoryOps;
    uint32_t numberOfFloatOps;

    uint32_t flags;
    uint64_t baseAddress;

public:
    BasicBlock(uint32_t idx, FlowGraph* cfg);
    ~BasicBlock();

    bool containsOnlyControl();

    void printInstructions();
    void print();
    void printSourceBlocks();
    void printTargetBlocks();

    uint32_t addSourceBlock(BasicBlock* srcBlock);
    uint32_t addTargetBlock(BasicBlock* tgtBlock);

    uint32_t getNumberOfSources() { return sourceBlocks.size(); }
    uint32_t getNumberOfTargets() { return targetBlocks.size(); }
    uint32_t getNumberOfMemoryOps() { return numberOfMemoryOps; }
    uint32_t getNumberOfFloatOps() { return numberOfFloatOps; }
    
    uint32_t addInstruction(Instruction* inst);

    bool passesControlToNext();
    bool findExitInstruction();

    uint64_t findInstrumentationPoint();
    bool inRange(uint64_t addr);

    bool verify();
    void dump (BinaryOutputFile* binaryOutputFile, uint32_t offset);
    FlowGraph* getFlowGraph() { return flowGraph; }
    Function* getFunction() { return flowGraph->getFunction(); }

    uint32_t getBlockSize();
    uint64_t getAddress();
    uint64_t getTargetAddress();
    uint32_t getIndex() { return index; }

    uint32_t getNumberOfInstructions() { return instructions.size(); }
    Instruction* getInstruction(uint32_t idx) { return instructions[idx]; }
    Instruction* getInstructionAtAddress(uint64_t addr);

    bool isPadding()  { return (flags & PaddingMask); }
    bool isEntry()    { return (flags & EntryMask); }
    bool isExit()     { return (flags & ExitMask); }
    bool isOnlyCtrl() { return (flags & OnlyCtrlMask); }
    bool isNoPath()   { return (flags & NoPathMask); }
    bool isNoInst()   { return (flags & NoInstMask); }

    uint32_t setPadding()  { flags |= PaddingMask; return flags; }
    uint32_t setEntry()    { flags |= EntryMask; return flags; }
    uint32_t setExit()     { flags |= ExitMask; return flags; }
    uint32_t setOnlyCtrl() { flags |= OnlyCtrlMask; return flags; }
    uint32_t setNoPath()   { flags |= NoPathMask; return flags; }
    uint32_t setNoInst()   { flags |= NoInstMask; return flags; }

    bool isUnreachable() { return isNoPath(); }
    bool isReachable() { return !isNoPath(); }

    Vector<Instruction*>* swapInstructions(uint64_t addr, Vector<Instruction*>* replacements);

    HashCode getHashCode() { return hashCode; }

    bool isDominatedBy(BasicBlock* bb);
    BasicBlock* getSourceBlock(uint32_t idx) { return sourceBlocks[idx]; }
    BasicBlock* getTargetBlock(uint32_t idx) { return targetBlocks[idx]; }
    void setIndex(uint32_t idx);

    void findMemoryFloatOps();
    void setImmDominator(BasicBlock* bb) { immDominatedBy = bb; }
    BasicBlock* getImmDominator() { return immDominatedBy; }

};


#endif /* _BasicBlock_h_ */

