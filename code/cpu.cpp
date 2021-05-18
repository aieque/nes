void reset_cpu(Cpu *cpu) {
    cpu->p = 0x34;
    cpu->a = 0;
    cpu->x = 0;
    cpu->y = 0;
    cpu->sp = 0xFD;
    
    // Set PC to the reset vector.
    cpu->pc = (read_byte(0xFFFD)) << 8 | read_byte(0xFFFC);
}

void print_cpu_registers() {
    Cpu *cpu = &nes->cpu;
    
    printf("Status : %x\n", cpu->p);
    printf("A : %x\n", cpu->a);
    printf("X : %x\n", cpu->x);
    printf("Y : %x\n", cpu->y);
    printf("Stack Pointer : %x\n", cpu->sp);
    printf("Program Counter : %x\n", cpu->pc);
    
}

void cycle_cpu() {
    u8 opcode = read_byte(nes->cpu.pc++);
    
    printf("Opcode: %x\n", opcode);
}