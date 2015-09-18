#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#undef DEBUG
#define HRUM_PC_VERSION	"1.1b"

/* Hrum 3.5 compressor
   Based on original code
   Nikita Burnashev, 2005

   original depacker added by psb, 2007
   version 1.1b
*/

typedef struct tagENTRY
{
    unsigned char *p;
    struct tagENTRY *pn;
}
ENTRY;

ENTRY *tail[256];
ENTRY dict[4096];

unsigned char *po;
unsigned short *pb;
int bbuf;

void bit(int value)
{
    bbuf = (bbuf << 1) | value;
    if (bbuf & 0x10000)
    {
        *pb = bbuf & 0xffff;
        pb = (unsigned short *) po;
        po += 2;
        bbuf = 1;
    }
}

int compress(unsigned char *pIn, unsigned char *pInEnd, unsigned char *pOut, int opt)
{
    unsigned char *pi = pIn, cc;
    ENTRY *pd = dict;

    pb = (unsigned short *) pOut;
    po = pOut + 2;
    bbuf = 1;

    pd->p = pi;
    cc = *pi++;
    pd->pn = tail[cc];
    tail[cc] = pd;
    pd++;

    *po++ = cc;

    do
    {
        unsigned char *pp;
        ENTRY *pds;
        int msize, mofs;

        /* find match */
        cc = *pi;
        pds = tail[cc];
        pp = pi;
        msize = 0;
        while (pds != NULL && pds->p < pp && *(pds->p) == cc)
        {
            unsigned char *pwc, *pic;
            int nsize, nofs;

            pwc = pds->p + 1;
            pic = pi + 1;
            nsize = 1;
            while (pic < pInEnd && nsize < 255 && *pwc == *pic)
            {
                pwc++;
                pic++;
                nsize++;
            }

            nofs = pi - pds->p;
            if (nsize > msize && ((nsize == 1 && nofs <= 8)
                || (nsize == 2 && nofs <= 256) || (nsize >= 3 && nofs <= 4096)))
            {
                msize = nsize;
                mofs = nofs;
            }

            pp = pds->p;
            pds = pds->pn;
        }

#ifdef DEBUG
        if (msize == 0) printf("0x%04x 0x%04x 0x%02x\n", pi - pIn, po - pOut, cc);
        else printf("0x%04x 0x%04x %d 0x%04x\n", pi - pIn, po - pOut, msize, -mofs & 0xffff);
#endif

        pd->p = pi;
        cc = *pi;
        pd->pn = tail[cc];
        tail[cc] = pd;
        pd++;
        if (pd == dict + 4096) pd = dict;

        /* store match */
        if (msize == 0)
        {
            bit(1);
            *po++ = *pi++;
        }
        else if (msize == 1)
        {
            bit(0);
            bit(0);
            bit(0);
            bit(-mofs >> 2 & 1);
            bit(-mofs >> 1 & 1);
            bit(-mofs & 1);
            pi++;
        }
        else if (msize == 2)
        {
            bit(0);
            bit(0);
            bit(1);
            *po++ = -mofs & 0xff;
        }
        else
        {
            if (msize == 3)
            {
                bit(0);
                bit(1);
                bit(0);
            }
            else if (msize < 15)
            {
                int i = msize;

                bit(0);
                while (i >= 3)
                {
                    bit(1);
                    bit(1);
                    i -= 3;
                }
                bit(i >> 1);
                bit(i & 1);
            }
            else if (msize == 15)
            {
                int i;

                bit(0);
                for (i = 0; i < 10; i++) bit(1);
            }
            else
            {
                bit(0);
                bit(1);
                bit(1);
                bit(0);
                bit(0);
                *po++ = msize & 0xff;
            }
            if (mofs <= 256) bit(0);
            else
            {
                bit(1);
                bit(-mofs >> 11 & 1);
                bit(-mofs >> 10 & 1);
                bit(-mofs >> 9 & 1);
                bit(-mofs >> 8 & 1);
            }
            *po++ = -mofs & 0xff;
        }

        if (msize > 1)
        {
            if (msize < 255 || opt)
            {
                pi++;
                while (--msize)
                {
                    pd->p = pi;
                    cc = *pi++;
                    pd->pn = pds = tail[cc];
                    tail[cc] = pd;
                    pd++;
                    if (pd == dict + 4096) pd = dict;
                }
            }
            else
            {
                pi += msize;
                pd += msize - 1;
                if (pd >= dict + 4096) pd -= 4096;
            }
        }
    }
    while (pi < pInEnd);

    bit(0);
    bit(1);
    bit(1);
    bit(0);
    bit(0);
    *po++ = 0;

    if (bbuf != 1)
    {
        while (!(bbuf & 0x10000)) bbuf <<= 1;
        *pb = bbuf & 0xffff;
    }

    return (po - pOut);
}




//standart hrum depacker
unsigned char hrum_depacker[0x91]={
	0xF3, 0xED, 0x73, 0x8B, 0x80, 0x21, 0x1F, 0x80, 0x11, 0x89, 0x5B, 0x01, 0x77, 0x00, 0xD5, 0xED,
	0xB0, 0x11, 0x00, 0x80, 0xD9, 0x21, 0x50, 0x94, 0x11, 0x4F, 0xBA, 0x01, 0xBB, 0x13, 0xC9, 0xED,
	0xB8, 0x16, 0x03, 0x31, 0x95, 0xA6, 0xC1, 0xE1, 0x3B, 0xF1, 0xD9, 0x12, 0x13, 0xD9, 0x29, 0x10,
	0x02, 0xE1, 0x41, 0x38, 0xF3, 0x1E, 0x01, 0x3E, 0x80, 0x29, 0x10, 0x02, 0xE1, 0x41, 0x17, 0x38,
	0xF8, 0xBA, 0x38, 0x05, 0x83, 0x5F, 0xA9, 0x20, 0xEE, 0x83, 0xBA, 0x3F, 0x30, 0x09, 0x9A, 0x28,
	0x28, 0x8A, 0x29, 0x10, 0x02, 0xE1, 0x41, 0xD9, 0x4F, 0x26, 0xFF, 0x3D, 0x3E, 0x3F, 0x28, 0x04,
	0x30, 0x0F, 0xCB, 0xAF, 0xD9, 0x29, 0x10, 0x02, 0xE1, 0x41, 0x17, 0x30, 0xF8, 0xD9, 0x28, 0x03,
	0x67, 0x3B, 0xF1, 0x6F, 0x19, 0xED, 0xB0, 0x18, 0xB4, 0x3B, 0xF1, 0xA7, 0x20, 0xD4, 0xD9, 0x21,
	0xFB, 0x5B, 0x0E, 0x05, 0xED, 0xB0, 0x21, 0x58, 0x27, 0xD9, 0x31, 0x00, 0x00, 0xFB, 0xC9, 0x00,
	0x00};

void dehrum_put_byte(int offs, char byte_)
{
  hrum_depacker[offs] = byte_;
}

void dehrum_put_word(int offs, int word_)
{
  hrum_depacker[offs] = word_ & 0xff;
  hrum_depacker[offs+1] = word_ >> 8;
}




/*
  start		- start adr of packed block (with depacker)
  depackto	- depack to adr
  dadr		- depacker adr
  sp		- new sp (-1==keep old sp)
  ei_inter	- enable int (1==ei, 0==di)
  jp		- jp adr (-1==ret)
  packed_len	- packed length -5 last bytes (it's stored in depacker)
  unpacked_len	- unpacked length -5 last bytes
*/

void dehrum_patch(int start_, int depackto_, int dadr_, int sp_, char ei_inter_, int jp_, int packed_len_, int unpacked_len_)
{
  int pk_st,pk_end,unp_st,unp_end;

  pk_st=start_+0x91+5;			//start of packed block
  pk_end=pk_st+packed_len_-1;		//last byte of packed block
  unp_st=depackto_;			//start of unpacked block
  unp_end=unp_st+unpacked_len_-1+5;	//last byte of unpacked block

  //SP
  if (sp_<0)
  {
    dehrum_put_word(0x03, start_+0x8b);		//no new sp, store old
  }
  else
  {
    dehrum_put_word(0x03, 0);			//new sp, don't store old
    dehrum_put_word(0x8b, sp_);			//new sp
  }

  dehrum_put_word(0x06, start_+0x1f);		//depacker offset

  dehrum_put_word(0x09, dadr_);			//depacker adr

  dehrum_put_word(0x12, depackto_);		//depack to adr


  if (pk_end<=unp_end)
  {
    dehrum_put_word(0x16, pk_end);		//end of packed block
    dehrum_put_word(0x19, unp_end);		//end of unpacked block
    //dehrum_put_byte(0x20, 0xb8);		//lddr
  }
  else
  {
    dehrum_put_word(0x16, pk_st);		//start of packed block
    dehrum_put_word(0x19, unp_end-packed_len_+1);//start of moved packed block
    dehrum_put_byte(0x20, 0xb0);		//ldir
  }

  dehrum_put_word(0x24, unp_end-packed_len_+1);	//start of moved block

  dehrum_put_word(0x1c, packed_len_);		//packed block length

  dehrum_put_word(0x80, dadr_+0x72);		//adr of data after depacker

  dehrum_put_byte(0x8d, (ei_inter_) ? 0xfb : 0xf3);	//interrupt

  if (jp_>=0)
  {
    dehrum_put_byte(0x8e, 0xc3);		//jp
    dehrum_put_word(0x8f, jp_);			//adr
  }
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







unsigned char bufIn[49152];
unsigned char bufOut[49152];

int main(int argc, char *argv[])
{
    int i, opt;
    char *nameIn, *nameOut, *p;
    FILE *pfi, *pfo;
    int sizeIn, sizeOut;

    int start, depackto, dadr, sp, jp;
    char ei_inter;

     start=0x6000;
     depackto=0x6000;
     dadr=0x5b89;
     sp=-1;
     jp=-1;
     ei_inter=1;

    nameIn = NULL;
    nameOut = NULL;
    opt = 0;
    for (i = 1; i < argc; i++)
    {
        p = argv[i];
        if (*p == '-')
        {
            if (strcmp(p, "-opt") == 0) opt = 1;
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
         start<0 || start>0xffff ||
         depackto<0 || depackto>0xffff ||
         dadr<0 || dadr>0xffff ||
         sp>0xffff || jp>0xffff)

    {
        set_color(clr_info);
        printf("\nHrum 3.5i, version " HRUM_PC_VERSION "\n");
        set_color(clr_default);
        printf("Original packer by Hrumer, PC-version by Nikita Burnashev,\n"
               "depacker added by psb.\n\n");
        set_color(clr_info);
        printf("Usage: hrum35 [-opt] [-ei | -di] [-start <addr>]\n"
               "              [-depackto <addr>] [-dadr <addr>] [-sp <addr>] [-jp <addr>]\n"
               "              <input.bin> <output.bin>\n\n");
        set_color(clr_info_);
        printf("-opt      set optimisation (rudiment.) (no)\n"
               "-ei       enable int on exit (yes)\n"
               "-di       disable int on exit (no)\n"
               "-start    set start address of packed block (with depacker) (#6000)\n"
               "-depackto set depack address (#6000)\n"
               "-dadr     set address of depacker (#5B89)\n"
               "-sp       set new SP on exit (no)\n"
               "-jp       set address of jump to (no)\n\n");
        set_color(clr_default);
        return 1;
    }

    pfi = fopen(nameIn, "rb");
    if (!pfi)
    {
        set_color(clr_warning);
        printf("Hrum 3.5i: Can't open '%s' for reading.\n", nameIn);
        set_color(clr_default);
        return 2;
    }

    sizeIn = fread(bufIn, 1, sizeof(bufIn), pfi);
    fclose(pfi);

    if (sizeIn>49033)
    {
        set_color(clr_warning);
        printf("Hrum 3.5i: Input file '%s' too big!\n", nameIn);
        set_color(clr_default);
        return 3;
    }

    if (sizeIn<7)
    {
        set_color(clr_warning);
        printf("Hrum 3.5i: Input file '%s' too short!\n", nameIn);
        set_color(clr_default);
        return 3;
    }


    pfo = fopen(nameOut, "wb");
    if (!pfo)
    {
        set_color(clr_warning);
        printf("Hrum 3.5i: Can't open '%s' for writting.\n", nameOut);
        set_color(clr_default);
        return 2;
    }

    /* Hrum stores last 5 bytes unpacked */
    sizeIn -= 5;

    memset(tail, 0, sizeof(tail));
    bufOut[0]=0x10; bufOut[1]=0x10;
    sizeOut = compress(bufIn, bufIn + sizeIn, bufOut+2, opt)+2;

    dehrum_patch(start, depackto, dadr, sp, ei_inter, jp, sizeOut, sizeIn);
    fwrite(hrum_depacker, 1, sizeof(hrum_depacker), pfo);

    fwrite(bufIn+sizeIn, 1, 5, pfo);
    fwrite(bufOut, 1, sizeOut, pfo);
    fclose(pfo);


    set_color(clr_done);
    printf("Hrum 3.5i: File '%s' packed.\tRatio: %3.2f%%.\n",
           nameIn,
           (1.0-(float)(sizeOut+5+sizeof(hrum_depacker))/(float)(sizeIn+5))*100.0 );
    set_color(clr_default);

    return 0;
}





