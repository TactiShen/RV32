#include <stdio.h>
#include <iostream>
#include <fstream>
#include <cstdint>
#include <memory>
#include <array>
#include <string.h>

constexpr auto MEM_SIZE = 0x100000; //This gives us 1 MB of memory //maybe 0x100000
uint8_t mem[MEM_SIZE] = {}; //might need to have this global or do memory allocation

int main()
{
    // Create a text string, which is used to output the text file
    uint32_t reg[32] = {};
    uint32_t Resultreg[32] = {};
    uint32_t pc = 0x0;
    uint32_t next_pc = 0x0;
    uint32_t mem_start_addr = 0x0;
    std::string TestFile;
    std::string ResFile;
    std::string FileName = "t15.bin";
    bool is_running = false;

    // Read from the text file
    TestFile = (char *)"task4/" + FileName;
    ResFile = (char *)"test_res/" + FileName;

    std::ifstream MyReadFile(TestFile, std::ios::in | std::ios::binary);
    
    //You won't need this when it's working, mostly for making sure VS is working
    if (!MyReadFile.is_open()) {
        std::cout << "ERROR: Could not open " << TestFile << std::endl;
        std::cout << "Current directory: ";
        system("cd");  // Print current directory
        return 1;
    }

    // Reads the file in to the full size of MEM, typecasts mem as a string
    MyReadFile.read((char*)mem, MEM_SIZE);

    // Close the file
    MyReadFile.close();

    
    std::ifstream MyReadFile2(ResFile, std::ios::in | std::ios::binary);
    // You won't need this when it's working, mostly for making sure VS is working
    if (!MyReadFile2.is_open()) {
        std::cout << "ERROR: Could not open " << ResFile << std::endl;
        std::cout << "Current directory: ";
        system("cd");  // Print current directory
        return 1;
    }

    // I feel like MEM_SIZE should be too large here need to look in to the .read thingy
    MyReadFile2.read((char*)Resultreg, MEM_SIZE);

    // Close the file
    MyReadFile2.close();

    // checked memory helpers for byte-addressable memory (little-endian)
    auto check_range = [&](uint32_t addr, uint32_t size) -> bool {
        // Prevent overflow: require addr <= MEM_SIZE and addr+size <= MEM_SIZE
        if (addr > MEM_SIZE || size > MEM_SIZE || addr + size > MEM_SIZE) {
            std::cerr << "Memory access out of bounds: addr=0x" << std::hex << addr
                      << " size=" << std::dec << size << " at pc=0x" << std::hex << pc << std::dec << "\n";
            std::exit(1); // or set is_running=false and break gracefully
            return false;
        }
        return true;
    };

    auto check_align = [&](uint32_t addr, uint32_t align)->void {
        if ((addr & (align - 1)) != 0) {
            std::cerr << "Unaligned access: addr=0x" << std::hex << addr << " align=" << std::dec << align
                      << " at pc=0x" << std::hex << pc << std::dec << "\n";
            // Decide behavior: fatal, warn, or emulate
            // std::exit(1);
        }
    };

    auto read_u8 = [&](uint32_t addr)->uint8_t {
        check_range(addr, 1);
        return mem[addr];
    };
    auto read_i8 = [&](uint32_t addr)->int8_t {
        check_range(addr, 1);
        return (int8_t)mem[addr];
    };
    auto read_u16 = [&](uint32_t addr)->uint16_t {
        check_range(addr, 2);
        // optional alignment check:
        // check_align(addr, 2);
        return (uint16_t)mem[addr] | ((uint16_t)mem[addr + 1] << 8);
    };
    auto read_i16 = [&](uint32_t addr)->int16_t {
        return (int16_t)read_u16(addr);
    };
    auto read_u32 = [&](uint32_t addr)->uint32_t {
        check_range(addr, 4);
        // optional alignment check:
        // check_align(addr, 4);
        return (uint32_t)mem[addr]
            | ((uint32_t)mem[addr + 1] << 8)
            | ((uint32_t)mem[addr + 2] << 16)
            | ((uint32_t)mem[addr + 3] << 24);
    };
    auto write_u8 = [&](uint32_t addr, uint8_t v) {
        check_range(addr, 1);
        mem[addr] = v;
    };
    auto write_u16 = [&](uint32_t addr, uint16_t v) {
        check_range(addr, 2);
        // check_align(addr, 2);
        mem[addr]     = v & 0xFF;
        mem[addr + 1] = (v >> 8) & 0xFF;
    };
    auto write_u32 = [&](uint32_t addr, uint32_t v) {
        check_range(addr, 4);
        // check_align(addr, 4);
        mem[addr]     = v & 0xFF;
        mem[addr + 1] = (v >> 8) & 0xFF;
        mem[addr + 2] = (v >> 16) & 0xFF;
        mem[addr + 3] = (v >> 24) & 0xFF;
    };

    is_running = true;
    reg[0] = 0; // should be a noop
    reg[2] = mem_start_addr + MEM_SIZE - 4;

    while (is_running) {
        //might be possible to organize this better
        uint32_t instr = read_u32(pc);
        uint32_t opcode = instr & 0x7f;
        uint32_t rd = (instr >> 7) & 0x01f;
        uint32_t rs1 = (instr >> 15) & 0x01f;
        uint32_t rs2 = (instr >> 20) & 0x1F;
        int32_t offset = (((instr >> 8) & 0xF) | (((instr >> 25) & 0x3F) << 4) | (((instr >> 7) & 0x1) << 10) | ((((int32_t)instr) >> 31)) << 11) << 1; //this is so weirdly written jesus fuck

        uint32_t funct3 = (instr >> 12) & 0x7; //14 - 12
        uint32_t funct7 = (instr >> 25) & 0x7F; // 31-25
        int32_t imm = ((int32_t)instr >> 20);
        uint32_t result;

        next_pc = pc + 4;
        switch (opcode) {
            case 0x13: {// immediate functions
                switch (funct3) {
                case 0x0: {// ADDI
                    result = (int32_t)reg[rs1] + imm; //unsure about the type cast on reg here
                    break;
                }
                case 0x2: {// slti
                    result = (int32_t)reg[rs1] < (int32_t)imm; // might not need the int type cast on imm
                    break;
                }

                case 0x3: {// sltiu
                    result = (uint32_t)reg[rs1] < (uint32_t)imm; // might not need the uint type cast on reg
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

                default: { //fix later don't need this I think
                    std::cout << "opcode: " << opcode << "funct3: " << funct3 << " not yet implemented" << std::endl;
                    return 1;
                }
                }
                if (rd != 0)
                    reg[rd] = result;
                break;
            }
            case 0x33: { // Integer Register-Register Operations + Integer Multiplication Extension

                if (funct7 & 0x1) {
                    break;
                }
                else {
                    if ((funct7 & 0x5F) != 0x0) {
                        // TODO: throw illegal instruction exception
                        return 1;
                    }
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


                        default: {
                            std::cout << "opcode: " << opcode << "funct7: " << funct7 << " not yet implemented" << std::endl;
                            break;
                            
                        }
                    }
                }
                if (rd != 0)
                    reg[rd] = result;
                break;
            }

            case 0x37: { // lui
                if (rd != 0)
                    reg[rd] = (int32_t)(instr & 0xFFFFF000);
                break;
            }
            
            case 0x17: { // auipc
                if (rd != 0)
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
                
                    default: { //fix later don't need this I think
                        std::cout << "opcode: " << opcode << "funct3: " << funct3 << " not yet implemented" << std::endl;

                        break;
                    }
                }
                if (branch) {
                    next_pc = (int32_t)pc + offset;
                }
				break;
                    
            }

            case 0x3: { // loads (I-type immediate used: imm)
                uint32_t addr = (uint32_t)((int32_t)reg[rs1] + imm);
                switch (funct3) {
                case 0x0: {// lb
                    int8_t val = read_i8(addr);
                    if (rd != 0)
                        reg[rd] = (int32_t)val;
                    break;
                }
                case 0x1: {// lh
                    int16_t val = read_i16(addr);
                    if (rd != 0)
                        reg[rd] = (int32_t)val;
                    break;
                }
                case 0x2: {// lw
                    if (rd != 0)
                        reg[rd] = read_u32(addr);
                    break;
                }
                case 0x4: {// lbu
                    uint8_t val = read_u8(addr);
                    if (rd != 0)
                        reg[rd] = (uint32_t)val;
                    break;
                }
                case 0x5: {// lhu
                    uint16_t val = read_u16(addr);
                    if (rd != 0)
                        reg[rd] = (uint32_t)val;
                    break;
                }
                default: {
                    std::cout << "opcode: " << opcode << "funct3: " << funct3 << " not yet implemented" << std::endl;
                    break;
                }
                }
                break;
            }

            case 0x23: { // stores (S-type immediate must be constructed)
                // construct S-type immediate: bits [31:25] <<5 | bits[11:7]
                int32_t imm_s = (int32_t)(((instr >> 25) << 5) | ((instr >> 7) & 0x1F));
                // sign-extend 12-bit immediate
                imm_s = (imm_s << 20) >> 20;
                uint32_t addr = (uint32_t)((int32_t)reg[rs1] + imm_s);
                switch (funct3) {
                case 0x0: {// sb
                    uint8_t val = reg[rs2] & 0xFF;
                    write_u8(addr, val);
                    break;
                }
                case 0x1: {// sh
                    uint16_t val = reg[rs2] & 0xFFFF;
                    write_u16(addr, val);
                    break;
                }
                case 0x2: {// sw
                    write_u32(addr, reg[rs2]);
                    break;
                }
                default: {
                    std::cout << "opcode: " << opcode << "funct3: " << funct3 << " not yet implemented" << std::endl;
                    break;
                }
                }
                break;
            }
            
            case 0x6F: { // jal
                if (rd != 0)
                    reg[rd] = pc + 4;
				int32_t imm_j = (((instr >> 21) & 0x3FF) << 1) | (((instr >> 20) & 0x1) << 11) | (((instr >> 12) & 0xFF) << 12) | ((((int32_t)instr) >> 31) << 20); //imm[20|10:1|11|19:12]
                imm_j = (imm_j << 11) >> 11; // sign-extend
                next_pc = (int32_t)pc + imm_j;
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


                default: { //fix later don't need this I think
                    std::cout << "opcode: " << opcode << " a10 = " << reg[17] << " not yet implemented" << std::endl;
                    break;
                }
                }
                break;
            }

            default: {
                std::cout << "Opcode " << opcode << " not yet implemented" << std::endl;
                break;
            }
            break;
        }
        pc = next_pc; // One instruction is four bytes

        if (instr == 0) { // TODO: remove this
            is_running = false;
            break;
        }
    }

    for (int i = 0; i < 32; i++)
    {
        bool same[32] = {};
        same[i] = (reg[i] == Resultreg[i]);
        printf("reg %02d: %08X resReg %02d: %08X || Correct: %s \n", i, reg[i], i, Resultreg[i], same[i] ? "true" : "false");
        
    }
    return 0;

}