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
#include <linux/slab.h>

#define _GC_OBJ_ZONE    gcvZONE_DEVICE

#define DEBUG_FILE          "galcore_trace"
#define PARENT_FILE         "gpu"

#define gcdDEBUG_FS_WARN    "Experimental debug entry, may be removed in future release, do NOT rely on it!\n"

static gckGALDEVICE galDevice;

extern gcTA globalTA[16];
extern int galcore_irq;  //zxw
/******************************************************************************\
******************************** Debugfs Support *******************************
\******************************************************************************/

/******************************************************************************\
***************************** DEBUG SHOW FUNCTIONS *****************************
\******************************************************************************/

int gc_info_show(struct seq_file* m, void* data)
{
    gcsINFO_NODE *node = m->private;
    gckGALDEVICE device = node->device;
    int i = 0;
    gceCHIPMODEL chipModel;
    gctUINT32 chipRevision;
    gctUINT32 productID = 0;
    gctUINT32 ecoID = 0;

    for (i = 0; i < gcdMAX_GPU_COUNT; i++)
    {
        if (device->irqLines[i] != -1)
        {
            {
                chipModel = device->kernels[i]->hardware->identity.chipModel;
                chipRevision = device->kernels[i]->hardware->identity.chipRevision;
                productID = device->kernels[i]->hardware->identity.productID;
                ecoID = device->kernels[i]->hardware->identity.ecoID;
            }

            seq_printf(m, "gpu      : %d\n", i);
            seq_printf(m, "model    : %4x\n", chipModel);
            seq_printf(m, "revision : %4x\n", chipRevision);
            seq_printf(m, "product  : %4x\n", productID);
            seq_printf(m, "eco      : %4x\n", ecoID);
            seq_printf(m, "\n");
        }
    }

    return 0;
}

int gc_clients_show(struct seq_file* m, void* data)
{
    gcsINFO_NODE *node = m->private;
    gckGALDEVICE device = node->device;

    gckKERNEL kernel = _GetValidKernel(device);

    gcsDATABASE_PTR database;
    gctINT i, pid;
    char name[24];

    seq_printf(m, "%-8s%s\n", "PID", "NAME");
    seq_printf(m, "------------------------\n");

    /* Acquire the database mutex. */
    gcmkVERIFY_OK(
        gckOS_AcquireMutex(kernel->os, kernel->db->dbMutex, gcvINFINITE));

    /* Walk the databases. */
    for (i = 0; i < gcmCOUNTOF(kernel->db->db); ++i)
    {
        for (database = kernel->db->db[i];
             database != gcvNULL;
             database = database->next)
        {
            pid = database->processID;

            gcmkVERIFY_OK(gckOS_GetProcessNameByPid(pid, gcmSIZEOF(name), name));

            seq_printf(m, "%-8d%s\n", pid, name);
        }
    }

    /* Release the database mutex. */
    gcmkVERIFY_OK(gckOS_ReleaseMutex(kernel->os, kernel->db->dbMutex));

    /* Success. */
    return 0;
}

int gc_meminfo_show(struct seq_file* m, void* data)
{
    gcsINFO_NODE *node = m->private;
    gckGALDEVICE device = node->device;
    gckKERNEL kernel = _GetValidKernel(device);
    gckVIDMEM memory;
    gceSTATUS status;
    gcsDATABASE_PTR database;
    gctUINT32 i;

    gctUINT32 free = 0, used = 0, total = 0, minFree = 0, maxUsed = 0;

    gcsDATABASE_COUNTERS virtualCounter = {0, 0, 0};
    gcsDATABASE_COUNTERS nonPagedCounter = {0, 0, 0};

    status = gckKERNEL_GetVideoMemoryPool(kernel, gcvPOOL_SYSTEM, &memory);

    if (gcmIS_SUCCESS(status))
    {
        gcmkVERIFY_OK(
            gckOS_AcquireMutex(memory->os, memory->mutex, gcvINFINITE));

        free    = memory->freeBytes;
        minFree = memory->minFreeBytes;
        used    = memory->bytes - memory->freeBytes;
        maxUsed = memory->bytes - memory->minFreeBytes;
        total   = memory->bytes;

        gcmkVERIFY_OK(gckOS_ReleaseMutex(memory->os, memory->mutex));
    }

    seq_printf(m, "VIDEO MEMORY:\n");
    seq_printf(m, "  POOL SYSTEM:\n");
    seq_printf(m, "    Free :    %10u B\n", free);
    seq_printf(m, "    Used :    %10u B\n", used);
    seq_printf(m, "    MinFree : %10u B\n", minFree);
    seq_printf(m, "    MaxUsed : %10u B\n", maxUsed);
    seq_printf(m, "    Total :   %10u B\n", total);

    /* Acquire the database mutex. */
    gcmkVERIFY_OK(
        gckOS_AcquireMutex(kernel->os, kernel->db->dbMutex, gcvINFINITE));

    /* Walk the databases. */
    for (i = 0; i < gcmCOUNTOF(kernel->db->db); ++i)
    {
        for (database = kernel->db->db[i];
             database != gcvNULL;
             database = database->next)
        {
            gcsDATABASE_COUNTERS * counter;
            counter = &database->vidMemPool[gcvPOOL_VIRTUAL];
            virtualCounter.bytes += counter->bytes;
            virtualCounter.maxBytes += counter->maxBytes;

            counter = &database->nonPaged;
            nonPagedCounter.bytes += counter->bytes;
            nonPagedCounter.bytes += counter->maxBytes;
        }
    }

    /* Release the database mutex. */
    gcmkVERIFY_OK(gckOS_ReleaseMutex(kernel->os, kernel->db->dbMutex));

    seq_printf(m, "  POOL VIRTUAL:\n");
    seq_printf(m, "    Used :    %10llu B\n", virtualCounter.bytes);
    seq_printf(m, "    MaxUsed : %10llu B\n", virtualCounter.bytes);

    return 0;
}

static const char * vidmemTypeStr[gcvVIDMEM_TYPE_COUNT] =
{
    "Generic",
    "Index",
    "Vertex",
    "Texture",
    "RenderTarget",
    "Depth",
    "Bitmap",
    "TileStatus",
    "Image",
    "Mask",
    "Scissor",
    "HZ",
    "ICache",
    "TxDesc",
    "Fence",
    "TFBHeader",
    "Command",
};

static const char * poolStr[gcvPOOL_NUMBER_OF_POOLS] =
{
    "Unknown",
    "Default",
    "Local",
    "Internal",
    "External",
    "Unified",
    "System",
    "Virtual",
    "User",
};

static void
_ShowDummyRecord(
    IN struct seq_file *File,
    IN gcsDATABASE_PTR Database
    )
{
}

static void
_ShowVideoMemoryRecord(
    IN struct seq_file *m,
    IN gcsDATABASE_PTR Database
    )
{
    gctUINT i;
    gctUINT32 handle;
    gckVIDMEM_NODE nodeObject;
    gctPHYS_ADDR_T physical;
    gctINT32 refCount = 0;
    gctINT32 lockCount = 0;
    gceSTATUS status;

    seq_printf(m, "Video Memory Node:\n");
    seq_printf(m, "  handle         nodeObject       size         type     pool     physical  ref lock\n");

    for (i = 0; i < gcmCOUNTOF(Database->list); i++)
    {
        gcsDATABASE_RECORD_PTR r = Database->list[i];

        while (r != NULL)
        {
            gcsDATABASE_RECORD_PTR record = r;
            r = r->next;

            if (record->type != gcvDB_VIDEO_MEMORY)
            {
                continue;
            }

            handle = gcmPTR2INT32(record->data);

            status = gckVIDMEM_HANDLE_Lookup2(
                record->kernel,
                Database,
                handle,
                &nodeObject
                );

            if (gcmIS_ERROR(status))
            {
                seq_printf(m, "%6u Invalid Node\n", handle);
                continue;
            }

            gckVIDMEM_NODE_GetPhysical(record->kernel, nodeObject, 0, &physical);
            gckVIDMEM_NODE_GetReference(record->kernel, nodeObject, &refCount);
            gckVIDMEM_NODE_GetLockCount(record->kernel, nodeObject, &lockCount);

            seq_printf(m, "%#8x %#18lx %10lu %12s %8s %#12llx %4d %4d\n",
                handle,
                (unsigned long)nodeObject,
                (unsigned long)record->bytes,
                vidmemTypeStr[nodeObject->type],
                poolStr[nodeObject->pool],
                physical,
                refCount,
                lockCount
                );
        }
    }
}

static void
_ShowNonPagedRecord(
    IN struct seq_file *m,
    IN gcsDATABASE_PTR Database
    )
{
    gctUINT i;

    seq_printf(m, "NonPaged Memory:\n");
    seq_printf(m, "  name              vaddr       size\n");

    for (i = 0; i < gcmCOUNTOF(Database->list); i++)
    {
        gcsDATABASE_RECORD_PTR r = Database->list[i];

        while (r != NULL)
        {
            gcsDATABASE_RECORD_PTR record = r;
            r = r->next;

            if (record->type != gcvDB_NON_PAGED)
            {
                continue;
            }

            seq_printf(m, "%6u %#18lx %10lu\n",
                gcmPTR2INT32(record->physical),
                (unsigned long)record->data,
                (unsigned long)record->bytes
                );
        }
    }
}

static void
_ShowSignalRecord(
    IN struct seq_file *m,
    IN gcsDATABASE_PTR Database
    )
{
    gctUINT i;

    seq_printf(m, "User signal:\n");

    for (i = 0; i < gcmCOUNTOF(Database->list); i++)
    {
        gcsDATABASE_RECORD_PTR r = Database->list[i];

        while (r != NULL)
        {
            gcsDATABASE_RECORD_PTR record = r;
            r = r->next;

            if (record->type != gcvDB_SIGNAL)
            {
                continue;
            }

            seq_printf(m, "%#10x\n", gcmPTR2INT32(record->data));
        }
    }
}

static void
_ShowLockRecord(
    IN struct seq_file *m,
    IN gcsDATABASE_PTR Database
    )
{
    gctUINT i;
    gceSTATUS status;
    gctUINT32 handle;
    gckVIDMEM_NODE nodeObject;

    seq_printf(m, "Video Memory Lock:\n");
    seq_printf(m, "  handle         nodeObject              vaddr\n");

    for (i = 0; i < gcmCOUNTOF(Database->list); i++)
    {
        gcsDATABASE_RECORD_PTR r = Database->list[i];

        while (r != NULL)
        {
            gcsDATABASE_RECORD_PTR record = r;
            r = r->next;

            if (record->type != gcvDB_VIDEO_MEMORY_LOCKED)
            {
                continue;
            }

            handle = gcmPTR2INT32(record->data);

            status = gckVIDMEM_HANDLE_Lookup2(
                record->kernel,
                Database,
                handle,
                &nodeObject
                );

            if (gcmIS_ERROR(status))
            {
                nodeObject = gcvNULL;
            }

            seq_printf(m, "%#8x %#18lx %#18lx\n",
                handle,
                (unsigned long)nodeObject,
                (unsigned long)record->physical
                );
        }
    }
}

static void
_ShowContextRecord(
    IN struct seq_file *m,
    IN gcsDATABASE_PTR Database
    )
{
    gctUINT i;

    seq_printf(m, "Context:\n");

    for (i = 0; i < gcmCOUNTOF(Database->list); i++)
    {
        gcsDATABASE_RECORD_PTR r = Database->list[i];

        while (r != NULL)
        {
            gcsDATABASE_RECORD_PTR record = r;
            r = r->next;

            if (record->type != gcvDB_CONTEXT)
            {
                continue;
            }

            seq_printf(m, "%6u\n", gcmPTR2INT32(record->data));
        }
    }
}

static void
_ShowMapMemoryRecord(
    IN struct seq_file *m,
    IN gcsDATABASE_PTR Database
    )
{
    gctUINT i;

    seq_printf(m, "Map Memory:\n");
    seq_printf(m, "  name              vaddr       size\n");

    for (i = 0; i < gcmCOUNTOF(Database->list); i++)
    {
        gcsDATABASE_RECORD_PTR r = Database->list[i];

        while (r != NULL)
        {
            gcsDATABASE_RECORD_PTR record = r;
            r = r->next;

            if (record->type != gcvDB_MAP_MEMORY)
            {
                continue;
            }

            seq_printf(m, "%#6lx %#18lx %10lu\n",
                (unsigned long)record->physical,
                (unsigned long)record->data,
                (unsigned long)record->bytes
                );
        }
    }
}

static void
_ShowShbufRecord(
    IN struct seq_file *m,
    IN gcsDATABASE_PTR Database
    )
{
    gctUINT i;

    seq_printf(m, "ShBuf:\n");

    for (i = 0; i < gcmCOUNTOF(Database->list); i++)
    {
        gcsDATABASE_RECORD_PTR r = Database->list[i];

        while (r != NULL)
        {
            gcsDATABASE_RECORD_PTR record = r;
            r = r->next;

            if (record->type != gcvDB_SHBUF)
            {
                continue;
            }

            seq_printf(m, "%#8x\n", gcmPTR2INT32(record->data));
        }
    }
}

static void
_ShowDatabase(
    IN struct seq_file *File,
    IN gcsDATABASE_PTR Database
    )
{
    gctINT pid;
    gctUINT i;
    char name[24];
    gctBOOL hasType[gcvDB_NUM_TYPES] = {0,};
    void (* showFuncs[])(struct seq_file *, gcsDATABASE_PTR) =
    {
        _ShowDummyRecord,
        _ShowVideoMemoryRecord,
        _ShowNonPagedRecord,
        _ShowSignalRecord,
        _ShowLockRecord,
        _ShowContextRecord,
        _ShowDummyRecord,
        _ShowMapMemoryRecord,
        _ShowShbufRecord,
    };

    gcmSTATIC_ASSERT(gcmCOUNTOF(showFuncs) == gcvDB_NUM_TYPES,
                     "DB type mismatch");

    /* Process ID and name */
    pid = Database->processID;
    gcmkVERIFY_OK(gckOS_GetProcessNameByPid(pid, gcmSIZEOF(name), name));

    seq_printf(File, "--------------------------------------------------------------------------------\n");
    seq_printf(File, "Process: %-8d %s\n", pid, name);

    for (i = 0; i < gcmCOUNTOF(Database->list); i++)
    {
        gcsDATABASE_RECORD_PTR record = Database->list[i];

        while (record != NULL)
        {
            hasType[record->type] = gcvTRUE;
            record = record->next;
        }
    }

    for (i = 0; i < gcvDB_NUM_TYPES; i++)
    {
        if (hasType[i])
        {
            showFuncs[i](File, Database);
        }
    }
}

static int
gc_db_show(struct seq_file *m, void *data)
{
    gcsDATABASE_PTR database;
    gctINT i;
    static gctUINT64 idleTime = 0;
    gcsINFO_NODE *node = m->private;
    gckGALDEVICE device = node->device;
    gckKERNEL kernel = _GetValidKernel(device);

    /* Acquire the database mutex. */
    gcmkVERIFY_OK(
        gckOS_AcquireMutex(kernel->os, kernel->db->dbMutex, gcvINFINITE));

    if (kernel->db->idleTime)
    {
        /* Record idle time if DB upated. */
        idleTime = kernel->db->idleTime;
        kernel->db->idleTime = 0;
    }

    /* Idle time since last call */
    seq_printf(m, "GPU Idle: %llu ns\n",  idleTime);

    /* Walk the databases. */
    for (i = 0; i < gcmCOUNTOF(kernel->db->db); ++i)
    {
        for (database = kernel->db->db[i];
             database != gcvNULL;
             database = database->next)
        {
            _ShowDatabase(m, database);
        }
    }

    /* Release the database mutex. */
    gcmkVERIFY_OK(gckOS_ReleaseMutex(kernel->os, kernel->db->dbMutex));

    return 0 ;
}

static int
gc_version_show(struct seq_file *m, void *data)
{
    gcsINFO_NODE *node = m->private;
    gckGALDEVICE device = node->device;
    gcsPLATFORM * platform = device->platform;

    seq_printf(m, "%s built at %s\n",  gcvVERSION_STRING, HOST);

    if (platform->name)
    {
        seq_printf(m, "Platform path: %s\n", platform->name);
    }
    else
    {
        seq_printf(m, "Code path: %s\n", __FILE__);
    }

    return 0 ;
}

static void print_ull(char dest[32], unsigned long long u)
{
    unsigned t[7];
    int i;

    if (u < 1000)
    {
        sprintf(dest, "%27llu", u);
        return;
    }

    for (i = 0; i < 7 && u; i++)
    {
        t[i] = do_div(u, 1000);
    }

    dest += sprintf(dest, "%*s", (7 - i) * 4, "");
    dest += sprintf(dest, "%3u", t[--i]);

    for (i--; i >= 0; i--)
    {
        dest += sprintf(dest, ",%03u", t[i]);
    }
}

/*******************************************************************************
**
** Show PM state timer.
**
** Entry is called as 'idle' for compatible reason, it shows more information
** than idle actually.
**
**  Start: Start time of this counting period.
**  End: End time of this counting peroid.
**  On: Time GPU stays in gcvPOWER_0N.
**  Off: Time GPU stays in gcvPOWER_0FF.
**  Idle: Time GPU stays in gcvPOWER_IDLE.
**  Suspend: Time GPU stays in gcvPOWER_SUSPEND.
*/
static int
gc_idle_show(struct seq_file *m, void *data)
{
    gcsINFO_NODE *node = m->private;
    gckGALDEVICE device = node->device;
    gckKERNEL kernel = _GetValidKernel(device);
    char str[32];

    gctUINT64 on;
    gctUINT64 off;
    gctUINT64 idle;
    gctUINT64 suspend;

    gckHARDWARE_QueryStateTimer(kernel->hardware, &on, &off, &idle, &suspend);

    /* Idle time since last call */
    print_ull(str, on);
    seq_printf(m, "On:      %s ns\n",  str);
    print_ull(str, off);
    seq_printf(m, "Off:     %s ns\n",  str);
    print_ull(str, idle);
    seq_printf(m, "Idle:    %s ns\n",  str);
    print_ull(str, suspend);
    seq_printf(m, "Suspend: %s ns\n",  str);

    return 0 ;
}

extern void
_DumpState(
    IN gckKERNEL Kernel
    );

/*******************************************************************************
**
** Show PM state timer.
**
** Entry is called as 'idle' for compatible reason, it shows more information
** than idle actually.
**
**  Start: Start time of this counting period.
**  End: End time of this counting peroid.
**  On: Time GPU stays in gcvPOWER_0N.
**  Off: Time GPU stays in gcvPOWER_0FF.
**  Idle: Time GPU stays in gcvPOWER_IDLE.
**  Suspend: Time GPU stays in gcvPOWER_SUSPEND.
*/

static int dumpCore = 0;

static int
gc_dump_trigger_show(struct seq_file *m, void *data)
{
#if gcdENABLE_3D
    gcsINFO_NODE *node = m->private;
    gckGALDEVICE device = node->device;
    gckKERNEL kernel = gcvNULL;

    if (dumpCore >= gcvCORE_MAJOR && dumpCore < gcvCORE_COUNT)
    {
        kernel = device->kernels[dumpCore];
    }
#endif

    seq_printf(m, gcdDEBUG_FS_WARN);

#if gcdENABLE_3D
    seq_printf(m, "Get dump from /proc/kmsg or /sys/kernel/debug/gc/galcore_trace\n");

    if (kernel && kernel->hardware->options.powerManagement == gcvFALSE)
    {
        _DumpState(kernel);
    }
#endif

    return 0;
}

static int dumpProcess = 0;

static void
_ShowVideoMemory(
    struct seq_file *File,
    gcsDATABASE_PTR Database
    )
{
    gctUINT i = 0;

    static const char * otherCounterNames[] = {
        "AllocNonPaged",
        "MapMemory",
    };

    gcsDATABASE_COUNTERS * otherCounters[] = {
        &Database->nonPaged,
        &Database->mapMemory,
    };

    seq_printf(File, "%-16s %16s %16s %16s\n", "", "Current", "Maximum", "Total");

    /* Print surface type counters. */
    seq_printf(File, "%-16s %16llu %16llu %16llu\n",
               "All-Types",
               Database->vidMem.bytes,
               Database->vidMem.maxBytes,
               Database->vidMem.totalBytes);

    for (i = 1; i < gcvVIDMEM_TYPE_COUNT; i++)
    {
        seq_printf(File, "%-16s %16llu %16llu %16llu\n",
                   vidmemTypeStr[i],
                   Database->vidMemType[i].bytes,
                   Database->vidMemType[i].maxBytes,
                   Database->vidMemType[i].totalBytes);
    }
    seq_puts(File, "\n");

    /* Print surface pool counters. */
    seq_printf(File, "%-16s %16llu %16llu %16llu\n",
               "All-Pools",
               Database->vidMem.bytes,
               Database->vidMem.maxBytes,
               Database->vidMem.totalBytes);

    for (i = 1; i < gcvPOOL_NUMBER_OF_POOLS; i++)
    {
        seq_printf(File, "%-16s %16llu %16llu %16llu\n",
                   poolStr[i],
                   Database->vidMemPool[i].bytes,
                   Database->vidMemPool[i].maxBytes,
                   Database->vidMemPool[i].totalBytes);
    }
    seq_puts(File, "\n");

    /* Print other counters. */
    for (i = 0; i < gcmCOUNTOF(otherCounterNames); i++)
    {
        seq_printf(File, "%-16s %16llu %16llu %16llu\n",
                   otherCounterNames[i],
                   otherCounters[i]->bytes,
                   otherCounters[i]->maxBytes,
                   otherCounters[i]->totalBytes);
    }
    seq_puts(File, "\n");
}

static int gc_vidmem_show(struct seq_file *m, void *unused)
{
    gceSTATUS status;
    gcsDATABASE_PTR database;
    gcsINFO_NODE *node = m->private;
    gckGALDEVICE device = node->device;
    char name[64];
    int i;

    gckKERNEL kernel = _GetValidKernel(device);

    if (dumpProcess == 0)
    {
        /* Acquire the database mutex. */
        gcmkVERIFY_OK(
        gckOS_AcquireMutex(kernel->os, kernel->db->dbMutex, gcvINFINITE));

        for (i = 0; i < gcmCOUNTOF(kernel->db->db); i++)
        {
            for (database = kernel->db->db[i];
                 database != gcvNULL;
                 database = database->next)
            {
                gckOS_GetProcessNameByPid(database->processID, gcmSIZEOF(name), name);
                seq_printf(m, "Memory Usage (Process %u: %s):\n", database->processID, name);
                _ShowVideoMemory(m, database);
                seq_puts(m, "\n");
            }
        }

        /* Release the database mutex. */
        gcmkVERIFY_OK(gckOS_ReleaseMutex(kernel->os, kernel->db->dbMutex));
    }
    else
    {
        /* Find the database. */
        status = gckKERNEL_FindDatabase(kernel, dumpProcess, gcvFALSE, &database);

        if (gcmIS_ERROR(status))
        {
            seq_printf(m, "ERROR: process %d not found\n", dumpProcess);
            return 0;
        }

        gckOS_GetProcessNameByPid(dumpProcess, gcmSIZEOF(name), name);
        seq_printf(m, "Memory Usage (Process %d: %s):\n", dumpProcess, name);
        _ShowVideoMemory(m, database);
    }

    return 0;
}

static inline int strtoint_from_user(const char __user *s,
                        size_t count, int *res)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,0,0)
    int ret = kstrtoint_from_user(s, count, 10, res);

    return ret < 0 ? ret : count;
#else
    /* sign, base 2 representation, newline, terminator */
    char buf[1 + sizeof(long) * 8 + 1 + 1];

    size_t len = min(count, sizeof(buf) - 1);

    if (copy_from_user(buf, s, len))
        return -EFAULT;
    buf[len] = '\0';

    *res = (int) simple_strtol(buf, NULL, 0);

    return count;
#endif
}

static int gc_vidmem_write(const char __user *buf, size_t count, void* data)
{
    return strtoint_from_user(buf, count, &dumpProcess);
}

static int gc_dump_trigger_write(const char __user *buf, size_t count, void* data)
{
    return strtoint_from_user(buf, count, &dumpCore);
}

static int gc_clk_show(struct seq_file* m, void* data)
{
    gcsINFO_NODE *node = m->private;
    gckGALDEVICE device = node->device;
    gctUINT i;

    for (i = gcvCORE_MAJOR; i < gcvCORE_COUNT; i++)
    {
        if (device->kernels[i])
        {
            gckHARDWARE hardware = device->kernels[i]->hardware;


            if (hardware->mcClk)
            {
                seq_printf(m, "gpu%d mc clock: %d HZ.\n", i, hardware->mcClk);
            }

            if (hardware->shClk)
            {
                seq_printf(m, "gpu%d sh clock: %d HZ.\n", i, hardware->shClk);
            }
        }
    }

    return 0;
}

static gcsINFO InfoList[] =
{
    {"info", gc_info_show},
    {"clients", gc_clients_show},
    {"meminfo", gc_meminfo_show},
    {"idle", gc_idle_show},
    {"database", gc_db_show},
    {"version", gc_version_show},
    {"vidmem", gc_vidmem_show, gc_vidmem_write},
    {"dump_trigger", gc_dump_trigger_show, gc_dump_trigger_write},
    {"clk", gc_clk_show},
};

static gceSTATUS
_DebugfsInit(
    IN gckGALDEVICE Device
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gckDEBUGFS_DIR dir = &Device->debugfsDir;

    gcmkONERROR(gckDEBUGFS_DIR_Init(dir, gcvNULL, "gc"));
    gcmkONERROR(gckDEBUGFS_DIR_CreateFiles(dir, InfoList, gcmCOUNTOF(InfoList), Device));

OnError:
    return status;
}

static void
_DebugfsCleanup(
    IN gckGALDEVICE Device
    )
{
    gckDEBUGFS_DIR dir = &Device->debugfsDir;

    if (Device->debugfsDir.root)
    {
        gcmkVERIFY_OK(gckDEBUGFS_DIR_RemoveFiles(dir, InfoList, gcmCOUNTOF(InfoList)));

        gckDEBUGFS_DIR_Deinit(dir);
    }
}


/******************************************************************************\
*************************** Memory Allocation Wrappers *************************
\******************************************************************************/

static gceSTATUS
_AllocateMemory(
    IN gckGALDEVICE Device,
    IN gctSIZE_T Bytes,
    OUT gctPOINTER *Logical,
    OUT gctPHYS_ADDR *Physical,
    OUT gctUINT32 *PhysAddr
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gctPHYS_ADDR_T physAddr;
	
    gcmkHEADER_ARG("Device=%p Bytes=0x%zx", Device, Bytes);

    gcmkVERIFY_ARGUMENT(Device != NULL);
    gcmkVERIFY_ARGUMENT(Logical != NULL);
    gcmkVERIFY_ARGUMENT(Physical != NULL);
    gcmkVERIFY_ARGUMENT(PhysAddr != NULL);
//printk(KERN_INFO "zxw:_AllocateMemory begin\n");
    gcmkONERROR(gckOS_AllocateNonPagedMemory(
        Device->os, gcvFALSE, gcvALLOC_FLAG_CONTIGUOUS, &Bytes, Physical, Logical
        ));
//printk(KERN_INFO "zxw:_AllocateMemory 1,status %d\n",status);
    gcmkONERROR(gckOS_GetPhysicalFromHandle(
        Device->os, *Physical, 0, &physAddr
        ));
//printk(KERN_INFO "zxw:_AllocateMemory 3, status %d\n",status);
    gcmkSAFECASTPHYSADDRT(*PhysAddr, physAddr);
//printk(KERN_INFO "zxw:_AllocateMemory 5,status %d\n",status);
OnError:
    gcmkFOOTER_ARG(
        "*Logical=%p *Physical=%p *PhysAddr=0x%llx",
        gcmOPT_POINTER(Logical), gcmOPT_POINTER(Physical), gcmOPT_VALUE(PhysAddr)
        );
	//printk(KERN_INFO "zxw:_AllocateMemory end,%d\n",status);
    return status;
}

static gceSTATUS
_FreeMemory(
    IN gckGALDEVICE Device,
    IN gctPOINTER Logical,
    IN gctPHYS_ADDR Physical
    )
{
    gceSTATUS status;

    gcmkHEADER_ARG("Device=%p Logical=%p Physical=%p",
                   Device, Logical, Physical);

    gcmkVERIFY_ARGUMENT(Device != NULL);

    status = gckOS_FreeNonPagedMemory(
        Device->os, Physical, Logical,
        ((PLINUX_MDL) Physical)->numPages * PAGE_SIZE
        );

    gcmkFOOTER();
    return status;
}

static gceSTATUS
_SetupVidMem(
    IN gckGALDEVICE Device,
    IN gctUINT32 ContiguousBase,
    IN gctSIZE_T ContiguousSize,
    IN gctSIZE_T BankSize,
    IN gcsDEVICE_CONSTRUCT_ARGS * Args
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gctUINT32 physAddr = ~0U;
    gckGALDEVICE device = Device;

    gcmkHEADER_ARG("Device=%p ContiguousBase=0x%x ContiguousSize=0x%zx BankSize=0x%zx Args=%p",
                   Device, ContiguousBase, ContiguousSize, BankSize, Args);

    /* set up the contiguous memory */
    device->contiguousBase = ContiguousBase;
    device->contiguousSize = ContiguousSize;
	printk(KERN_INFO"zxw:_SetupVidMem baseaddr-size 0x%x-0x%zx\n",ContiguousBase,ContiguousSize);
    if (ContiguousSize > 0)
    {
        if (ContiguousBase == 0)
        {
            while (device->contiguousSize > 0)
            {
				//printk("this should enter,before _AllocateMemory");
                /* Allocate contiguous memory. */
                status = _AllocateMemory(
                    device,
                    device->contiguousSize,
                    &device->contiguousLogical,
                    &device->contiguousPhysical,
                    &physAddr
                    );
				//printk("after _AllocateMemory,status is %d\n",status);
				if(status != 0)
				{
					status = _AllocateMemory(
						device,
						device->contiguousSize,
						&device->contiguousLogical,
						&device->contiguousPhysical,
						&physAddr
						);	
					printk("after _AllocateMemory 2,status is %d\n",status);
				}
				
                if (gcmIS_SUCCESS(status))
                {
                    status = gckVIDMEM_Construct(
                        device->os,
                        physAddr,
                        device->contiguousSize,
                        64,
                        BankSize,
                        &device->contiguousVidMem
                        );

                    if (gcmIS_SUCCESS(status))
                    {
                        gckALLOCATOR allocator = ((PLINUX_MDL)device->contiguousPhysical)->allocator;
                        device->contiguousVidMem->capability = allocator->capability | gcvALLOC_FLAG_MEMLIMIT;
                        device->contiguousVidMem->physical = device->contiguousPhysical;
                        device->contiguousBase = physAddr;
                        break;
                    }

                    gcmkONERROR(_FreeMemory(
                        device,
                        device->contiguousLogical,
                        device->contiguousPhysical
                        ));

                    device->contiguousLogical  = gcvNULL;
                    device->contiguousPhysical = gcvNULL;
                }

                if (device->contiguousSize <= (4 << 20))
                {
                    device->contiguousSize = 0;
                }
                else
                {
                    device->contiguousSize -= (4 << 20);
                }
            }
        }
        else
        {
            /* Create the contiguous memory heap. */
            status = gckVIDMEM_Construct(
                device->os,
                ContiguousBase,
                ContiguousSize,
                64, BankSize,
                &device->contiguousVidMem
                );

            if (gcmIS_ERROR(status))
            {
                /* Error, disable contiguous memory pool. */
                device->contiguousVidMem = gcvNULL;
                device->contiguousSize   = 0;
            }
            else
            {
                gckALLOCATOR allocator;

                gcmkONERROR(gckOS_RequestReservedMemory(
                    device->os, ContiguousBase, ContiguousSize,
                    "galcore contiguous memory",
                    Args->contiguousRequested,
                    &device->contiguousPhysical
                    ));

                allocator = ((PLINUX_MDL)device->contiguousPhysical)->allocator;
                device->contiguousVidMem->capability = allocator->capability | gcvALLOC_FLAG_MEMLIMIT;
                device->contiguousVidMem->physical = device->contiguousPhysical;
                device->requestedContiguousBase = ContiguousBase;
                device->requestedContiguousSize = ContiguousSize;

                device->contiguousPhysicalName = 0;
                device->contiguousSize = ContiguousSize;
            }
        }
        printk(KERN_INFO "Galcore ContiguousBase=0x%llx Contiguoussize=0x%x\n", device->contiguousBase, (gctUINT32)device->contiguousSize);
    }

OnError:
    gcmkFOOTER();
	printk(KERN_INFO "zxw:_SetupVidMem status is %d\n",status);
    return status;
}

void
_SetupRegisterPhysical(
    IN gckGALDEVICE Device,
    IN gcsDEVICE_CONSTRUCT_ARGS * Args
    )
{
    gctINT *irqs = Args->irqs;
    gctUINT *registerBases = Args->registerBases;
    gctUINT *registerSizes = Args->registerSizes;

    gctINT i = 0;

    for (i = 0; i < gcvCORE_COUNT; i++)
    {
        if (irqs[i] != -1)
        {
            Device->requestedRegisterMemBases[i] = registerBases[i];
            Device->requestedRegisterMemSizes[i] = registerSizes[i];
			
            gcmkTRACE_ZONE(gcvLEVEL_INFO, _GC_OBJ_ZONE,
                           "Get register base %llx of core %d",
                           registerBases[i], i);
        }
    }
}

/******************************************************************************\
******************************* Interrupt Handler ******************************
\******************************************************************************/
static irqreturn_t isrRoutine(int irq, void *ctxt)
{
    gceSTATUS status;
    gckGALDEVICE device;
    gceCORE core = (gceCORE)gcmPTR2INT32(ctxt) - 1;

    device = galDevice;
	//printk("zxw:isrRoutine enter,core:%d\n",core);
    /* Call kernel interrupt notification. */
    status = gckHARDWARE_Interrupt(device->kernels[core]->hardware);
	//printk("zxw:isrRoutine enter,status:%d\n",status);
    if (gcmIS_SUCCESS(status))
    {
        up(&device->semas[core]);
        return IRQ_HANDLED;
    }

    return IRQ_NONE;
}

static irqreturn_t isrRoutineVG(int irq, void *ctxt)
{
	//printk("zxw:good for isrRoutineVG enter\n");
    return IRQ_NONE;
}

static const char *isrNames[] =
{
    "galcore:0",
    "galcore:3d-1",
    "galcore:3d-2",
    "galcore:3d-3",
    "galcore:3d-4",
    "galcore:3d-5",
    "galcore:3d-6",
    "galcore:3d-7",
    "galcore:2d",
    "galcore:vg",
#if gcdDEC_ENABLE_AHB
    "galcore:dec"
#endif
};

static gceSTATUS
_SetupIsr(
    IN gceCORE Core
    )
{
    gctINT ret = 0;
    gceSTATUS status = gcvSTATUS_OK;
    gckGALDEVICE Device = galDevice;
    irq_handler_t handler;

    gcmkHEADER_ARG("Device=%p Core=%d", Device, Core);

    gcmkVERIFY_ARGUMENT(Device != NULL);
	printk(KERN_INFO "zxw:_SetupIsr Start,Core is %d\n",Core);
	
	Core = gcvCORE_MAJOR;
	Device->irqLines[Core] = galcore_irq;
    if (Device->irqLines[Core] < 0)
    {
		printk(KERN_INFO "zxw:_SetupIsr irqLine error\n");
        gcmkONERROR(gcvSTATUS_GENERIC_IO);
    }
    printk(KERN_INFO "zxw:_SetupIsr 1\n");
    gcmSTATIC_ASSERT(gcvCORE_COUNT == gcmCOUNTOF(isrNames),
                     "isrNames array does not match core types");

    handler = (Core == gcvCORE_VG) ? isrRoutineVG : isrRoutine;

    /*
     * Hook up the isr based on the irq line.
     * For shared irq, device-id can not be 0, but CORE_MAJOR value is.
     * Add by 1 here and subtract by 1 in isr to fix the issue.
     */
	printk(KERN_INFO "zxw:before request_irq,%d\n",galcore_irq);
	
	//gcdIRQF_FLAG to IRQF_SHARED
    ret = request_irq(
        Device->irqLines[Core], handler, IRQF_SHARED,
        "galcore", (void *)(uintptr_t)(Core + 1)
        );
	printk(KERN_INFO "zxw:after request_irq,%d\n",ret);
    if (ret != 0)
    {
        gcmkTRACE_ZONE(
            gcvLEVEL_ERROR, gcvZONE_DRIVER,
            "%s(%d): Could not register irq line %d (error=%d)\n",
            __FUNCTION__, __LINE__,
            Device->irqLines[Core], ret
            );

        gcmkONERROR(gcvSTATUS_GENERIC_IO);
    }

    /* Mark ISR as initialized. */
    Device->isrInitializeds[Core] = gcvTRUE;

OnError:
    gcmkFOOTER();
    return status;
}

static gceSTATUS
_ReleaseIsr(
    IN gceCORE Core
    )
{
    gckGALDEVICE Device = galDevice;

    gcmkHEADER_ARG("Device=%p Core=%d", Device, Core);

    gcmkVERIFY_ARGUMENT(Device != NULL);

    /* release the irq */
    if (Device->isrInitializeds[Core])
    {
        free_irq(Device->irqLines[Core], (void *)(uintptr_t)(Core + 1));
        Device->isrInitializeds[Core] = gcvFALSE;
    }

    //gcmkFOOTER_NO();
    return gcvSTATUS_OK;
}

static int threadRoutine(void *ctxt)
{
    gckGALDEVICE device = galDevice;
    gceCORE core = (gceCORE) gcmPTR2INT32(ctxt);

    gcmkTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_DRIVER,
                   "Starting isr Thread with extension=%p",
                   device);


    for (;;)
    {
        int down;

        down = down_interruptible(&device->semas[core]);
        if (down) {} /* To make gcc 4.6 happy. */

        if (unlikely(device->killThread))
        {
            /* The daemon exits. */
            while (!kthread_should_stop())
            {
                gckOS_Delay(device->os, 1);
            }

            return 0;
        }

        gckKERNEL_Notify(device->kernels[core], gcvNOTIFY_INTERRUPT);
    }
}

static gceSTATUS
_StartThread(
    IN gckGALDEVICE Device,
    IN gceCORE Core
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gckGALDEVICE device = galDevice;
    struct task_struct * task;

    if (device->kernels[Core] != gcvNULL)
    {
        /* Start the kernel thread. */
        task = kthread_run(threadRoutine, (void *)Core,
                "galcore_deamon/%d", Core);

        if (IS_ERR(task))
        {
            gcmkTRACE_ZONE(
                gcvLEVEL_ERROR, gcvZONE_DRIVER,
                "%s(%d): Could not start the kernel thread.\n",
                __FUNCTION__, __LINE__
                );

            gcmkONERROR(gcvSTATUS_GENERIC_IO);
        }

        device->threadCtxts[Core]         = task;
        device->threadInitializeds[Core]  = gcvTRUE;
    }
    else
    {
        device->threadInitializeds[Core]  = gcvFALSE;
    }

OnError:
    return status;
}

static void
_StopThread(
    gckGALDEVICE Device,
    gceCORE Core
    )
{
    if (Device->threadInitializeds[Core])
    {
        Device->killThread = gcvTRUE;
        up(&Device->semas[Core]);

        kthread_stop(Device->threadCtxts[Core]);
        Device->threadCtxts[Core]        = gcvNULL;
        Device->threadInitializeds[Core] = gcvFALSE;
    }
}

/*******************************************************************************
**
**  gckGALDEVICE_Destroy
**
**  Class destructor.
**
**  INPUT:
**
**      Nothing.
**
**  OUTPUT:
**
**      Nothing.
**
**  RETURNS:
**
**      Nothing.
*/
gceSTATUS gckGALDEVICE_Destroy(gckGALDEVICE Device)
{
    gctINT i;
    gckKERNEL kernel = gcvNULL;

    //gcmkHEADER_ARG("Device=%p", Device);

    if (Device != gcvNULL)
    {
        /* Grab the first availiable kernel */
        for (i = 0; i < gcdMAX_GPU_COUNT; i++)
        {
            if (Device->irqLines[i] != -1)
            {
                kernel = Device->kernels[i];
                break;
            }
        }

        if (kernel)
        {
            if (Device->internalPhysicalName != 0)
            {
                gcmRELEASE_NAME(Device->internalPhysicalName);
                Device->internalPhysicalName = 0;
            }
            if (Device->externalPhysicalName != 0)
            {
                gcmRELEASE_NAME(Device->externalPhysicalName);
                Device->externalPhysicalName = 0;
            }
            if (Device->contiguousPhysicalName != 0)
            {
                gcmRELEASE_NAME(Device->contiguousPhysicalName);
                Device->contiguousPhysicalName = 0;
            }
        }

        for (i = 0; i < gcdMAX_GPU_COUNT; i++)
        {
            if (Device->kernels[i] != gcvNULL)
            {
                Device->kernels[i] = gcvNULL;
            }
        }

        if (Device->internalLogical != gcvNULL)
        {
            /* Unmap the internal memory. */
            iounmap(Device->internalLogical);
            Device->internalLogical = gcvNULL;
        }

        if (Device->internalVidMem != gcvNULL)
        {
            /* Destroy the internal heap. */
            gcmkVERIFY_OK(gckVIDMEM_Destroy(Device->internalVidMem));
            Device->internalVidMem = gcvNULL;
        }

        if (Device->externalPhysical != gcvNULL)
        {
            gckOS_ReleaseReservedMemory(
                Device->os,
                Device->externalPhysical
                );
        }

        if (Device->externalLogical != gcvNULL)
        {
            Device->externalLogical = gcvNULL;
        }

        if (Device->externalVidMem != gcvNULL)
        {
            /* destroy the external heap */
            gcmkVERIFY_OK(gckVIDMEM_Destroy(Device->externalVidMem));
            Device->externalVidMem = gcvNULL;
        }

        if (Device->device)
        {
            gcmkVERIFY_OK(gckDEVICE_Destroy(Device->os, Device->device));

            for (i = 0; i < gcdMAX_GPU_COUNT; i++)
            {
                if (globalTA[i])
                {
                    gcTA_Destroy(globalTA[i]);
                    globalTA[i] = gcvNULL;
                }
            }

            Device->device = gcvNULL;
        }

        /*
         * Destroy contiguous memory pool after gckDEVICE destroyed. gckDEVICE
         * may allocates GPU memory types from SYSTEM pool.
         */
        if (Device->contiguousPhysical != gcvNULL)
        {
            if (Device->requestedContiguousBase == 0)
            {
                gcmkVERIFY_OK(_FreeMemory(
                    Device,
                    Device->contiguousLogical,
                    Device->contiguousPhysical
                    ));
            }
            else
            {
                gckOS_ReleaseReservedMemory(
                    Device->os,
                    Device->contiguousPhysical
                    );

                Device->requestedContiguousBase = 0;
                Device->requestedContiguousSize = 0;
            }

            Device->contiguousLogical  = gcvNULL;
            Device->contiguousPhysical = gcvNULL;
        }

        if (Device->contiguousVidMem != gcvNULL)
        {
            /* Destroy the contiguous heap. */
            gcmkVERIFY_OK(gckVIDMEM_Destroy(Device->contiguousVidMem));
            Device->contiguousVidMem = gcvNULL;
        }

        for (i = 0; i < gcdMAX_GPU_COUNT; i++)
        {
            if (Device->registerBases[i])
            {
                /* Unmap register memory. */
                if (Device->requestedRegisterMemBases[i] != 0)
                {
#if USE_LINUX_PCIE
                    pci_iounmap(Device->platform->device, Device->registerBases[i]);
#else

                    iounmap(Device->registerBases[i]);
                    release_mem_region(Device->requestedRegisterMemBases[i],
                            Device->requestedRegisterMemSizes[i]);
#endif
                }

                Device->registerBases[i] = gcvNULL;
                Device->requestedRegisterMemBases[i] = 0;
                Device->requestedRegisterMemSizes[i] = 0;
            }
        }


        if (Device->taos)
        {
            gcmkVERIFY_OK(gctaOS_DestroyOS(Device->taos));
            Device->taos = gcvNULL;
        }

        /* Destroy the gckOS object. */
        if (Device->os != gcvNULL)
        {
            gcmkVERIFY_OK(gckOS_Destroy(Device->os));
            Device->os = gcvNULL;
        }

        if (Device->dbgNode)
        {
            gckDEBUGFS_FreeNode(Device->dbgNode);

            if(Device->dbgNode != gcvNULL)
            {
                kfree(Device->dbgNode);
                Device->dbgNode = gcvNULL;
            }
        }

        _DebugfsCleanup(Device);

        /* Free the device. */
        kfree(Device);
    }

    //gcmkFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gckGALDEVICE_Construct
**
**  Constructor.
**
**  INPUT:
**
**  OUTPUT:
**
**      gckGALDEVICE * Device
**          Pointer to a variable receiving the gckGALDEVICE object pointer on
**          success.
*/
gceSTATUS
gckGALDEVICE_Construct(
    IN gctINT IrqLine,
    IN gctUINT32 RegisterMemBase,
    IN gctSIZE_T RegisterMemSize,
    IN gctINT IrqLine2D,
    IN gctUINT32 RegisterMemBase2D,
    IN gctSIZE_T RegisterMemSize2D,
    IN gctINT IrqLineVG,
    IN gctUINT32 RegisterMemBaseVG,
    IN gctSIZE_T RegisterMemSizeVG,
    IN gctPHYS_ADDR_T ContiguousBase,
    IN gctSIZE_T ContiguousSize,
    IN gctPHYS_ADDR_T ExternalBase,
    IN gctSIZE_T ExternalSize,
    IN gctSIZE_T BankSize,
    IN gctINT FastClear,
    IN gctINT Compression,
    IN gctUINT32 PhysBaseAddr,
    IN gctUINT32 PhysSize,
    IN gctINT Signal,
    IN gctUINT LogFileSize,
    IN gctINT PowerManagement,
    IN gctINT GpuProfiler,
    IN gcsDEVICE_CONSTRUCT_ARGS * Args,
    OUT gckGALDEVICE *Device
    )
{
    gctUINT32 internalBaseAddress = 0, internalAlignment = 0;
    gctUINT32 externalAlignment = 0;
    gctUINT32 physical;
    gckGALDEVICE device;
    gctINT32 i;
    gceHARDWARE_TYPE type;
    gckKERNEL kernel = gcvNULL;
    gceSTATUS status = gcvSTATUS_OK;
	//printk(KERN_INFO "zxw:init gckGALDEVICE_Construct,status is %d\n",status);
    gcmkHEADER_ARG("IrqLine=%d RegisterMemBase=0x%08x RegisterMemSize=0x%zx "
                   "IrqLine2D=%d RegisterMemBase2D=0x%08x RegisterMemSize2D=0x%zx "
                   "IrqLineVG=%d RegisterMemBaseVG=0x%08x RegisterMemSizeVG=0x%zx "
                   "ContiguousBase=0x%08x ContiguousSize=0x%zx BankSize=0x%zx "
                   "FastClear=%d Compression=%d PhysBaseAddr=0x%x PhysSize=%d Signal=%d",
                   IrqLine, RegisterMemBase, RegisterMemSize,
                   IrqLine2D, RegisterMemBase2D, RegisterMemSize2D,
                   IrqLineVG, RegisterMemBaseVG, RegisterMemSizeVG,
                   ContiguousBase, ContiguousSize, BankSize, FastClear, Compression,
                   PhysBaseAddr, PhysSize, Signal);

#if !gcdENABLE_3D
    IrqLine = -1; 
#endif

    IrqLine2D = -1;
    /* Allocate device structure. */
	printk(KERN_INFO "zxw:kmalloc before,size is %ld,count %d\n",sizeof(struct _gckGALDEVICE),gcvCORE_COUNT);
    device = kmalloc(sizeof(struct _gckGALDEVICE), GFP_KERNEL | __GFP_NOWARN);
    if (!device)
    {
		printk(KERN_INFO "zxw:gckGALDEVICE_Construct,kmalloc is fail\n");
        gcmkONERROR(gcvSTATUS_OUT_OF_MEMORY);
    }
	printk(KERN_INFO "zxw:after kmalloc,status is %d\n",status);
    memset(device, 0, sizeof(struct _gckGALDEVICE));

    device->dbgNode = gcvNULL;

    device->platform = Args->platform;

    device->args = *Args;

    /* set up the contiguous memory */
    device->contiguousSize = ContiguousSize;

    /* Clear irq lines. */
    for (i = 0; i < gcdMAX_GPU_COUNT; i++)
    {
        device->irqLines[i] = -1;
    }
	printk(KERN_INFO "zxw:before debug create,status is %d\n",status);
    gcmkONERROR(_DebugfsInit(device));
	printk(KERN_INFO "zxw:after debug create,status is %d\n",status);
    if (gckDEBUGFS_CreateNode(
            device, LogFileSize, device->debugfsDir.root ,DEBUG_FILE, &(device->dbgNode)))
    {
        gcmkTRACE_ZONE(
            gcvLEVEL_ERROR, gcvZONE_DRIVER,
            "%s(%d): Failed to create  the debug file system  %s/%s \n",
            __FUNCTION__, __LINE__,
            PARENT_FILE, DEBUG_FILE
        );
    }
    else if (LogFileSize)
    {
        gckDEBUGFS_SetCurrentNode(device->dbgNode);
    }

    //_SetupRegisterPhysical(device, Args);
	printk(KERN_INFO "zxw:IrqLine is %d,IrqLine2D:%d,IrqLineVG:%d\n",IrqLine,IrqLine2D,IrqLineVG);
	
	IrqLine2D = IrqLineVG = -1;     //zxw
    if (IrqLine != -1)
    {
        device->requestedRegisterMemBases[gcvCORE_MAJOR] = RegisterMemBase;
        device->requestedRegisterMemSizes[gcvCORE_MAJOR] = RegisterMemSize;
    }

    if (IrqLine2D != -1)
    {
        device->requestedRegisterMemBases[gcvCORE_2D] = RegisterMemBase2D;
        device->requestedRegisterMemSizes[gcvCORE_2D] = RegisterMemSize2D;
    }

    if (IrqLineVG != -1)
    {
        device->requestedRegisterMemBases[gcvCORE_VG] = RegisterMemBaseVG;
        device->requestedRegisterMemSizes[gcvCORE_VG] = RegisterMemSizeVG;
    }
#if 0  //gcdDEC_ENABLE_AHB
    {
        device->requestedRegisterMemBases[gcvCORE_DEC] = Args->registerMemBaseDEC300;
        device->requestedRegisterMemSizes[gcvCORE_DEC] = Args->registerMemSizeDEC300;
    }



    for (i = gcvCORE_MAJOR; i < gcvCORE_COUNT; i++)
    {
        if (Args->irqs[i] != -1)
        {
            device->requestedRegisterMemBases[i] = Args->registerBases[i];
            device->requestedRegisterMemSizes[i] = Args->registerSizes[i];

            gcmkTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_DEVICE,
                           "%s(%d): Core = %d, RegiseterBase = %x",
                           __FUNCTION__, __LINE__,
                           i, Args->registerBases[i]
                           );
        }
    }
#endif

    /* Initialize the ISR. */
    device->irqLines[gcvCORE_MAJOR] = IrqLine;
    device->irqLines[gcvCORE_2D] = IrqLine2D;
    device->irqLines[gcvCORE_VG] = IrqLineVG;    //zxw

    //for (i = gcvCORE_MAJOR; i < gcvCORE_COUNT; i++)
    //{
        //if (Args->irqs[i] != -1)
        //{
        //    device->irqLines[i] = Args->irqs[i];
        //}
   // }

    device->requestedContiguousBase  = 0;
    device->requestedContiguousSize  = 0;

    for (i = 0; i < gcdMAX_GPU_COUNT; i++)
    {
        physical = device->requestedRegisterMemBases[i];
		//printk(KERN_INFO "zxw:physical is 0x%x\n",physical);

        /* Set up register memory region. */
        if (physical != 0)
        {

            if ( Args->registerMemMapped )
            {
                device->registerBases[i] = Args->registerMemAddress;
                device->requestedRegisterMemBases[i] = 0;

            }
            else
            {
#if USE_LINUX_PCIE
                device->registerBases[i] = (gctPOINTER) pci_iomap(device->platform->device, 1,
                        device->requestedRegisterMemSizes[i]);
#else
				//printk(KERN_INFO "zxw:request_mem_region enter,size is %ld\n",device->requestedRegisterMemSizes[i]);
				if(device->requestedRegisterMemSizes[i] == 0)
				{
					device->requestedRegisterMemSizes[i] = 0x668;
					
				}
                if (!request_mem_region(physical, device->requestedRegisterMemSizes[i], "galcore register region"))
                {
					printk(KERN_INFO "zxw:request_mem_region error\n");
                    gcmkTRACE_ZONE(
                            gcvLEVEL_ERROR, gcvZONE_DRIVER,
                            "%s(%d): Failed to claim %lu bytes @ 0x%zx\n",
                            __FUNCTION__, __LINE__,
                            physical, device->requestedRegisterMemSizes[i]
                     );

                    gcmkONERROR(gcvSTATUS_OUT_OF_RESOURCES);
                }
                device->registerBases[i] = (gctPOINTER)ioremap_nocache(
                        physical, device->requestedRegisterMemSizes[i]);
#endif

                if (device->registerBases[i] == gcvNULL)
                {
                    gcmkTRACE_ZONE(
                            gcvLEVEL_ERROR, gcvZONE_DRIVER,
                            "%s(%d): Unable to map %ld bytes @ 0x%zx\n",
                            __FUNCTION__, __LINE__,
                            physical, device->requestedRegisterMemSizes[i]
                    );

                    gcmkONERROR(gcvSTATUS_OUT_OF_RESOURCES);
                }
            }

            physical += device->requestedRegisterMemSizes[i];
        }
    }

    /* Set the base address */
    device->baseAddress = device->physBase = PhysBaseAddr;
    device->physSize = PhysSize;
	//printk(KERN_INFO "zxw:before gckOS_Construct\n");
    /* Construct the gckOS object. */
    gcmkONERROR(gckOS_Construct(device, &device->os));
	//printk(KERN_INFO "zxw: gckOS_Construct after,%d\n",status);
    /* Construct the gckDEVICE object for os independent core management. */
    gcmkONERROR(gckDEVICE_Construct(device->os, &device->device));
	//printk(KERN_INFO "zxw: gckDEVICE_Construct after,%d\n",status);
    if (device->irqLines[gcvCORE_MAJOR] != -1)
    {
		printk(KERN_INFO "zxw: gctaOS_ConstructOS before,%d\n",status);
        gcmkONERROR(gctaOS_ConstructOS(device->os, &device->taos));
    }
	//printk(KERN_INFO "zxw: gctaOS_ConstructOS after,%d\n",status);
    ContiguousSize = 0x1600000;   //shoudnot so large
	ContiguousBase = 0;
    gcmkONERROR(_SetupVidMem(device, ContiguousBase, ContiguousSize, BankSize, Args));
	//printk(KERN_INFO "zxw: _SetupVidMem after,%d\n",status);
    /* Set external base and size */
    device->externalBase = ExternalBase;
    device->externalSize = ExternalSize;

    if (device->irqLines[gcvCORE_MAJOR] != -1)
    {
		//printk(KERN_INFO "zxw: gcTA_Construct before,%d\n",status);
        gcmkONERROR(gcTA_Construct(device->taos, gcvCORE_MAJOR, &globalTA[gcvCORE_MAJOR]));
		//printk(KERN_INFO "zxw: gckDEVICE_AddCore before,%d\n",status);
        gcmkONERROR(gckDEVICE_AddCore(device->device, gcvCORE_MAJOR, Args->chipIDs[gcvCORE_MAJOR], device, &device->kernels[gcvCORE_MAJOR]));
		//printk(KERN_INFO "zxw: gckHARDWARE_SetFastClear before,%d\n",status);
        gcmkONERROR(gckHARDWARE_SetFastClear(
            device->kernels[gcvCORE_MAJOR]->hardware, FastClear, Compression
            ));
		//printk(KERN_INFO "zxw: gckHARDWARE_SetPowerManagement before,%d\n",status);
        gcmkONERROR(gckHARDWARE_SetPowerManagement(
            device->kernels[gcvCORE_MAJOR]->hardware, PowerManagement
            ));
		//printk(KERN_INFO "zxw: gckHARDWARE_SetPowerManagement after,%d\n",status);
#if gcdENABLE_FSCALE_VAL_ADJUST
        gcmkONERROR(gckHARDWARE_SetMinFscaleValue(
            device->kernels[gcvCORE_MAJOR]->hardware, Args->gpu3DMinClock
            ));
#endif

        gcmkONERROR(gckHARDWARE_SetGpuProfiler(
            device->kernels[gcvCORE_MAJOR]->hardware, GpuProfiler
            ));
			//printk(KERN_INFO "zxw: gckHARDWARE_SetGpuProfiler after,%d\n",status);
    }
    else
    {
        device->kernels[gcvCORE_MAJOR] = gcvNULL;
    }

    if (device->irqLines[gcvCORE_2D] != -1)
    {
        gcmkONERROR(gckDEVICE_AddCore(device->device, gcvCORE_2D, gcvCHIP_ID_DEFAULT, device, &device->kernels[gcvCORE_2D]));

        /* Verify the hardware type */
        gcmkONERROR(gckHARDWARE_GetType(device->kernels[gcvCORE_2D]->hardware, &type));

        if (type != gcvHARDWARE_2D)
        {
            gcmkTRACE_ZONE(
                gcvLEVEL_ERROR, gcvZONE_DRIVER,
                "%s(%d): Unexpected hardware type: %d\n",
                __FUNCTION__, __LINE__,
                type
                );

            gcmkONERROR(gcvSTATUS_INVALID_ARGUMENT);
        }

        gcmkONERROR(gckHARDWARE_SetPowerManagement(
            device->kernels[gcvCORE_2D]->hardware, PowerManagement
            ));

#if gcdENABLE_FSCALE_VAL_ADJUST
        gcmkONERROR(gckHARDWARE_SetMinFscaleValue(
            device->kernels[gcvCORE_2D]->hardware, 1
            ));
#endif
    }
    else
    {
        device->kernels[gcvCORE_2D] = gcvNULL;
    }

    if (device->irqLines[gcvCORE_VG] != -1)
    {
    }
    else
    {
        device->kernels[gcvCORE_VG] = gcvNULL;
    }

    /* Add core for multiple core. */
	#if 0
    for (i = gcvCORE_3D1; i <= gcvCORE_3D_MAX; i++)
    {
        if (Args->irqs[i] != -1)
        {
            gcmkONERROR(gcTA_Construct(device->taos, (gceCORE)i, &globalTA[i]));
            gckDEVICE_AddCore(device->device, i, Args->chipIDs[i], device, &device->kernels[i]);

            gcmkONERROR(
            gckHARDWARE_SetFastClear(device->kernels[i]->hardware,
                 FastClear,
                Compression));

            gcmkONERROR(gckHARDWARE_SetPowerManagement(
                device->kernels[i]->hardware, PowerManagement
                ));

            gcmkONERROR(gckHARDWARE_SetGpuProfiler(
                device->kernels[i]->hardware, GpuProfiler
                ));
        }
    }
	#endif
	
    /* Initialize the kernel thread semaphores. */
    for (i = 0; i < gcdMAX_GPU_COUNT; i++)
    {
        if (device->irqLines[i] != -1)
        {
            sema_init(&device->semas[i], 0);
        }
    }

    device->signal = Signal;

    for (i = 0; i < gcdMAX_GPU_COUNT; i++)
    {
        if (device->kernels[i] != gcvNULL) break;
    }
	
    if (i == gcdMAX_GPU_COUNT)
    {
        gcmkONERROR(gcvSTATUS_INVALID_ARGUMENT);
    }
	printk(KERN_INFO "zxw: gcvSTATUS_INVALID_ARGUMENT after,%d\n",status);
    /* Grab the first availiable kernel */
    for (i = 0; i < gcdMAX_GPU_COUNT; i++)
    {
        if (device->irqLines[i] != -1)
        {
            kernel = device->kernels[i];
            break;
        }
    }

    /* Set up the internal memory region. */
    if (device->internalSize > 0)
    {
        status = gckVIDMEM_Construct(
            device->os,
            internalBaseAddress, device->internalSize, internalAlignment,
            0, &device->internalVidMem
            );

        if (gcmIS_ERROR(status))
        {
            /* Error, disable internal heap. */
            device->internalSize = 0;
        }
        else
        {
            /* Map internal memory. */
            device->internalLogical
                = (gctPOINTER) ioremap_nocache(physical, device->internalSize);

            if (device->internalLogical == gcvNULL)
            {
                gcmkONERROR(gcvSTATUS_OUT_OF_RESOURCES);
            }

            device->internalPhysical = (gctPHYS_ADDR)(gctUINTPTR_T) physical;
            physical += device->internalSize;
        }
    }

    if (device->externalSize > 0)
    {
        /* create the external memory heap */
        status = gckVIDMEM_Construct(
            device->os,
            device->externalBase, device->externalSize, externalAlignment,
            0, &device->externalVidMem
            );

        if (gcmIS_ERROR(status))
        {
            /* Error, disable external heap. */
            device->externalSize = 0;
        }
        else
        {
            /* Map external memory. */
            gcmkONERROR(gckOS_RequestReservedMemory(
                    device->os,
                    device->externalBase, device->externalSize,
                    "galcore external memory",
                    gcvTRUE,
                    &device->externalPhysical
                    ));
            device->externalVidMem->physical = device->externalPhysical;
        }
    }

    if (device->internalPhysical)
    {
        device->internalPhysicalName = gcmPTR_TO_NAME(device->internalPhysical);
    }

    if (device->externalPhysical)
    {
        device->externalPhysicalName = gcmPTR_TO_NAME(device->externalPhysical);
    }

    if (device->contiguousPhysical)
    {
        device->contiguousPhysicalName = gcmPTR_TO_NAME(device->contiguousPhysical);
    }

    /* Return pointer to the device. */
    *Device = galDevice = device;
	printk(KERN_INFO "zxw:gckGALDEVICE_Construct is over,%d\n",status);
OnError:
    if (gcmIS_ERROR(status))
    {
        /* Roll back. */
        gcmkVERIFY_OK(gckGALDEVICE_Destroy(device));
    }

    gcmkFOOTER();
    return status;
}


gceSTATUS gckGALDEVICE_QueryFrequency(IN gckGALDEVICE Device)
{
    gctUINT64 mcStart[gcvCORE_COUNT], shStart[gcvCORE_COUNT];
    gctUINT32 mcClk[gcvCORE_COUNT], shClk[gcvCORE_COUNT];
    gckHARDWARE hardware = gcvNULL;
    gceSTATUS status;
    gctUINT i;

    //gcmkHEADER_ARG("Device=0x%p", Device);

    for (i = gcvCORE_MAJOR; i < gcvCORE_COUNT; i++)
    {

        if (Device->kernels[i])
        {
            hardware = Device->kernels[i]->hardware;

            mcStart[i] = shStart[i] = 0;

            gckHARDWARE_EnterQueryClock(hardware,
                                        &mcStart[i], &shStart[i]);
        }
    }

    gcmkONERROR(gckOS_Delay(Device->os, 50));

    for (i = gcvCORE_MAJOR; i < gcvCORE_COUNT; i++)
    {
        mcClk[i] = shClk[i] = 0;


        if (Device->kernels[i] && mcStart[i])
        {
            hardware = Device->kernels[i]->hardware;

            gckHARDWARE_ExitQueryClock(hardware,
                                       mcStart[i], shStart[i],
                                       &mcClk[i], &shClk[i]);

            hardware->mcClk = mcClk[i];
            hardware->shClk = shClk[i];
        }
    }

OnError:
    //gcmkFOOTER_NO();

    return status;
}

/*******************************************************************************
**
**  gckGALDEVICE_Start
**
**  Start the gal device, including the following actions: setup the isr routine
**  and start the daemoni thread.
**
**  INPUT:
**
**      gckGALDEVICE Device
**          Pointer to an gckGALDEVICE object.
**
**  OUTPUT:
**
**      Nothing.
**
**  RETURNS:
**
**      gcvSTATUS_OK
**          Start successfully.
*/
gceSTATUS
gckGALDEVICE_Start(
    IN gckGALDEVICE Device
    )
{
    gctUINT i;
    gceSTATUS status = gcvSTATUS_OK;

    gcmkHEADER_ARG("Device=%p", Device);
	//printk(KERN_INFO "zxw:gckGALDEVICE_Start begin\n");
    /* Start the kernel threads. */
    for (i = 0; i < gcvCORE_COUNT; ++i)
    {
        if (i == gcvCORE_VG)
        {
            continue;
        }

        gcmkONERROR(_StartThread(Device, i));
    }

    gcmkONERROR(gckGALDEVICE_QueryFrequency(Device));
	//printk(KERN_INFO "zxw:gckGALDEVICE_Start 1\n");
    for (i = 0; i < gcvCORE_COUNT; i++)
    {
        if (Device->kernels[i] == gcvNULL)
        {
            continue;
        }

        /* Setup the ISR routine. */
        gcmkONERROR(_SetupIsr(i));

        if (i == gcvCORE_VG)
        {
			printk(KERN_INFO "zxw:gcvCORE_VG enter,do nothing\n");
        }
        else
        {
			printk(KERN_INFO "zxw:gcvCORE_MAJOR enter,i:%d\n",i);
			break;  //zxw,not suspend power,maybe this have some bugs
            /* Switch to SUSPEND power state. */
            gcmkONERROR(gckHARDWARE_SetPowerManagementState(
                Device->kernels[i]->hardware, gcvPOWER_OFF_BROADCAST
                ));
        }
    }
	//printk(KERN_INFO "zxw:gckGALDEVICE_Start end,status:%d\n",status);
OnError:
    gcmkFOOTER();
    return status;
}

/*******************************************************************************
**
**  gckGALDEVICE_Stop
**
**  Stop the gal device, including the following actions: stop the daemon
**  thread, release the irq.
**
**  INPUT:
**
**      gckGALDEVICE Device
**          Pointer to an gckGALDEVICE object.
**
**  OUTPUT:
**
**      Nothing.
**
**  RETURNS:
**
**      Nothing.
*/
gceSTATUS
gckGALDEVICE_Stop(
    gckGALDEVICE Device
    )
{
    gctUINT i;
    gceSTATUS status = gcvSTATUS_OK;

    gcmkHEADER_ARG("Device=%p", Device);

    gcmkVERIFY_ARGUMENT(Device != NULL);

    for (i = 0; i < gcvCORE_COUNT; i++)
    {
        if (Device->kernels[i] == gcvNULL)
        {
            continue;
        }

        if (i == gcvCORE_VG)
        {
        }
        else
        {
            gcmkONERROR(gckHARDWARE_SetPowerManagement(
                Device->kernels[i]->hardware, gcvTRUE
                ));

            /* Switch to OFF power state. */
            gcmkONERROR(gckHARDWARE_SetPowerManagementState(
                Device->kernels[i]->hardware, gcvPOWER_OFF
                ));
        }

        /* Stop the ISR routine. */
        gcmkONERROR(_ReleaseIsr(gcvCORE_VG));

    }

    /* Stop the kernel thread. */
    for (i = 0; i < gcvCORE_COUNT; i++)
    {
        _StopThread(Device, i);
        _ReleaseIsr(i);
    }

OnError:
    gcmkFOOTER();
    return status;
}