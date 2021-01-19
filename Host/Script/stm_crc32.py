
Stm32Code = """
#include "stm32f10x_crc.h"
UINT32 StmCrc (void)
{
  BYTE i;

  RCC->AHBENR |= RCC_AHBPeriph_CRC;

  CRC->CR  |= CRC_CR_RESET;
  for (i=0; i<0x4; i++) {
    CRC->DR = (UINT32)(0x11111111 * (i + 1));
  }
  CRC->DR = 0x7788;

  return CRC->DR;
}
"""

custom_crc_table = {}

def generate_crc32_table(_poly):
    global custom_crc_table
    for i in range(256):
        c = i << 24
        for j in range(8):
            c = (c << 1) ^ _poly if (c & 0x80000000) else c << 1
        custom_crc_table[i] = c & 0xffffffff

def crc32_stm(data):
    length    = len(data)
    bytes_arr = data + b'\x00' * ((4 - (length % 4)) & 3)
    length    = len(bytes_arr)
    crc       = 0xffffffff
    for k in range(0, length, 4):
        v = int.from_bytes(bytes_arr[k:k+4], 'big')
        for i in range(4):
            crc = ((crc << 8) & 0xffffffff) ^ custom_crc_table[0xFF & ((crc >> 24) ^ (v >> i*8))]
    return crc

poly = 0x04C11DB7
dbuf = [0x11111111, 0x11111111*2, 0x11111111*3, 0x11111111*4]
buf  = bytearray()
for each in dbuf:
  buf.extend(each.to_bytes(4, 'little'))
buf.extend(b'\x88\x77')
generate_crc32_table(poly)
print(buf)
crc_stm = crc32_stm(bytearray(buf))
print ("%08x" % crc_stm)

