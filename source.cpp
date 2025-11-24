#include <stdio.h>
#include <iostream>
#include <fstream>
#include <cstdint>
#include <memory>
#include <array>
#include <string.h>
#include <iomanip>

constexpr auto MEM_SIZE = 0x100000; //This gives us 1 MB of memory
uint8_t mem[MEM_SIZE] = {};

int main()
{
    uint32_t reg[32] = {};
    uint32_t Resultreg[32] = {};
    uint32_t pc = 0x0;
    uint32_t next_pc = 0x0;
    uint32_t mem_start_addr = 0x0;
    std::string TestFile;
    std::string ResFile;
    std::string FileName = "loop.bin";//File to read from 
    bool is_running = false;
	bool using_folders = true; //if using unique folders for tests and results set to true, else false

    // Read from the text file
    if (using_folders) {
        TestFile = (char*)"task3/" + FileName; //Folder containing the test file
        ResFile = (char*)"test_res/" + FileName; //Folder containing the result file
    }
    else {
        TestFile = FileName;
    }
    

    std::ifstream MyReadFile(TestFile, std::ios::in | std::ios::binary);
    
    if (!MyReadFile.is_open()) {
        std::cout << "ERROR: Could not open " << TestFile << std::endl;
        std::cout << "Current directory: ";
        std::cout << "consider changing the using_folders value || " ;
		std::cout << "currently set to: " << std::boolalpha << using_folders << std::endl;
        system("cd");  // Print current directory
        return 1;
    }

    // Reads the file in to the full size of MEM, typecasts mem as a string
    MyReadFile.read((char*)mem, MEM_SIZE);

    // Close the file
    MyReadFile.close();

    
    std::ifstream MyReadFile2(ResFile, std::ios::in | std::ios::binary);
   
    if (!MyReadFile2.is_open()) {
        std::cout << "ERROR: Could not open " << ResFile << std::endl;
        std::cout << "Current directory: ";
        system("cd");  // Print current directory
        //return 1;
    }

    // I feel like MEM_SIZE should be too large here need to look in to the .read thingy
    MyReadFile2.read((char*)Resultreg, MEM_SIZE);

    // Close the file
    MyReadFile2.close();

    is_running = true;
    reg[0] = 0; // should be a noop

    while (is_running) {
        uint32_t instr = (uint32_t)mem[pc]
            | ((uint32_t)mem[pc + 1] << 8)
            | ((uint32_t)mem[pc + 2] << 16)
            | ((uint32_t)mem[pc + 3] << 24);
        uint32_t opcode = instr & 0x7f;
        uint32_t rd = (instr >> 7) & 0x01f;
        uint32_t rs1 = (instr >> 15) & 0x01f;
        uint32_t rs2 = (instr >> 20) & 0x1F;
		int32_t offset = (((instr >> 8) & 0xF) // offset[12|10:5|4:1|11] shifted left by 1
            | (((instr >> 25) & 0x3F) << 4) 
            | (((instr >> 7) & 0x1) << 10) 
            | ((((int32_t)instr) >> 31)) << 11) << 1; 

        uint32_t funct3 = (instr >> 12) & 0x7; //14 - 12
        uint32_t funct7 = (instr >> 25) & 0x7F; // 31-25
        int32_t imm = ((int32_t)instr >> 20);
        uint32_t result;

        next_pc = pc + 4; // One instruction is four bytes
        switch (opcode) {
            case 0x13: {// immediate functions
                switch (funct3) {
                case 0x0: {// ADDI
                    result = (int32_t)reg[rs1] + imm; 
                    break;
                }
                case 0x2: {// slti
                    result = (int32_t)reg[rs1] < (int32_t)imm; 
                    break;
                }

                case 0x3: {// sltiu
                    result = (uint32_t)reg[rs1] < (uint32_t)imm; 
                    break;
                }
                
                case 0x4: {// xori
                    result = reg[rs1] ^ imm;
                    break;
                }
                
                case 0x6: {// ori
                    result = reg[rs1] | imm;
                    break;
                }

                case 0x7: {// andi
                    result = reg[rs1] & imm;
                    break;
                }

                case 0x1: {// slli
                    uint32_t shamt = imm & (0x1F);
                    result = reg[rs1] << shamt;
                    break;
                }

                case 0x5: {// srli and srai
                    uint32_t shamt = imm & 0x1F;
                    if ((instr >> 27) & 0x08) //srai 
                        result = (int32_t)reg[rs1] >> (int32_t)shamt;
                    else //srli
                        result = reg[rs1] >> shamt;
                    
                    break;
                }

                default: { //used for debuging in case of errors
                    std::cout << "opcode: " << opcode << "funct3: " << funct3 << " not yet implemented" << std::endl;
                    return 1;
                }
                }
                if (rd != 0) //don't write to reg[0]
                    reg[rd] = result;
                break;
            }
            case 0x33: { // Register to Register Operations
                if (!(funct7 & 0x1)) {
                    switch (funct3) {
                    case 0x0: { // ADD / SUB
                        if (funct7) {
                            result = (int32_t)(reg[rs1] - reg[rs2]);
                        }
                        else {
                            result = (int32_t)(reg[rs1] + reg[rs2]);
                        }
                        break;
                    }

                    case 0x1: { //sll
                        result = reg[rs1] << (reg[rs2] & 0x1F);
                        break;
                    }

                    case 0x2: { //slt
                        result = (int32_t)reg[rs1] < (int32_t)reg[rs2];
                        break;
                    }

                    case 0x3: { //sltu
                        result = (uint32_t)reg[rs1] < (uint32_t)reg[rs2];
                        break;
                    }

                    case 0x4: { //xor
                        result = reg[rs1] ^ reg[rs2];
                        break;
                    }

                    case 0x5: { //srl and sra
                        if ((instr >> 27) & 0x08) //sra
                            result = (int32_t)reg[rs1] >> (int32_t)(reg[rs2] & 0x1F);
                        else //srl
                            result = (uint32_t)reg[rs1] >> (uint32_t)(reg[rs2] & 0x1F);

                        break;
                    }

                    case 0x6: { //or
                        result = reg[rs1] | reg[rs2];
                        break;
                    }

                    case 0x7: { //and
                        result = reg[rs1] & reg[rs2];
                        break;
                    }


                    default: { //used for debuging in case of errors
                        std::cout << "opcode: " << opcode << "funct7: " << funct7 << " not yet implemented" << std::endl;
                        break;

                    }
                    }
                }
                if (rd != 0) //don't write to reg[0]
                    reg[rd] = result;
                break;
            }

            case 0x37: { // lui
                if (rd != 0) //don't write to reg[0]
                    reg[rd] = (int32_t)(instr & 0xFFFFF000);
                break;
            }
            
            case 0x17: { // auipc
                if (rd != 0) //don't write to reg[0]
                    reg[rd] = pc + (int32_t)(instr & 0xFFFFF000);
                break;
            }

            case 0x63: { // branches
                bool branch = false;
                switch (funct3) {
                    case 0x0: {// beq
                        branch = reg[rs1] == reg[rs2];
                        break;
                    }

                    case 0x1: {// bne
                        branch = reg[rs1] != reg[rs2];
                        break;
                    }

                    case 0x4: {// blt
                        branch = (int32_t)reg[rs1] < (int32_t)reg[rs2];
                        break;
                    }

                    case 0x5: {// bge
                        branch = (int32_t)reg[rs1] >= (int32_t)reg[rs2];
                        break;
                    }

                    case 0x6: {// bltu
                        branch = reg[rs1] < reg[rs2];
                        break;
                    }

                    case 0x7: {// bgeu
                        branch = reg[rs1] >= reg[rs2];
                        break;
                    }
                
                    default: { //used for debuging in case of errors
                        std::cout << "opcode: " << opcode << "funct3: " << funct3 << " not yet implemented" << std::endl;

                        break;
                    }
                }
                if (branch) {
                    next_pc = (int32_t)pc + offset;
                }
				break;
                    
            }

            case 0x3: { // loads
                uint32_t addr = (uint32_t)((int32_t)reg[rs1] + imm);
                switch (funct3) {
                case 0x0: {// lb
                    int8_t val = (int8_t)mem[addr];
                    if (rd != 0)
                        reg[rd] = (int32_t)val;
                    break;
                }
                case 0x1: {// lh
                    int16_t val = (int16_t)mem[addr] | ((int16_t)mem[addr + 1] << 8);
                    if (rd != 0)
                        reg[rd] = (int32_t)val;
                    break;
                }
                case 0x2: {// lw
                    if (rd != 0)
                        reg[rd] = (uint32_t)mem[addr]
                        | ((uint32_t)mem[addr + 1] << 8)
                        | ((uint32_t)mem[addr + 2] << 16)
                        | ((uint32_t)mem[addr + 3] << 24);
                    break;
                }
                case 0x4: {// lbu
                    uint8_t val = mem[addr];
                    if (rd != 0)
                        reg[rd] = (uint32_t)val;
                    break;
                }
                case 0x5: {// lhu
                    uint16_t val = (uint16_t)mem[addr] | ((uint16_t)mem[addr + 1] << 8);
                    if (rd != 0)
                        reg[rd] = (uint32_t)val;
                    break;
                }
                default: {  //used for debuging in case of errors
                    std::cout << "opcode: " << opcode << "funct3: " << funct3 << " not yet implemented" << std::endl;
                    break;
                }
                }
                break;
            }

            case 0x23: { // stores 
                int32_t imm_store = (int32_t)(((instr >> 25) << 5) | ((instr >> 7) & 0x1F));
                // sign-extend 12-bit immediate
                imm_store = (imm_store << 20) >> 20;
                uint32_t addr = (uint32_t)((int32_t)reg[rs1] + imm_store);
                switch (funct3) {
                case 0x0: {// sb
                    uint8_t val = reg[rs2] & 0xFF;
                    mem[addr] = val;
                    break;
                }
                case 0x1: {// sh
                    uint16_t val = reg[rs2] & 0xFFFF;
                    mem[addr] = val & 0xFF;
                    mem[addr + 1] = (val >> 8) & 0xFF;
                    break;
                }
                case 0x2: {// sw
                    mem[addr] = reg[rs2] & 0xFF;
                    mem[addr + 1] = (reg[rs2] >> 8) & 0xFF;
                    mem[addr + 2] = (reg[rs2] >> 16) & 0xFF;
                    mem[addr + 3] = (reg[rs2] >> 24) & 0xFF;
                    break;
                }
                default: {  //used for debuging in case of errors
                    std::cout << "opcode: " << opcode << "funct3: " << funct3 << " not yet implemented" << std::endl;
                    break;
                }
                }
                break;
            }
            
            case 0x6F: { // jal
                if (rd != 0)
                    reg[rd] = pc + 4;
				int32_t imm_jump = (((instr >> 21) & 0x3FF) << 1) | (((instr >> 20) & 0x1) << 11) | (((instr >> 12) & 0xFF) << 12) | ((((int32_t)instr) >> 31) << 20); //imm[20|10:1|11|19:12]
                imm_jump = (imm_jump << 11) >> 11; // sign-extend
                next_pc = (int32_t)pc + imm_jump;
                break;
			}

            case 0x67: { // jalr
                if (rd != 0)
                    reg[rd] = pc + 4;
				next_pc = ((int32_t)reg[rs1] + imm) & ~1; // set LSB to 0 (binary anding with ...1110)
                break;
			}


            case 0x73: { // ecall
                switch (reg[17]) {
                case 0x1: {// print_int
                    //unused
                    break;
                }

                case 0x2: {// print_float
                    //unused
                    break;
                }

                case 0x4: {// print_string
                    //unused
                    break;
                }

                case 0xA: {// exit
                    is_running = false;
                    break;
                }

                case 0xB: {// print_char
                    //unused
                    break;
                }

                case 0x22: {// print_hex
                    //unused
                    break;
                }

                case 0x23: {// print_bin
                    //unused
                    break;
                }

                case 0x24: {// print_unsigned
                    //unused
                    break;
                }
                
                case 0x5D: {// exit with status code
                    //unused
                    break;
                }


                default: { //used for debuging in case of errors
                    std::cout << "opcode: " << opcode << " ecall: " << reg[17] << " not yet implemented" << std::endl;
                    break;
                }
                }
                break;
            }

            default: { //used for debuging in case of errors
                std::cout << "Opcode " << opcode << " not yet implemented" << std::endl;
                break;
            }
            break;
        }
        pc = next_pc; 

        if (instr == 0) {
            is_running = false;
            break;
        }
    }

    if (using_folders)
    {
        for (int i = 0; i < 32; i++) //prints the content of all the registers after program is done running
        {
            bool same[32] = {};
            same[i] = (reg[i] == Resultreg[i]); //compares with the content of the result file
            printf("reg %02d: %08X resReg %02d: %08X || Correct: %s \n", i, reg[i], i, Resultreg[i], same[i] ? "true" : "false");

        }
    }
    else
    {
        for (int i = 0; i < 32; i++) //prints the content of all the registers after program is done running
        {
            printf("reg %02d: %08X\n", i, reg[i]);
        }
	}
   
    return 0;

}