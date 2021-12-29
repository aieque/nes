internal void
ResetCPU(cpu *CPU, mmu *MMU) {
    CPU->Status = 0x34;
    CPU->A = 0;
    CPU->X = 0;
    CPU->Y = 0;
    CPU->SP = 0xFD;
    
    CPU->PC = (ReadMemory(MMU, 0xFFFD) << 8) | ReadMemory(MMU, 0xFFFC);
}

internal void
ShowCPURegisters(cpu *CPU, ui *UI) {
    UI_BeginWindow(UI, "CPU Registers", V2(50, 50), V2(300, 500),
                   UI_WINDOW_FLAG_TITLEBAR | UI_WINDOW_FLAG_FORCE_SIZE);
    {
        UI_FormatLabel(UI, "Program Counter: 0x%4x", CPU->PC);
        UI_FormatLabel(UI, "Stack Pointer: 0x%2x", CPU->SP);
        UI_AddPadding(UI, 10);
        UI_FormatLabel(UI, "A: 0x%2x", CPU->A);
        UI_FormatLabel(UI, "X: 0x%2x", CPU->X);
        UI_FormatLabel(UI, "Y: 0x%2x", CPU->Y);
        UI_AddPadding(UI, 10);
        UI_FormatLabel(UI, "Status: 0x%2x", CPU->Status);
        
    }
    UI_EndWindow(UI);
}

internal bool
ClockCPU(cpu *CPU, mmu *MMU) {
    bool Result = false;
    
    if (CPU->Cycles == 0) {
        uint8 Opcode = ReadMemory(MMU, CPU->PC);
        CPU->PC += 1;
        
        char Buffer[60];
        sprintf(Buffer, "Opcode: 0x%x\n", Opcode);
        OutputDebugStringA(Buffer);
        
        switch (Opcode) {
            case 0x78: {
                
            };
        }
        
        Result = true;
    }
    
    CPU->Cycles -= 1;
    
    return Result;
}