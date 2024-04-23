#pragma once

#include <types.h>

s32 MuSelCharTblAcces_GetTableLen();
u8 MuSelCharTblAcces_GetCharKind(s32 index);

// TODO: Determine actual type. This is a hypothetical class to house
// the bss variable.
class MuSelectCharacterAccess {
public:
    static u32 bss_loc_0;
};
