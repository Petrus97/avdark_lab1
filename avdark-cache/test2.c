/**
 * Cache simulator test case - Stress
 *
 * Course: Advanced Computer Architecture, Uppsala University
 * Course Part: Lab assignment 1
 *
 * Author: Per Ekemark <per.ekemark@it.uu.se>
 * Author: Andreas Sandberg <andreas.sandberg@it.uu.se>
 *
 */

#include "avdark-cache.h"

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

// https://stackoverflow.com/questions/5867834/assert-with-message
#define log_error(M, ...) fprintf(stderr, "[ABORT] (%s:%d) " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define assertf(A, M, ...) if(!(A)) {log_error(M, ##__VA_ARGS__); assert(A); }


#define STAT_ASSERT(c, r, rm, w, wm) do {                       \
                assertf(c->stat_data_read == (r), "got: %ld - expected %d", c->stat_data_read, r); \ 
                assertf(c->stat_data_read_miss == (rm), "got: %ld - expected %d", c->stat_data_read_miss, rm); \ 
                assertf(c->stat_data_write == (w), "got: %ld - expected %d", c->stat_data_write, w); \ 
                assertf(c->stat_data_write_miss == (wm), "got: %ld - expected %d", c->stat_data_write_miss, wm); \ 
        } while (0)

#define TEST_SIMPLE_STAT() \
        if (type == AVDC_READ)                                  \
                STAT_ASSERT(cache, hits+misses, misses, 0, 0);   \
        else if (type == AVDC_WRITE)                            \
                STAT_ASSERT(cache, 0, 0, hits+misses, misses);  \
        else                                                    \
                abort()

static void
test_stress(avdark_cache_t *cache, avdc_access_type_t type)
{
        int i, j;
        int hits = 0;
        int misses = 0;

        avdc_reset_statistics(cache);
        STAT_ASSERT(cache, 0, 0, 0, 0);

        /* Access all bytes, we should get 1 miss per block */
        for (i = 0; i < cache->size; i++) {
                avdc_access(cache, i, type);
                if (i % cache->block_size == 0)
                        misses++;
                else
                        hits++;
                if(i == 256)
                    putc('\n', stdout);
                else if( i == 254)
                    putchar('\0');
                TEST_SIMPLE_STAT();
        }

        /* Now, access all the bytes again, we shouldn't get any misses */
        for (i = 0; i < cache->size; i++) {
                avdc_access(cache, i, type);
                hits++;
                TEST_SIMPLE_STAT();
        }

        /* Access 1 new cache line that for every set that replaces an
         * existing cache line (different ways for different sets) */
        for (i = cache->size, j = 0; i < cache->size * 2; i += cache->block_size * cache->assoc, j++) {
                avdc_access(cache, i + (j % cache->assoc), type);
                misses++;
                TEST_SIMPLE_STAT();
        }

        /* Access all lines initially touched, except for the first line in
         * every set (which has been replaced by the LRU algorithm).
         * We'll do it backwards for good measure.
         * We should only get hits here. */
        for (i = cache->size - cache->block_size; i >= 0; i -= cache->block_size) {
                if ((i / cache->block_size) % cache->assoc != 0) {
                        avdc_access(cache, i, type);
                        hits++;
                        TEST_SIMPLE_STAT();
                }
        }

        /* Now, access the first line of every set again, these should be misses */
        for (i = 0; i < cache->size; i += cache->block_size * cache->assoc) {
                avdc_access(cache, i, type);
                misses++;
                TEST_SIMPLE_STAT();
        }

        /* Revisit the cachelines used to replace the first ones, they should now be misses again */
        for (i = cache->size, j = 0; i < cache->size * 2; i += cache->block_size * cache->assoc, j++) {
                avdc_access(cache, i + (j % cache->assoc), type);
                misses++;
                TEST_SIMPLE_STAT();
        }
}

int
main(int argc, char *argv[])
{
        avdark_cache_t *cache;

        cache = avdc_new(512, 64, 1);
        assert(cache);
        avdc_print_info(cache);

        // printf("Stress [read]\n");
        // test_stress(cache, AVDC_READ);
        // printf("Stress [write]\n");
        // avdc_flush_cache(cache);
        // test_stress(cache, AVDC_WRITE);


        // avdc_resize(cache, 512, 128, 1);
        // avdc_print_info(cache);

        // printf("Stress [read]\n");
        // test_stress(cache, AVDC_READ);
        // printf("Stress [write]\n");
        // avdc_flush_cache(cache);
        // test_stress(cache, AVDC_WRITE);


        // avdc_resize(cache, 256, 64, 1);
        // avdc_print_info(cache);

        // printf("Stress [read]\n");
        // test_stress(cache, AVDC_READ);
        // printf("Stress [write]\n");
        // avdc_flush_cache(cache);
        // test_stress(cache, AVDC_WRITE);


        printf("Switching to assoc 2, assuming LRU\n");

        avdc_resize(cache, 512, 64, 2);
        avdc_print_info(cache);

        printf("Stress [read]\n");
        test_stress(cache, AVDC_READ);
        printf("Stress [write]\n");
        avdc_flush_cache(cache);
        test_stress(cache, AVDC_WRITE);


        avdc_resize(cache, 8*1024*1024, 128, 2);
        avdc_print_info(cache);

        printf("Stress [read]\n");
        test_stress(cache, AVDC_READ);
        printf("Stress [write]\n");
        avdc_flush_cache(cache);
        test_stress(cache, AVDC_WRITE);


        // /* This set of tests are only useful if the cache simulator supports
        //  * higher associativity than 2 */
        // avdc_resize(cache, 4*1024, 32, 4);
        // avdc_print_info(cache);

        // printf("Stress [read]\n");
        // test_stress(cache, AVDC_READ);
        // printf("Stress [write]\n");
        // avdc_flush_cache(cache);
        // test_stress(cache, AVDC_WRITE);

 
        avdc_delete(cache);

        printf("%s done.\n", argv[0]);
        return 0;
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
