u8 mapper_00_read(u16 address) {
    Cartridge *cart = &nes->cart;
    
    if (address >= 0x8000 && address <= 0xFFFF) {
        return cart->prg_rom[(address - 0x8000) % (0x4000 * cart->header.prg_size)];
    }
    
    printf("[mapper]: invalid address %4x\n", address);
    return 0;
}