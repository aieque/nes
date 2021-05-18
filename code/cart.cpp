struct Cartridge_Header {
    char constant_nes_text[4];
    
    u8 prg_size;
    u8 chr_size;
    
    // @TODO Certain flags...
    union {
        struct {
            b8 mirroring_mode : 1;
            b8 has_battery : 1;
            b8 has_trainer : 1;
            b8 four_screen_vram : 1;
        };
        
        char padding[10];
    };
};

struct Cartridge {
    Cartridge_Header header;
    u8 *trainer;
    u8 *prg_rom, *chr_rom;
};

b8 load_cartridge(const char *rom_path, Cartridge *cart) {
    FILE *fp = fopen(rom_path, "rb");
    
    if (fp == NULL) {
        printf("[cart]: unable to open file: %s\n", rom_path);
        return false;
    }
    
    fread(&cart->header, sizeof(Cartridge_Header), 1, fp);
    
    if (cart->header.has_trainer)  {
        cart->trainer = (u8 *)malloc(512);
        fread(cart->trainer, 512, 1, fp);;
    }
    
    i32 prg_size = 16384 * cart->header.prg_size;
    cart->prg_rom = (u8 *)malloc(prg_size);
    fread(cart->prg_rom, prg_size, 1, fp);
    
    i32 chr_size = 8192 * cart->header.chr_size;
    cart->chr_rom = (u8 *)malloc(chr_size);
    fread(cart->chr_rom, chr_size, 1, fp);
    
    // @TODO PlayChoice stuff
    
    return true;
}