/**
 * A code skeleton for the matrix multiply bonus assignment.
 * 
 * Course: Advanced Computer Architecture, Uppsala University
 * Course Part: Lab assignment 1
 *
 * Author: Andreas Sandberg <andreas.sandberg@it.uu.se>
 *
 * $Id: multiply.c 81 2012-09-13 08:01:46Z andse541 $
 */

#include <unistd.h>
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* Size of the matrices to multiply */
#ifndef SIZE
#define SIZE 700
#endif

/* HINT: The Makefile allows you to specify L1 and L2 block sizes as
 * compile time options.These may be specified when calling make,
 * e.g. "make L1_BLOCK_SIZE=256 L2_BLOCK_SIZE=1024". If the block
 * sizes have been specified on the command line, makefile defines the
 * macros L1_BLOCK_SIZE and L2_BLOCK_SIZE. If you decide to use them,
 * you should setup defaults here if they are undefined.
 */


static volatile double mat_a[SIZE][SIZE];
static volatile double mat_b[SIZE][SIZE];
static volatile double mat_c[SIZE][SIZE];
static volatile double mat_ref[SIZE][SIZE];

/**
 * Matrix multiplication. This is the procedure you should try to
 * optimize.
 */
static void
matmul_opt()
{
        /* TASK: Implement your optimized matrix multiplication
         * here. It should calculate mat_c := mat_a * mat_b. See
         * matmul_ref() for a reference solution.
         */
}

/**
 * Reference implementation of the matrix multiply algorithm. Used to
 * verify the answer from matmul_opt. Do NOT change this function.
 */
static void
matmul_ref()
{
        int i, j, k;

        for (i = 0; i < SIZE; i++) {
                for (j = 0; j < SIZE; j++) {
                        for (k = 0; k < SIZE; k++) {
                                mat_ref[i][j] += mat_a[i][k] * mat_b[k][j];
                        }
                }
        }
}

/**
 * Function used to verify the result. No need to change this one.
 */
static int
verify_result()
{
        double e_sum;
        int i, j;

        e_sum = 0;
        for (i = 0; i < SIZE; i++) {
                for (j = 0; j < SIZE; j++) {
                        e_sum += mat_c[i][j] < mat_ref[i][j] ?
                                mat_ref[i][j] - mat_c[i][j] :
                                mat_c[i][j] - mat_ref[i][j];
                }
        }

        return e_sum < 1E-6;
}

/**
 * Get the time in seconds since some arbitrary point. Used for high
 * precision timing measurements.
 */
static double
get_time()
{
        struct timeval tv;

        if (gettimeofday(&tv, NULL)) {
                fprintf(stderr, "gettimeofday failed. Aborting.\n");
                abort();
        }
        return tv.tv_sec + tv.tv_usec * 1E-6;
}

/**
 * Initialize mat_a and mat_b with "random" data. Write to every
 * element in mat_c to make sure that the kernel allocates physical
 * memory to every page in the matrix before we start doing
 * benchmarking.
 */
static void
init_matrices()
{
        int i, j;

        for (i = 0; i < SIZE; i++) {
                for (j = 0; j < SIZE; j++) {
                        mat_a[i][j] = ((i + j) & 0x0F) * 0x1P-4;
                        mat_b[i][j] = (((i << 1) + (j >> 1)) & 0x0F) * 0x1P-4;
                }
        }

        memset((void*)mat_c, 0, sizeof(mat_c));
        memset((void*)mat_ref, 0, sizeof(mat_ref));
}

static void
run_multiply(int verify, int reference)
{
        double time_start, time_stop;

        printf("Matrix size: %dx%d\n", SIZE, SIZE);

        if (!reference) {
                printf("Running optimised solution...\n");
                time_start = get_time();
                /* mat_c = mat_a * mat_b */
                matmul_opt();
                time_stop = get_time();
                printf("Optimised runtime: %.4f\n", time_stop - time_start);
        }

        if (reference || verify) {
                printf("Running reference solution...\n");
                time_start = get_time();
                matmul_ref();
                time_stop = get_time();
                printf("Reference runtime: %.4f\n", time_stop - time_start);
        }

        if (verify) {
                printf("Verifying solution... ");

                if (verify_result())
                        printf("OK\n");
                else
                        printf("MISMATCH\n");
        }
}

static void
usage(FILE *out, const char *argv0)
{
        fprintf(out,
                "Usage: %s [OPTION]...\n"
                "\n"
                "Options:\n"
                "\t-r\tRun only reference solution\n"
                "\t-v\tVerify solution\n"
                "\t-h\tDisplay usage\n",
                argv0);
}

int
main(int argc, char *argv[])
{
        int c;
        int errexit;
        int verify;
        int reference;
        extern char *optarg;
        extern int optind, optopt, opterr;

        errexit = 0;
        verify = 0;
        reference = 0;
        while ((c = getopt(argc, argv, "vrh")) != -1) {
                switch (c) {
                case 'v':
                        verify = 1;
                        break;
                case 'r':
                        reference = 1;
                        break;
                case 'h':
                        usage(stdout, argv[0]);
                        exit(0);
                        break;
                case ':':
                        fprintf(stderr, "%s: option -%c requries an operand\n",
                                argv[0], optopt);
                        errexit = 1;
                        break;
                case '?':
                        fprintf(stderr, "%s: illegal option -- %c\n",
                                argv[0], optopt);
                        errexit = 1;
                        break;
                default:
                        abort();
                }
        }

        if (errexit) {
                usage(stderr, argv[0]);
                exit(2);
        }

        /* Initialize the matrices with some "random" data. */
        init_matrices();

        run_multiply(verify, reference);

        return 0;
}


/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 8
 * indent-tabs-mode: nil
 * c-file-style: "linux"
 * compile-command: "make -k"
 * End:
 */
