#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define HRUST_PC_VERSION	"1.1bi"

/*
   Hrust 1.3 compressor
   Based on original code
   Nikita Burnashev, 2005

   depacker added by psb, 2007
   version 1.1bi

   1.1bi: some little bug improved,
          input file size check (print warning)
          some cosmetic changes
          bufOut size added!
*/

#define MAXWND 0x2000
#define BASE 0x2000
#define RES 0x1000

unsigned char bufIn[MAXWND + BASE + RES];
unsigned char hdr[12];
unsigned char bufOut[BASE + BASE / 8 +BASE ];   //???
unsigned char *pIn, *pInRun, *pInEnd, *pOut, *pOutBits;

int wnd;

unsigned long bitBuf;
int bitLen;

int runLen;
int longOfs, longOfsLen;

#define TAILPOS bufIn

typedef struct tagENTRY
{
    unsigned char *pMatch;
    struct tagENTRY *peNext;
}
ENTRY, *PENTRY;

ENTRY head[256];
ENTRY dict[MAXWND];
PENTRY peCur;

int fGap, gapOfs;
int matchSize, matchOfs;

/* Proto */
void hrustStoreRun(void);
void hrustMatch(void);
void hrustStore(void);
void hrustFinish(void);





void hrustStart(int awnd)
{
    PENTRY pe;

    wnd = awnd;
    bitBuf = bitLen = runLen = 0;
    longOfs = -0x400;
    longOfsLen = 2;

    for (pe = head; pe < head + 256; pe++) pe->pMatch = TAILPOS;
    for (pe = dict; pe < dict + MAXWND; pe++) pe->pMatch = TAILPOS;
    peCur = dict;

    pe = &head[*pIn];
    pe->pMatch = pIn + 1;
    pe->peNext = peCur++;

    *pOut++ = *pIn++;
}

void hrustMatch()
{
    PENTRY pe;
    unsigned char *pm, *pn, *pl;
    int cnt, ofs;

    fGap = matchSize = 0;
    pe = &head[*pIn];
    while (pe->pMatch + wnd > pIn)
    {
        pm = pe->pMatch;
        pn = pIn + 1;
        ofs = pm - pn;
        /* Original code switched off gaps at pInLim - 5. Any ideas why? */
        if (pn + 1 < pInEnd && !fGap && pm[1] == pn[1] && ofs >= -79)
        {
            fGap = 1;
            gapOfs = ofs;
        }
        pl = pIn + 0xeff;
        if (pl > pInEnd) pl = pInEnd;
        while (pn < pl && *pm == *pn)
        {
            pm++;
            pn++;
        }
        cnt = pn - pIn;
        if (cnt > matchSize && ((cnt == 1 && ofs >= -8)
            || (cnt == 2 && ofs >= -0x300) || cnt >= 3))
        {
            matchSize = cnt;
            matchOfs = ofs;
        }
        if (cnt >= 0xff) break;
        pe = pe->peNext;
    }
    pe = &head[*pIn];
    memcpy(peCur, pe, sizeof(ENTRY));
    pe->pMatch = pIn + 1;
    pe->peNext = peCur++;
    if (peCur == dict + MAXWND) peCur = dict;

    if (matchSize > 1) fGap = 0;
    if (fGap && matchSize == 1)
    {
        pe = &head[pIn[1]];
        while (pe->pMatch + wnd > pIn + 1)
        {
            pm = pe->pMatch;
            pn = pIn + 2;
            ofs = pm - pn;
            while (pn < pIn + 4 && *pm == *pn)
            {
                pm++;
                pn++;
            }
            cnt = pn - pIn - 1;
            if ((cnt == 1 && ofs >= -8)
                || (cnt == 2 && ofs >= -0x300) || cnt >= 3)
            {
                fGap = 0;
                break;
            }
            pe = pe->peNext;
        }
    }
}

void _fastcall hrustBits(int len, unsigned long buf)
{
    buf |= bitBuf << len;
    len += bitLen;
    while (len >= 16)
    {
        len -= 16;
        pOutBits[0] = (unsigned char) (buf >> len);
        pOutBits[1] = (unsigned char) (buf >> len >> 8);
        pOutBits = pOut;
        pOut += 2;
    }
    bitBuf = buf;
    bitLen = len;
}

void hrustStoreRun()
{
    unsigned char *pb = pInRun;
    int cnt = runLen;

    if (cnt >= 12)
    {
        hrustBits(7, 0x31); /* 0110001 */
        hrustBits(4, (cnt - 12) >> 1);
        while (cnt > 1)
        {
            *pOut++ = *pb++;
            *pOut++ = *pb++;
            cnt -= 2;
        }
    }
    while (cnt--)
    {
        hrustBits(1, 1); /* 1 */
        *pOut++ = *pb++;
    }
    runLen = 0;
}

const int lenTab[17] = { 0, 0, 5, 5, 7, 7, 9, 9, 9, 11, 11, 11, 13, 13, 13, 13, 2 };

const unsigned long bitTab[4][17] =
{
    { 0, 0, 7, 0xa, 0x36, 0x3a, 0xf2, 0xf6, 0xfa, 0x3f2, 0x3f6, 0x3fa, 0xff2, 0xff6, 0xffa, 0xffe, 2 },
    { 0, 0, 6, 0x9, 0x35, 0x39, 0xf1, 0xf5, 0xf9, 0x3f1, 0x3f5, 0x3f9, 0xff1, 0xff5, 0xff9, 0xffd, 1 },
    { 0, 0, 5, 0x8, 0x34, 0x38, 0xf0, 0xf4, 0xf8, 0x3f0, 0x3f4, 0x3f8, 0xff0, 0xff4, 0xff8, 0xffc, 0 },
    { 0, 0, 4, 0xb, 0x37, 0x3b, 0xf3, 0xf7, 0xfb, 0x3f3, 0x3f7, 0x3fb, 0xff3, 0xff7, 0xffb, 0xfff, 3 }
};

void hrustStore()
{
    int cnt = matchSize, len;
    PENTRY pe;

    hrustStoreRun();
    if (fGap)
    {
        if (gapOfs >= -0x10)
        {
            hrustBits(6, 0x19); /* 011001 */
            hrustBits(4, gapOfs & 0xf);
        }
        else
        {
            hrustBits(5, (gapOfs & 1) ? 9 : 6); /* 01001 : 00110 */
            *pOut++ = (gapOfs + 0xf) >> 1 ^ 1;
        }
        *pOut++ = pIn[1];
        cnt = 3;
    }
    else if (cnt == 1)
    {
        hrustBits(3, 0); /* 000 */
        hrustBits(3, matchOfs & 7);
    }
    else
    {
        while (matchOfs < longOfs)
        {
            longOfs = longOfs << 1;
            longOfsLen++;
            hrustBits(5, 6); /* 00110 */
            *pOut++ = 0xfe;
        }
        if (cnt > 15)
        {
            hrustBits(7, 0x30); /* 0110000 */
            if (cnt < 0x80) hrustBits(7, cnt);
            else
            {
                hrustBits(7, cnt >> 8);
                *pOut++ = cnt & 0xff;
            }
            cnt = 16;
        }
        len = lenTab[cnt];
        if (matchOfs >= -0x20)
        {
            hrustBits(len, bitTab[0][cnt]);
            hrustBits(5, matchOfs & 0x1f);
        }
        else
        {
            if (matchOfs >= -0x100) hrustBits(len, bitTab[1][cnt]);
            else if (matchOfs >= -0x200) hrustBits(len, bitTab[2][cnt]);
            else
            {
                hrustBits(len, bitTab[3][cnt]);
                if (cnt > 2) hrustBits(longOfsLen, (matchOfs & ~longOfs) >> 8);
            }
            *pOut++ = matchOfs & 0xff;
        }
        cnt = matchSize;
    }
    /* Fix dictionary for match */
    if (cnt >= 256)
    {
        pIn += cnt - 256;
        peCur += cnt - 256;
        while (peCur >= dict + MAXWND) peCur -= MAXWND;
        cnt = 256;
    }
    cnt--;
    pIn++;
    while (cnt--)
    {
        pe = &head[*pIn++];
        memcpy(peCur, pe, sizeof(ENTRY));
        pe->pMatch = pIn;
        pe->peNext = peCur++;
        if (peCur == dict + MAXWND) peCur = dict;
    }
}

void hrustCompress(unsigned char *pInLim)
{
    while (pIn < pInLim)
    {
        hrustMatch();
        if (!fGap && matchSize == 0)
        {
            if (runLen == 0) pInRun = pIn;
            runLen++;
            if (runLen == 42) hrustStoreRun();
            pIn++;
        }
        else hrustStore();
    }
    hrustStoreRun();
}

void hrustFinish()
{
    hrustBits(14, 0x180f); /* 01100000001111 */
    if (bitLen > 0) hrustBits(16 - bitLen, 0);
    pOut -= 2;
}

void shiftEntries(PENTRY pe, PENTRY peEnd)
{
    while (pe < peEnd)
    {
        pe->pMatch -= MAXWND;
        if (pe->pMatch < bufIn) pe->pMatch = TAILPOS;
        pe++;
    }
}



//standart hrust depacker
unsigned char hrust_depacker[0x103]={
	0xF3, 0xED, 0x73, 0xFD, 0x80, 0x11, 0x00, 0x5B, 0x21, 0x25, 0x80, 0x01, 0xDE, 0x00, 0xD5, 0xED,
	0xB0, 0x13, 0x13, 0xD5, 0xDD, 0xE1, 0x0E, 0x06, 0x09, 0xED, 0xB0, 0x21, 0x55, 0x8E, 0x11, 0xA7,
	0x99, 0x01, 0x47, 0x0D, 0xC9, 0xED, 0xB8, 0x13, 0xEB, 0xF9, 0x11, 0x00, 0x7F, 0xD9, 0x16, 0xBF,
	0x01, 0x10, 0x10, 0xE1, 0x3B, 0xF1, 0xD9, 0x12, 0x13, 0xD9, 0x29, 0x10, 0x02, 0xE1, 0x41, 0x38,
	0xF3, 0x1E, 0x01, 0x3E, 0x80, 0x29, 0x10, 0x02, 0xE1, 0x41, 0x17, 0x38, 0xF8, 0xFE, 0x03, 0x38,
	0x05, 0x83, 0x5F, 0xA9, 0x20, 0xED, 0x83, 0xFE, 0x04, 0x28, 0x5A, 0xCE, 0xFF, 0xFE, 0x02, 0xD9,
	0x4F, 0xD9, 0x3E, 0xBF, 0x38, 0x14, 0x29, 0x10, 0x02, 0xE1, 0x41, 0x17, 0x38, 0xF8, 0x28, 0x05,
	0x3C, 0x82, 0x30, 0x08, 0x92, 0x3C, 0x20, 0x0C, 0x3E, 0xEF, 0x0F, 0xBF, 0x29, 0x10, 0x02, 0xE1,
	0x41, 0x17, 0x38, 0xF8, 0xD9, 0x26, 0xFF, 0x28, 0x06, 0x67, 0x3B, 0x3C, 0x28, 0x0C, 0xF1, 0x6F,
	0x19, 0xED, 0xB0, 0x18, 0xA4, 0xD9, 0xCB, 0x0A, 0x18, 0xA0, 0xF1, 0xFE, 0xE0, 0x38, 0xF0, 0x07,
	0xA9, 0x3C, 0x28, 0xF1, 0xD6, 0x10, 0x6F, 0x4F, 0x26, 0xFF, 0x19, 0xED, 0xA0, 0x3B, 0xF1, 0x12,
	0x23, 0x13, 0x7E, 0x18, 0x82, 0x3E, 0x80, 0x29, 0x10, 0x02, 0xE1, 0x41, 0x8F, 0x20, 0x19, 0x38,
	0xF6, 0x3E, 0xFC, 0x18, 0x16, 0x3B, 0xC1, 0x48, 0x47, 0x3F, 0x18, 0x95, 0xFE, 0x0F, 0x38, 0xF5,
	0x20, 0x8E, 0xC6, 0xF4, 0xDD, 0xF9, 0x18, 0x14, 0x9F, 0x3E, 0xEF, 0x29, 0x10, 0x02, 0xE1, 0x41,
	0x17, 0x38, 0xF8, 0xD9, 0x20, 0xC0, 0xCB, 0x7F, 0x28, 0xE2, 0xD6, 0xEA, 0xEB, 0xD1, 0x73, 0x23,
	0x72, 0x23, 0x3D, 0x20, 0xF8, 0xEB, 0x30, 0x9B, 0x21, 0x58, 0x27, 0xD9, 0x31, 0x00, 0x00, 0xFB,
	0xC3, 0x52, 0x00};

void dehrust_put_byte(int offs, char byte_)
{
  hrust_depacker[offs] = byte_;
}

void dehrust_put_word(int offs, int word_)
{
  hrust_depacker[offs] = word_ & 0xff;
  hrust_depacker[offs+1] = word_ >> 8;
}




/*
  start		- start adr of packed block (with depacker)
  depackto	- depack to adr
  dadr		- depacker adr
  sp		- new sp (-1==keep old sp)
  ei_inter	- enable int (1==ei, 0==di)
  jp		- jp adr (-1==ret)
  packed_len	-
  unpacked_len	-
*/

void dehrust_patch(int start_, int depackto_, int dadr_, int sp_, char ei_inter_, int jp_, int packed_len_, int unpacked_len_)
{
  if (sp_<0)
    dehrust_put_word(0x03, start_+0xfd);	//no new sp, store old
  else
    dehrust_put_word(0x03, 0);			//new sp, don't store old

  dehrust_put_word(0x06, dadr_);		//depacker adr

  dehrust_put_word(0x09, start_+0x25);		//depacker offset

  dehrust_put_word(0x1c, start_+0x102+packed_len_);	//end of packed block

  if ( (start_+0x102+packed_len_) < (depackto_+unpacked_len_-1) )	//move or not packed block
    dehrust_put_word(0x1f, depackto_+unpacked_len_-1);
  else
    dehrust_put_word(0x1f, start_+0x102+packed_len_);

  dehrust_put_word(0x22, packed_len_-12);	//packed block length

  dehrust_put_word(0x2b, depackto_);		//depack to adr

  if (sp_>=0)
    dehrust_put_word(0xfd, sp_);		//new sp
  dehrust_put_byte(0xff, (ei_inter_) ? 0xfb : 0xf3);	//interrupt
  dehrust_put_word(0x101, (jp_>=0) ? jp_ : 0x0052);	//jp adr
}




const WORD clr_default	= 0x07;
const WORD clr_info	= 0x0a;
const WORD clr_info_	= 0x0e;
const WORD clr_warning	= 0x0c;
const WORD clr_done  	= 0x0f;

void set_color(WORD ink)
{
  SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), ink);
}






int main(int argc, char *argv[])
{
    int i, spd;
    char *nameIn, *nameOut, *p;
    FILE *pfi, *pfo;
    size_t sizeIn, sizeOut, rd, sh;

    char ei_inter;
    int depacker, start, depackto, dadr, sp, jp;

    nameIn = NULL;
    nameOut = NULL;
    spd = 3;

        depacker=0;
        ei_inter=1;
        start=0x6000;
        depackto=start;
        dadr=0x5b00;
        sp=-1;
        jp=-1;

    for (i = 1; i < argc; i++)
    {
        p = argv[i];
        if (*p == '-')
        {
            if (strcmp(p, "-spd") == 0) spd = atoi(argv[++i]);
            else if (strcmp(p, "-depacker") == 0) depacker=sizeof(hrust_depacker);
            else if (strcmp(p, "-ei") == 0) ei_inter=1;
            else if (strcmp(p, "-di") == 0) ei_inter=0;
            else if (strcmp(p, "-start") == 0) start = atoi(argv[++i]);
            else if (strcmp(p, "-depackto") == 0) depackto = atoi(argv[++i]);
            else if (strcmp(p, "-dadr") == 0) dadr = atoi(argv[++i]);
            else if (strcmp(p, "-sp") == 0) sp = atoi(argv[++i]);
            else if (strcmp(p, "-jp") == 0) jp = atoi(argv[++i]);
        }
        else if (!nameIn) nameIn = p;
        else if (!nameOut) nameOut = p;
    }
    if (!nameIn || !nameOut ||
         spd < 0 || spd > 3 ||
         start<0 || start>0xffff ||
         depackto<0 || depackto>0xffff ||
         dadr<0 || dadr>0xffff ||
         sp>0xffff || jp>0xffff)
    {
        set_color(clr_info);
        printf("\nHrust 1.3, version " HRUST_PC_VERSION "\n");
        set_color(clr_default);
        printf("Original packer by Hrumer, PC-version by Nikita Burnashev,\n"
               "depacker added by psb.\n\n");
        set_color(clr_info);
        printf("Usage: hrust13 [-spd <speed>] [-depacker] [-ei | -di] [-start <addr>]\n"
               "               [-depackto <addr>] [-dadr <addr>] [-sp <addr>] [-jp <addr>]\n"
               "               <input.bin> <output.bin>\n\n");
        set_color(clr_info_);
        printf("-spd      set speed: 0=FASTLY ... 3=NORMAL (default)\n"
               "-depacker turn on depacker (no)\n"
               "-ei       enable int on exit (yes)\n"
               "-di       disable int on exit (no)\n"
               "-start    set start address of packed block (with depacker) (#6000)\n"
               "-depackto set depack address (#6000)\n"
               "-dadr     set address of depacker (#5B00)\n"
               "-sp       set new SP on exit (no)\n"
               "-jp       set address of jump to (no)\n\n");
        set_color(clr_default);
        return 1;
    }

    pfi = fopen(nameIn, "rb");
    if (!pfi)
    {
        set_color(clr_warning);
        printf("Hrust1.3: Can't open '%s' for reading.\n", nameIn);
        set_color(clr_default);
        return 2;
    }

    pfo = fopen(nameOut, "wb");
    if (!pfo)
    {
        fclose(pfi);
        set_color(clr_warning);
        printf("Hrust1.3: Can't open '%s' for writting.\n", nameOut);
        set_color(clr_default);
        return 2;
    }

    pIn = bufIn + MAXWND;
    pInEnd = bufIn + MAXWND + BASE + RES;
    rd = fread(pIn, 1, BASE + RES, pfi);
    sizeIn = rd;

      if (rd<7)
      {
        fclose(pfi); fclose(pfo);
        set_color(clr_warning);
        printf("Hrust1.3: Input file '%s' too short!\n", nameIn);
        set_color(clr_default);
        return 3;
      }

    if (depacker) fwrite(hrust_depacker, 1, depacker, pfo);


    pOut = bufOut + sizeof(hdr) + 2;
    pOutBits = bufOut + sizeof(hdr);
    sizeOut = 0;

    hrustStart(0x400 << spd);
    while (rd == BASE + RES)
    {
        hrustCompress(bufIn + MAXWND + BASE);
        sizeOut += fwrite(bufOut, 1, pOutBits - bufOut, pfo);

        sh = pOut - pOutBits;
        memcpy(bufOut, pOutBits, sh);
        pOut = bufOut + sh;
        pOutBits = bufOut;

        memmove(bufIn, bufIn + MAXWND, BASE + RES);
        pIn -= MAXWND;
        shiftEntries(head, head + 256);
        shiftEntries(dict, dict + MAXWND);

        rd = fread(bufIn + BASE + RES, 1, MAXWND, pfi);
        sizeIn += rd;
        rd += RES;
    }
    pInEnd = bufIn + MAXWND + rd - 6;
    hrustCompress(pInEnd);
    hrustFinish();
    sizeOut += fwrite(bufOut, 1, pOut - bufOut, pfo);

      if (sizeIn>0xBEFD)
      {
        set_color(clr_warning);
        printf("Hrust1.3: Warning! Input file '%s' too big!\n", nameIn);
        set_color(clr_default);
      }

    memcpy(hdr, "HR", 2);
    memcpy(hdr + 2, &sizeIn, 2);
    memcpy(hdr + 4, &sizeOut, 2);
    memcpy(hdr + 6, pInEnd, 6);
    fseek(pfo, 0, SEEK_SET);

    if (depacker)
    {
      dehrust_patch(start, depackto, dadr, sp, ei_inter, jp, sizeOut, sizeIn);
      fwrite(hrust_depacker, 1, depacker, pfo);
    }

    fwrite(hdr, 1, 12, pfo);

    fclose(pfo);
    fclose(pfi);

    set_color(clr_done);
    printf("Hrust1.3: File '%s' packed.\tRatio: %3.2f%%\t(%3.2f%%).\n",
           nameIn,
           (1.0-(float)sizeOut/(float)sizeIn)*100.0,
           (1.0-(float)(sizeOut+depacker)/(float)sizeIn)*100.0 );
    set_color(clr_default);
    return 0;
}


