#include "stm32f10x_conf.h"
#include "ModuleApi.h"
#include "Flash.h"

#define  CS_HIGH_LOW    {CS_HIGH; CS_LOW;}

BYTE  ExtSpiWriteByte(BYTE data)
{        
	/* Send byte through the SPI peripheral */
	SPI_I2S_SendData(EXTERNAL_SPI, data);

	/* Wait to receive a byte */
	while (SPI_I2S_GetFlagStatus(EXTERNAL_SPI, SPI_I2S_FLAG_RXNE) == RESET);

	/* Return the byte read from the SPI bus */
	return SPI_I2S_ReceiveData(EXTERNAL_SPI);      
} 

BYTE  SpiWriteByte(BYTE data)  
{        
	/* Send byte through the SPI peripheral */
	SPI_I2S_SendData(ONBOARD_SPI, data);

	/* Wait to receive a byte */
	while (SPI_I2S_GetFlagStatus(ONBOARD_SPI, SPI_I2S_FLAG_RXNE) == RESET);

	/* Return the byte read from the SPI bus */
	return SPI_I2S_ReceiveData(ONBOARD_SPI);      
}  
 
UINT32 SpiReadId ()
{
  UINT8  Idx;
  UINT32 Id;  

  Id = 0;
  CS_HIGH_LOW;  
  SpiWriteByte (0x9F);    // Read command
  for (Idx = 0; Idx < 4; Idx++) {
    Id = (Id << 8) | SpiWriteByte (0xFF);
  }
  CS_HIGH;
  return Id;
}

void SpiSetCs (BYTE lvl)
{
  if      (lvl == 1)  CS_HIGH;  
  else if (lvl == 0)  CS_LOW;  
  else                CS_HIGH_LOW;
}

void SpiReadCmdExt (DWORD addr)
{
  CS_HIGH_LOW;                     // CS = 1, 0
  SpiWriteByte (0x03); 
  SpiWriteByte ((BYTE)(addr>>16)); // Send address        
  SpiWriteByte ((BYTE)(addr>>8));  // Send address      
  SpiWriteByte ((BYTE)addr);       // Always block 63  
}

void UnlockFlash ()
{
  // Enabel Write SR     
  CS_HIGH_LOW;                     // CS = 1, 0
  SpiWriteByte (0x06);             // Enable Write SR                 
  CS_HIGH_LOW;                     // Deassert SPI_CS
  SpiWriteByte (0x01);             // Write Status Cmd 
  SpiWriteByte (0x02);             // Write Status Data      
  CS_HIGH;                         // Deassert SPI_CS    
}

BYTE EraseFlash (BYTE blk)
{
  BYTE  cnt;
    
  CS_HIGH_LOW;                      // CS = 1, 0
  SpiWriteByte (0x06);              // Enable Write SR                 
  CS_HIGH_LOW;                      // CS = 1, 0  
  
  SpiWriteByte (0xD8);              // Erase block
  SpiWriteByte (blk);               // Erase block address
  SpiWriteByte (0x00);              // Erase block address
  SpiWriteByte (0x00);              // Erase block address  
  CS_HIGH_LOW;                      // CS = 1, 0  
 
  // Wait for Done
  while (1) {    
    SpiWriteByte (0x05);            // Read Status                            
    cnt = SpiWriteByte (0xFF);      // write to shift data in          
    CS_HIGH_LOW;                    // CS = 1, 0            
    if (!(cnt & 1)) {   
      break;
    }
  }
  
  // Check status 
  if (cnt & (1<<5)) {    
    SpiWriteByte (0x30);            // Clear failure
    cnt = 1;
  } else {
    cnt = 0;
  }
  CS_HIGH;                          // CS = 1
  return cnt;
}


BYTE WriteFlash (WORD pageidx, BYTE *pagebuf)
{  
  BYTE          cnt;
  
  CS_HIGH_LOW;                        // CS = 1, 0
  SpiWriteByte (0x06);                // Enable Write SR               
  CS_HIGH_LOW;                        // CS = 1, 0
  
  // Write a page 256 bytes
  SpiWriteByte (0x02);                // Write page  
  SpiWriteByte ((BYTE)(pageidx>>8));  // Address
  SpiWriteByte ((BYTE)pageidx);       // Address  
  SpiWriteByte (0);                   // Address      
  
  cnt = 0;
  while (1) {    
    SpiWriteByte (*pagebuf);          // Write data    
    if (cnt==0xFF) break;
    pagebuf++;
    cnt++;
  }  
  CS_HIGH_LOW;                        // CS = 1

  // Wait for Done
  while (1) {
    SpiWriteByte (0x05);              // Read Status                          
    cnt = SpiWriteByte (0xFF);
    CS_HIGH_LOW;                      // CS = 1, 0
    if (!(cnt & 1)) {            
      break;
    }
  }  

  // Check status  
  if (cnt & (1<<6)) {    
    SpiWriteByte (0x30);              // Clear failure    
    cnt = 1;
  } else {
    cnt = 0;
  }

  CS_HIGH_LOW;                        // CS = 1
  return cnt;
}
