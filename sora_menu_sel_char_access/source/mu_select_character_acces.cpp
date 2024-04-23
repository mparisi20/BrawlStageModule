#include <types.h>

#include "mu_select_character_acces.h"

u32 MuSelectCharacterAccess::bss_loc_0;

static const s32 NumChars = 35;
static u16 MuSelCharTbl[NumChars] = {
    0x0001, 0x0101,
    0x0201, 0x0301,
    0x0601, 0x0701,
    0x0801, 0x1101,
    0x1201, 0x0901,
    0x1A01, 0x0E01,
    0x1701, 0x1601,
    0x1301, 0x1B01,
    0x2101, 0x2601,
    0x0D01, 0x0501,
    0x1401, 0x1001,
    0x1F01, 0x2501,
    0x2001, 0x0B01,
    0x2701, 0x0C01,
    0x1501, 0x2401,
    0x2201, 0x1801,
    0x0A01, 0x2301,
    0x1901
};

static const u8 InvalidCharKind = 40;

s32 MuSelCharTblAcces_GetTableLen() {
    return NumChars;
}

u8 MuSelCharTblAcces_GetCharKind(s32 index) {
    if (index < 0 || index >= NumChars) {
        return InvalidCharKind;
    }

    return ((u8*)(MuSelCharTbl))[index * 2];
}
