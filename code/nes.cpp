#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64; // Not even sure if this will be needed.

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef i8 b8;

#define internal static
#define global static
#define local_persist static

struct Nes;
global Nes *nes;

u8 read_byte(u16 address);
void write_byte(u16 address, u8 value);

#include "cart.cpp"
#include "cpu.h"

struct Nes {
    Cartridge cart;
    Cpu cpu;
    
    u8 internal_ram[0x800];
};

#include "mapper.cpp"
#include "cpu.cpp"

u8 read_byte(u16 address) {
    if (address >= 0x0000 && address <= 0x1FFF) {
        return nes->internal_ram[address & 0x7FF];
    }
    
    // @TODO: Actually read the mapper.
    if (address >= 0x4020 && address <= 0xFFFF) {
        return mapper_00_read(address);
    }
    
    printf("[nes]: trying to read at invalid address %x.\n", address);
    return -1;
}

void write_byte(u16 address, u8 value) {
    if (address >= 0x0000 && address <= 0x1FFF) {
        nes->internal_ram[address & 0x7FF] = value;
    }
    
    printf("[nes]: trying to write to invalid address %x.\n", address);
}

void init_nes() {
    nes = (Nes *)malloc(sizeof(Nes));
    
    load_cartridge("roms/ice_climbers.nes", &nes->cart);
    reset_cpu(&nes->cpu);
}

int main(int argument_count, char **arguments) {
    init_nes();
    
    printf("NMI Vector: %x%x\n", read_byte(0xFFFA), read_byte(0xFFFB));
    printf("Reset Vector: %x%x\n", read_byte(0xFFFC), read_byte(0xFFFD));
    printf("IRQ Vector: %x%x\n", read_byte(0xFFFE), read_byte(0xFFFF));
    
    cycle_cpu();
    cycle_cpu();
    cycle_cpu();
    cycle_cpu();
    cycle_cpu();
    
}