/****************************************************************************
 * libs/libc/modlib/modlib_load.c
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/param.h>
#include <sys/types.h>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <debug.h>

#include <nuttx/lib/modlib.h>

#include "libc.h"
#include "modlib/modlib.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define ELF_ALIGN_MASK   ((1 << CONFIG_MODLIB_ALIGN_LOG2) - 1)
#define ELF_ALIGNUP(a)   (((unsigned long)(a) + ELF_ALIGN_MASK) & ~ELF_ALIGN_MASK)
#define ELF_ALIGNDOWN(a) ((unsigned long)(a) & ~ELF_ALIGN_MASK)

/* _ALIGN_UP: 'a' is assumed to be a power of two */

#define _ALIGN_UP(v, a) (((v) + ((a) - 1)) & ~((a) - 1))

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: modlib_elfsize
 *
 * Description:
 *   Calculate total memory allocation for the ELF file.
 *
 * Returned Value:
 *   0 (OK) is returned on success and a negated errno is returned on
 *   failure.
 *
 ****************************************************************************/

static void modlib_elfsize(struct mod_loadinfo_s *loadinfo)
{
  size_t textsize;
  size_t datasize;
  int i;

  /* Accumulate the size each section into memory that is marked SHF_ALLOC */

  textsize = 0;
  datasize = 0;

  if (loadinfo->ehdr.e_phnum > 0) 
    {
      for (i = 0; i < loadinfo->ehdr.e_phnum; i++)
        {
          FAR Elf_Phdr *phdr = &loadinfo->phdr[i];
          FAR void *textaddr = NULL;
    
          if (phdr->p_type == PT_LOAD)
            {
              if (phdr->p_flags & PF_X)
                {
                  textsize += phdr->p_memsz;
                  textaddr = (void *) phdr->p_vaddr;
                }
              else
                {
                  datasize += phdr->p_memsz;
                  loadinfo->datasec = phdr->p_vaddr;
                  loadinfo->segpad  = phdr->p_vaddr -
                                      ((uintptr_t) textaddr + textsize);
                }
            }
        }
    }
  else
    {
      for (i = 0; i < loadinfo->ehdr.e_shnum; i++)
        {
          FAR Elf_Shdr *shdr = &loadinfo->shdr[i];
    
          /* SHF_ALLOC indicates that the section requires memory during
           * execution.
           */
    
          if ((shdr->sh_flags & SHF_ALLOC) != 0)
            {
              /* SHF_WRITE indicates that the section address space is write-
               * able
               */
    
              if ((shdr->sh_flags & SHF_WRITE) != 0)
                {
                  datasize = _ALIGN_UP(datasize, shdr->sh_addralign);
                  datasize += ELF_ALIGNUP(shdr->sh_size);
                  if (loadinfo->dataalign < shdr->sh_addralign)
                    {
                      loadinfo->dataalign = shdr->sh_addralign;
                    }
                }
              else
                {
                  textsize = _ALIGN_UP(textsize, shdr->sh_addralign);
                  textsize += ELF_ALIGNUP(shdr->sh_size);
                  if (loadinfo->textalign < shdr->sh_addralign)
                    {
                      loadinfo->textalign = shdr->sh_addralign;
                    }
                }
            }
        }
    }

  /* Save the allocation size */

  loadinfo->textsize = textsize;
  loadinfo->datasize = datasize;
}

/****************************************************************************
 * Name: modlib_loadfile
 *
 * Description:
 *   Read the section data into memory. Section addresses in the shdr[] are
 *   updated to point to the corresponding position in the memory.
 *
 * Returned Value:
 *   0 (OK) is returned on success and a negated errno is returned on
 *   failure.
 *
 ****************************************************************************/

static inline int modlib_loadfile(FAR struct mod_loadinfo_s *loadinfo)
{
  FAR uint8_t *text;
  FAR uint8_t *data;
  FAR uint8_t **pptr;
  int ret;
  int i;

  /* Read each PT_LOAD area into memory */

  binfo("Loading sections - text: %p.%x data: %p.%x\n",
        (void *)loadinfo->textalloc, (int) loadinfo->textsize,
        (void *)loadinfo->datastart, (int) loadinfo->datasize);
  text = (FAR uint8_t *)loadinfo->textalloc;
  data = (FAR uint8_t *)loadinfo->datastart;

  if (loadinfo->ehdr.e_phnum > 0)
    {
      for (i = 0; i < loadinfo->ehdr.e_phnum; i++)
        {
          FAR Elf_Phdr *phdr = &loadinfo->phdr[i];
    
          if (phdr->p_type == PT_LOAD)
            {
              if (phdr->p_flags & PF_X)
                {
                  ret = modlib_read(loadinfo, text, phdr->p_filesz,
                                    phdr->p_offset);
                }
              else
                {
                  int bssSize = phdr->p_memsz - phdr->p_filesz;
                  ret = modlib_read(loadinfo, data, phdr->p_filesz,
                                    phdr->p_offset);
                  memset((FAR void *)((uintptr_t) data + phdr->p_filesz), 0,
                         bssSize);
                }
    
              if (ret < 0)
                {
                  berr("ERROR: Failed to read section %d: %d\n", i, ret);
                  return ret;
                }
            }
        }
    }
  else
    {
      for (i = 0; i < loadinfo->ehdr.e_shnum; i++)
        {
          FAR Elf_Shdr *shdr = &loadinfo->shdr[i];
    
          /* SHF_ALLOC indicates that the section requires memory during
           * execution
           */
    
          if ((shdr->sh_flags & SHF_ALLOC) == 0)
            {
              continue;
            }
    
          /* SHF_WRITE indicates that the section address space is write-
           * able
           */
    
          if ((shdr->sh_flags & SHF_WRITE) != 0)
            {
              pptr = &data;
            }
          else
            {
              pptr = &text;
            }
    
          *pptr = (FAR uint8_t *)_ALIGN_UP((uintptr_t)*pptr, shdr->sh_addralign);
    
          /* SHT_NOBITS indicates that there is no data in the file for the
           * section.
           */
    
          if (shdr->sh_type != SHT_NOBITS)
            {
              /* Read the section data from sh_offset to the memory region */
    
              ret = modlib_read(loadinfo, *pptr, shdr->sh_size, shdr->sh_offset);
              if (ret < 0)
                {
                  berr("ERROR: Failed to read section %d: %d\n", i, ret);
                  return ret;
                }
            }
    
          /* If there is no data in an allocated section, then the allocated
           * section must be cleared.
           */
    
          else
            {
              memset(*pptr, 0, shdr->sh_size);
            }
    
          /* Update sh_addr to point to copy in memory */
    
          binfo("%d. %08lx->%08lx\n", i,
                (unsigned long)shdr->sh_addr, (unsigned long)*pptr);
    
          shdr->sh_addr = (uintptr_t)*pptr;
    
          /* Setup the memory pointer for the next time through the loop */
    
          *pptr += ELF_ALIGNUP(shdr->sh_size);
        }
    }

  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: modlib_load
 *
 * Description:
 *   Loads the binary into memory, allocating memory, performing relocations
 *   and initializing the data and bss segments.
 *
 * Returned Value:
 *   0 (OK) is returned on success and a negated errno is returned on
 *   failure.
 *
 ****************************************************************************/

int modlib_load(FAR struct mod_loadinfo_s *loadinfo)
{
  int ret;

  binfo("loadinfo: %p\n", loadinfo);
  DEBUGASSERT(loadinfo && loadinfo->filfd >= 0);

  /* Load section and program headers into memory */

  ret = modlib_loadhdrs(loadinfo);
  if (ret < 0)
    {
      berr("ERROR: modlib_loadhdrs failed: %d\n", ret);
      goto errout_with_buffers;
    }

  /* Determine total size to allocate */

  modlib_elfsize(loadinfo);

  /* Allocate (and zero) memory for the ELF file. */

  /* Allocate memory to hold the ELF image */

  if (loadinfo->textsize > 0)
    {
#if defined(CONFIG_ARCH_USE_TEXT_HEAP)
      loadinfo->textalloc = (uintptr_t)
                            up_textheap_memalign(loadinfo->textalign,
                                                 loadinfo->textsize +
                                                 loadinfo->datasize +
                                                 loadinfo->segpad);
#else
      loadinfo->textalloc = (uintptr_t)lib_memalign(loadinfo->textalign,
                                                    loadinfo->textsize +
                                                    loadinfo->datasize +
                                                    loadinfo->segpad);
#endif
      if (!loadinfo->textalloc)
        {
          berr("ERROR: Failed to allocate memory for the module text\n");
          ret = -ENOMEM;
          goto errout_with_buffers;
        }
    }

  if (loadinfo->datasize > 0)
    {
      loadinfo->datastart = loadinfo->textalloc +
                            loadinfo->textsize +
                            loadinfo->segpad;
    }

  /* Load ELF section data into memory */

  ret = modlib_loadfile(loadinfo);
  if (ret < 0)
    {
      berr("ERROR: modlib_loadfile failed: %d\n", ret);
      goto errout_with_buffers;
    }

  return OK;

  /* Error exits */

errout_with_buffers:
  modlib_unload(loadinfo);
  return ret;
}
