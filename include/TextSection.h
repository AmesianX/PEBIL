#ifndef _TextSection_h_
#define _TextSection_h_

#include <Base.h>
#include <RawSection.h>
#include <BinaryFile.h>

class ElfFile;
class Instruction;

class TextSection : public RawSection {
protected:
    uint32_t numberOfInstructions;
    Instruction** instructions;
    uint32_t index;
    
public:
    TextSection(char* filePtr, uint64_t size, uint16_t scnIdx, uint32_t idx, ElfFile* elf)
        : RawSection(ElfClassTypes_TextSection,filePtr,size,scnIdx,elf),index(idx),numberOfInstructions(0),instructions(NULL) {}

    ~TextSection();

    uint32_t readNoFile();
    uint32_t getIndex() { return index; }
    uint32_t disassemble();
    uint32_t printDisassembledCode();
    uint32_t addInstruction(char* bytes, uint32_t length, uint64_t addr);
    uint32_t read(BinaryInputFile* b);

    const char* briefName() { return "TextSection"; }
    void dump (BinaryOutputFile* binaryOutputFile, uint32_t offset);

    uint32_t getNumberOfInstruction() { return numberOfInstructions; }
    Instruction* getInstruction(uint32_t idx);
};


#endif /* _TextSection_h_ */

