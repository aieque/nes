uint8 ReadMemory(mmu *MMU, uint16 Address) {
    if (Address >= 0x8000 && Address <= 0xFFFF) {
        return MMU->Cartridge->PRGROM[(Address - 0x8000) % (0x4000 * MMU->Cartridge->Header.PRGSize)];
    }
    
    return 0;
}