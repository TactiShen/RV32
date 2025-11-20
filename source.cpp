
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <cstdint>
#include <memory>

#define MEM_SIZE 0x10000 //amount of data on stack probably 30k ish
#define FileName "addlarge.bin" //File to read from

int main()
{

    // Create a text string, which is used to output the text file
    uint32_t mem[MEM_SIZE] = {}; //Might need to be uint32_t

    // Read from the text file
    std::ifstream MyReadFile(FileName, std::ios::in | std::ios::binary);
    
    //You won't need this when it's working, mostly for making sure VS is working
    if (!MyReadFile.is_open()) {
        std::cout << "ERROR: Could not open addlarge.bin!" << std::endl;
        std::cout << "Current directory: ";
        system("cd");  // Print current directory
        return 1;
    }

    // Reads the file in to the full size of MEM, typecasts mem as a string
    MyReadFile.read((char*)mem, MEM_SIZE);

    // Close the file
    MyReadFile.close();

    //prints out the content of mem to check if read correctly
    for (int i = 0; i < 100; i++)
    {
        printf("%02x ", mem[i] );
    }
}