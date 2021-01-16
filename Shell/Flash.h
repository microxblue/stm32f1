#ifndef __FLASH_H__
#define __FLASH_H__

BYTE   SpiWriteByte(BYTE data);

UINT32 SpiReadId ();
void   SpiSetCs (BYTE lvl);
void   SpiReadCmdExt (DWORD addr);
void   UnlockFlash ();
BYTE   WriteFlash  (WORD pageidx, BYTE *pagebuf);
BYTE   EraseFlash  (BYTE blk);
BYTE   ExtSpiWriteByte(BYTE data);

#endif
