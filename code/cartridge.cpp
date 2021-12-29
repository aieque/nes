
internal bool
LoadCartridge(char *ROMPath, cartridge *Cartridge, memory_arena *Arena) {
    bool Result = false;
    
    file_read_result ReadResult = Platform->DEBUGReadFile(ROMPath);
    
    if (ReadResult.Contents) {
        int FileOffset = 0;
        
#define Read(Dest, Size) \
memcpy(Dest, ReadResult.Contents + FileOffset, Size); \
FileOffset += Size;
        
        Read(&Cartridge->Header, sizeof(cartridge_header));
        
        if (Cartridge->Header.HasTrainer) {
            Cartridge->Trainer = (uint8 *)MemoryArenaPush(Arena, 512);
            Read(Cartridge->Trainer, 512);
        }
        
        int PRGSize = 16384 * Cartridge->Header.PRGSize;
        Cartridge->PRGROM = (uint8 *)MemoryArenaPush(Arena, PRGSize);
        Read(Cartridge->PRGROM, PRGSize);
        
        int CHRSize = 8192 * Cartridge->Header.CHRSize;
        Cartridge->CHRROM = (uint8 *)MemoryArenaPush(Arena, CHRSize);
        Read(Cartridge->CHRROM, CHRSize);
        
        Result = true;
    }
    
    return Result;
}