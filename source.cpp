
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <cstdint>
#include <memory>
#include <array>

constexpr auto MEM_SIZE = 0x40000; //This gives us 1 MB of memory
constexpr auto FileName = "task1/shift2.bin"; //File to read from
uint32_t mem[MEM_SIZE] = {}; //might need to have this global or do memory allocation

int main()
{

    // Create a text string, which is used to output the text file
    uint32_t reg[32] = {};
    uint32_t pc = 0x0;
    uint32_t next_pc = 0x0;
    uint32_t mem_start_addr = 0x0;

    bool is_running = false;

    // Read from the text file
    std::ifstream MyReadFile(FileName, std::ios::in | std::ios::binary);
    
    //You won't need this when it's working, mostly for making sure VS is working
    if (!MyReadFile.is_open()) {
        std::cout << "ERROR: Could not open " << FileName << std::endl;
        std::cout << "Current directory: ";
        system("cd");  // Print current directory
        return 1;
    }

    // Reads the file in to the full size of MEM, typecasts mem as a string
    MyReadFile.read((char*)mem, MEM_SIZE);

    // Close the file
    MyReadFile.close();

    //prints out the content of mem to check if read correctly
    
    is_running = true;
    reg[0] = 0; // should be a noop
    reg[2] = mem_start_addr + MEM_SIZE - 4;

    while (is_running) {
        //might be possible to organize this better
        uint32_t instr = mem[pc >> 2];
        uint32_t opcode = instr & 0x7f;
        uint32_t rd = (instr >> 7) & 0x01f;
        uint32_t rs1 = (instr >> 15) & 0x01f;
        uint32_t rs2 = (instr >> 20) & 0x1F;

        uint32_t funct3 = (instr >> 12) & 0x7; //14 - 12
        uint32_t funct7 = (instr >> 25) & 0x7F; // 31-25
        int32_t imm = ((int32_t)instr >> 20);
        uint32_t result;

        switch (opcode) {
            case 0x13: {// immediate functions
                //uint32_t funct3 = (instr >> 12) & 0x7; //shift right by 12 bit anding with bit mask 0b111
                //int32_t imm = ((int32_t)instr >> 20); //shift right by 20 bit result is 12 most significant bits
                //uint32_t result;
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
                    std::cout << "funct3 " << funct3 << " not yet implemented" << std::endl;
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
                            std::cout << "funct7 " << funct7 << " not yet implemented" << std::endl;
                            return 1;
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

            default: {
                std::cout << "Opcode " << opcode << " not yet implemented" << std::endl;
                break;
            }
        }
        pc += 4; // One instruction is four bytes

        if (instr == 0) { // TODO: remove this
            is_running = false;
            break;
        }
    }

    for (int i = 0; i < 32; i++)
    {
        printf("reg %d: %08X \n", i, reg[i]);
    }
    return 0;

}