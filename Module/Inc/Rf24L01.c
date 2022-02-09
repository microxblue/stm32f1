
#define   TX_ADDR_WITDH    5
#define   RX_ADDR_WITDH    5
#define   TX_DATA_WITDH    32
#define   RX_DATA_WITDH    32

#define   R_REGISTER      0x00
#define   W_REGISTER      0x20
#define   ACTIVATE        0x50
#define   R_RX_PL_WID     0x60
#define   R_RX_PAYLOAD    0x61
#define   W_TX_PAYLOAD    0xa0
#define   W_ACK_PAYLOAD   0xa8
#define   FLUSH_TX        0xe1
#define   FLUSH_RX        0xe2
#define   REUSE_TX_PL     0xe3
#define   RNOP            0xff

#define   CONFIG          0x00
#define   EN_AA           0x01
#define   EN_RXADDR       0x02
#define   SETUP_AW        0x03
#define   SETUP_RETR      0x04
#define   RF_CH           0x05
#define   RF_SETUP        0x06
#define   STATUS          0x07
#define   OBSERVE_TX      0x08
#define   CD              0x09
#define   RX_ADDR_P0      0x0a
#define   RX_ADDR_P1      0x0b
#define   RX_ADDR_P2      0x0c
#define   RX_ADDR_P3      0x0d
#define   RX_ADDR_P4      0x0e
#define   RX_ADDR_P5      0x0f
#define   TX_ADDR         0x10
#define   RX_PW_P0        0x11
#define   RX_PW_P1        0x12
#define   RX_PW_P2        0x13
#define   RX_PW_P3        0x14
#define   RX_PW_P4        0x15
#define   RX_PW_P5        0x16
#define   FIFO_STATUS     0x17
#define   DYNPD           0x1C
#define   FEATURE         0x1D

#define   RX_DR   (sta & (1<<6))
#define   TX_DS   (sta & (1<<5))
#define   MAX_RT  (sta & (1<<4))
#define   RX_EMPTY (sta & (1<<0))

#define   ENABLE_ACK_PAYLOAD   1

BYTE gTxAddr[]   = {0x34, 0x43, 0x10, 0x10, 0x01};


BYTE ProcessPacket (BYTE *RxBuf, BYTE *AckBuf);

void NRF24L01Init()
{
  CE (0);
  CSN (1);
  DelayMs (10);
}

BYTE SpiRead ()
{
  return SpiWrite (0xFF);
}

BYTE NRFReadReg (BYTE RegAddr)
{
  BYTE BackData;
  CSN (0);
  SpiWrite (RegAddr);
  BackData = SpiRead();
  CSN (1);
  return (BackData);
}

void NRFWriteReg (BYTE RegAddr, BYTE Data)
{
  CSN (0);
  SpiWrite (RegAddr);
  SpiWrite (Data);
  CSN (1);
}

void NRFToggleFeature ()
{
  NRFWriteReg (ACTIVATE, 0x73);
}

BYTE NRFGetStatus()
{
  BYTE  Val;

  CSN (0);
  Val = SpiWrite (RNOP);
  CSN (1);

  return Val;
}

void NRFReadRxData (BYTE RegAddr, BYTE *RxData, BYTE DataLen)
{
  BYTE i;
  CSN (0);
  SpiWrite (RegAddr);
  for (i = 0; i < DataLen; i++) {
    RxData[i] = SpiRead();
  }
  CSN (1);
}

void NRFWriteTxData (BYTE RegAddr, BYTE *TxData, BYTE DataLen)
{
  BYTE i;
  CSN (0);
  SpiWrite (RegAddr);
  for (i = 0; i < DataLen; i++) {
    if (TxData == 0) {
      SpiWrite (0);
    } else {
      SpiWrite (*TxData++);
    }
  }
  CSN (1);
}

void NRFFlush(BYTE Mask)
{
  if (Mask & 1) {
    CSN (0);
    SpiWrite (FLUSH_TX);
    CSN (1);
  }
  if (Mask & 2) {
    CSN (0);
    SpiWrite (FLUSH_RX);
    CSN (1);
  }
  if ((Mask & 3) == 3) {
    NRFWriteReg (W_REGISTER + STATUS, 0x70);
  }

  #if ENABLE_ACK_PAYLOAD
  if (Mask & 1) {
    if (NRFReadReg (R_REGISTER + CONFIG) & 1) {
      // RX Mode, arm ACK payload
      NRFWriteTxData (W_ACK_PAYLOAD, 0, 1);
    }
  }
  #endif

}

void NRFSetMode()
{
  BYTE  Val;

  NRFFlush (FLUSH_TX | FLUSH_RX);

  NRFWriteReg (W_REGISTER + EN_AA, 0x01);
  NRFWriteReg (W_REGISTER + EN_RXADDR, 0x01);
  NRFWriteReg (W_REGISTER + RF_CH, 123);
  NRFWriteReg (W_REGISTER + RF_SETUP, 0x07);
  NRFWriteReg (W_REGISTER + SETUP_RETR, 0x8f);
  NRFWriteReg (W_REGISTER + STATUS, 0x70);

  #if ENABLE_ACK_PAYLOAD
    Val = NRFReadReg  (R_REGISTER + FEATURE);
    if ((Val & 0x06) != 0x06) {
      NRFToggleFeature ();
      NRFWriteReg (W_REGISTER + FEATURE, 0x06);
    }
    NRFWriteReg (W_REGISTER + DYNPD,  0x01);
  #else
    Val = 0;
  #endif
}

void NRFBegin()
{
  CE  (0);
  CSN (1);
  DelayMs(5);
  NRFWriteReg (W_REGISTER + SETUP_RETR, 0x4f);
}

void NRFStartListening()
{
  BYTE  Val;

  Val = NRFReadReg (R_REGISTER + CONFIG);
  NRFWriteReg (W_REGISTER + CONFIG, Val | 0x03);
  NRFWriteReg (W_REGISTER + STATUS, 0x70);
  NRFFlush (FLUSH_TX | FLUSH_RX);
  CE (1);
  DelayUs(130);
}

void NRFStopListening()
{
  CE (0);
  NRFFlush (FLUSH_TX | FLUSH_RX);
}


void NRFSetTxMode()
{
  CE (0);
  NRFWriteTxData (W_REGISTER + TX_ADDR, gTxAddr, TX_ADDR_WITDH);
  NRFWriteTxData (W_REGISTER + RX_ADDR_P0, gTxAddr, TX_ADDR_WITDH);
  NRFSetMode ();
  NRFWriteReg (W_REGISTER + CONFIG, 0x0e);
  CE (1);
}


void NRFSetRxMode()
{
  CE (0);
  NRFWriteTxData (W_REGISTER + RX_ADDR_P0, gTxAddr, TX_ADDR_WITDH);
  NRFSetMode ();
  NRFWriteReg (W_REGISTER + RX_PW_P0, TX_DATA_WITDH);
  NRFWriteReg (W_REGISTER + CONFIG, 0x0f);
  CE (1);
  #if ENABLE_ACK_PAYLOAD
    NRFWriteTxData (W_ACK_PAYLOAD, 0, 1);
  #endif
}

BYTE CheckACK()
{
  BYTE sta;

  #if ENABLE_ACK_PAYLOAD
    sta  = NRFReadReg (R_REGISTER + FIFO_STATUS);
    if (!RX_EMPTY) {
      // Successful
      return 3;
    } else {
      // In progress
      return 0;
    }
  #else
    sta = NRFReadReg (R_REGISTER + STATUS);
    if (TX_DS || MAX_RT) {
      NRFWriteReg (W_REGISTER + STATUS, sta);
      if (TX_DS) {
        // Successful
        return 1;
      } else {
        // Timeout, abort packet
        NRFFlush (FLUSH_TX);
        return 2;
      }
    } else {
      // In progress
      return 0;
    }
  #endif
}

BYTE NRFRxPacket (BYTE *RxData)
{
  BYTE sta;
  BYTE Len;
  BYTE RevFlags;
  BYTE AckPld[RX_DATA_WITDH];

  sta = NRFGetStatus();
  if (RX_DR) {
    NRFReadRxData (R_RX_PAYLOAD, RxData, RX_DATA_WITDH);
    #if ENABLE_ACK_PAYLOAD
      Len = ProcessPacket (RxData, AckPld);
      NRFWriteTxData (W_ACK_PAYLOAD, AckPld, Len);
    #else
      Len = 0;
    #endif
    RevFlags = 1;
  } else {
    RevFlags = 0;
  }
  if (sta & (RX_DR | MAX_RT)) {
    NRFWriteReg (W_REGISTER + STATUS, sta);
  }
  return RevFlags;
}

BYTE NRFTxPacket (BYTE *TxData)
{
  CE (0);
  NRFWriteTxData (W_TX_PAYLOAD, TxData, TX_DATA_WITDH);
  CE (1);
  return 0;
}

void Rf2400Dump ()
{
  BYTE Idx;
  for (Idx = 0; Idx < 0x20; Idx++) {
    printf ("%02X ", NRFReadReg (Idx));
    if ((Idx & 0x0F) == 0x0F) {
      printf ("\n");
    }
  }
  printf ("\n");
}

BYTE Rf2400Init (SPI_TypeDef *Spi)
{
  // Reconfig SPI2
  Spi->CR1 &= 0xFFBF;
  Spi->CR1  = (SPI2->CR1 & 0xFFFC) | (SPI_CPOL_Low | SPI_CPHA_1Edge);
  Spi->CR1 |= 0x0040;
  NRF24L01Init ();

  if (NRFReadReg (SETUP_AW) == 3) {
    return 0;
  } else {
    return 1;
  }
}


