/**
 * Cache simulation using a functional system simulator.
 *
 * Course: Advanced Computer Architecture, Uppsala University
 * Course Part: Lab assignment 1
 *
 * Original authors: UART 1.0(?)
 * Modified by: Andreas Sandberg <andreas.sandberg@it.uu.se>
 *
 * $Id: avdark-cache.c 14 2011-08-24 09:55:20Z ansan501 $
 */

#include "avdark-cache.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <inttypes.h>
#include <stdbool.h>

#ifdef SIMICS
/* Simics stuff  */
#include <simics/api.h>
#include <simics/alloc.h>
#include <simics/utils.h>

#define AVDC_MALLOC(nelems, type) MM_MALLOC(nelems, type)
#define AVDC_FREE(p) MM_FREE(p)

#else

#define AVDC_MALLOC(nelems, type) malloc(nelems * sizeof(type))
#define AVDC_FREE(p) free(p)

#endif

/** Notes about the assignment
 * - Rearrange the cache line array or/and fix the tag and index computations.
 * - The cache model never handles actual data.
 * - The cache model only contains tags and valid bits -> you may need to add more metadata to implement the LRU policy.
 * https://www.cs.jhu.edu/~yairamir/cs418/os6/tsld021.htm
 */

// just one bit for a 2-way associativity, in general assoc / lg2(assoc)
// we can check which of this bit has been set as a least recently used to understand
// which block of cache we need to remove

/**
 * Cache block information.
 *
 * HINT: You will probably need to change this structure
 */
struct avdc_cache_line
{
        avdc_tag_t tag;
        int valid;
        bool used_recently;
};

/**
 * Extract the cache line tag from a physical address.
 *
 * You probably don't want to change this function, instead you may
 * want to change how the tag_shift field is calculated in
 * avdc_resize().
 */
static inline avdc_pa_t
tag_from_pa(avdark_cache_t *self, avdc_pa_t pa)
{
        return pa >> self->tag_shift;
}

/**
 * Calculate the cache line index from a physical address.
 *
 * Feel free to experiment and change this function
 */
static inline int
index_from_pa(avdark_cache_t *self, avdc_pa_t pa)
{
        return (pa >> self->block_size_log2) & (self->number_of_sets - 1);
}

/**
 * Computes the log2 of a 32 bit integer value. Used in dc_init
 *
 * Do NOT modify!
 */
static int
log2_int32(uint32_t value)
{
        int i;

        for (i = 0; i < 32; i++)
        {
                value >>= 1;
                if (value == 0)
                        break;
        }
        return i;
}

/**
 * Check if a number is a power of 2. Used for cache parameter sanity
 * checks.
 *
 * Do NOT modify!
 */
static int
is_power_of_two(uint64_t val)
{
        return ((((val) & (val - 1)) == 0) && (val > 0));
}

void avdc_dbg_log(avdark_cache_t *self, const char *msg, ...)
{
        va_list ap;

        if (self->dbg)
        {
                const char *name = self->dbg_name ? self->dbg_name : "AVDC";
                fprintf(stderr, "[%s] dbg: ", name);
                va_start(ap, msg);
                vfprintf(stderr, msg, ap);
                va_end(ap);
        }
}

bool check_hit(avdark_cache_t *self, int index, avdc_tag_t tag)
{
        bool hit = false;
        for (size_t assoc_idx = 0; assoc_idx < self->assoc; assoc_idx++)
        {
                bool cache_line_hit = self->lines[index][assoc_idx].valid && self->lines[index][assoc_idx].tag == tag;
                hit = hit || cache_line_hit;
        }
        return hit;
}

void remove_cache_line(avdark_cache_t *self, avdc_tag_t tag, int index)
{
        if (self->assoc == 1)
        {
                self->lines[index][0].valid = 1;
                self->lines[index][0].tag = tag;
        }
        else
        {
                for (size_t assoc_idx = 0; assoc_idx < self->assoc; assoc_idx++)
                {
                        if (self->lines[index][assoc_idx].used_recently == false)
                        {
                                self->lines[index][assoc_idx].valid = 1;
                                // update the correct tag
                                self->lines[index][assoc_idx].tag = tag;
                                // fetched information from the memory, this is actually used
                                self->lines[index][assoc_idx].used_recently = true;
                                break; 
                        }
                }
        }
}

void avdc_access(avdark_cache_t *self, avdc_pa_t pa, avdc_access_type_t type)
{
        /* HINT: You will need to update this function */
        avdc_tag_t tag = tag_from_pa(self, pa);
        int index = index_from_pa(self, pa);
        bool hit = false;

        hit = check_hit(self, index, tag);
        if (!hit) // MISS
        {
                // bring all the cache line in the cache
                // remove the cacheline which has the least recently used value
                remove_cache_line(self, tag, index);
        }
        else // HIT
        {
                if (self->assoc > 1)
                {
                        for (size_t assoc_idx = 0; assoc_idx < self->assoc; assoc_idx++)
                        {
                                // switch the recently used flag
                                if (self->lines[index][assoc_idx].tag == tag)
                                {
                                        self->lines[index][assoc_idx].used_recently = true;
                                }
                                else
                                {
                                        self->lines[index][assoc_idx].used_recently = false;
                                }
                        }
                }
                else
                {
                        /* do nothing */
                }
        }

        switch (type)
        {
        case AVDC_READ: /* Read accesses */
                avdc_dbg_log(self, "read: pa: 0x%.16lx, tag: 0x%.16lx, index: %d, hit: %d\n",
                             (unsigned long)pa, (unsigned long)tag, index, hit);
                self->stat_data_read += 1;
                if (!hit) // MISS
                        self->stat_data_read_miss += 1;
                break;

        case AVDC_WRITE: /* Write accesses */
                avdc_dbg_log(self, "write: pa: 0x%.16lx, tag: 0x%.16lx, index: %d, hit: %d\n",
                             (unsigned long)pa, (unsigned long)tag, index, hit);
                self->stat_data_write += 1;
                if (!hit) // MISS
                        self->stat_data_write_miss += 1;
                break;
        }
}

void avdc_flush_cache(avdark_cache_t *self)
{
        /* HINT: You will need to update this function */
        for (int i = 0; i < self->number_of_sets; i++)
        {
                for (size_t j = 0; j < self->assoc; j++)
                {
                        self->lines[i][j].valid = 0;
                        self->lines[i][j].tag = 0;
                        self->lines[i][j].used_recently = false;
                }
        }
}

int avdc_resize(avdark_cache_t *self,
                avdc_size_t size, avdc_block_size_t block_size, avdc_assoc_t assoc)
{
        /* HINT: This function precomputes some common values and
         * allocates the self->lines array. You will need to update
         * this to reflect any changes to how this array is supposed
         * to be allocated.
         */

        /* Verify that the parameters are sane */
        if (!is_power_of_two(size) ||
            !is_power_of_two(block_size) ||
            !is_power_of_two(assoc))
        {
                fprintf(stderr, "size, block-size and assoc all have to be powers of two and > zero\n");
                return 0;
        }

        /* Update the stored parameters */
        self->size = size;
        self->block_size = block_size; // Single cache line
        self->assoc = assoc;

        /* Cache some common values */
        self->number_of_sets = (self->size / self->block_size) / self->assoc; // sets = entries / associativity
        self->block_size_log2 = log2_int32(self->block_size);                 // bits to identify a word + mux select bits
        // int index = log2_int32(self->number_of_sets);
        self->tag_shift = self->block_size_log2 + log2_int32(self->number_of_sets);

        // printf("Size=%d\nAssoc=%d-way\nCL=%d\nsets=%d\n", self->size, self->assoc, self->block_size, self->number_of_sets);
        // printf("block_size_lg2=%d\nindex=%d\ntag_shift=%d\n", self->block_size_log2, index, self->tag_shift);

        /* (Re-)Allocate space for the tags array */
        if (self->lines)
                AVDC_FREE(self->lines);
        /* HINT: If you change this, you may have to update
         * avdc_delete() to reflect changes to how thie self->lines
         * array is allocated. */
        // associativity = 1 -> direct mapped (1D array)
        // associativity = 2 -> more then 1 cache line on each set (2D array)
        self->lines = (avdc_cache_line_t**)calloc(self->number_of_sets, sizeof(avdc_cache_line_t*)); // allocate n_sets pointers
        for (size_t i = 0; i < self->number_of_sets; i++)
        {
                self->lines[i] = (avdc_cache_line_t*)calloc(self->assoc, sizeof(avdc_cache_line_t));
        }

        /* Flush the cache, this initializes the tag array to a known state */
        avdc_flush_cache(self);

        return 1;
}

void avdc_print_info(avdark_cache_t *self)
{
        fprintf(stderr, "Cache Info\n");
        fprintf(stderr, "size: %d, assoc: %d, line-size: %d\n",
                self->size, self->assoc, self->block_size);
}

void avdc_print_internals(avdark_cache_t *self)
{
        int i;

        fprintf(stderr, "Cache Internals\n");
        fprintf(stderr, "size: %d, assoc: %d, line-size: %d\n",
                self->size, self->assoc, self->block_size);

        for (i = 0; i < self->number_of_sets; i++)
        {
                for (size_t assoc_idx = 0; assoc_idx < self->assoc; assoc_idx++)
                {
                        fprintf(stderr, "tag: <0x%.16lx> valid: %d\n",
                                (long unsigned int)self->lines[i][assoc_idx].tag,
                                self->lines[i][assoc_idx].valid);
                }
        }
}

void avdc_reset_statistics(avdark_cache_t *self)
{
        self->stat_data_read = 0;
        self->stat_data_read_miss = 0;
        self->stat_data_write = 0;
        self->stat_data_write_miss = 0;
}

avdark_cache_t *
avdc_new(avdc_size_t size, avdc_block_size_t block_size,
         avdc_assoc_t assoc)
{
        avdark_cache_t *self;

        self = AVDC_MALLOC(1, avdark_cache_t);

        memset(self, 0, sizeof(*self));
        self->dbg = 0;

        if (!avdc_resize(self, size, block_size, assoc))
        {
                AVDC_FREE(self);
                return NULL;
        }

        return self;
}

void avdc_delete(avdark_cache_t *self)
{
        if (self->lines)
        {
                for (size_t i = 0; i < self->number_of_sets; i++)
                {
                        AVDC_FREE(self->lines[i]);
                }
                AVDC_FREE(self->lines);
        }

        AVDC_FREE(self);
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 8
 * indent-tabs-mode: nil
 * c-file-style: "linux"
 * compile-command: "make -k -C ../../"
 * End:
 */
