#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define DEHRUM_PC_VERSION	"1.0"




BYTE bufIn[49152];
BYTE bufOut[49152];

BYTE *hr_d_output;
BYTE *hr_d_input;
WORD  hr_d_tagbyte;/*буфер для битов.*/
BYTE  hr_d_tagcalc;/*счетчик этих битов*/

void hr_d_getbitinit(void)
{
  hr_d_tagbyte = *hr_d_input++;
  hr_d_tagbyte = hr_d_tagbyte + ((*hr_d_input++) << 8);
  hr_d_tagcalc = 0x10;
}

BYTE hr_d_getbit(void)
{
  WORD tmp;

  tmp = hr_d_tagbyte & 0x8000;
  hr_d_tagbyte <<= 1;
  hr_d_tagcalc--;

  if(!hr_d_tagcalc)
  {
    hr_d_getbitinit();
  }
  return (tmp >> 15);
}

WORD hr_d_getlen(void)
{
  WORD tmp, result = 1;
  do
  {
    tmp = (hr_d_getbit() << 1) + hr_d_getbit();
    result += tmp;
  } while(tmp == 0x03 && result != 0x10);

  return(result);
}

WORD hr_d_getdist(void)
{
  WORD result, tmp;

  if(!hr_d_getbit())
  {
    result = 0xff00 + *hr_d_input++;
  }
  else
  {
    result = 0xffff;
    for(tmp=4; tmp; tmp--) result = (result << 1) + hr_d_getbit();
    result = (result << 8) + *hr_d_input++;
  }
  return (result);
}

void dehrum(BYTE* source, BYTE* destination)
{
  BYTE  tmpbuf[5];
  WORD  len;
  short offs;

  hr_d_input   = source + 0x91;
  hr_d_output  = destination;

  for(int i = 0; i < 5; i++) tmpbuf[i] = *hr_d_input++;
  hr_d_input+=2;/*пропустили 2 байта 0x1010 - инициализация переменных*/
  hr_d_getbitinit();/*взяли 16 битов в буфер*/

  *hr_d_output++ = *hr_d_input++;

  bool done = false;
  while(!done)
  {
    if(!hr_d_getbit())
    {
      len = hr_d_getlen();
      offs = 0xffff;

      if(len == 4)
      {
    len = *hr_d_input++;

    if (len == 0)
      done = true;
    else
    {
      offs = hr_d_getdist();
      for(; len; len--)  *hr_d_output = *(hr_d_output + offs), hr_d_output++;
    }
      }
      else
      {
  if(len > 4) len--;
  if(len == 1)
  {
    for(int i = 3; i; i--) offs = (offs << 1) + hr_d_getbit();
  }
  else
  {
    if(len == 2)
      offs = 0xff00 + *hr_d_input++;
    else
      offs = hr_d_getdist();
  }
  for(; len; len--) *hr_d_output = *(hr_d_output + offs), hr_d_output++;

      }
    }
    else
    {
      *hr_d_output++ = *hr_d_input++;
    }
  }

  for(int i = 0; i < 5; i++) *hr_d_output++ = tmpbuf[i];
}





bool check_hrum35(int *pksz, int *unpsz)
{
  if ( bufIn[5]  != 0x21 || bufIn[8]  != 0x11 ||
       bufIn[11] != 0x01 || bufIn[12] != 0x77 ||
       bufIn[13] != 0x00 || bufIn[14] != 0xd5 ||
       bufIn[15] != 0xed ||
       bufIn[0x96] != 0x10 || bufIn[0x97] != 0x10
     ) return false;

  *pksz  = bufIn[0x1c]+256*bufIn[0x1d] + 5;
  *unpsz = bufIn[0x19]+256*bufIn[0x1a] - (bufIn[0x12]+256*bufIn[0x13]);
  *unpsz+= (bufIn[0x20] == 0xb8) ? 1 : bufIn[0x1c]+256*bufIn[0x1d];

  //if ( *pksz > *unpsz ) return false; //is it right check?
  if ( *pksz > 49033 || *pksz < 11) return false;
  if ( *unpsz > 49033 || *unpsz < 7) return false;
  return true;
}








const WORD clr_default	= 0x07;
const WORD clr_info		= 0x0a;
const WORD clr_info_		= 0x0e;
const WORD clr_warning	= 0x0c;
const WORD clr_done  	= 0x0f;

void set_color(WORD ink)
{
  SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), ink);
}







int main(int argc, char* argv[])
{
    int i;
    char *nameIn, *nameOut, *p;
    FILE *pfi, *pfo;
    int unp_size, pk_size;

    nameIn = NULL;
    nameOut = NULL;

    for (i = 1; i < argc; i++)
    {
        p = argv[i];
        if (*p != '-')
        {
          if (!nameIn) nameIn = p;
          else if (!nameOut) nameOut = p;
        }
    }
    if (!nameIn || !nameOut)
    {
        set_color(clr_info);
        printf("\nHrum 3.5i depacker, version " DEHRUM_PC_VERSION "\n");
        set_color(clr_default);
        printf("Original depacker by Hrumer, C++ version by Hrumer & HalfElf,\n"
               "console version by psb.\n\n");
        set_color(clr_info);
        printf("Usage: dehrum35 <packed.bin> <depacked.bin>\n\n");
        set_color(clr_default);
        return 1;
    }

    pfi = fopen(nameIn, "rb");
    if (!pfi)
    {
        set_color(clr_warning);
        printf("DeHrum 3.5i: Can't open '%s' for reading.\n", nameIn);
        set_color(clr_default);
        return 2;
    }

    pfo = fopen(nameOut, "wb");
    if (!pfo)
    {
        fclose(pfi);
        set_color(clr_warning);
        printf("DeHrum 3.5i: Can't open '%s' for writting.\n", nameOut);
        set_color(clr_default);
        return 2;
    }

  i=fread(bufIn, 1, 0xc000, pfi);

  if (!check_hrum35(&pk_size,&unp_size) || i>=49152 || i<156)
  {
    fclose(pfi); fclose(pfo);
    set_color(clr_warning);
    printf("DeHrum 3.5i: '%s' is not hrum3.5i file.\n", nameIn);
    set_color(clr_default);
    return 2;
  }

  dehrum( bufIn, bufOut );

  fwrite(bufOut, 1, unp_size, pfo);
  fclose(pfi); fclose(pfo);

  set_color(clr_done);
  printf("DeHrum 3.5i: File '%s' depacked.\n",nameIn);
  set_color(clr_default);

  return 0;
}





