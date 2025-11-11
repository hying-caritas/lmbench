/*
 * bw_mem64l.c - simple memory write bandwidth benchmark
 *
 * Usage: bw_mem64l [-P <parallelism>] [-W <warmup>] [-N <repetitions>] size what
 *        what: rdX wrX rdwrX cpX bzero bcopy
 *
 * Copyright (c) 1994-1996 Larry McVoy.  Distributed under the FSF GPL with
 * additional restriction that results may published only if
 * (1) the benchmark is unmodified, and
 * (2) the version in the sccsid below is included in the report.
 * Support for this development by Sun Microsystems is gratefully acknowledged.
 */
char	*id = "$Id$";

#include "bench.h"

#define CHUNK_SIZE	16384
#define TYPE    int64_t

/*
 * rdX - 8 byte read, X byte stride
 * wrX - 8 byte write, X byte stride
 * rdwrX - 8 byte read followed by 8 byte write to same place, X byte stride
 * cpX - 8 byte read then 8 byte write to different place, X byte stride
 *
 * All tests do 16k byte chunks in a loop.
 */
void	loop_bzero(iter_t iterations, void *cookie);
void	loop_bcopy(iter_t iterations, void *cookie);
void	init_overhead(iter_t iterations, void *cookie);
void	init_loop(iter_t iterations, void *cookie);
void	cleanup(iter_t iterations, void *cookie);

void	rd64(iter_t iterations, void *cookie);
void	wr64(iter_t iterations, void *cookie);
void	rd128(iter_t iterations, void *cookie);
void	wr128(iter_t iterations, void *cookie);
void	rdwr128(iter_t iterations, void *cookie);
void	mcp128(iter_t iterations, void *cookie);
void	rd512(iter_t iterations, void *cookie);
void	wr512(iter_t iterations, void *cookie);
void	rdwr512(iter_t iterations, void *cookie);
void	mcp512(iter_t iterations, void *cookie);
void	rd1024(iter_t iterations, void *cookie);
void	wr1024(iter_t iterations, void *cookie);
void	rdwr1024(iter_t iterations, void *cookie);
void	mcp1024(iter_t iterations, void *cookie);
void	rd2048(iter_t iterations, void *cookie);
void	wr2048(iter_t iterations, void *cookie);
void	rdwr2048(iter_t iterations, void *cookie);
void	mcp2048(iter_t iterations, void *cookie);

void	wrc(iter_t iterations, void *cookie);
void	wrs(iter_t iterations, void *cookie);

typedef struct _state {
	double	overhead;
	size_t	nbytes;
	int	need_buf2;
	int	aligned;
	TYPE	*buf;
	TYPE	*buf2;
	TYPE	*buf2_orig;
	TYPE	*lastone;
	size_t	N;
} state_t;

void	adjusted_bandwidth(uint64 t, uint64 b, uint64 iter, double ovrhd);

int
main(int ac, char **av)
{
	int	parallel = 1;
	int	warmup = 0;
	int	repetitions = TRIES;
	size_t	nbytes;
	state_t	state;
	int	c;
	char	*usage = "[-P <parallelism>] [-W <warmup>] [-N <repetitions>] <size> what [conflict]\nwhat: rdX wrX rdwrX cpX bzero bcopy\n<size> must be larger than 16384\n";

	state.overhead = 0;

	while (( c = getopt(ac, av, "P:W:N:")) != EOF) {
		switch(c) {
		case 'P':
			parallel = atoi(optarg);
			if (parallel <= 0) lmbench_usage(ac, av, usage);
			break;
		case 'W':
			warmup = atoi(optarg);
			break;
		case 'N':
			repetitions = atoi(optarg);
			break;
		default:
			lmbench_usage(ac, av, usage);
			break;
		}
	}

	/* should have two, possibly three [indicates align] arguments left */
	state.aligned = state.need_buf2 = 0;
	if (optind + 3 == ac) {
		state.aligned = 1;
	} else if (optind + 2 != ac) {
		lmbench_usage(ac, av, usage);
	}

	nbytes = state.nbytes = bytes(av[optind]);
	if (state.nbytes < CHUNK_SIZE) { /* this is the number of bytes in the loop */
		lmbench_usage(ac, av, usage);
	}

	if (streq(av[optind+1], "cp") || streq(av[optind+1], "bcopy")) {
		state.need_buf2 = 1;
	}

	if (streq(av[optind+1], "bzero")) {
		benchmp(init_loop, loop_bzero, cleanup, 0, parallel,
			warmup, repetitions, &state);
	} else if (streq(av[optind+1], "bcopy")) {
		benchmp(init_loop, loop_bcopy, cleanup, 0, parallel,
			warmup, repetitions, &state);
	} else if (streq(av[optind+1], "rd64")) {
		benchmp(init_loop, rd64, cleanup, 0, parallel,
			warmup, repetitions, &state);
	} else if (streq(av[optind+1], "wr64")) {
		benchmp(init_loop, wr64, cleanup, 0, parallel,
			warmup, repetitions, &state);
	} else if (streq(av[optind+1], "rd128")) {
		benchmp(init_loop, rd128, cleanup, 0, parallel,
			warmup, repetitions, &state);
	} else if (streq(av[optind+1], "wr128")) {
		benchmp(init_loop, wr128, cleanup, 0, parallel,
			warmup, repetitions, &state);
	} else if (streq(av[optind+1], "rdwr128")) {
		benchmp(init_loop, rdwr128, cleanup, 0, parallel,
			warmup, repetitions, &state);
	} else if (streq(av[optind+1], "cp128")) {
		benchmp(init_loop, mcp128, cleanup, 0, parallel,
			warmup, repetitions, &state);
	} else if (streq(av[optind+1], "rd512")) {
		benchmp(init_loop, rd512, cleanup, 0, parallel,
			warmup, repetitions, &state);
	} else if (streq(av[optind+1], "wr512")) {
		benchmp(init_loop, wr512, cleanup, 0, parallel,
			warmup, repetitions, &state);
	} else if (streq(av[optind+1], "rdwr512")) {
		benchmp(init_loop, rdwr512, cleanup, 0, parallel,
			warmup, repetitions, &state);
	} else if (streq(av[optind+1], "cp512")) {
		benchmp(init_loop, mcp512, cleanup, 0, parallel,
			warmup, repetitions, &state);
	} else if (streq(av[optind+1], "rd1024")) {
		benchmp(init_loop, rd1024, cleanup, 0, parallel,
			warmup, repetitions, &state);
	} else if (streq(av[optind+1], "wr1024")) {
		benchmp(init_loop, wr1024, cleanup, 0, parallel,
			warmup, repetitions, &state);
	} else if (streq(av[optind+1], "rdwr1024")) {
		benchmp(init_loop, rdwr1024, cleanup, 0, parallel,
			warmup, repetitions, &state);
	} else if (streq(av[optind+1], "cp1024")) {
		benchmp(init_loop, mcp1024, cleanup, 0, parallel,
			warmup, repetitions, &state);
	} else if (streq(av[optind+1], "rd2048")) {
		benchmp(init_loop, rd2048, cleanup, 0, parallel,
			warmup, repetitions, &state);
	} else if (streq(av[optind+1], "wr2048")) {
		benchmp(init_loop, wr2048, cleanup, 0, parallel,
			warmup, repetitions, &state);
	} else if (streq(av[optind+1], "rdwr2048")) {
		benchmp(init_loop, rdwr2048, cleanup, 0, parallel,
			warmup, repetitions, &state);
	} else if (streq(av[optind+1], "cp2048")) {
		benchmp(init_loop, mcp2048, cleanup, 0, parallel,
			warmup, repetitions, &state);
	} else if (streq(av[optind+1], "wrc")) {
		benchmp(init_loop, wrc, cleanup, 0, parallel,
			warmup, repetitions, &state);
	} else if (streq(av[optind+1], "wrs")) {
		benchmp(init_loop, wrs, cleanup, 0, parallel,
			warmup, repetitions, &state);
	} else {
		lmbench_usage(ac, av, usage);
	}
	adjusted_bandwidth(gettime(), nbytes,
			   get_n() * parallel, state.overhead);
	return(0);
}

void
init_overhead(iter_t iterations, void *cookie)
{
	state_t *state = (state_t *) cookie;
}

void
init_loop(iter_t iterations, void *cookie)
{
	state_t *state = (state_t *) cookie;

	if (iterations) return;

        state->buf = (TYPE *)valloc(state->nbytes);
	state->buf2_orig = NULL;
	state->lastone = (TYPE*)state->buf - 1;
	state->lastone = (TYPE*)((char *)state->buf + state->nbytes - CHUNK_SIZE);
	state->N = state->nbytes;

	if (!state->buf) {
		perror("malloc");
		exit(1);
	}
	bzero((void*)state->buf, state->nbytes);

	if (state->need_buf2 == 1) {
		state->buf2_orig = state->buf2 = (TYPE *)valloc(state->nbytes + 2048);
		if (!state->buf2) {
			perror("malloc");
			exit(1);
		}

		/* default is to have stuff unaligned wrt each other */
		/* XXX - this is not well tested or thought out */
		if (state->aligned) {
			char	*tmp = (char *)state->buf2;

			tmp += 2048 - 128;
			state->buf2 = (TYPE *)tmp;
		}
	}
}

void
cleanup(iter_t iterations, void *cookie)
{
	state_t *state = (state_t *) cookie;

	if (iterations) return;

	free(state->buf);
	if (state->buf2_orig) free(state->buf2_orig);
}

void
loop_bzero(iter_t iterations, void *cookie)
{
	state_t *state = (state_t *) cookie;
	register TYPE *p = state->buf;
	register TYPE *dst = state->buf2;
	register size_t  N = state->N;

	while (iterations-- > 0) {
		bzero(p, N);
	}
}

void
loop_bcopy(iter_t iterations, void *cookie)
{
	state_t *state = (state_t *) cookie;
	register TYPE *p = state->buf;
	register TYPE *dst = state->buf2;
	register size_t  N = state->N;

	while (iterations-- > 0) {
		bcopy(p,dst,N);
	}
}

/*
 * Almost like bandwidth() in lib_timing.c, but we need to adjust
 * bandwidth based upon loop overhead.
 */
void adjusted_bandwidth(uint64 time, uint64 bytes, uint64 iter, double overhd)
{
#define MB	(1000. * 1000.)
	extern FILE *ftiming;
	double secs = ((double)time / (double)iter - overhd) / 1000000.0;
	double mb;

        mb = bytes / MB;

	if (secs <= 0.)
		return;

        if (!ftiming) ftiming = stderr;
	if (mb < 1.) {
		(void) fprintf(ftiming, "%.6f ", mb);
	} else {
		(void) fprintf(ftiming, "%.2f ", mb);
	}
	if (mb / secs < 1.) {
		(void) fprintf(ftiming, "%.6f\n", mb/secs);
	} else {
		(void) fprintf(ftiming, "%.2f\n", mb/secs);
	}
}

#define DOIT_7(START, STEP)						\
	DOIT(START) DOIT(START + STEP) DOIT(START + 2*STEP)		\
	DOIT(START + 3*STEP) DOIT(START + 4*STEP) DOIT(START + 5*STEP)	\
	DOIT(START + 6*STEP)
#define DOIT_8(START, STEP)						\
	DOIT_7(START, STEP) DOIT(START + 7*STEP)
#define DOIT_15(START, STEP)						\
	DOIT_8(START, STEP) DOIT_7(START + 8*STEP, STEP)
#define DOIT_16(START, STEP)						\
	DOIT_8(START, STEP) DOIT_8(START + 8*STEP, STEP)
#define DOIT_31(START, STEP)						\
	DOIT_16(START, STEP) DOIT_15(START + 16*STEP, STEP)
#define DOIT_32(START, STEP)						\
	DOIT_16(START, STEP) DOIT_16(START + 16*STEP, STEP)
#define DOIT_63(START, STEP)						\
	DOIT_32(START, STEP) DOIT_31(START + 32*STEP, STEP)
#define DOIT_64(START, STEP)						\
	DOIT_32(START, STEP) DOIT_32(START + 32*STEP, STEP)
#define DOIT_127(START, STEP)						\
	DOIT_64(START, STEP) DOIT_63(START + 64*STEP, STEP)
#define DOIT_128(START, STEP)						\
	DOIT_64(START, STEP) DOIT_64(START + 64*STEP, STEP)
#define DOIT_255(START, STEP)						\
	DOIT_128(START, STEP) DOIT_127(START + 128*STEP, STEP)
#define DOIT_256(START, STEP)						\
	DOIT_128(START, STEP) DOIT_128(START + 128*STEP, STEP)

void
rd64(iter_t iterations, void *cookie)
{
	state_t *state = (state_t *) cookie;
	register TYPE *lastone = state->lastone;
	register TYPE sum = 0;

	while (iterations-- > 0) {
	    register TYPE *p = state->buf;
	    while (p <= lastone) {
		sum +=
#define	DOIT(i)	p[i]+
		DOIT_255(0, 8)
		p[2048 - 8];
		p +=  2048;
	    }
	}
	use_int(sum);
}
#undef	DOIT

void
wr64(iter_t iterations, void *cookie)
{
	state_t *state = (state_t *) cookie;
	register TYPE *lastone = state->lastone;

	while (iterations-- > 0) {
	    register TYPE *p = state->buf;
	    while (p <= lastone) {
#define	DOIT(i)	p[i] = 1;
		DOIT_256(0, 8);
		p +=  2048;
	    }
	}
}
#undef	DOIT

void
rd128(iter_t iterations, void *cookie)
{
	state_t *state = (state_t *) cookie;
	register TYPE *lastone = state->lastone;
	register TYPE sum = 0;

	while (iterations-- > 0) {
	    register TYPE *p = state->buf;
	    while (p <= lastone) {
		sum +=
#define	DOIT(i)	p[i]+
		DOIT_127(0, 16)
		p[2048 - 16];
		p +=  2048;
	    }
	}
	use_int(sum);
}
#undef	DOIT

void
wr128(iter_t iterations, void *cookie)
{
	state_t *state = (state_t *) cookie;
	register TYPE *lastone = state->lastone;

	while (iterations-- > 0) {
	    register TYPE *p = state->buf;
	    while (p <= lastone) {
#define	DOIT(i)	p[i] = 1;
		DOIT_128(0, 16);
		p +=  2048;
	    }
	}
}
#undef	DOIT

void
rdwr128(iter_t iterations, void *cookie)
{
	state_t *state = (state_t *) cookie;
	register TYPE *lastone = state->lastone;
	register TYPE sum = 0;

	while (iterations-- > 0) {
	    register TYPE *p = state->buf;
	    while (p <= lastone) {
#define	DOIT(i)	sum += p[i]; p[i] = 1;
		DOIT_128(0, 16);
		p +=  2048;
	    }
	}
	use_int(sum);
}
#undef	DOIT

void
mcp128(iter_t iterations, void *cookie)
{
	state_t *state = (state_t *) cookie;
	register TYPE *lastone = state->lastone;
	TYPE* p_save = NULL;

	while (iterations-- > 0) {
	    register TYPE *p = state->buf;
	    register TYPE *dst = state->buf2;
	    while (p <= lastone) {
#define	DOIT(i)	dst[i] = p[i];
		DOIT_128(0, 16);
		p +=  2048;
		dst += 2048;
	    }
	    p_save = p;
	}
	use_pointer(p_save);
}
#undef	DOIT

void
rd512(iter_t iterations, void *cookie)
{
	state_t *state = (state_t *) cookie;
	register TYPE *lastone = state->lastone;
	register TYPE sum = 0;

	while (iterations-- > 0) {
	    register TYPE *p = state->buf;
	    while (p <= lastone) {
		sum +=
#define	DOIT(i)	p[i]+
		DOIT_31(0, 64)
		p[2048 - 64];
		p +=  2048;
	    }
	}
	use_int(sum);
}
#undef	DOIT

void
wr512(iter_t iterations, void *cookie)
{
	state_t *state = (state_t *) cookie;
	register TYPE *lastone = state->lastone;

	while (iterations-- > 0) {
	    register TYPE *p = state->buf;
	    while (p <= lastone) {
#define	DOIT(i)	p[i] = 1;
		DOIT_32(0, 64);
		p +=  2048;
	    }
	}
}
#undef	DOIT

void
rdwr512(iter_t iterations, void *cookie)
{
	state_t *state = (state_t *) cookie;
	register TYPE *lastone = state->lastone;
	register TYPE sum = 0;

	while (iterations-- > 0) {
	    register TYPE *p = state->buf;
	    while (p <= lastone) {
#define	DOIT(i)	sum += p[i]; p[i] = 1;
		DOIT_32(0, 64);
		p +=  2048;
	    }
	}
	use_int(sum);
}
#undef	DOIT

void
mcp512(iter_t iterations, void *cookie)
{
	state_t *state = (state_t *) cookie;
	register TYPE *lastone = state->lastone;
	TYPE* p_save = NULL;

	while (iterations-- > 0) {
	    register TYPE *p = state->buf;
	    register TYPE *dst = state->buf2;
	    while (p <= lastone) {
#define	DOIT(i)	dst[i] = p[i];
		DOIT_32(0, 64);
		p +=  2048;
		dst += 2048;
	    }
	    p_save = p;
	}
	use_pointer(p_save);
}
#undef	DOIT

void
rd1024(iter_t iterations, void *cookie)
{
	state_t *state = (state_t *) cookie;
	register TYPE *lastone = state->lastone;
	register TYPE sum = 0;

	while (iterations-- > 0) {
	    register TYPE *p = state->buf;
	    while (p <= lastone) {
		sum +=
#define	DOIT(i)	p[i]+
		DOIT_15(0, 128)
		p[2048 - 128];
		p +=  2048;
	    }
	}
	use_int(sum);
}
#undef	DOIT

void
wr1024(iter_t iterations, void *cookie)
{
	state_t *state = (state_t *) cookie;
	register TYPE *lastone = state->lastone;

	while (iterations-- > 0) {
	    register TYPE *p = state->buf;
	    while (p <= lastone) {
#define	DOIT(i)	p[i] = 1;
		DOIT_16(0, 128);
		p +=  2048;
	    }
	}
}
#undef	DOIT

void
rdwr1024(iter_t iterations, void *cookie)
{
	state_t *state = (state_t *) cookie;
	register TYPE *lastone = state->lastone;
	register TYPE sum = 0;

	while (iterations-- > 0) {
	    register TYPE *p = state->buf;
	    while (p <= lastone) {
#define	DOIT(i)	sum += p[i]; p[i] = 1;
		DOIT_16(0, 128);
		p +=  2048;
	    }
	}
	use_int(sum);
}
#undef	DOIT

void
mcp1024(iter_t iterations, void *cookie)
{
	state_t *state = (state_t *) cookie;
	register TYPE *lastone = state->lastone;
	TYPE* p_save = NULL;

	while (iterations-- > 0) {
	    register TYPE *p = state->buf;
	    register TYPE *dst = state->buf2;
	    while (p <= lastone) {
#define	DOIT(i)	dst[i] = p[i];
		DOIT_16(0, 128);
		p +=  2048;
		dst += 2048;
	    }
	    p_save = p;
	}
	use_pointer(p_save);
}
#undef	DOIT

void
rd2048(iter_t iterations, void *cookie)
{
	state_t *state = (state_t *) cookie;
	register TYPE *lastone = state->lastone;
	register TYPE sum = 0;

	while (iterations-- > 0) {
	    register TYPE *p = state->buf;
	    while (p <= lastone) {
		sum +=
#define	DOIT(i)	p[i]+
		DOIT_7(0, 256)
		p[2048 - 256];
		p +=  2048;
	    }
	}
	use_int(sum);
}
#undef	DOIT

void
wr2048(iter_t iterations, void *cookie)
{
	state_t *state = (state_t *) cookie;
	register TYPE *lastone = state->lastone;

	while (iterations-- > 0) {
	    register TYPE *p = state->buf;
	    while (p <= lastone) {
#define	DOIT(i)	p[i] = 1;
		DOIT_8(0, 256)
		p +=  2048;
	    }
	}
}
#undef	DOIT

void
rdwr2048(iter_t iterations, void *cookie)
{
	state_t *state = (state_t *) cookie;
	register TYPE *lastone = state->lastone;
	register TYPE sum = 0;

	while (iterations-- > 0) {
	    register TYPE *p = state->buf;
	    while (p <= lastone) {
#define	DOIT(i)	sum += p[i]; p[i] = 1;
		DOIT_8(0, 256);
		p +=  2048;
	    }
	}
	use_int(sum);
}
#undef	DOIT

void
mcp2048(iter_t iterations, void *cookie)
{
	state_t *state = (state_t *) cookie;
	register TYPE *lastone = state->lastone;
	TYPE* p_save = NULL;

	while (iterations-- > 0) {
	    register TYPE *p = state->buf;
	    register TYPE *dst = state->buf2;
	    while (p <= lastone) {
#define	DOIT(i)	dst[i] = p[i];
		DOIT_8(0, 256);
		p += 2048;
		dst += 2048;
	    }
	    p_save = p;
	}
	use_pointer(p_save);
}
#undef	DOIT

void
wrc(iter_t iterations, void *cookie)
{
	state_t *state = (state_t *) cookie;
	register TYPE *lastone = state->lastone;
	TYPE* p_save = NULL;

	while (iterations-- > 0) {
	    register TYPE *p = state->buf;
	    while (p <= lastone) {
#if defined(__x86_64)
		    asm volatile(
			    "mov $1, %%rax\n\t"
			    "vpbroadcastq %%rax, %%zmm0\n\t"
			    "vmovdqa64 %%zmm0, 0(%0)\n\t"
			    "vmovdqa64 %%zmm0, 64(%0)\n\t"
			    "vmovdqa64 %%zmm0, 128(%0)\n\t"
			    "vmovdqa64 %%zmm0, 192(%0)\n\t"
			    "vmovdqa64 %%zmm0, 256(%0)\n\t"
			    "vmovdqa64 %%zmm0, 320(%0)\n\t"
			    "vmovdqa64 %%zmm0, 384(%0)\n\t"
			    "vmovdqa64 %%zmm0, 448(%0)"
			    :
			    : "r" (p)
			    : "rax", "zmm0", "memory"
			    );
		    p += 64;
#elif defined(__aarch64__)
		    asm volatile(
			    "dc zva, %[ptr]\n"
			    "dc zva, %[ptr1]\n"
			    "dc zva, %[ptr2]\n"
			    "dc zva, %[ptr3]\n"
			    "dc zva, %[ptr4]\n"
			    "dc zva, %[ptr5]\n"
			    "dc zva, %[ptr6]\n"
			    "dc zva, %[ptr7]"
			    : // no output
			    : [ptr]  "r" (p),
			      [ptr1] "r" (p + 8),
			      [ptr2] "r" (p + 16),
			      [ptr3] "r" (p + 24),
			      [ptr4] "r" (p + 32),
			      [ptr5] "r" (p + 40),
			      [ptr6] "r" (p + 48),
			      [ptr7] "r" (p + 56)
			    : "memory");
		    p += 64;
#endif
	    }
	    p_save = p;
	}
	use_pointer(p_save);
}

void
wrs(iter_t iterations, void *cookie)
{
	state_t *state = (state_t *) cookie;
	register TYPE *lastone = state->lastone;
	TYPE* p_save = NULL;

	while (iterations-- > 0) {
	    register TYPE *p = state->buf;
	    while (p <= lastone) {
#if defined(__x86_64)
		    long count = 64;
		    asm volatile (
			    "cld\n\t"
			    "rep stosq"
			    : "+D" (p), "+c" (count)
			    : "a" (1)
			    : "memory"
			    );
#else
		    p += 64;
#endif
	    }
	    p_save = p;
	}
	use_pointer(p_save);
}
