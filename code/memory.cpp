internal memory_arena
InitializeMemoryArena() {
    memory_arena Arena = {};
    Arena.Size = MEMORY_ARENA_MAX;
    Arena.Base = Platform->Reserve(Arena.Size);
    Arena.AllocPosition = 0;
    Arena.CommitPosition = 0;
    return Arena;
}

internal void *
MemoryArenaPush(memory_arena *Arena, uint64 Size) {
    void *Memory = 0;
    if (Arena->AllocPosition + Size > Arena->CommitPosition) {
        uint64 CommitSize = Size;
        CommitSize += MEMORY_ARENA_COMMIT_SIZE - 1;
        CommitSize -= CommitSize % MEMORY_ARENA_COMMIT_SIZE;
        
        Platform->Commit((uint8 *)Arena->Base + Arena->CommitPosition, CommitSize);
        Arena->CommitPosition += CommitSize;
    }
    
    Memory = (uint8 *)Arena->Base + Arena->AllocPosition;
    Arena->AllocPosition += Size;
    
    return Memory;
}

internal void *
MemoryArenaPushZero(memory_arena *Arena, uint64 Size) {
    void *Memory = MemoryArenaPush(Arena, Size);
    memset(Memory, 0, Size);
    return Memory;
}

internal void
MemoryArenaPop(memory_arena *Arena, uint64 Size) {
    Size = Min(Size, Arena->AllocPosition);
    Arena->AllocPosition -= Size;
}

internal void
MemoryArenaClear(memory_arena *Arena) {
    MemoryArenaPop(Arena, Arena->AllocPosition);
}

internal void
MemoryArenaRelease(memory_arena *Arena) {
    Platform->Release(Arena->Base);
}
