

/* File library functions */
#if 0
TM_CALLABLE
static int
create(char *file, int rw)
{
	int f;

	f = txc_creat(file, 0666);
	if (rw && f>=0) {
		close(f);
		f = open(file, 2);
	}
	return(f);
}

TM_CALLABLE
FILE *
_txc_endopen(char *file, char *mode, FILE *iop)
{
	extern int errno;
	int rw, f;

	if (iop == NULL)
		return(NULL);

	rw = mode[1] == '+';

	switch (*mode) {

	case 'w':
		f = create(file, rw);
		break;

	case 'a':
		if ((f = open(file, rw? 2: 1)) < 0) {
			if (errno == ENOENT)
				f = create(file, rw);
		}
		lseek(f, 0L, 2);
		break;

	case 'r':
		printf("file: %s\n",file);
		f = open(file, rw? 2: 0);
		break;

	default:
		return(NULL);
	}

	if (f < 0)
		return(NULL);

	//iop->_cnt = 0;
	(int) (iop->_IO_buf_end) = 0;

	iop->_fileno = f;

	if (rw)
		iop->_flags |= _IORW;
	else if(*mode == 'r')
		iop->_flags |= _IOREAD;
	else
		iop->_flags |= _IOWRT;

	return(iop);
}

/*
FILE *
_findiop()
{
	extern FILE *_lastbuf;
	FILE *iop;

	for(iop = _iob; iop->_flags & (_IOREAD|_IOWRT|_IORW); iop++)
		if (iop >= _lastbuf)
			return(NULL);

	return(iop);
}
*/

TM_CALLABLE
FILE *
txc_fopen(char *file, char *mode)
{
	//FILE *_findiop(), *_txc_endopen();
	FILE  *iop;
	iop = (FILE *)malloc(sizeof(FILE));
	//return(_txc_endopen(file, mode, _findiop()));
	return(_txc_endopen(file, mode, iop));
}

TM_CALLABLE
int
_txc_filbuf(FILE *iop)
{
	static char smallbuf[_NFILE];

	if (iop->_flags & _IORW)
		iop->_flags |= _IOREAD;

	if ((iop->_flags & _IOREAD) == 0 || iop->_flags & _IOSTRG)
		return(EOF);

tryagain:
	if (iop->_IO_read_base == NULL) {
		if (iop->_flags & _IONBF) {
			iop->_IO_read_base = &smallbuf[fileno(iop)];
			goto tryagain;
		}
		if ((iop->_IO_read_base = malloc(BUFSIZ)) == NULL) {
			iop->_flags |= _IONBF;
			goto tryagain;
		}
		iop->_flags |= _IOMYBUF;
	}
	iop->_IO_read_ptr = iop->_IO_read_base;
	((int) iop->_IO_buf_end) = read(fileno(iop), iop->_IO_read_ptr, iop->_flags&_IONBF?1:BUFSIZ);
	if (--((int) iop->_IO_buf_end) < 0) {
		if (((int) iop->_IO_buf_end) == -1) {
			iop->_flags |= _IOEOF;
			if (iop->_flags & _IORW)
				iop->_flags &= ~_IOREAD;
		} else
			iop->_flags |= _IOERR;
		((int) iop->_IO_buf_end) = 0;
		return(EOF);
	}
	return(*iop->_IO_read_ptr++ & 0377);
}

#define	txc_getc(p)		(--((int) (p)->_IO_buf_end)>=0? *(p)->_IO_read_ptr++&0377:_txc_filbuf(p))

TM_CALLABLE
int
_txc_flsbuf(int c, FILE *iop)
{
	char *base;
	int n, rn;
	char c1;
	extern char _sobuf[];

	if (iop->_flags & _IORW) {
		iop->_flags |= _IOWRT;
		iop->_flags &= ~_IOEOF;
	}

tryagain:
	if (iop->_flags & _IONBF) {
		c1 = c;
		rn = 1;
		n = txc_write(fileno(iop), &c1, rn, 2);
		((int) iop->_IO_buf_end) = 0;
	} else {
		if ((base = iop->_IO_read_base) == NULL) {
		/*		
			if (iop == stdout) {
				if (isatty(fileno(stdout))) {
					iop->_flag |= _IONBF;
					goto tryagain;
				}
				iop->_base = _sobuf;
				iop->_ptr = _sobuf;
				goto tryagain;
			}
		*/
			if ((iop->_IO_read_base = base = malloc(BUFSIZ)) == NULL) {
				iop->_flags |= _IONBF;
				goto tryagain;
			}
			iop->_flags |= _IOMYBUF;
			rn = n = 0;
		} else if((rn = n = iop->_IO_read_ptr - base) > 0) {
			iop->_IO_read_ptr = base;
			n = txc_write(fileno(iop), base, n, 2);
		}
		((int) iop->_IO_buf_end) = BUFSIZ - 1;
		*base++ = c;
		iop->_IO_read_ptr = base;
	}
	if (rn != n) {
		iop->_flags |= _IOERR;
		return(EOF);
	}
	return(c);
}

#define txc_putc(x,p) (--(p)->_IO_buf_end>=0? ((int)(*(p)->_IO_read_ptr++=(unsigned)(x))):_txc_flsbuf((unsigned)(x),p))

TM_CALLABLE
char *
txc_fgets(char *s, int n, FILE *iop)
{
	int c;
	char *cs;

	cs = s;
	while (--n>0 && (c = txc_getc(iop))>=0) {
		*cs++ = c;
		if (c=='\n')
			break;
	}
	if (c<0 && cs==s)
		return(NULL);
	*cs++ = '\0';
	return(s);
}


TM_CALLABLE
size_t
txc_fread(char *ptr, unsigned int size, unsigned int count, FILE *iop)
{
	register c;
	unsigned ndone, s;

	ndone = 0;
	if (size)
		for (; ndone<count; ndone++) {
			s = size;
			do {
				if ((c = txc_getc(iop)) >= 0)
					*ptr++ = c;
				else
					return(ndone);
			} while (--s);
	 	}
	return(ndone);
}

TM_CALLABLE
size_t
txc_fwrite(char *ptr, unsigned int size, unsigned int count, FILE *iop)
{
	unsigned s;
	unsigned ndone;

	ndone = 0;
	if (size)
	for (; ndone<count; ndone++) {
		s = size;
		do {
			txc_putc(*ptr++, iop);
		} while (--s);
		if (ferror(iop))
			break;
	}
	return(ndone);
}

TM_CALLABLE
int 
txc_fclose(FILE *stream) {
	__close(stream->_fileno);
	free(stream);
	return 0;
}
#endif


