#include <stdio.h>
#include <stdlib.h>
#include "smt.h"

#ifdef SMT_SS
#include "context.h"
#endif

#include "host.h"
#include "misc.h"
#include "target-risc/elf.h"
#include "loader.h"
#include "symbol.h"

/* #define PRINT_SYMS */

/* symbol database in no particular order */
struct sym_sym_t *sym_db = NULL;

/* all symbol sorted by address */
int sym_nsyms = 0;
struct sym_sym_t **sym_syms = NULL;

/* all symbols sorted by name */
struct sym_sym_t **sym_syms_by_name = NULL;

/* text symbols sorted by address */
int sym_ntextsyms = 0;
struct sym_sym_t **sym_textsyms = NULL;

/* text symbols sorted by name */
struct sym_sym_t **sym_textsyms_by_name = NULL;

/* data symbols sorted by address */
int sym_ndatasyms = 0;
struct sym_sym_t **sym_datasyms = NULL;

/* data symbols sorted by name */
struct sym_sym_t **sym_datasyms_by_name = NULL;

/* symbols loaded? */
static int syms_loaded = FALSE;

#ifdef PRINT_SYMS
/* convert BFD symbols flags to a printable string */
static char *			/* symbol flags string */
flags2str(unsigned int flags)	/* bfd symbol flags */
{
  static char buf[256];
  char *p;

  if (!flags)
    return "";

  p = buf;
  *p = '\0';

  if (flags & BSF_LOCAL)
    {
      *p++ = 'L';
      *p++ = '|';
    }
  if (flags & BSF_GLOBAL)
    {
      *p++ = 'G';
      *p++ = '|';
    }
  if (flags & BSF_DEBUGGING)
    {
      *p++ = 'D';
      *p++ = '|';
    }
  if (flags & BSF_FUNCTION)
    {
      *p++ = 'F';
      *p++ = '|';
    }
  if (flags & BSF_KEEP)
    {
      *p++ = 'K';
      *p++ = '|';
    }
  if (flags & BSF_KEEP_G)
    {
      *p++ = 'k'; *p++ = '|';
    }
  if (flags & BSF_WEAK)
    {
      *p++ = 'W';
      *p++ = '|';
    }
  if (flags & BSF_SECTION_SYM)
    {
      *p++ = 'S'; *p++ = '|';
    }
  if (flags & BSF_OLD_COMMON)
    {
      *p++ = 'O';
      *p++ = '|';
    }
  if (flags & BSF_NOT_AT_END)
    {
      *p++ = 'N';
      *p++ = '|';
    }
  if (flags & BSF_CONSTRUCTOR)
    {
      *p++ = 'C';
      *p++ = '|';
    }
  if (flags & BSF_WARNING)
    {
      *p++ = 'w';
      *p++ = '|';
    }
  if (flags & BSF_INDIRECT)
    {
      *p++ = 'I';
      *p++ = '|';
    }
  if (flags & BSF_FILE)
    {
      *p++ = 'f';
      *p++ = '|';
    }

  if (p == buf)
    panic("no flags detected");

  *--p = '\0';
  return buf;
}
#endif /* PRINT_SYMS */

/* qsort helper function */
static int
acmp(struct sym_sym_t **sym1, struct sym_sym_t **sym2)
{
  return (int)((*sym1)->addr - (*sym2)->addr);
}

/* qsort helper function */
static int
ncmp(struct sym_sym_t **sym1, struct sym_sym_t **sym2)
{
  return strcmp((*sym1)->name, (*sym2)->name);
}

#define RELEVANT_SCOPE(SYM)						\
(/* global symbol */							\
 ((SYM)->flags & BSF_GLOBAL)						\
 || (/* local symbol */							\
     (((SYM)->flags & (BSF_LOCAL|BSF_DEBUGGING)) == BSF_LOCAL)		\
     && (SYM)->name[0] != '$')						\
 || (/* compiler local */						\
     load_locals							\
     && ((/* basic block idents */					\
	  ((SYM)->flags&(BSF_LOCAL|BSF_DEBUGGING))==(BSF_LOCAL|BSF_DEBUGGING)\
	  && (SYM)->name[0] == '$')					\
	 || (/* local constant idents */				\
	     ((SYM)->flags & (BSF_LOCAL|BSF_DEBUGGING)) == (BSF_LOCAL)	\
	     && (SYM)->name[0] == '$'))))
     

/* load symbols out of FNAME */
void
sym_loadsyms(char *fname,	/* file name containing symbols */
	     int load_locals, int threadid)	/* load local symbols */
{
	int i, debug_cnt;
	int len;
	FILE *fobj;
	// struct ecoff_filehdr fhdr;
	// struct ecoff_aouthdr ahdr;
	// struct ecoff_symhdr_t symhdr;
	Elf32_Ehdr *ehdr;
	Elf32_Phdr *phdr;
	Elf32_Shdr *shdr;
	Elf32_Sym  *symhdr;
	#ifdef SMT_SS
  context *current;
  current = thecontexts[threadid];
#endif

	ehdr=(Elf32_Ehdr *)malloc(sizeof(Elf32_Ehdr));
	phdr=(Elf32_Phdr *)malloc(sizeof(Elf32_Phdr));
	shdr=(Elf32_Shdr *)malloc(sizeof(Elf32_Shdr));
	
	char *strtab = NULL;
	char *shstrtab = NULL;
	// struct ecoff_EXTR *extr;
	if (syms_loaded)
    {
      /* symbols are already loaded */
      /* FIXME: can't handle symbols from multiple files */
      return;
	}
	/* load the program into memory, try both endians */
#if defined(__CYGWIN32__) || defined(_MSC_VER)
	fobj = fopen(fname, "rb");
#else
	fobj = fopen(fname, "r");
#endif
	if (!fobj)
		fatal("cannot open executable `%s'", fname);
	
	if (fread(ehdr, sizeof(Elf32_Ehdr), 1, fobj) < 1)
		fatal("cannot read header from executable `%s'", fname);

	for(i=1; i<ehdr->e_shnum; i++)
	{
		fseek(fobj, ehdr->e_shoff+ehdr->e_shentsize*i, 0);
		if(fread(shdr, sizeof(Elf32_Shdr), 1, fobj)<1)
			fatal("could not read section header");
		switch(shdr->sh_type)
		{
			case SHT_STRTAB:
			{
				if(i==ehdr->e_shstrndx)
				{
					shstrtab=(char *)malloc(shdr->sh_size);
					fseek(fobj, shdr->sh_offset, 0);
					fread(shstrtab, shdr->sh_size, 1, fobj);
					// printf("name=%d %s\n",shdr->sh_name, shstrtab+shdr->sh_name);
				}
				else
				{
					strtab=(char *)malloc(shdr->sh_size);
					fseek(fobj, shdr->sh_offset, 0);
					fread(strtab, shdr->sh_size, 1, fobj);
				}
			}
				break;
			case SHT_SYMTAB:
			{
				sym_nsyms=shdr->sh_size/shdr->sh_entsize;
				symhdr=(Elf32_Sym *)malloc(sizeof(Elf32_Sym)*sym_nsyms);
				fseek(fobj, shdr->sh_offset, 0);
				fread(symhdr, shdr->sh_size, 1, fobj);
			}
				break;
			default:
				break;
		}
		//printf("section%d sh_offset=%0x\n",i-1, shdr->sh_offset);
	}
	sym_db = (struct sym_sym_t *)calloc(sym_nsyms, sizeof(struct sym_sym_t));
	for(i=0; i<sym_nsyms; i++)
	{
		sym_db[i].name = mystrdup(&strtab[(symhdr+i)->st_name]); 
		//printf("st_info=%d name=%s\n",((symhdr+i)->st_info&0x0f), sym_db[i].name);
		if(((symhdr+i)->st_info&0x0f)==STT_FUNC)
		{
			sym_ntextsyms++;
			sym_db[i].seg = ss_text;         
		}
		else
		{
			sym_db[i].seg = ss_data;                        
			sym_ndatasyms++;
		}
		sym_db[i].initialized = TRUE;   
		sym_db[i].pub = TRUE;           
		sym_db[i].local = FALSE;        
		sym_db[i].addr = (symhdr+i)->st_value;            
	}
	if (fclose(fobj))
		fatal("could not close executable `%s'", fname);
	sym_syms =(struct sym_sym_t **)calloc(sym_nsyms, sizeof(struct sym_sym_t *));
	sym_syms_by_name =(struct sym_sym_t **)calloc(sym_nsyms, sizeof(struct sym_sym_t *));
	for (debug_cnt=0, i=0; i<sym_nsyms; i++)
    {
      sym_syms[debug_cnt] = &sym_db[i];
      sym_syms_by_name[debug_cnt] = &sym_db[i];
      debug_cnt++;
    }
	/* sort by address */
	qsort(sym_syms, sym_nsyms, sizeof(struct sym_sym_t *), (void *)acmp);

	/* sort by name */
	qsort(sym_syms_by_name, sym_nsyms, sizeof(struct sym_sym_t *), (void *)ncmp);
	 sym_textsyms =
    (struct sym_sym_t **)calloc(sym_ntextsyms, sizeof(struct sym_sym_t *));
  if (!sym_textsyms)
    fatal("out of virtual memory");

  sym_textsyms_by_name =
    (struct sym_sym_t **)calloc(sym_ntextsyms, sizeof(struct sym_sym_t *));
  if (!sym_textsyms_by_name)
    fatal("out of virtual memory");

  for (debug_cnt=0, i=0; i<sym_nsyms; i++)
    {
      if (sym_db[i].seg == ss_text)
	{
	  sym_textsyms[debug_cnt] = &sym_db[i];
	  sym_textsyms_by_name[debug_cnt] = &sym_db[i];
	  debug_cnt++;
	}
    }
  /* sanity check */
  if (debug_cnt != sym_ntextsyms)
    panic("could not locate all text symbols");

  /* sort by address */
  qsort(sym_textsyms, sym_ntextsyms, sizeof(struct sym_sym_t *), (void *)acmp);

  /* sort by name */
  qsort(sym_textsyms_by_name, sym_ntextsyms,
	sizeof(struct sym_sym_t *), (void *)ncmp);

  /* data segment sorted by address and name */
  sym_datasyms =
    (struct sym_sym_t **)calloc(sym_ndatasyms, sizeof(struct sym_sym_t *));
  if (!sym_datasyms)
    fatal("out of virtual memory");

  sym_datasyms_by_name =
    (struct sym_sym_t **)calloc(sym_ndatasyms, sizeof(struct sym_sym_t *));
  if (!sym_datasyms_by_name)
    fatal("out of virtual memory");

  for (debug_cnt=0, i=0; i<sym_nsyms; i++)
    {
      if (sym_db[i].seg == ss_data)
	{
	  sym_datasyms[debug_cnt] = &sym_db[i];
	  sym_datasyms_by_name[debug_cnt] = &sym_db[i];
	  debug_cnt++;
	}
    }
  /* sanity check */
  if (debug_cnt != sym_ndatasyms)
    panic("could not locate all data symbols");
      
  /* sort by address */
  qsort(sym_datasyms, sym_ndatasyms, sizeof(struct sym_sym_t *), (void *)acmp);

  /* sort by name */
  qsort(sym_datasyms_by_name, sym_ndatasyms,
	sizeof(struct sym_sym_t *), (void *)ncmp);

  /* compute symbol sizes */
#ifdef SMT_SS
  for (i=0; i<sym_ntextsyms; i++)
    {
      sym_textsyms[i]->size =
	(i != (sym_ntextsyms - 1)
	 ? (sym_textsyms[i+1]->addr - sym_textsyms[i]->addr)
	 : ((current->ld_text_base + current->ld_text_size) - sym_textsyms[i]->addr));
    }
  for (i=0; i<sym_ndatasyms; i++)
    {
      sym_datasyms[i]->size =
	(i != (sym_ndatasyms - 1)
	 ? (sym_datasyms[i+1]->addr - sym_datasyms[i]->addr)
	 : ((current->ld_data_base + current->ld_data_size) - sym_datasyms[i]->addr));
    }
#else 
  for (i=0; i<sym_ntextsyms; i++)
    {
      sym_textsyms[i]->size =
	(i != (sym_ntextsyms - 1)
	 ? (sym_textsyms[i+1]->addr - sym_textsyms[i]->addr)
	 : ((ld_text_base + ld_text_size) - sym_textsyms[i]->addr));
    }
  for (i=0; i<sym_ndatasyms; i++)
    {
      sym_datasyms[i]->size =
	(i != (sym_ndatasyms - 1)
	 ? (sym_datasyms[i+1]->addr - sym_datasyms[i]->addr)
	 : ((ld_data_base + ld_data_size) - sym_datasyms[i]->addr));
    }
#endif
  /* symbols are now available for use */
  syms_loaded = TRUE;
}

/* dump symbol SYM to output stream FD */
void
sym_dumpsym(struct sym_sym_t *sym,	/* symbol to display */
	    FILE *fd)			/* output stream */
{
  fprintf(fd,
    "sym `%s': %s seg, init-%s, pub-%s, local-%s, addr=0x%08x, size=%d\n",
	  sym->name,
	  sym->seg == ss_data ? "data" : "text",
	  sym->initialized ? "y" : "n",
	  sym->pub ? "y" : "n",
	  sym->local ? "y" : "n",
	  sym->addr,
	  sym->size);
}

/* dump all symbols to output stream FD */
void
sym_dumpsyms(FILE *fd)			/* output stream */
{
  int i;

  for (i=0; i < sym_nsyms; i++)
    sym_dumpsym(sym_syms[i], fd);
}

/* dump all symbol state to output stream FD */
void
sym_dumpstate(FILE *fd)			/* output stream */
{
  int i;

  if (fd == NULL)
    fd = stderr;

  fprintf(fd, "** All symbols sorted by address:\n");
  for (i=0; i < sym_nsyms; i++)
    sym_dumpsym(sym_syms[i], fd);

  fprintf(fd, "\n** All symbols sorted by name:\n");
  for (i=0; i < sym_nsyms; i++)
    sym_dumpsym(sym_syms_by_name[i], fd);

  fprintf(fd, "** Text symbols sorted by address:\n");
  for (i=0; i < sym_ntextsyms; i++)
    sym_dumpsym(sym_textsyms[i], fd);

  fprintf(fd, "\n** Text symbols sorted by name:\n");
  for (i=0; i < sym_ntextsyms; i++)
    sym_dumpsym(sym_textsyms_by_name[i], fd);

  fprintf(fd, "** Data symbols sorted by address:\n");
  for (i=0; i < sym_ndatasyms; i++)
    sym_dumpsym(sym_datasyms[i], fd);

  fprintf(fd, "\n** Data symbols sorted by name:\n");
  for (i=0; i < sym_ndatasyms; i++)
    sym_dumpsym(sym_datasyms_by_name[i], fd);
}

/* bind address ADDR to a symbol in symbol database DB, the address must
   match exactly if EXACT is non-zero, the index of the symbol in the
   requested symbol database is returned in *PINDEX if the pointer is
   non-NULL */
struct sym_sym_t *			/* symbol found, or NULL */
sym_bind_addr(md_addr_t addr,		/* address of symbol to locate */
	      int *pindex,		/* ptr to index result var */
	      int exact,		/* require exact address match? */
	      enum sym_db_t db)		/* symbol database to search */
{
  int nsyms, low, high, pos;
  struct sym_sym_t **syms;

  switch (db)
    {
    case sdb_any:
      syms = sym_syms;
      nsyms = sym_nsyms;
      break;
    case sdb_text:
      syms = sym_textsyms;
      nsyms = sym_ntextsyms;
      break;
    case sdb_data:
      syms = sym_datasyms;
      nsyms = sym_ndatasyms;
      break;
    default:
      panic("bogus symbol database");
    }

  /* any symbols to search? */
  if (!nsyms)
    {
      if (pindex)
	*pindex = -1;
      return NULL;
    }

  /* binary search symbol database (sorted by address) */
  low = 0;
  high = nsyms-1;
  pos = (low + high) >> 1;
  while (!(/* exact match */
	   (exact && syms[pos]->addr == addr)
	   /* in bounds match */
	   || (!exact
	       && syms[pos]->addr <= addr
	       && addr < (syms[pos]->addr + MAX(1, syms[pos]->size)))))
    {
      if (addr < syms[pos]->addr)
	high = pos - 1;
      else
	low = pos + 1;
      if (high >= low)
	pos = (low + high) >> 1;
      else
	{
	  if (pindex)
	    *pindex = -1;
	  return NULL;
	}
    }

  /* bound! */
  if (pindex)
    *pindex = pos;
  return syms[pos];
}

/* bind name NAME to a symbol in symbol database DB, the index of the symbol
   in the requested symbol database is returned in *PINDEX if the pointer is
   non-NULL */
struct sym_sym_t *				/* symbol found, or NULL */
sym_bind_name(char *name,			/* symbol name to locate */
	      int *pindex,			/* ptr to index result var */
	      enum sym_db_t db)			/* symbol database to search */
{
  int nsyms, low, high, pos, cmp;
  struct sym_sym_t **syms;

  switch (db)
    {
    case sdb_any:
      syms = sym_syms_by_name;
      nsyms = sym_nsyms;
      break;
    case sdb_text:
      syms = sym_textsyms_by_name;
      nsyms = sym_ntextsyms;
      break;
    case sdb_data:
      syms = sym_datasyms_by_name;
      nsyms = sym_ndatasyms;
      break;
    default:
      panic("bogus symbol database");
    }

  /* any symbols to search? */
  if (!nsyms)
    {
      if (pindex)
	*pindex = -1;
      return NULL;
    }

  /* binary search symbol database (sorted by name) */
  low = 0;
  high = nsyms-1;
  pos = (low + high) >> 1;
  while (!(/* exact string match */!(cmp = strcmp(syms[pos]->name, name))))
    {
      if (cmp > 0)
	high = pos - 1;
      else
	low = pos + 1;
      if (high >= low)
	pos = (low + high) >> 1;
      else
	{
	  if (pindex)
	    *pindex = -1;
	  return NULL;
	}
    }

  /* bound! */
  if (pindex)
    *pindex = pos;
  return syms[pos];
}
