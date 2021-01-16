#ifndef _MODULE_HELPER_H_
#define _MODULE_HELPER_H_

#include "Common.h"

BYTE FindCurrAppSlot (BYTE idx);

BYTE FindCurrModSlot (BYTE idx);

BYTE LoadModulePage  (BYTE idx);

BYTE CallModApi      (BYTE idx);

BYTE FindCmdInModSlot (BYTE blkidx, BYTE slot);

#endif


