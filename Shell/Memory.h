#ifndef _MEMORY_H_
#define _MEMORY_H_

#include "Common.h"

void WriteMem (UINT32 addr, UINT32 value, int unit);
void ReadMem  (UINT32 addr, UINT32 len, int unit);

#endif