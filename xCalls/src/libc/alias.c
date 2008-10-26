#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <byteswap.h>
#include <txc/int/memcopy.h>
#include <txc/int/pagecopy.h>
#include <txc/libc.h>

#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <assert.h>
#include <ia32intrin.h>


# if __BYTE_ORDER == __BIG_ENDIAN
#  define WORDS_BIGENDIAN
# endif

#ifdef WORDS_BIGENDIAN
# define CMP_LT_OR_GT(a, b) ((a) > (b) ? 1 : -1)
#else
# define CMP_LT_OR_GT(a, b) memcmp_bytes ((a), (b))
#endif


//#define LITTLE_ENDIAN 1234
//#define BIG_ENDIAN    4321
//#define BYTE_ORDER LITTLE_ENDIAN //i386 is little endian

//typedef unsigned char byte;


# define __set_errno(Val) errno = (Val)

int dummy;

/*
FIXME: When this is not commented we get an error
TM_CALLABLE
uint32_t
htonl (uint32_t x)
{
#if BYTE_ORDER == BIG_ENDIAN
  	return x;
#elif BYTE_ORDER == LITTLE_ENDIAN
		return __bswap_32 (x);
#endif
}
*/

#if 0
TM_WAIVER
void 
txc_va_start(va_list ap, const char* format) {
	va_start(ap, format);
}

TM_WAIVER
void 
txc_va_end(va_list ap) {
	va_end(ap);
}
#endif
	
TM_WAIVER
int*
txc_alias_errno_location(void) {
	return __errno_location();
}

#define txc__errno_location __errno_location

TM_WAIVER
void 
txc_abort(void) {
	abort();
}

#if 0
TM_CALLABLE
char *
txc_alias_catgets (nl_catd catalog_desc, int set, int message, const char *string)
{
  __nl_catd catalog;
  size_t idx;
  size_t cnt;

  /* Be generous if catalog which failed to be open is used.  */
  if (catalog_desc == (nl_catd) -1 || ++set <= 0 || message < 0)
    return (char *) string;

  catalog = (__nl_catd) catalog_desc;

  idx = ((set * message) % catalog->plane_size) * 3;
  cnt = 0;
  do
    {
      if (catalog->name_ptr[idx + 0] == (u_int32_t) set
	  && catalog->name_ptr[idx + 1] == (u_int32_t) message)
	return (char *) &catalog->strings[catalog->name_ptr[idx + 2]];

      idx += catalog->plane_size * 3;
    }
  while (++cnt < catalog->plane_depth);

  __set_errno (ENOMSG);
  return (char *) string;
}
#endif 

//Some dummy wrappers
TM_WAIVER
pid_t 
getpid(void)
{
	return __getpid();
}

TM_WAIVER
void
usleep(unsigned long usec)
{
	__nanosleep(1000*usec);
}

TM_WAIVER
int select(int n, fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
			struct timeval *timeout)
{
	return __select(n, readfds, writefds, exceptfds, timeout);
}

TM_CALLABLE
static enum charset parsecharset(const char* s) {
	if (!strcasecmp(s,"UTF-8")) return UTF_8; else
	if (!strcasecmp(s,"UCS-2") || !strcasecmp(s,"UCS2")) return UCS_2; else
	if (!strcasecmp(s,"UCS-4") || !strcasecmp(s,"UCS4")) return UCS_4; else
	if (!strcasecmp(s,"ISO-8859-1") || !strcasecmp(s,"LATIN1")) return ISO_8859_1; else
	if (!strcasecmp(s,"US-ASCII")) return ISO_8859_1; else
	if (!strcasecmp(s,"UTF-16")) return UTF_16; else
	if (!strcasecmp(s,"UTF-16BE")) return UTF_16_BE; else
	if (!strcasecmp(s,"UTF-16LE")) return UTF_16_LE; else
		return INVALID;
}

TM_CALLABLE
iconv_t iconv_open(const char* tocode, const char* fromcode) {
	int f,t;

	f=parsecharset(fromcode);
	t=parsecharset(tocode);

	if (f==INVALID || t==INVALID) {
		errno=EINVAL;
		return (iconv_t)(-1);
	}
	return (f|t<<16);
}

TM_CALLABLE
int iconv_close(iconv_t cd) {
	(void)cd; /* shut gcc up about unused cd */
	return 0;
}

TM_CALLABLE
void* memchr(const void *s, int c, size_t n) {
	const unsigned char *pc = (unsigned char *) s;
	for (;n--;pc++)
		if (*pc == c)
			return ((void *) pc);
	return 0;
}

TM_CALLABLE
int 
stm_isdigit (int ch){
    return (unsigned int)(ch - '0') < 10u;
}

TM_CALLABLE
int
atoi(char *p)
{
	int n, f;

	n = 0;
	f = 0;
	for(;;p++) {
		switch(*p) {
			case ' ':
			case '\t':
				continue;
			case '-':
				f++;
			case '+':
				p++;
		}
		break;
	}
	while(*p >= '0' && *p <= '9')
		n = n*10 + *p++ - '0';
	return(f? -n: n);
}

TM_CALLABLE
double
atof(char *p)
{
	register c;
	double fl, flexp, exp5;
	double big = 72057594037927936.;  /*2^56*/
	double ldexp();
	int nd;
	register eexp, exp, neg, negexp, bexp;

	neg = 1;
	while((c = *p++) == ' ')
		;
	if (c == '-')
		neg = -1;
	else if (c=='+')
		;
	else
		--p;

	exp = 0;
	fl = 0;
	nd = 0;
	while ((c = *p++), stm_isdigit(c)) {
		if (fl<big)
			fl = 10*fl + (c-'0');
		else
			exp++;
		nd++;
	}

	if (c == '.') {
		while ((c = *p++), stm_isdigit(c)) {
			if (fl<big) {
				fl = 10*fl + (c-'0');
				exp--;
			}
		nd++;
		}
	}

	negexp = 1;
	eexp = 0;
	if ((c == 'E') || (c == 'e')) {
		if ((c= *p++) == '+')
			;
		else if (c=='-')
			negexp = -1;
		else
			--p;

		while ((c = *p++), stm_isdigit(c)) {
			eexp = 10*eexp+(c-'0');
		}
		if (negexp<0)
			eexp = -eexp;
		exp = exp + eexp;
	}

	negexp = 1;
	if (exp<0) {
		negexp = -1;
		exp = -exp;
	}


	if((nd+exp*negexp) < -LOGHUGE){
		fl = 0;
		exp = 0;
	}
	flexp = 1;
	exp5 = 5;
	bexp = exp;
	for (;;) {
		if (exp&01)
			flexp *= exp5;
		exp >>= 1;
		if (exp==0)
			break;
		exp5 *= exp5;
	}
	if (negexp<0)
		fl /= flexp;
	else
		fl *= flexp;
	fl = ldexp(fl, negexp*bexp);
	if (neg<0)
		fl = -fl;
	return(fl);
}

TM_CALLABLE
void *
memset(void* dstpp, int c, size_t len)
{
  long int dstp = (long int) dstpp;

  if (len >= 8)
    {
      size_t xlen;
      op_t cccc;

      cccc = (unsigned char) c;
      cccc |= cccc << 8;
      cccc |= cccc << 16;
      if (OPSIZ > 4)
	/* Do the shift in two steps to avoid warning if long has 32 bits.  */
	cccc |= (cccc << 16) << 16;

      /* There are at least some bytes to set.
	 No need to test for LEN == 0 in this alignment loop.  */
      while (dstp % OPSIZ != 0)
	{
	  ((byte *) dstp)[0] = c;
	  dstp += 1;
	  len -= 1;
	}

      /* Write 8 `op_t' per iteration until less than 8 `op_t' remain.  */
      xlen = len / (OPSIZ * 8);
      while (xlen > 0)
	{
	  ((op_t *) dstp)[0] = cccc;
	  ((op_t *) dstp)[1] = cccc;
	  ((op_t *) dstp)[2] = cccc;
	  ((op_t *) dstp)[3] = cccc;
	  ((op_t *) dstp)[4] = cccc;
	  ((op_t *) dstp)[5] = cccc;
	  ((op_t *) dstp)[6] = cccc;
	  ((op_t *) dstp)[7] = cccc;
	  dstp += 8 * OPSIZ;
	  xlen -= 1;
	}
      len %= OPSIZ * 8;

      /* Write 1 `op_t' per iteration until less than OPSIZ bytes remain.  */
      xlen = len / OPSIZ;
      while (xlen > 0)
	{
	  ((op_t *) dstp)[0] = cccc;
	  dstp += OPSIZ;
	  xlen -= 1;
	}
      len %= OPSIZ;
    }

  /* Write the last few bytes.  */
  while (len > 0)
    {
      ((byte *) dstp)[0] = c;
      dstp += 1;
      len -= 1;
    }

  return dstpp;
}

TM_CALLABLE
void *
memcpy(void* dstpp, const void* srcpp, size_t len)
{
  unsigned long int dstp = (long int) dstpp;
  unsigned long int srcp = (long int) srcpp;

  /* Copy from the beginning to the end.  */

  /* If there not too few bytes to copy, use word copy.  */
  if (len >= OP_T_THRES)
    {
      /* Copy just a few bytes to make DSTP aligned.  */
      len -= (-dstp) % OPSIZ;
      BYTE_COPY_FWD (dstp, srcp, (-dstp) % OPSIZ);

      /* Copy whole pages from SRCP to DSTP by virtual address manipulation,
	 as much as possible.  */

      PAGE_COPY_FWD_MAYBE (dstp, srcp, len, len);

      /* Copy from SRCP to DSTP taking advantage of the known alignment of
	 DSTP.  Number of bytes remaining is put in the third argument,
	 i.e. in LEN.  This number may vary from machine to machine.  */

      WORD_COPY_FWD (dstp, srcp, len, len);

      /* Fall out and copy the tail.  */
    }

  /* There are just a few bytes to copy.  Use byte memory operations.  */
  BYTE_COPY_FWD (dstp, srcp, len);

  return dstpp;
}

#ifndef WORDS_BIGENDIAN
/* memcmp_bytes -- Compare A and B bytewise in the byte order of the machine.
   A and B are known to be different.
   This is needed only on little-endian machines.  */

//static int memcmp_bytes (op_t, op_t) __THROW;

TM_CALLABLE
static int
memcmp_bytes (a, b)
     op_t a, b;
{
  long int srcp1 = (long int) &a;
  long int srcp2 = (long int) &b;
  op_t a0, b0;

  do
    {
      a0 = ((byte *) srcp1)[0];
      b0 = ((byte *) srcp2)[0];
      srcp1 += 1;
      srcp2 += 1;
    }
  while (a0 == b0);
  return a0 - b0;
}
#endif

//static int memcmp_common_alignment (long, long, size_t) __THROW;

/* memcmp_common_alignment -- Compare blocks at SRCP1 and SRCP2 with LEN `op_t'
   objects (not LEN bytes!).  Both SRCP1 and SRCP2 should be aligned for
   memory operations on `op_t's.  */
TM_CALLABLE
static int
memcmp_common_alignment (srcp1, srcp2, len)
     long int srcp1;
     long int srcp2;
     size_t len;
{
  op_t a0, a1;
  op_t b0, b1;

  switch (len % 4)
    {
    default: /* Avoid warning about uninitialized local variables.  */
    case 2:
      a0 = ((op_t *) srcp1)[0];
      b0 = ((op_t *) srcp2)[0];
      srcp1 -= 2 * OPSIZ;
      srcp2 -= 2 * OPSIZ;
      len += 2;
      goto do1;
    case 3:
      a1 = ((op_t *) srcp1)[0];
      b1 = ((op_t *) srcp2)[0];
      srcp1 -= OPSIZ;
      srcp2 -= OPSIZ;
      len += 1;
      goto do2;
    case 0:
      if (OP_T_THRES <= 3 * OPSIZ && len == 0)
	return 0;
      a0 = ((op_t *) srcp1)[0];
      b0 = ((op_t *) srcp2)[0];
      goto do3;
    case 1:
      a1 = ((op_t *) srcp1)[0];
      b1 = ((op_t *) srcp2)[0];
      srcp1 += OPSIZ;
      srcp2 += OPSIZ;
      len -= 1;
      if (OP_T_THRES <= 3 * OPSIZ && len == 0)
	goto do0;
      /* Fall through.  */
    }

  do
    {
      a0 = ((op_t *) srcp1)[0];
      b0 = ((op_t *) srcp2)[0];
      if (a1 != b1)
	return CMP_LT_OR_GT (a1, b1);

    do3:
      a1 = ((op_t *) srcp1)[1];
      b1 = ((op_t *) srcp2)[1];
      if (a0 != b0)
	return CMP_LT_OR_GT (a0, b0);

    do2:
      a0 = ((op_t *) srcp1)[2];
      b0 = ((op_t *) srcp2)[2];
      if (a1 != b1)
	return CMP_LT_OR_GT (a1, b1);

    do1:
      a1 = ((op_t *) srcp1)[3];
      b1 = ((op_t *) srcp2)[3];
      if (a0 != b0)
	return CMP_LT_OR_GT (a0, b0);

      srcp1 += 4 * OPSIZ;
      srcp2 += 4 * OPSIZ;
      len -= 4;
    }
  while (len != 0);

  /* This is the right position for do0.  Please don't move
     it into the loop.  */
 do0:
  if (a1 != b1)
    return CMP_LT_OR_GT (a1, b1);
  return 0;
}

//static int memcmp_not_common_alignment (long, long, size_t) __THROW;

/* memcmp_not_common_alignment -- Compare blocks at SRCP1 and SRCP2 with LEN
   `op_t' objects (not LEN bytes!).  SRCP2 should be aligned for memory
   operations on `op_t', but SRCP1 *should be unaligned*.  */
TM_CALLABLE
static int
memcmp_not_common_alignment (srcp1, srcp2, len)
     long int srcp1;
     long int srcp2;
     size_t len;
{
  op_t a0, a1, a2, a3;
  op_t b0, b1, b2, b3;
  op_t x;
  int shl, shr;

  /* Calculate how to shift a word read at the memory operation
     aligned srcp1 to make it aligned for comparison.  */

  shl = 8 * (srcp1 % OPSIZ);
  shr = 8 * OPSIZ - shl;

  /* Make SRCP1 aligned by rounding it down to the beginning of the `op_t'
     it points in the middle of.  */
  srcp1 &= -OPSIZ;

  switch (len % 4)
    {
    default: /* Avoid warning about uninitialized local variables.  */
    case 2:
      a1 = ((op_t *) srcp1)[0];
      a2 = ((op_t *) srcp1)[1];
      b2 = ((op_t *) srcp2)[0];
      srcp1 -= 1 * OPSIZ;
      srcp2 -= 2 * OPSIZ;
      len += 2;
      goto do1;
    case 3:
      a0 = ((op_t *) srcp1)[0];
      a1 = ((op_t *) srcp1)[1];
      b1 = ((op_t *) srcp2)[0];
      srcp2 -= 1 * OPSIZ;
      len += 1;
      goto do2;
    case 0:
      if (OP_T_THRES <= 3 * OPSIZ && len == 0)
	return 0;
      a3 = ((op_t *) srcp1)[0];
      a0 = ((op_t *) srcp1)[1];
      b0 = ((op_t *) srcp2)[0];
      srcp1 += 1 * OPSIZ;
      goto do3;
    case 1:
      a2 = ((op_t *) srcp1)[0];
      a3 = ((op_t *) srcp1)[1];
      b3 = ((op_t *) srcp2)[0];
      srcp1 += 2 * OPSIZ;
      srcp2 += 1 * OPSIZ;
      len -= 1;
      if (OP_T_THRES <= 3 * OPSIZ && len == 0)
	goto do0;
      /* Fall through.  */
    }

  do
    {
      a0 = ((op_t *) srcp1)[0];
      b0 = ((op_t *) srcp2)[0];
      x = MERGE(a2, shl, a3, shr);
      if (x != b3)
	return CMP_LT_OR_GT (x, b3);

    do3:
      a1 = ((op_t *) srcp1)[1];
      b1 = ((op_t *) srcp2)[1];
      x = MERGE(a3, shl, a0, shr);
      if (x != b0)
	return CMP_LT_OR_GT (x, b0);

    do2:
      a2 = ((op_t *) srcp1)[2];
      b2 = ((op_t *) srcp2)[2];
      x = MERGE(a0, shl, a1, shr);
      if (x != b1)
	return CMP_LT_OR_GT (x, b1);

    do1:
      a3 = ((op_t *) srcp1)[3];
      b3 = ((op_t *) srcp2)[3];
      x = MERGE(a1, shl, a2, shr);
      if (x != b2)
	return CMP_LT_OR_GT (x, b2);

      srcp1 += 4 * OPSIZ;
      srcp2 += 4 * OPSIZ;
      len -= 4;
    }
  while (len != 0);

  /* This is the right position for do0.  Please don't move
     it into the loop.  */
 do0:
  x = MERGE(a2, shl, a3, shr);
  if (x != b3)
    return CMP_LT_OR_GT (x, b3);
  return 0;
}

TM_CALLABLE
int
memcmp (const __ptr_t s1, const __ptr_t s2, size_t len)
{
  unsigned long int a0;
  unsigned long int b0;
  long int srcp1 = (long int) s1;
  long int srcp2 = (long int) s2;
  unsigned long int res;

  if (len >= OP_T_THRES)
    {
      /* There are at least some bytes to compare.  No need to test
	 for LEN == 0 in this alignment loop.  */
      while (srcp2 % OPSIZ != 0)
	{
	  a0 = ((unsigned char *) srcp1)[0];
	  b0 = ((unsigned char *) srcp2)[0];
	  srcp1 += 1;
	  srcp2 += 1;
	  res = a0 - b0;
	  if (res != 0)
	    return res;
	  len -= 1;
	}

      /* SRCP2 is now aligned for memory operations on `op_t'.
	 SRCP1 alignment determines if we can do a simple,
	 aligned compare or need to shuffle bits.  */

      if (srcp1 % OPSIZ == 0)
	res = memcmp_common_alignment (srcp1, srcp2, len / OPSIZ);
      else
	res = memcmp_not_common_alignment (srcp1, srcp2, len / OPSIZ);
      if (res != 0)
	return res;

      /* Number of bytes remaining in the interval [0..OPSIZ-1].  */
      srcp1 += len & -OPSIZ;
      srcp2 += len & -OPSIZ;
      len %= OPSIZ;
    }

  /* There are just a few bytes to compare.  Use byte memory operations.  */
  while (len != 0)
    {
      a0 = ((unsigned char *) srcp1)[0];
      b0 = ((unsigned char *) srcp2)[0];
      srcp1 += 1;
      srcp2 += 1;
      res = a0 - b0;
      if (res != 0)
	return res;
      len -= 1;
    }

  return 0;
}

TM_CALLABLE
void *
memmove (void* dest, const void* src, size_t len)
{
  unsigned long int dstp = (long int) dest;
  unsigned long int srcp = (long int) src;

  /* This test makes the forward copying code be used whenever possible.
     Reduces the working set.  */
  if (dstp - srcp >= len)	/* *Unsigned* compare!  */
    {
      /* Copy from the beginning to the end.  */

      /* If there not too few bytes to copy, use word copy.  */
      if (len >= OP_T_THRES)
	{
	  /* Copy just a few bytes to make DSTP aligned.  */
	  len -= (-dstp) % OPSIZ;
	  BYTE_COPY_FWD (dstp, srcp, (-dstp) % OPSIZ);

	  /* Copy whole pages from SRCP to DSTP by virtual address
	     manipulation, as much as possible.  */

	  PAGE_COPY_FWD_MAYBE (dstp, srcp, len, len);

	  /* Copy from SRCP to DSTP taking advantage of the known
	     alignment of DSTP.  Number of bytes remaining is put
	     in the third argument, i.e. in LEN.  This number may
	     vary from machine to machine.  */

	  WORD_COPY_FWD (dstp, srcp, len, len);

	  /* Fall out and copy the tail.  */
	}

      /* There are just a few bytes to copy.  Use byte memory operations.  */
      BYTE_COPY_FWD (dstp, srcp, len);
    }
  else
    {
      /* Copy from the end to the beginning.  */
      srcp += len;
      dstp += len;

      /* If there not too few bytes to copy, use word copy.  */
      if (len >= OP_T_THRES)
	{
	  /* Copy just a few bytes to make DSTP aligned.  */
	  len -= dstp % OPSIZ;
	  BYTE_COPY_BWD (dstp, srcp, dstp % OPSIZ);

	  /* Copy from SRCP to DSTP taking advantage of the known
	     alignment of DSTP.  Number of bytes remaining is put
	     in the third argument, i.e. in LEN.  This number may
	     vary from machine to machine.  */

	  WORD_COPY_BWD (dstp, srcp, len, len);

	  /* Fall out and copy the tail.  */
	}

      /* There are just a few bytes to copy.  Use byte memory operations.  */
      BYTE_COPY_BWD (dstp, srcp, len);
    }

  return(dest);
}

TM_WAIVER
char *txc_strerror(int errnum) {
	return strerror(errnum);
}


TM_WAIVER
int 
txc_rand(void)
{
	return rand();
}

TM_CALLABLE
size_t iconv(iconv_t cd_orig, char* * inbuf, size_t *
    inbytesleft, char* * outbuf, size_t * outbytesleft) {
  size_t result=0,i,j,k;
  int bits;
  unsigned char* in,* out;
  unsigned int cd = (unsigned int) cd_orig; 	

  enum charset from = ic_from(cd);
  enum charset to = ic_to(cd);

  if (!inbuf || !*inbuf) return 0;
  in=(unsigned char*)(*inbuf);
  out=(unsigned char*)(*outbuf);
  k=0;
  while (*inbytesleft) {
    unsigned int v;
    v=*in;
    i=j=1;
    switch (from) {
    case UCS_2:
      if (*inbytesleft<2) {
starve:
  errno=EINVAL;
  return (size_t)-1;
      }
      v=(((unsigned long)in[0])<<8) |
        ((unsigned long)in[1]);
      i=2;
      break;
    case UCS_4:
      if (*inbytesleft<4) goto starve;
      v=(((unsigned long)in[0])<<24) |
        (((unsigned long)in[1])<<16) |
        (((unsigned long)in[2])<<8) |
        ((unsigned long)in[3]);
      i=4;
    case ISO_8859_1:
      break;
    case UTF_8:
      if (!(v&0x80)) break;
      for (i=0xC0; i!=0xFC; i=(i>>1)+0x80)
  if ((v&((i>>1)|0x80))==i) {
    v&=~i;
    break;
  }
      for (i=1; (in[i]&0xc0)==0x80; ++i) {
  if (i>*inbytesleft) goto starve;
  v=(v<<6)|(in[i]&0x3f);
      }
/*      printf("got %u in %u bytes, buflen %u\n",v,i,*inbytesleft); */
      break;
    case UTF_16:
      if (*inbytesleft<2) goto starve;
      if (v==0xff && in[1]==0xfe) {
  from=UTF_16_LE; *inbytesleft-=2; in+=2; goto utf16le;
      } else if (v==0xfe && in[1]==0xff) {
  from=UTF_16_BE; *inbytesleft-=2; in+=2; goto utf16be;
      }
ABEND:
      errno=EILSEQ;
      return (size_t)-1;
    case UTF_16_BE:
utf16be:
      if (*inbytesleft<2) goto starve;
      v=((unsigned long)in[0]<<8) | in[1];
joined:
      i=2;
      if (v>=0xd800 && v<=0xdfff) {
  long w;
  if (v>0xdbff) goto ABEND;
  if (*inbytesleft<4) goto starve;
  if (from==UTF_16_BE)
    w=((unsigned long)in[2]<<8) | in[3];
  else
    w=((unsigned long)in[3]<<8) | in[2];
  if (w<0xdc00 || w>0xdfff) goto ABEND;
  v=0x10000+(((v-0xd800) << 10) | (w-0xdc00));
  i=4;
      }
      break;
    case UTF_16_LE:
utf16le:
      v=((unsigned long)in[1]<<8) | in[0];
      goto joined;
    }
    if (v>=0xd800 && v<=0xd8ff) goto ABEND; /* yuck!  in-band signalling! */
    switch (to) {
    case ISO_8859_1:
      if (*outbytesleft<1) goto bloat;
      if (v>0xff) ++result;
      *out=(unsigned char)v;
      break;
    case UCS_2:
      if (*outbytesleft<2) goto bloat;
      if (v>0xffff) ++result;
      out[0]=v>>8;
      out[1]=v&0xff;
      j=2;
      break;
    case UCS_4:
      if (*outbytesleft<4) goto bloat;
      out[0]=(v>>23)&0xff;
      out[1]=(v>>16)&0xff;
      out[2]=(v>>8)&0xff;
      out[3]=v&0xff;
      j=4;
      break;
    case UTF_8:
      if (v>=0x04000000) { bits=30; *out=0xFC; j=6; } else
      if (v>=0x00200000) { bits=24; *out=0xF8; j=5; } else
      if (v>=0x00010000) { bits=18; *out=0xF0; j=4; } else
      if (v>=0x00000800) { bits=12; *out=0xE0; j=3; } else
      if (v>=0x00000080) { bits=6; *out=0xC0; j=2; } else
      { bits=0; *out=0; }
      *out|= (unsigned char)(v>>bits);
      if (*outbytesleft<j) {
bloat:
  errno=E2BIG;
  return (size_t)-1;
      }
      for (k=1; k<j; ++k) {
  bits-=6;
  out[k]=0x80+((v>>bits)&0x3F);
      }
      break;
    case UTF_16:
      if (*outbytesleft<4) goto bloat;
      to=UTF_16_LE;
      out[0]=0xff;
      out[1]=0xfe;
      out+=2; *outbytesleft-=2;
    case UTF_16_LE:
      if (v>0xffff) {
  long a,b;
  if (*outbytesleft<(j=4)) goto bloat;
  v-=0x10000;
  if (v>0xfffff) result++;
  a=0xd800+(v>>10); b=0xdc00+(v&0x3ff);
  out[1]=a>>8;
  out[0]=a&0xff;
  out[3]=b>>8;
  out[2]=b&0xff;
      } else {
  if (*outbytesleft<(j=2)) goto bloat;
  out[1]=(v>>8)&0xff;
  out[0]=v&0xff;
      }
      break;
    case UTF_16_BE:
      if (v>0xffff) {
  long a,b;
  if (*outbytesleft<(j=4)) goto bloat;
  v-=0x10000;
  if (v>0xfffff) result++;
  a=0xd800+(v>>10); b=0xdc00+(v&0x3ff);
  out[0]=a>>8;
  out[1]=a&0xff;
  out[2]=b>>8;
  out[3]=b&0xff;
      } else {
  if (*outbytesleft<(j=2)) goto bloat;
  out[0]=(v>>8)&0xff;
  out[1]=v&0xff;
      }
      break;
    }
    in+=i; *inbytesleft-=i;
    out+=j; *outbytesleft-=j;
  }
  *inbuf=(char*)in; *outbuf=(char*)out;
  return result;
}
