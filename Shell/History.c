/*********************************************************************
 ********************************************************************/ 
#include "Common.h"
#include "Console.h"

/********************************************************************/
#define CTRL_U  0x15  /* command line previous */
#define CTRL_D  0x04  /* command line next */
#define CTRL_E  0x05  /* command line next */
#define CTRL_R  0x12  /* last command repeat */

#define  FALSE   0
#define  TRUE    1

#define MAX_HISTORY_REC   8

typedef char HISTENT[MAX_LINE_LEN];
HISTENT      History[MAX_HISTORY_REC+1];

/*******************************************************************/
void HistoryInit (void)
{
  int  index;
  for (index = 0; index < MAX_HISTORY_REC; index++) {    
    memset((void*)History[index], 0, MAX_LINE_LEN);
  }
}

/********************************************************************/
static int HistoryDown (int *CurrHist, char *Line)
{
  *CurrHist -= 1;
  if (*CurrHist < 0)
  {
    *CurrHist = -1;
    Line[0] = '\0';
    return 0;
  }
  strcpy(Line, (void *)History[*CurrHist]);
  return strlen(Line);
}

/********************************************************************/
static int HistoryUp (int *CurrHist, char *Line)
{
  if (*CurrHist == -1) *CurrHist = 0;
  else {
    if (strlen(History[*CurrHist])) {
      (*CurrHist)++;
      if (*CurrHist >= MAX_HISTORY_REC) *CurrHist = (MAX_HISTORY_REC-1);
    }
  }
  strcpy(Line, (void *)History[*CurrHist]);
  return strlen(Line);
}

/********************************************************************/
static int HistoryRepeat (char *Line)
{
  strcpy(Line, (void *)History[0]);
  return strlen(Line);
}

/********************************************************************/
static void HistoryAdd (char *Line)
{
  int index;
  for (index = 0; index < MAX_HISTORY_REC; index++) {    
    if (!strcmp(History[index],Line)) {
      strcpy((void *)History[MAX_HISTORY_REC], (void *)History[0]);
      strcpy((void *)History[0],(void *)History[index]);
      strcpy((void *)History[index],(void *)History[MAX_HISTORY_REC]);
      return;
    }
  }    
  
  for (index = (MAX_HISTORY_REC-1); index > 0; --index) {    
    strcpy((void *)History[index], (void *)History[index-1]);
  }
  
  strcpy((void *)History[0], Line);  
}

/********************************************************************/
char *GetHistoryLine (char *UserLine)
{
  char Line[MAX_LINE_LEN];
  int  Pos,CurrHist,Repeat;
  int  ch, i;
  
  CurrHist = -1;
  Repeat   = FALSE;
  Pos      = 0;
  
  ch = (int)GetChar();
  while ( (ch != 0x0D /* CR */) &&
      (ch != 0x0A /* LF/NL */) &&
      (Pos < MAX_LINE_LEN))
  {
    if (ch==27) {
      ch = (int)GetChar();
      if (ch=='[') {
        ch = (int)GetChar();
        if (ch=='B')      ch=CTRL_D;
        else if (ch=='A') ch=CTRL_U;
        else if (ch=='C') ch=CTRL_R;
        else if (ch=='D') ch=CTRL_E;
        else ch=0x80;
      } 
    }     
    
    switch (ch)
    {
      case 0x08:    /* Backspace */
      case 0x7F:    /* Delete */
        if (Pos > 0)
        {
          Pos -= 1;
          PutChar(0x08);  /* backspace */
          PutChar(' ');
          PutChar(0x08);  /* backspace */
        }
        break;
      case CTRL_U:
      case CTRL_D:
      case CTRL_R:
      case CTRL_E:
        for (i = 0; i < Pos; ++i) {
          /* backspace */  
          PutChar('\b');
          PutChar(' ');
          PutChar('\b');
        }
        Pos = 0;  
        
        if (ch == CTRL_U)
        {
          Pos = HistoryUp(&CurrHist,Line);
          Puts(Line);
          break;
        }
        if (ch == CTRL_D)
        {
          Pos = HistoryDown(&CurrHist,Line);
          Puts(Line);
          break;
        }
        if (ch == CTRL_R)
        {
          Pos = HistoryRepeat(Line);
          Puts(Line);
          Repeat = TRUE;
        }
        break;
      default:
        if ((Pos+1) < MAX_LINE_LEN)
        {
          /* only printable characters */
          if ((ch > 0x1f) && (ch < 0x80))
          {
            Line[Pos++] = (char)ch;
            PutChar((char)ch);
          }
        }
        break;
    }

    if (Repeat)
    {
      break;
    }
    ch = (int)GetChar();
  }
  Line[Pos] = '\0';
  PutChar(0x0D);  /* CR */
  PutChar(0x0A);  /* LF */

  if ((strlen(Line) != 0) && !Repeat)
  {
    HistoryAdd(Line);
  }
  strcpy (UserLine,Line);

  return UserLine;
}

/********************************************************************/
