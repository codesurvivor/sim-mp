#include <stdio.h>
#include <stdlib.h>

#include "host.h"
#include "misc.h"
#include "machine.h"
#include "endian.h"
#include "regs.h"
#include "memory.h"
#include "sim.h"
#include "eio.h"
#include "loader.h"
#ifdef SMT_SS
#include "context.h"
#endif

#ifdef BFD_LOADER
#include <bfd.h>
#else /* !BFD_LOADER */
#include "target-risc/elf.h"
#endif /* BFD_LOADER */

/* amount of tail padding added to all loaded text segments */
#define TEXT_TAIL_PADDING 128

/* program text (code) segment base */
md_addr_t ld_text_base = 0;

/* program text (code) size in bytes */
unsigned int ld_text_size = 0;

/* program initialized data segment base */
md_addr_t ld_data_base = 0;

/* program initialized ".data" and uninitialized ".bss" size in bytes */
unsigned int ld_data_size = 0;

/* top of the data segment */
md_addr_t ld_brk_point = 0;

/* program stack segment base (highest address in stack) */
md_addr_t ld_stack_base = MD_STACK_BASE;

/* program initial stack size */
unsigned int ld_stack_size = 0;

/* lowest address accessed on the stack */
md_addr_t ld_stack_min = (md_addr_t)-1;

/* program file name */
char *ld_prog_fname = NULL;

/* program entry point (initial PC) */
md_addr_t ld_prog_entry = 0;

/* program environment base address address */
md_addr_t ld_environ_base = 0;

/* target executable endian-ness, non-zero if big endian */
int ld_target_big_endian;

/* register simulator-specific statistics */
void
ld_reg_stats(struct stat_sdb_t *sdb,int threadid)	/* stats data base */
{
  stat_reg_addr(sdb, "ld_text_base",
		"program text (code) segment base",
		&ld_text_base, ld_text_base, "  0x%08p");
  stat_reg_uint(sdb, "ld_text_size",
		"program text (code) size in bytes",
		&ld_text_size, ld_text_size, NULL);
  stat_reg_addr(sdb, "ld_data_base",
		"program initialized data segment base",
		&ld_data_base, ld_data_base, "  0x%08p");
  stat_reg_uint(sdb, "ld_data_size",
		"program init'ed `.data' and uninit'ed `.bss' size in bytes",
		&ld_data_size, ld_data_size, NULL);
  stat_reg_addr(sdb, "ld_stack_base",
		"program stack segment base (highest address in stack)",
		&ld_stack_base, ld_stack_base, "  0x%08p");
  stat_reg_uint(sdb, "ld_stack_size",
		"program initial stack size",
		&ld_stack_size, ld_stack_size, NULL);

  stat_reg_addr(sdb, "ld_prog_entry",
		"program entry point (initial PC)",
		&ld_prog_entry, ld_prog_entry, "  0x%08p");
  stat_reg_addr(sdb, "ld_environ_base",
		"program environment base address address",
		&ld_environ_base, ld_environ_base, "  0x%08p");
  stat_reg_int(sdb, "ld_target_big_endian",
	       "target executable endian-ness, non-zero if big endian",
	       &ld_target_big_endian, ld_target_big_endian, NULL);
}


/* load program text and initialized data into simulated virtual memory
   space and initialize program segment range variables */
void
ld_load_prog(char *fname,		/* program to load */
	     int argc, char **argv,	/* simulated program cmd line args */
	     char **envp,		/* simulated program environment */
	     struct regs_t *regs,	/* registers to initialize for load */
	     struct mem_t *mem,		/* memory space to load prog into */
	     int zero_bss_segs,int threadid)		/* zero uninit data segment? */
{
  int i;
  word_t temp;
    context *current;
  current = thecontexts[threadid];

  md_addr_t sp, data_break = 0, null_ptr = 0, argv_addr, envp_addr;
  {
    FILE *fobj;
    long floc;
    // struct ecoff_filehdr fhdr;
    // struct ecoff_aouthdr ahdr;
    // struct ecoff_scnhdr shdr;
	Elf32_Ehdr *ehdr;
	Elf32_Phdr *phdr;
	
	ehdr=(Elf32_Ehdr *)malloc(sizeof(Elf32_Ehdr));
	phdr=(Elf32_Phdr *)malloc(sizeof(Elf32_Phdr));
	
	
    /* set up a local stack pointer, this is where the argv and envp
       data is written into program memory */
    current->ld_stack_base = MD_STACK_BASE;
    sp = ROUND_DOWN(MD_STACK_BASE - MD_MAX_ENVIRON, sizeof(dfloat_t));
    current->ld_stack_size = current->ld_stack_base - sp;
  	current->stack_base = current->ld_stack_base - STACKSIZE;

    /* initial stack pointer value */
    current->ld_environ_base = sp;

    /* record profile file name */
    current->ld_prog_fname = argv[0];

    /* load the program into memory, try both endians */
#if defined(__CYGWIN32__) || defined(_MSC_VER)
    fobj = fopen(argv[0], "rb");
#else
    fobj = fopen(argv[0], "r");
#endif
    if (!fobj)
      fatal("cannot open executable `%s'", argv[0]);

    // if (fread(&fhdr, sizeof(struct ecoff_filehdr), 1, fobj) < 1)
      // fatal("cannot read header from executable `%s'", argv[0]);
	if (fread(ehdr, sizeof(Elf32_Ehdr), 1, fobj) < 1)
      fatal("cannot read header from executable `%s'", argv[0]);
	  

    /* record endian of target */
    // if (fhdr.f_magic == ECOFF_EB_MAGIC)
      // ld_target_big_endian = TRUE;
    // else if (fhdr.f_magic == ECOFF_EL_MAGIC)
      // ld_target_big_endian = FALSE;
    // else if (fhdr.f_magic == ECOFF_EB_OTHER || fhdr.f_magic == ECOFF_EL_OTHER)
      // fatal("PISA binary `%s' has wrong endian format", argv[0]);
    // else if (fhdr.f_magic == ECOFF_ALPHAMAGIC)
      // fatal("PISA simulator cannot run Alpha binary `%s'", argv[0]);
    // else
      // fatal("bad magic number in executable `%s' (not an executable?)",
	    // argv[0]);
	if(!((ehdr->e_ident[0]==ELFMAG0)&&(ehdr->e_ident[1]==ELFMAG1)
		&&(ehdr->e_ident[2]==ELFMAG2)&&(ehdr->e_ident[3]==ELFMAG3)))
		fatal("file format should be elf\n");
	if(ehdr->e_ident[5]==ELFDATA2MSB)
		current->ld_target_big_endian=TRUE;
	else if(ehdr->e_ident[5]==ELFDATA2LSB)
		current->ld_target_big_endian=FALSE;
	
	
    // if (fread(&ahdr, sizeof(struct ecoff_aouthdr), 1, fobj) < 1)
      // fatal("cannot read AOUT header from executable `%s'", argv[0]);

    // data_break = MD_DATA_BASE + ahdr.dsize + ahdr.bsize;

    /* seek to the beginning of the first section header, the file header comes
       first, followed by the optional header (this is the aouthdr), the size
       of the aouthdr is given in Fdhr.f_opthdr */
    for(i=0;i<ehdr->e_phnum;i++)
	{
		fseek(fobj,ehdr->e_phoff+(ehdr->e_phentsize*i),0);
		if(fread(phdr,sizeof(Elf32_Phdr),1,fobj)!=1)
			exit(3);
		//fseeker=ftell(fp);
		//printf("fseek_after=%08x\n",fseeker);
		switch(phdr->p_type)
		{
			case PT_LOAD:
				
				if(phdr->p_flags&PF_X)	
				// ---text segment---//
				{
					current->ld_text_size=(phdr->p_vaddr+phdr->p_memsz)-MD_TEXT_BASE;
					char *buffer;
					buffer=(char *)malloc(phdr->p_memsz);
					fseek(fobj,phdr->p_offset,0);
					//fseeker=ftell(fp);
					//printf("fseek_after=%08x\n",fseeker);
					
					fread(buffer,phdr->p_memsz,1,fobj);	
					
					mem_bcopy(mem_access, current->mem, Write, phdr->p_vaddr, buffer, phdr->p_filesz, current->id);
					//fwrite(buffer,phdr->p_memsz,1,flog);
					free(buffer);
					printf("text segment at %0x\n",phdr->p_vaddr);
				}
				else	
				// ---data segment---//
				{
					current->ld_data_size=(phdr->p_vaddr+phdr->p_memsz)-MD_DATA_BASE;
					char *buffer;
					buffer=(char *)malloc(phdr->p_memsz);
				
					fseek(fobj,phdr->p_offset,0);
					fread(buffer,phdr->p_memsz,1,fobj);	
					mem_bcopy(mem_access,current->mem, Write, phdr->p_vaddr, buffer, phdr->p_filesz,current->id);
					free(buffer);
					printf("data segment at %0x\n",phdr->p_vaddr);
				}
				break;
			default:
				break;
				
		}
	}
    /* compute data segment size from data break point */
    current->ld_text_base = MD_TEXT_BASE;
    current->ld_data_base = MD_DATA_BASE;
    // ld_prog_entry = ahdr.entry;
	current->ld_prog_entry = ehdr->e_entry;
    // ld_data_size = data_break - ld_data_base;

    /* done with the executable, close it */
    if (fclose(fobj))
      fatal("could not close executable `%s'", argv[0]);
	  
	free(ehdr);
	free(phdr);
  }
	
  /* perform sanity checks on segment ranges */
  if (!current->ld_text_base || !current->ld_text_size)
    fatal("executable is missing a `.text' section");
  if (!current->ld_data_base)
    fatal("executable is missing a `.data' section");
  if (!current->ld_prog_entry)
    fatal("program entry point not specified");

  /* determine byte/words swapping required to execute on this host */
  sim_swap_bytes = (endian_host_byte_order() != endian_target_byte_order(threadid));
  if (sim_swap_bytes)
    {
      fatal("binary endian does not match host endian");
    }
  sim_swap_words = (endian_host_word_order() != endian_target_word_order(threadid));
  if (sim_swap_words)
    {
      fatal("binary endian does not match host endian");
    }

  /* write [argc] to stack */
  temp = MD_SWAPW(argc);
  mem_access(current->mem, Write, sp, &temp, sizeof(word_t), current->id);
  sp += sizeof(word_t);

  /* skip past argv array and NULL */
  argv_addr = sp;
  sp = sp + (argc + 1) * sizeof(md_addr_t);

  /* save space for envp array and NULL */
  envp_addr = sp;
  for (i=0; envp[i]; i++)
    sp += sizeof(md_addr_t);
  sp += sizeof(md_addr_t);

  /* fill in the argv pointer array and data */
  for (i=0; i<argc; i++)
    {
      /* write the argv pointer array entry */
      temp = MD_SWAPW(sp);
      mem_access(current->mem, Write, argv_addr + i*sizeof(md_addr_t),
		 &temp, sizeof(md_addr_t), current->id);
      /* and the data */
      mem_strcpy(mem_access, current->mem, Write, sp, argv[i],current->id);
      sp += strlen(argv[i]) + 1;
    }
  /* terminate argv array with a NULL */
  mem_access(current->mem, Write, argv_addr + i*sizeof(md_addr_t),
	     &null_ptr, sizeof(md_addr_t), current->id);

  /* write envp pointer array and data to stack */
  for (i = 0; envp[i]; i++)
    {
      /* write the envp pointer array entry */
      temp = MD_SWAPW(sp);
      mem_access(current->mem, Write, envp_addr + i*sizeof(md_addr_t),
		 &temp, sizeof(md_addr_t), current->id);
      /* and the data */
      mem_strcpy(mem_access, current->mem, Write, sp, envp[i],current->id);
      sp += strlen(envp[i]) + 1;
    }
  /* terminate the envp array with a NULL */
  mem_access(current->mem, Write, envp_addr + i*sizeof(md_addr_t),
	     &null_ptr, sizeof(md_addr_t), current->id);

  /* did we tromp off the stop of the stack? */
  if (sp > current->ld_stack_base)
    {
      /* we did, indicate to the user that MD_MAX_ENVIRON must be increased,
	 alternatively, you can use a smaller environment, or fewer
	 command line arguments */
      fatal("environment overflow, increase MD_MAX_ENVIRON in ss.h");
    }

  /* initialize the bottom of heap to top of data segment */
  current->ld_brk_point = ROUND_UP(current->ld_data_base + current->ld_data_size, MD_PAGE_SIZE);

  /* set initial minimum stack pointer value to initial stack value */
  current->ld_stack_min = current->regs.regs_R[MD_REG_SP];

  /* set up initial register state */
  current->regs.regs_R[MD_REG_SP] = current->ld_environ_base;
  current->regs.regs_PC = current->ld_prog_entry;
	
  /* initialize for the GOT */
  current->regs.regs_R[MD_REG_FUNCRADD]= current->ld_prog_entry;
  
  debug("xinyuan ld_text_base: 0x%08x  ld_text_size: 0x%08x",
	ld_text_base, ld_text_size);
  debug("xinyuan ld_data_base: 0x%08x  ld_data_size: 0x%08x",
	ld_data_base, ld_data_size);
  debug("xinyuan ld_stack_base: 0x%08x  ld_stack_size: 0x%08x",
	ld_stack_base, ld_stack_size);
  debug("xinyuan ld_prog_entry: 0x%08x", ld_prog_entry);

  /* finally, predecode the text segment... 
  {
    md_addr_t addr;
    md_inst_t inst;
    enum md_fault_type fault;

    if (OP_MAX > 255)
      fatal("cannot perform fast decoding, too many opcodes");

    debug("sim: decoding text segment...");
    for (addr=ld_text_base;
	 addr < (ld_text_base+ld_text_size);
	 addr += sizeof(md_inst_t))
      {
	fault = mem_access(mem, Read, addr, &inst, sizeof(inst));
	if (fault != md_fault_none)
	  fatal("could not read instruction memory");
	inst.a = (inst.a & ~0xff) | (word_t)MD_OP_ENUM(MD_OPFIELD(inst));
	fault = mem_access(mem, Write, addr, &inst, sizeof(inst));
	if (fault != md_fault_none)
	  fatal("could not write instruction memory");
      }
  }*/
}
