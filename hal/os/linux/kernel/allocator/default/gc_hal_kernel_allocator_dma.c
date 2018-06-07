/****************************************************************************
*
*    The MIT License (MIT)
*
*    Copyright (c) 2014 - 2018 Vivante Corporation
*
*    Permission is hereby granted, free of charge, to any person obtaining a
*    copy of this software and associated documentation files (the "Software"),
*    to deal in the Software without restriction, including without limitation
*    the rights to use, copy, modify, merge, publish, distribute, sublicense,
*    and/or sell copies of the Software, and to permit persons to whom the
*    Software is furnished to do so, subject to the following conditions:
*
*    The above copyright notice and this permission notice shall be included in
*    all copies or substantial portions of the Software.
*
*    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
*    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
*    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
*    DEALINGS IN THE SOFTWARE.
*
*****************************************************************************
*
*    The GPL License (GPL)
*
*    Copyright (C) 2014 - 2018 Vivante Corporation
*
*    This program is free software; you can redistribute it and/or
*    modify it under the terms of the GNU General Public License
*    as published by the Free Software Foundation; either version 2
*    of the License, or (at your option) any later version.
*
*    This program is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with this program; if not, write to the Free Software Foundation,
*    Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*
*****************************************************************************
*
*    Note: This software is released under dual MIT and GPL licenses. A
*    recipient may use this file under the terms of either the MIT license or
*    GPL License. If you wish to use only one license not the other, you can
*    indicate your decision by deleting one of the above license notices in your
*    version of this file.
*
*****************************************************************************/


#include "gc_hal_kernel_linux.h"
#include "gc_hal_kernel_allocator.h"

#include <linux/pagemap.h>
#include <linux/seq_file.h>
#include <linux/mman.h>
#include <asm/atomic.h>
#include <linux/dma-mapping.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/of_reserved_mem.h>
#include <linux/cma.h>
#include <linux/dma-contiguous.h>


#define _GC_OBJ_ZONE    gcvZONE_OS

typedef struct _gcsDMA_PRIV * gcsDMA_PRIV_PTR;
typedef struct _gcsDMA_PRIV {
    atomic_t usage;
}
gcsDMA_PRIV;

struct mdl_dma_priv {
    gctPOINTER kvaddr;
    dma_addr_t dmaHandle;
};

/*
* Debugfs support.
*/
static int gc_dma_usage_show(struct seq_file* m, void* data)
{
    gcsINFO_NODE *node = m->private;
    gckALLOCATOR Allocator = node->device;
    gcsDMA_PRIV_PTR priv = Allocator->privateData;
    long long usage = (long long)atomic_read(&priv->usage);

    seq_printf(m, "type        n pages        bytes\n");
    seq_printf(m, "normal   %10llu %12llu\n", usage, usage * PAGE_SIZE);

    return 0;
}

static gcsINFO InfoList[] =
{
    {"dmausage", gc_dma_usage_show},
};

static void
_DebugfsInit(
    IN gckALLOCATOR Allocator,
    IN gckDEBUGFS_DIR Root
    )
{
    gcmkVERIFY_OK(
        gckDEBUGFS_DIR_Init(&Allocator->debugfsDir, Root->root, "dma"));

    gcmkVERIFY_OK(gckDEBUGFS_DIR_CreateFiles(
        &Allocator->debugfsDir,
        InfoList,
        gcmCOUNTOF(InfoList),
        Allocator
        ));
}

static void
_DebugfsCleanup(
    IN gckALLOCATOR Allocator
    )
{
    gcmkVERIFY_OK(gckDEBUGFS_DIR_RemoveFiles(
        &Allocator->debugfsDir,
        InfoList,
        gcmCOUNTOF(InfoList)
        ));

    gckDEBUGFS_DIR_Deinit(&Allocator->debugfsDir);
}

//#ifdef CONFIG_ARM64
static struct device *
_GetDevice(
    IN gckOS Os
    )
{
    gcsPLATFORM *platform;

    platform = Os->device->platform;

    if (!platform)
    {
        return gcvNULL;
    }

    return &platform->device->dev;
}
//#endif
unsigned int dma_addrs;

static gceSTATUS
_DmaAlloc(
    IN gckALLOCATOR Allocator,
    INOUT PLINUX_MDL Mdl,
    IN gctSIZE_T NumPages,
    IN gctUINT32 Flags
    )
{
    gceSTATUS status;
    u32 gfp = GFP_KERNEL | gcdNOWARN;
    gcsDMA_PRIV_PTR allocatorPriv = (gcsDMA_PRIV_PTR)Allocator->privateData;
	//struct page *venc_pages;  //zxw

    struct mdl_dma_priv *mdlPriv=gcvNULL;
    gckOS os = Allocator->os;

//#if defined CONFIG_ARM64  zxw
    struct device *dev = _GetDevice(os);
//#endif

    gcmkHEADER_ARG("Mdl=%p NumPages=0x%zx Flags=0x%x", Mdl, NumPages, Flags);
	printk("zxw:Mdl=%p NumPages=0x%zx Flags=0x%x", Mdl, NumPages, Flags);
//#if defined CONFIG_ARM64
    gcmkVERIFY_ARGUMENT(dev);
//#endif

    gcmkONERROR(gckOS_Allocate(os, sizeof(struct mdl_dma_priv), (gctPOINTER *)&mdlPriv));

#if defined(CONFIG_ZONE_DMA32) && LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37)
    if (Flags & gcvALLOC_FLAG_4GB_ADDR)
    {
        gfp |= __GFP_DMA32;
    }
#endif
//can test from alloc
	//devp->venc_pages = dma_alloc_from_contiguous(dev,NumPages, 0);

    mdlPriv->kvaddr  = dma_alloc_coherent(dev, NumPages * PAGE_SIZE, &mdlPriv->dmaHandle, gfp);
/*
#if defined CONFIG_ARM64
        = dma_alloc_coherent(dev, NumPages * PAGE_SIZE, &mdlPriv->dmaHandle, gfp);
		printk("CONFIG_ARM64 \n");
#elif defined CONFIG_MIPS || defined CONFIG_CPU_CSKYV2 || defined CONFIG_PPC
        = dma_alloc_coherent(gcvNULL, NumPages * PAGE_SIZE, &mdlPriv->dmaHandle, gfp);
		printk("CONFIG_MIPS \n");
#else
        = dma_alloc_writecombine(gcvNULL, NumPages * PAGE_SIZE,  &mdlPriv->dmaHandle, gfp);
		printk("config else \n");
#endif
*/
#ifdef CONFLICT_BETWEEN_BASE_AND_PHYS
    if ((os->device->baseAddress & 0x80000000) != (mdlPriv->dmaHandle & 0x80000000))
    {
        mdlPriv->dmaHandle = (mdlPriv->dmaHandle & ~0x80000000)
                            | (os->device->baseAddress & 0x80000000);
    }
#endif

    if (mdlPriv->kvaddr == gcvNULL)
    {
		printk("can't get kvaddr\n");
        gcmkONERROR(gcvSTATUS_OUT_OF_MEMORY);
    }

    Mdl->priv = mdlPriv;

    Mdl->dmaHandle = mdlPriv->dmaHandle;
	dma_addrs = Mdl->dmaHandle;
	//printk("zxw:dma alloc phy addr is 0x%x\n",Mdl->dmaHandle);
    /* Statistic. */
    atomic_add(NumPages, &allocatorPriv->usage);

    gcmkFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    if (mdlPriv)
    {
        gckOS_Free(os, mdlPriv);
    }

    gcmkFOOTER();
    return status;
}

static gceSTATUS
_DmaGetSGT(
    IN gckALLOCATOR Allocator,
    IN PLINUX_MDL Mdl,
    IN gctSIZE_T Offset,
    IN gctSIZE_T Bytes,
    OUT gctPOINTER *SGT
    )
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,6,0)
    struct page ** pages = gcvNULL;
    struct page * page = gcvNULL;
    struct sg_table *sgt = NULL;
    struct mdl_dma_priv *mdlPriv = (struct mdl_dma_priv*)Mdl->priv;

    gceSTATUS status = gcvSTATUS_OK;
    gctSIZE_T offset = Offset & ~PAGE_MASK; /* Offset to the first page */
    gctINT skipPages = Offset >> PAGE_SHIFT;     /* skipped pages */
    gctINT numPages = (PAGE_ALIGN(Offset + Bytes) >> PAGE_SHIFT) - skipPages;
    gctINT i;

    gcmkASSERT(Offset + Bytes <= Mdl->numPages << PAGE_SHIFT);

    sgt = kmalloc(sizeof(struct sg_table), GFP_KERNEL | gcdNOWARN);
    if (!sgt)
    {
        gcmkONERROR(gcvSTATUS_OUT_OF_MEMORY);
    }

    pages = kmalloc(sizeof(struct page*) * numPages, GFP_KERNEL | gcdNOWARN);
    if (!pages)
    {
        gcmkONERROR(gcvSTATUS_OUT_OF_MEMORY);
    }

    page = virt_to_page(mdlPriv->kvaddr);
    for (i = 0; i < numPages; ++i)
    {
        pages[i] = nth_page(page, i + skipPages);
    }

    if (sg_alloc_table_from_pages(sgt, pages, numPages, offset, Bytes, GFP_KERNEL) < 0)
    {
        gcmkONERROR(gcvSTATUS_GENERIC_IO);
    }

    *SGT = (gctPOINTER)sgt;

OnError:
    if (pages)
    {
        kfree(pages);
    }

    if (gcmIS_ERROR(status) && sgt)
    {
        kfree(sgt);
    }

    return status;
#else
    return gcvSTATUS_NOT_SUPPORTED;
#endif
}

static void
_DmaFree(
    IN gckALLOCATOR Allocator,
    IN OUT PLINUX_MDL Mdl
    )
{
    gckOS os = Allocator->os;
    struct mdl_dma_priv *mdlPriv=(struct mdl_dma_priv *)Mdl->priv;
    gcsDMA_PRIV_PTR allocatorPriv = (gcsDMA_PRIV_PTR)Allocator->privateData;

//#if defined CONFIG_ARM64
    dma_free_coherent(_GetDevice(os), Mdl->numPages * PAGE_SIZE, mdlPriv->kvaddr, mdlPriv->dmaHandle);
//#elif defined CONFIG_MIPS || defined CONFIG_CPU_CSKYV2 || defined CONFIG_PPC
//    dma_free_coherent(gcvNULL, Mdl->numPages * PAGE_SIZE, mdlPriv->kvaddr, mdlPriv->dmaHandle);
//#else
//    dma_free_writecombine(gcvNULL, Mdl->numPages * PAGE_SIZE, mdlPriv->kvaddr, mdlPriv->dmaHandle);
//#endif

    gckOS_Free(os, mdlPriv);

    /* Statistic. */
    atomic_sub(Mdl->numPages, &allocatorPriv->usage);
}

static gceSTATUS
_DmaMmap(
    IN gckALLOCATOR Allocator,
    IN PLINUX_MDL Mdl,
    IN gctBOOL Cacheable,
    IN gctSIZE_T skipPages,
    IN gctSIZE_T numPages,
    IN struct vm_area_struct *vma
    )
{
    struct mdl_dma_priv *mdlPriv = (struct mdl_dma_priv*)Mdl->priv;
    gceSTATUS status = gcvSTATUS_OK;
    int ret = 0;   //zxw
	gckOS os = Allocator->os;
    struct device *dev = _GetDevice(os);
	
    gcmkHEADER_ARG("Allocator=%p Mdl=%p vma=%p", Allocator, Mdl, vma);

    gcmkASSERT(skipPages + numPages <= Mdl->numPages);

    /* map kernel memory to user space.. */
#if defined CONFIG_MIPS || defined CONFIG_CPU_CSKYV2 || defined CONFIG_PPC
    if (remap_pfn_range(
            vma,
            vma->vm_start,
            (mdlPriv->dmaHandle >> PAGE_SHIFT) + skipPages,
            numPages << PAGE_SHIFT,
            pgprot_writecombine(vma->vm_page_prot)) < 0)
#else
	printk("before dma_mmap_writecombine\n");
	printk("PAGE_SHIFT:%d,ptr:%p,num:%ld\n",PAGE_SHIFT,(gctINT8_PTR)mdlPriv->kvaddr,numPages);
	printk("vma-start:%ld,vma-end:%ld,flag:%ld\n",vma->vm_start,vma->vm_end,vma->vm_flags);
    /* map kernel memory to user space.. */
	ret = dma_mmap_writecombine(dev,vma,
								(gctINT8_PTR)mdlPriv->kvaddr + (skipPages << PAGE_SHIFT),
								mdlPriv->dmaHandle + (skipPages << PAGE_SHIFT),
								(numPages<<PAGE_SHIFT));
	if (ret < 0)
#endif
    {
        gcmkTRACE_ZONE(
            gcvLEVEL_WARNING, gcvZONE_OS,
            "%s(%d): dma_mmap_attrs error",
            __FUNCTION__, __LINE__
            );
		printk("dma_mmap_writecombine ret:%d\n",ret);
        status = gcvSTATUS_OUT_OF_MEMORY;
    }
	printk("after dma_mmap_writecombine,status:%d\n",status);
    gcmkFOOTER();
    return status;
}

static void
_DmaUnmapUser(
    IN gckALLOCATOR Allocator,
    IN PLINUX_MDL Mdl,
    IN PLINUX_MDL_MAP MdlMap,
    IN gctUINT32 Size
    )
{
    if (unlikely(current->mm == gcvNULL))
    {
        /* Do nothing if process is exiting. */
        return;
    }

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,4,0)
    if (vm_munmap((unsigned long)MdlMap->vmaAddr, Size) < 0)
    {
        gcmkTRACE_ZONE(
                gcvLEVEL_WARNING, gcvZONE_OS,
                "%s(%d): vm_munmap failed",
                __FUNCTION__, __LINE__
                );
    }
#else
    down_write(&current->mm->mmap_sem);
    if (do_munmap(current->mm, (unsigned long)MdlMap->vmaAddr, Size) < 0)
    {
        gcmkTRACE_ZONE(
                gcvLEVEL_WARNING, gcvZONE_OS,
                "%s(%d): do_munmap failed",
                __FUNCTION__, __LINE__
                );
    }
    up_write(&current->mm->mmap_sem);
#endif
}

static gceSTATUS
_DmaMapUser(
    gckALLOCATOR Allocator,
    PLINUX_MDL Mdl,
    PLINUX_MDL_MAP MdlMap,
    gctBOOL Cacheable
    )
{
    gctPOINTER userLogical = gcvNULL;
    gceSTATUS status = gcvSTATUS_OK;


    gcmkHEADER_ARG("Allocator=%p Mdl=%p Cacheable=%d", Allocator, Mdl, Cacheable);

//#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0)
#if 1
	printk("_DmaMapUser,numpages:%d\n",Mdl->numPages);
    userLogical = (gctPOINTER)vm_mmap(gcvNULL,
                    0L,
                    Mdl->numPages * PAGE_SIZE,
                    PROT_READ | PROT_WRITE,
                    MAP_SHARED,
                    0);
#else
	printk("do_mmap_pgoff,numpages:%d\n",Mdl->numPages);
    down_write(&current->mm->mmap_sem);
    userLogical = (gctPOINTER)do_mmap_pgoff(gcvNULL,
                    0L,
                    Mdl->numPages * PAGE_SIZE,
                    PROT_READ | PROT_WRITE,
                    MAP_SHARED,
                    0);
    up_write(&current->mm->mmap_sem);
#endif

    gcmkTRACE_ZONE(
        gcvLEVEL_INFO, gcvZONE_OS,
        "%s(%d): vmaAddr->%p for phys_addr->%p",
        __FUNCTION__, __LINE__, userLogical, Mdl
        );

    if (IS_ERR(userLogical))
    {
    	printk("vm_mmap error\n");
        gcmkTRACE_ZONE(
            gcvLEVEL_INFO, gcvZONE_OS,
            "%s(%d): do_mmap_pgoff error",
            __FUNCTION__, __LINE__
            );

        gcmkONERROR(gcvSTATUS_OUT_OF_MEMORY);
    }

    down_write(&current->mm->mmap_sem);
    do
    {
        struct vm_area_struct *vma = find_vma(current->mm, (unsigned long)userLogical);
        if (vma == gcvNULL)
        {
            up_write(&current->mm->mmap_sem);
			printk("cannot find vma\n");
            gcmkTRACE_ZONE(
                gcvLEVEL_INFO, gcvZONE_OS,
                "%s(%d): find_vma error",
                __FUNCTION__, __LINE__
                );

            gcmkERR_BREAK(gcvSTATUS_OUT_OF_RESOURCES);
        }

        gcmkERR_BREAK(_DmaMmap(Allocator, Mdl, Cacheable, 0, Mdl->numPages, vma));

        MdlMap->vmaAddr = userLogical;
        MdlMap->cacheable = gcvFALSE;
        MdlMap->vma = vma;
    }
    while (gcvFALSE);
    up_write(&current->mm->mmap_sem);
	printk("over,status:%d\n",status);
OnError:
    if (gcmIS_ERROR(status) && userLogical)
    {
        _DmaUnmapUser(Allocator, Mdl, userLogical, Mdl->numPages * PAGE_SIZE);
    }
    gcmkFOOTER();
    return status;
}

static gceSTATUS
_DmaMapKernel(
    IN gckALLOCATOR Allocator,
    IN PLINUX_MDL Mdl,
    IN gctSIZE_T Offset,
    IN gctSIZE_T Bytes,
    OUT gctPOINTER *Logical
    )
{
    struct mdl_dma_priv *mdlPriv=(struct mdl_dma_priv *)Mdl->priv;
    *Logical = (uint8_t *)mdlPriv->kvaddr + Offset;
    return gcvSTATUS_OK;
}

static gceSTATUS
_DmaUnmapKernel(
    IN gckALLOCATOR Allocator,
    IN PLINUX_MDL Mdl,
    IN gctPOINTER Logical
    )
{
    return gcvSTATUS_OK;
}

static gceSTATUS
_DmaCache(
    IN gckALLOCATOR Allocator,
    IN PLINUX_MDL Mdl,
    IN gctSIZE_T Offset,
    IN gctPOINTER Logical,
    IN gctSIZE_T Bytes,
    IN gceCACHEOPERATION Operation
    )
{
    switch (Operation)
    {
    case gcvCACHE_CLEAN:
    case gcvCACHE_FLUSH:
        _MemoryBarrier();
        break;
    case gcvCACHE_INVALIDATE:
        break;
    default:
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    return gcvSTATUS_OK;
}

static gceSTATUS
_DmaPhysical(
    IN gckALLOCATOR Allocator,
    IN PLINUX_MDL Mdl,
    IN gctUINT32 Offset,
    OUT gctPHYS_ADDR_T * Physical
    )
{
    struct mdl_dma_priv *mdlPriv=(struct mdl_dma_priv *)Mdl->priv;

    *Physical = mdlPriv->dmaHandle + Offset;

    return gcvSTATUS_OK;
}

static void
_DmaAllocatorDestructor(
    gcsALLOCATOR *Allocator
    )
{
    _DebugfsCleanup(Allocator);

    if (Allocator->privateData)
    {
        kfree(Allocator->privateData);
    }

    kfree(Allocator);
}

/* Default allocator operations. */
gcsALLOCATOR_OPERATIONS DmaAllocatorOperations = {
    .Alloc              = _DmaAlloc,
    .Free               = _DmaFree,
    .Mmap               = _DmaMmap,
    .MapUser            = _DmaMapUser,
    .UnmapUser          = _DmaUnmapUser,
    .MapKernel          = _DmaMapKernel,
    .UnmapKernel        = _DmaUnmapKernel,
    .Cache              = _DmaCache,
    .Physical           = _DmaPhysical,
    .GetSGT             = _DmaGetSGT,
};

/* Default allocator entry. */
gceSTATUS
_DmaAlloctorInit(
    IN gckOS Os,
    IN gcsDEBUGFS_DIR *Parent,
    OUT gckALLOCATOR * Allocator
    )
{
    gceSTATUS status;
    gckALLOCATOR allocator = gcvNULL;
    gcsDMA_PRIV_PTR priv = gcvNULL;

    gcmkONERROR(gckALLOCATOR_Construct(Os, &DmaAllocatorOperations, &allocator));

    priv = kzalloc(gcmSIZEOF(gcsDMA_PRIV), GFP_KERNEL | gcdNOWARN);

    if (!priv)
    {
        gcmkONERROR(gcvSTATUS_OUT_OF_MEMORY);
    }

    atomic_set(&priv->usage, 0);

    /* Register private data. */
    allocator->privateData = priv;
    allocator->destructor  = _DmaAllocatorDestructor;

    _DebugfsInit(allocator, Parent);

    /*
     * DMA allocator is only used for NonPaged memory
     * when NO_DMA_COHERENT is not defined.
     */
    allocator->capability = gcvALLOC_FLAG_DMABUF_EXPORTABLE
#if defined(CONFIG_ZONE_DMA32) && LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37)
                          | gcvALLOC_FLAG_4GB_ADDR
#endif
                          ;

    *Allocator = allocator;

    return gcvSTATUS_OK;

OnError:
    if (allocator)
    {
        kfree(allocator);
    }
    return status;
}

