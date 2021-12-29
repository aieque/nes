#define global static
#define internal static
#define local_persist static

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>

#include <assert.h>
#define Assert assert

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef float real32;
typedef double real64;

#define Kilobytes(Value) ((Value) * 1024)
#define Megabytes(Value) (Kilobytes(Value) * 1024)
#define Gigabytes(Value) ((uint64)Megabytes(Value) * 1024)
#define Terabytes(Value) ((uint64)Gigabytes(Value) * 1024)

#define Min(A, B) ((A > B) ? B : A)
#define Max(A, B) ((A > B) ? A : B)
#define Clamp(Value, _Min, _Max) (Min(Max((Value), _Min), _Max))

#define PiF 3.1415926535897f
#define OneOverSquareRootOfTwoPi 0.3989422804
#define OneOverSquareRootOfTwoPiF 0.3989422804f
#define EulersNumber 2.7182818284590452353602874713527
#define EulersNumberf 2.7182818284590452353602874713527f

inline bool
IsDigit(char C) {
    return C >= '0' && C <= '9';
}

inline bool
IsAlphanumeric(char C) {
    return (C >= 'A' && C <= 'Z') || (C >= 'a' && C <= 'z') || IsDigit(C) || C == '.';
}

inline bool
IsWhitespace(char C) {
    return C == ' ' || C == '\t' || C == '\r' || C == '\n';
}

struct string {
    char *Text;
    int Length;
};

internal int
StringToInt(string String) {
    char Buffer[32];
    strncpy(Buffer, String.Text, String.Length);
    Buffer[String.Length] = 0;
    
    return atoi(Buffer);
}

internal uint32
StringToUInt32(string String) {
    char Buffer[32];
    strncpy(Buffer, String.Text, String.Length);
    Buffer[String.Length] = 0;
    
    return atoll(Buffer);
}

internal float
StringToFloat(string String) {
    char Buffer[32];
    strncpy(Buffer, String.Text, String.Length);
    Buffer[String.Length] = 0;
    
    return atof(Buffer);
}

//- Functions for parsing files 
struct file_parser {
    char *Base;
    int Offset;
};

internal string
GetNextString(file_parser *Parser) {
    string Result = {};
    Result.Text = Parser->Base + Parser->Offset;
    
    while (IsAlphanumeric(Parser->Base[Parser->Offset]) || Parser->Base[Parser->Offset] == '_') {
        ++Parser->Offset;
        ++Result.Length;
    }
    
    while (IsWhitespace(Parser->Base[Parser->Offset])) {
        ++Parser->Offset;
    }
    
    return Result;
}

union v2 {
    struct {
        real32 X;
        real32 Y;
    };
    struct {
        real32 Width;
        real32 Height;
    };
};

inline v2
V2(real32 X, real32 Y) {
    v2 Result = {X, Y};
    return Result;
}

inline v2
V2(real32 Value) {
    v2 Result = {Value, Value};
    return Result;
}

inline v2
operator+(v2 A, v2 B) {
    v2 Result = {A.X + B.X, A.Y + B.Y};
    return Result;
}

inline v2
operator*(v2 A, real32 S) {
    v2 Result = {A.X * S, A.Y * S};
    return Result;
}

inline v2
operator/(v2 A, real32 S) {
    v2 Result = {A.X / S, A.Y / S};
    return Result;
}

inline void
operator+=(v2& A, v2& B) {
    A = A + B;
}

inline v2
operator-(v2 A, v2 B) {
    v2 Result = {A.X - B.X, A.Y - B.Y};
    return Result;
}

inline void
operator-=(v2& A, v2& B) {
    A = A - B;
}

inline real32
V2LengthSquared(v2 V) {
    return V.X * V.X + V.Y * V.Y;
}

#include <math.h> // For sqrtf

inline real32
V2Length(v2 V) {
    return sqrtf(V2LengthSquared(V));
}

inline v2
V2Mod(v2 V, real32 S) {
    V.X = fmodf(V.X, S);
    V.Y = fmodf(V.Y, S);
    
    return V;
}

inline v2
V2Lerp(v2 A, v2 B, real32 C) {
    return A + (B - A) * C;
}

inline real32
Dot(v2 A, v2 B) {
    return A.X * B.X + A.Y * B.Y;
}

union v4 {
    struct {
        real32 X, Y;
        union {
            struct {
                real32 Z, W;
            };
            
            struct {
                real32 Width, Height;
            };
        };
    };
    
    struct {
        real32 R, G, B, A;
    };
    
    struct {
        v2 Position;
        v2 Size;
    };
    
    struct {
        v2 P0;
        v2 P1;
    };
};

inline v4
V4(real32 Value) {
    v4 Result = {Value, Value, Value, Value};
    return Result;
}

inline v4
V4(real32 X, real32 Y, real32 Z, real32 W) {
    v4 Result = {X, Y, Z, W};
    return Result;
}

inline v4
V4(v2 Position, v2 Size) {
    v4 Result;
    Result.Position = Position;
    Result.Size = Size;
    
    return Result;
}

inline v4
MakeRect(v2 Position, v2 Size) {
    v2 P1 = Position;
    v2 P2 = {Position.X + Size.X, Position.Y + Size.Y};
    
    return V4(P1, P2);
}

inline v4
IntersectV4(v4 A, v4 B) {
    v4 Result = {};
    Result.X = Max(A.X, B.X);
    Result.Y = Max(A.Y, B.Y);
    Result.Z = Min(A.Z, B.Z);
    Result.W = Min(A.W, B.W);
    
    return Result;
}

inline bool
V2InV4(v2 P, v4 R) {
    return ((P.X > R.X && P.X < R.Z) &&
            (P.Y > R.Y && P.Y < R.W));
}

struct m4 {
    real32 Elements[4][4];
}; 

internal m4
M4Orthographic(real32 Left, real32 Right, real32 Top, real32 Bottom, real32 Near, real32 Far) {
    m4 Result = {};
    
    Result.Elements[0][0] = 2.0f / (Right - Left);
    Result.Elements[1][1] = 2.0f / (Top - Bottom);
    Result.Elements[3][2] = -2.0f / (Far - Near);
    Result.Elements[3][3] = 1.0f;
    Result.Elements[3][0] = (Left + Right) / (Left - Right);
    Result.Elements[3][1] = (Bottom + Top) / (Bottom - Top);
    Result.Elements[3][2] = (Near + Far) / (Near - Far);
    
    return Result;
}

struct memory_arena {
    void *Base;
    uint64 Size;
    uint64 AllocPosition;
    uint64 CommitPosition;
};

#define MEMORY_ARENA_MAX Gigabytes(2)
#define MEMORY_ARENA_COMMIT_SIZE Kilobytes(4)

internal uint32
HashCString(char *String) {
    uint32 Hash = 5381;
    
    int CurrentChar;
    while ((CurrentChar = *String++)) {
        Hash = ((Hash << 5 + Hash)) + CurrentChar;
    }
    
    return Hash;
}

struct string_buffer {
    char *Base;
    uint32 Used;
    uint32 Size;
};