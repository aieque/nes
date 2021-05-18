struct Cpu {
    u8 a, x, y;
    u16 pc; // The program counter.
    u8 sp; // The stack pointer.
    u8 p; // The status register.
};
