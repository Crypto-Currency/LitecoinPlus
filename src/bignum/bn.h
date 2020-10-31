/* crypto/bn/bn.h */
/* Copyright (C) 1995-1997 Eric Young (eay@cryptsoft.com)
 * All rights reserved.
 *
 * This package is an SSL implementation written
 * by Eric Young (eay@cryptsoft.com).
 * The implementation was written so as to conform with Netscapes SSL.
 * 
 * This library is free for commercial and non-commercial use as long as
 * the following conditions are aheared to.  The following conditions
 * apply to all code found in this distribution, be it the RC4, RSA,
 * lhash, DES, etc., code; not just the SSL code.  The SSL documentation
 * included with this distribution is covered by the same copyright terms
 * except that the holder is Tim Hudson (tjh@cryptsoft.com).
 * 
 * Copyright remains Eric Young's, and as such any Copyright notices in
 * the code are not to be removed.
 * If this package is used in a product, Eric Young should be given attribution
 * as the author of the parts of the library used.
 * This can be in the form of a textual message at program startup or
 * in documentation (online or textual) provided with the package.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *    "This product includes cryptographic software written by
 *     Eric Young (eay@cryptsoft.com)"
 *    The word 'cryptographic' can be left out if the rouines from the library
 *    being used are not cryptographic related :-).
 * 4. If you include any Windows specific code (or a derivative thereof) from 
 *    the apps directory (application code) you must include an acknowledgement:
 *    "This product includes software written by Tim Hudson (tjh@cryptsoft.com)"
 * 
 * THIS SOFTWARE IS PROVIDED BY ERIC YOUNG ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * 
 * The licence and distribution terms for any publically available version or
 * derivative of this code cannot be changed.  i.e. this code cannot simply be
 * copied and put under another distribution licence
 * [including the GNU Public Licence.]
 */
/* ====================================================================
 * Copyright (c) 1998-2006 The OpenSSL Project.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer. 
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgment:
 *    "This product includes software developed by the OpenSSL Project
 *    for use in the OpenSSL Toolkit. (http://www.openssl.org/)"
 *
 * 4. The names "OpenSSL Toolkit" and "OpenSSL Project" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. For written permission, please contact
 *    openssl-core@openssl.org.
 *
 * 5. Products derived from this software may not be called "OpenSSL"
 *    nor may "OpenSSL" appear in their names without prior written
 *    permission of the OpenSSL Project.
 *
 * 6. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by the OpenSSL Project
 *    for use in the OpenSSL Toolkit (http://www.openssl.org/)"
 *
 * THIS SOFTWARE IS PROVIDED BY THE OpenSSL PROJECT ``AS IS'' AND ANY
 * EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE OpenSSL PROJECT OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 * ====================================================================
 *
 * This product includes cryptographic software written by Eric Young
 * (eay@cryptsoft.com).  This product includes software written by Tim
 * Hudson (tjh@cryptsoft.com).
 *
 */
/* ====================================================================
 * Copyright 2002 Sun Microsystems, Inc. ALL RIGHTS RESERVED.
 *
 * Portions of the attached software ("Contribution") are developed by 
 * SUN MICROSYSTEMS, INC., and are contributed to the OpenSSL project.
 *
 * The Contribution is licensed pursuant to the Eric Young open source
 * license provided above.
 *
 * The binary polynomial arithmetic software is originally written by 
 * Sheueling Chang Shantz and Douglas Stebila of Sun Microsystems Laboratories.
 *
 */

#ifndef HEADER_BNLCP_H
#define HEADER_BNLCP_H


//#include "bn_misc.h"
//#include <bignum/symhacks.h>

//#include <bignum/e_os2.h>
#ifndef OPENSSL_NO_FP_API
#include <stdio.h> /* FILE */
#endif
//#include <bignum/ossl_typ.h>
//#include <bignum/crypto.h>
//#include <bignum/comp.h>


// Check windows
#if _WIN32 || _WIN64
#if _WIN64
#define SIXTY_FOUR_BIT
#undef SIXTY_FOUR_BIT_LONG
#else
#define THIRTY_TWO_BIT
#endif
#endif

// Check GCC
#if __GNUC__
#if __x86_64__ || __ppc64__
#define SIXTY_FOUR_BIT
#undef SIXTY_FOUR_BIT_LONG
#else
#define THIRTY_TWO_BIT
#endif
#endif

#ifdef  __cplusplus
extern "C" {
#endif

/* These preprocessor symbols control various aspects of the bignum headers and
 * library code. They're not defined by any "normal" configuration, as they are
 * intended for development and testing purposes. NB: defining all three can be
 * useful for debugging application code as well as openssl itself.
 *
 * BN_LCP_DEBUG - turn on various debugging alterations to the bignum code
 * BN_LCP_DEBUG_RAND - uses random poisoning of unused words to trip up
 * mismanagement of bignum internals. You must also define BN_LCP_DEBUG.
 */
/* #define BN_LCP_DEBUG */
/* #define BN_LCP_DEBUG_RAND */

#ifndef OPENSSL_SMALL_FOOTPRINT
#define BN_LCP_MUL_COMBA
#define BN_LCP_SQR_COMBA
#define BN_LCP_RECURSION
#endif

/* This next option uses the C libraries (2 word)/(1 word) function.
 * If it is not defined, I use my C version (which is slower).
 * The reason for this flag is that when the particular C compiler
 * library routine is used, and the library is linked with a different
 * compiler, the library is missing.  This mostly happens when the
 * library is built with gcc and then linked using normal cc.  This would
 * be a common occurrence because gcc normally produces code that is
 * 2 times faster than system compilers for the big number stuff.
 * For machines with only one compiler (or shared libraries), this should
 * be on.  Again this in only really a problem on machines
 * using "long long's", are 32bit, and are not using my assembler code. */
#if defined(OPENSSL_SYS_MSDOS) || defined(OPENSSL_SYS_WINDOWS) || \
    defined(OPENSSL_SYS_WIN32) || defined(linux)
# ifndef BN_LCP_DIV2W
#  define BN_LCP_DIV2W
# endif
#endif

/* assuming long is 64bit - this is the DEC Alpha
 * unsigned long long is only 64 bits :-(, don't define
 * BN_LCP_LLONG for the DEC Alpha */
#ifdef SIXTY_FOUR_BIT_LONG
#define BN_LCP_ULLONG	unsigned long long
#define BN_LCP_ULONG	unsigned long
#define BN_LCP_LONG		long
#define BN_LCP_BITS		128
#define BN_LCP_BYTES	8
#define BN_LCP_BITS2	64
#define BN_LCP_BITS4	32
#define BN_LCP_MASK		(0xffffffffffffffffffffffffffffffffLL)
#define BN_LCP_MASK2	(0xffffffffffffffffL)
#define BN_LCP_MASK2l	(0xffffffffL)
#define BN_LCP_MASK2h	(0xffffffff00000000L)
#define BN_LCP_MASK2h1	(0xffffffff80000000L)
#define BN_LCP_TBIT		(0x8000000000000000L)
#define BN_LCP_DEC_CONV	(10000000000000000000UL)
#define BN_LCP_DEC_FMT1	"%lu"
#define BN_LCP_DEC_FMT2	"%019lu"
#define BN_LCP_DEC_NUM	19
#define BN_LCP_HEX_FMT1	"%lX"
#define BN_LCP_HEX_FMT2	"%016lX"
#endif

/* This is where the long long data type is 64 bits, but long is 32.
 * For machines where there are 64bit registers, this is the mode to use.
 * IRIX, on R4000 and above should use this mode, along with the relevant
 * assembler code :-).  Do NOT define BN_LCP_LLONG.
 */
#ifdef SIXTY_FOUR_BIT
#undef BN_LCP_LLONG
#undef BN_LCP_ULLONG
#define BN_LCP_ULONG	unsigned long long
#define BN_LCP_LONG		long long
#define BN_LCP_BITS		128
#define BN_LCP_BYTES	8
#define BN_LCP_BITS2	64
#define BN_LCP_BITS4	32
#define BN_LCP_MASK2	(0xffffffffffffffffLL)
#define BN_LCP_MASK2l	(0xffffffffL)
#define BN_LCP_MASK2h	(0xffffffff00000000LL)
#define BN_LCP_MASK2h1	(0xffffffff80000000LL)
#define BN_LCP_TBIT		(0x8000000000000000LL)
#define BN_LCP_DEC_CONV	(10000000000000000000ULL)
#define BN_LCP_DEC_FMT1	"%llu"
#define BN_LCP_DEC_FMT2	"%019llu"
#define BN_LCP_DEC_NUM	19
#define BN_LCP_HEX_FMT1	"%llX"
#define BN_LCP_HEX_FMT2	"%016llX"
#endif

#ifdef THIRTY_TWO_BIT
#ifdef BN_LCP_LLONG
# if defined(_WIN32) && !defined(__GNUC__)
#  define BN_LCP_ULLONG	unsigned __int64
#  define BN_LCP_MASK	(0xffffffffffffffffI64)
# else
#  define BN_LCP_ULLONG	unsigned long long
#  define BN_LCP_MASK	(0xffffffffffffffffLL)
# endif
#endif
#define BN_LCP_ULONG	unsigned int
#define BN_LCP_LONG		int
#define BN_LCP_BITS		64
#define BN_LCP_BYTES	4
#define BN_LCP_BITS2	32
#define BN_LCP_BITS4	16
#define BN_LCP_MASK2	(0xffffffffL)
#define BN_LCP_MASK2l	(0xffff)
#define BN_LCP_MASK2h1	(0xffff8000L)
#define BN_LCP_MASK2h	(0xffff0000L)
#define BN_LCP_TBIT		(0x80000000L)
#define BN_LCP_DEC_CONV	(1000000000L)
#define BN_LCP_DEC_FMT1	"%u"
#define BN_LCP_DEC_FMT2	"%09u"
#define BN_LCP_DEC_NUM	9
#define BN_LCP_HEX_FMT1	"%X"
#define BN_LCP_HEX_FMT2	"%08X"
#endif

/* 2011-02-22 SMS.
 * In various places, a size_t variable or a type cast to size_t was
 * used to perform integer-only operations on pointers.  This failed on
 * VMS with 64-bit pointers (CC /POINTER_SIZE = 64) because size_t is
 * still only 32 bits.  What's needed in these cases is an integer type
 * with the same size as a pointer, which size_t is not certain to be. 
 * The only fix here is VMS-specific.
 */
#if defined(OPENSSL_SYS_VMS)
# if __INITIAL_POINTER_SIZE == 64
#  define PTR_SIZE_INT long long
# else /* __INITIAL_POINTER_SIZE == 64 */
#  define PTR_SIZE_INT int
# endif /* __INITIAL_POINTER_SIZE == 64 [else] */
#else /* defined(OPENSSL_SYS_VMS) */
# define PTR_SIZE_INT size_t
#endif /* defined(OPENSSL_SYS_VMS) [else] */

#define BN_LCP_DEFAULT_BITS	1280

#define BN_LCP_FLG_MALLOCED		0x01
#define BN_LCP_FLG_STATIC_DATA	0x02
#define BN_LCP_FLG_CONSTTIME	0x04 /* avoid leaking exponent information through timing,
                                      * BN_LCP_mod_exp_mont() will call BN_LCP_mod_exp_mont_consttime,
                                      * BN_LCP_div() will call BN_LCP_div_no_branch,
                                      * BN_LCP_mod_inverse() will call BN_LCP_mod_inverse_no_branch.
                                      */

#ifndef OPENSSL_NO_DEPRECATED
#define BN_LCP_FLG_EXP_CONSTTIME BN_LCP_FLG_CONSTTIME /* deprecated name for the flag */
                                      /* avoid leaking exponent information through timings
                                      * (BN_LCP_mod_exp_mont() will call BN_LCP_mod_exp_mont_consttime) */
#endif

#ifndef OPENSSL_NO_DEPRECATED
#define BN_LCP_FLG_FREE		0x8000	/* used for debuging */
#endif
#define BN_LCP_set_flags(b,n)	((b)->flags|=(n))
#define BN_LCP_get_flags(b,n)	((b)->flags&(n))

/* get a clone of a BIGNUM_LCP with changed flags, for *temporary* use only
 * (the two BIGNUMs cannot not be used in parallel!) */
#define BN_LCP_with_flags(dest,b,n)  ((dest)->d=(b)->d, \
                                  (dest)->top=(b)->top, \
                                  (dest)->dmax=(b)->dmax, \
                                  (dest)->neg=(b)->neg, \
                                  (dest)->flags=(((dest)->flags & BN_LCP_FLG_MALLOCED) \
                                                 |  ((b)->flags & ~BN_LCP_FLG_MALLOCED) \
                                                 |  BN_LCP_FLG_STATIC_DATA \
                                                 |  (n)))

/* Don't use this structure directly. */
typedef struct crypto_threadid_lcp_st
	{
	void *ptr;
	unsigned long val;
	} CRYPTO_THREADID_LCP;
/* Only use CRYPTO_THREADID_LCP_set_[numeric|pointer]() within callbacks */
void CRYPTO_THREADID_LCP_set_numeric(CRYPTO_THREADID_LCP *id, unsigned long val);
void CRYPTO_THREADID_LCP_set_pointer(CRYPTO_THREADID_LCP *id, void *ptr);
int CRYPTO_THREADID_LCP_set_callback(void (*threadid_func)(CRYPTO_THREADID_LCP *));
void (*CRYPTO_THREADID_LCP_get_callback(void))(CRYPTO_THREADID_LCP *);
void CRYPTO_THREADID_LCP_current(CRYPTO_THREADID_LCP *id);
int CRYPTO_THREADID_LCP_cmp(const CRYPTO_THREADID_LCP *a, const CRYPTO_THREADID_LCP *b);
void CRYPTO_THREADID_LCP_cpy(CRYPTO_THREADID_LCP *dest, const CRYPTO_THREADID_LCP *src);
unsigned long CRYPTO_THREADID_LCP_hash(const CRYPTO_THREADID_LCP *id);

/* Already declared in ossl_typ.h */
typedef struct bignum_st_lcp BIGNUM_LCP;
/* Used for temp variables (declaration hidden in bn_lcl.h) */
typedef struct bignum_ctx BN_LCP_CTX;

typedef struct bn_lcp_mont_ctx_st BN_LCP_MONT_CTX;

struct bn_lcp_blinding_st
	{
	BIGNUM_LCP *A;
	BIGNUM_LCP *Ai;
	BIGNUM_LCP *e;
	BIGNUM_LCP *mod; /* just a reference */
#ifndef OPENSSL_NO_DEPRECATED
	unsigned long thread_id; /* added in OpenSSL 0.9.6j and 0.9.7b;
				  * used only by crypto/rsa/rsa_eay.c, rsa_lib.c */
#endif
	CRYPTO_THREADID_LCP tid;
	int counter;
	unsigned long flags;
	BN_LCP_MONT_CTX *m_ctx;
	int (*bn_lcp_mod_exp)(BIGNUM_LCP *r, const BIGNUM_LCP *a, const BIGNUM_LCP *p,
			  const BIGNUM_LCP *m, BN_LCP_CTX *ctx,
			  BN_LCP_MONT_CTX *m_ctx);
	};

struct bignum_st_lcp
	{
	BN_LCP_ULONG *d;	/* Pointer to an array of 'BN_LCP_BITS2' bit chunks. */
	int top;	/* Index of last used d +1. */
	/* The next are internal book keeping for bn_expand. */
	int dmax;	/* Size of the d array. */
	int neg;	/* one if the number is negative */
	int flags;
	};

/* Used for montgomery multiplication */
struct bn_lcp_mont_ctx_st
	{
	int ri;        /* number of bits in R */
	BIGNUM_LCP RR;     /* used to convert to montgomery form */
	BIGNUM_LCP N;      /* The modulus */
	BIGNUM_LCP Ni;     /* R*(1/R mod N) - N*Ni = 1
	                * (Ni is only stored for bignum algorithm) */
	BN_LCP_ULONG n0[2];/* least significant word(s) of Ni;
	                  (type changed with 0.9.9, was "BN_LCP_ULONG n0;" before) */
	int flags;
	};

/* Used for reciprocal division/mod functions
 * It cannot be shared between threads
 */
struct bn_lcp_recp_ctx_st
	{
	BIGNUM_LCP N;	/* the divisor */
	BIGNUM_LCP Nr;	/* the reciprocal */
	int num_bits;
	int shift;
	int flags;
	};

typedef struct bn_lcp_gencb_st BN_LCP_GENCB;

/* Used for slow "generation" functions. */
struct bn_lcp_gencb_st
	{
	unsigned int ver;	/* To handle binary (in)compatibility */
	void *arg;		/* callback-specific data */
	union
		{
		/* if(ver==1) - handles old style callbacks */
		void (*cb_1)(int, int, void *);
		/* if(ver==2) - new callback style */
		int (*cb_2)(int, int, BN_LCP_GENCB *);
		} cb;
	};


typedef struct bn_lcp_blinding_st BN_LCP_BLINDING;
typedef struct bn_lcp_recp_ctx_st BN_LCP_RECP_CTX;


/* Wrapper function to make using BN_LCP_GENCB easier,  */
int BN_LCP_GENCB_call(BN_LCP_GENCB *cb, int a, int b);
/* Macro to populate a BN_LCP_GENCB structure with an "old"-style callback */
#define BN_LCP_GENCB_set_old(gencb, callback, cb_arg) { \
		BN_LCP_GENCB *tmp_gencb = (gencb); \
		tmp_gencb->ver = 1; \
		tmp_gencb->arg = (cb_arg); \
		tmp_gencb->cb.cb_1 = (callback); }
/* Macro to populate a BN_LCP_GENCB structure with a "new"-style callback */
#define BN_LCP_GENCB_set(gencb, callback, cb_arg) { \
		BN_LCP_GENCB *tmp_gencb = (gencb); \
		tmp_gencb->ver = 2; \
		tmp_gencb->arg = (cb_arg); \
		tmp_gencb->cb.cb_2 = (callback); }

#define BN_LCP_prime_checks 0 /* default: select number of iterations
			     based on the size of the number */

/* number of Miller-Rabin iterations for an error rate  of less than 2^-80
 * for random 'b'-bit input, b >= 100 (taken from table 4.4 in the Handbook
 * of Applied Cryptography [Menezes, van Oorschot, Vanstone; CRC Press 1996];
 * original paper: Damgaard, Landrock, Pomerance: Average case error estimates
 * for the strong probable prime test. -- Math. Comp. 61 (1993) 177-194) */
#define BN_LCP_prime_checks_for_size(b) ((b) >= 1300 ?  2 : \
                                (b) >=  850 ?  3 : \
                                (b) >=  650 ?  4 : \
                                (b) >=  550 ?  5 : \
                                (b) >=  450 ?  6 : \
                                (b) >=  400 ?  7 : \
                                (b) >=  350 ?  8 : \
                                (b) >=  300 ?  9 : \
                                (b) >=  250 ? 12 : \
                                (b) >=  200 ? 15 : \
                                (b) >=  150 ? 18 : \
                                /* b >= 100 */ 27)

#define BN_LCP_num_bytes(a)	((BN_LCP_num_bits(a)+7)/8)

/* Note that BN_LCP_abs_is_word didn't work reliably for w == 0 until 0.9.8 */
#define BN_LCP_abs_is_word(a,w) ((((a)->top == 1) && ((a)->d[0] == (BN_LCP_ULONG)(w))) || \
				(((w) == 0) && ((a)->top == 0)))
#define BN_LCP_is_zero(a)       ((a)->top == 0)
#define BN_LCP_is_one(a)        (BN_LCP_abs_is_word((a),1) && !(a)->neg)
#define BN_LCP_is_word(a,w)     (BN_LCP_abs_is_word((a),(w)) && (!(w) || !(a)->neg))
#define BN_LCP_is_odd(a)	    (((a)->top > 0) && ((a)->d[0] & 1))

#define BN_LCP_one(a)	(BN_LCP_set_word((a),1))
#define BN_LCP_zero_ex(a) \
	do { \
		BIGNUM_LCP *_tmp_bn = (a); \
		_tmp_bn->top = 0; \
		_tmp_bn->neg = 0; \
	} while(0)
#ifdef OPENSSL_NO_DEPRECATED
#define BN_LCP_zero(a)	BN_LCP_zero_ex(a)
#else
#define BN_LCP_zero(a)	(BN_LCP_set_word((a),0))
#endif

const BIGNUM_LCP *BN_LCP_value_one(void);
char *	BN_LCP_options(void);
BN_LCP_CTX *BN_LCP_CTX_new(void);
#ifndef OPENSSL_NO_DEPRECATED
void	BN_LCP_CTX_init(BN_LCP_CTX *c);
#endif
void	BN_LCP_CTX_free(BN_LCP_CTX *c);
void	BN_LCP_CTX_start(BN_LCP_CTX *ctx);
BIGNUM_LCP *BN_LCP_CTX_get(BN_LCP_CTX *ctx);
void	BN_LCP_CTX_end(BN_LCP_CTX *ctx);
int     BN_LCP_rand(BIGNUM_LCP *rnd, int bits, int top,int bottom);
int     BN_LCP_pseudo_rand(BIGNUM_LCP *rnd, int bits, int top,int bottom);
int	BN_LCP_rand_range(BIGNUM_LCP *rnd, const BIGNUM_LCP *range);
int	BN_LCP_pseudo_rand_range(BIGNUM_LCP *rnd, const BIGNUM_LCP *range);
int	BN_LCP_num_bits(const BIGNUM_LCP *a);
int	BN_LCP_num_bits_word(BN_LCP_ULONG);
BIGNUM_LCP *BN_LCP_new(void);
void	BN_LCP_init(BIGNUM_LCP *);
void	BN_LCP_clear_free(BIGNUM_LCP *a);
BIGNUM_LCP *BN_LCP_copy(BIGNUM_LCP *a, const BIGNUM_LCP *b);
void	BN_LCP_swap(BIGNUM_LCP *a, BIGNUM_LCP *b);
BIGNUM_LCP *BN_LCP_bin2bn(const unsigned char *s,int len,BIGNUM_LCP *ret);
int	BN_LCP_bn2bin(const BIGNUM_LCP *a, unsigned char *to);
BIGNUM_LCP *BN_LCP_mpi2bn(const unsigned char *s,int len,BIGNUM_LCP *ret);
int	BN_LCP_bn2mpi(const BIGNUM_LCP *a, unsigned char *to);
int	BN_LCP_sub(BIGNUM_LCP *r, const BIGNUM_LCP *a, const BIGNUM_LCP *b);
int	BN_LCP_usub(BIGNUM_LCP *r, const BIGNUM_LCP *a, const BIGNUM_LCP *b);
int	BN_LCP_uadd(BIGNUM_LCP *r, const BIGNUM_LCP *a, const BIGNUM_LCP *b);
int	BN_LCP_add(BIGNUM_LCP *r, const BIGNUM_LCP *a, const BIGNUM_LCP *b);
int	BN_LCP_mul(BIGNUM_LCP *r, const BIGNUM_LCP *a, const BIGNUM_LCP *b, BN_LCP_CTX *ctx);
int	BN_LCP_sqr(BIGNUM_LCP *r, const BIGNUM_LCP *a,BN_LCP_CTX *ctx);
/** BN_LCP_set_negative sets sign of a BIGNUM_LCP
 * \param  b  pointer to the BIGNUM_LCP object
 * \param  n  0 if the BIGNUM_LCP b should be positive and a value != 0 otherwise 
 */
void	BN_LCP_set_negative(BIGNUM_LCP *b, int n);
/** BN_LCP_is_negative returns 1 if the BIGNUM_LCP is negative
 * \param  a  pointer to the BIGNUM_LCP object
 * \return 1 if a < 0 and 0 otherwise
 */
#define BN_LCP_is_negative(a) ((a)->neg != 0)

int	BN_LCP_div(BIGNUM_LCP *dv, BIGNUM_LCP *rem, const BIGNUM_LCP *m, const BIGNUM_LCP *d,
	BN_LCP_CTX *ctx);
#define BN_LCP_mod(rem,m,d,ctx) BN_LCP_div(NULL,(rem),(m),(d),(ctx))
int	BN_LCP_nnmod(BIGNUM_LCP *r, const BIGNUM_LCP *m, const BIGNUM_LCP *d, BN_LCP_CTX *ctx);
int	BN_LCP_mod_add(BIGNUM_LCP *r, const BIGNUM_LCP *a, const BIGNUM_LCP *b, const BIGNUM_LCP *m, BN_LCP_CTX *ctx);
int	BN_LCP_mod_add_quick(BIGNUM_LCP *r, const BIGNUM_LCP *a, const BIGNUM_LCP *b, const BIGNUM_LCP *m);
int	BN_LCP_mod_sub(BIGNUM_LCP *r, const BIGNUM_LCP *a, const BIGNUM_LCP *b, const BIGNUM_LCP *m, BN_LCP_CTX *ctx);
int	BN_LCP_mod_sub_quick(BIGNUM_LCP *r, const BIGNUM_LCP *a, const BIGNUM_LCP *b, const BIGNUM_LCP *m);
int	BN_LCP_mod_mul(BIGNUM_LCP *r, const BIGNUM_LCP *a, const BIGNUM_LCP *b,
	const BIGNUM_LCP *m, BN_LCP_CTX *ctx);
int	BN_LCP_mod_sqr(BIGNUM_LCP *r, const BIGNUM_LCP *a, const BIGNUM_LCP *m, BN_LCP_CTX *ctx);
int	BN_LCP_mod_lshift1(BIGNUM_LCP *r, const BIGNUM_LCP *a, const BIGNUM_LCP *m, BN_LCP_CTX *ctx);
int	BN_LCP_mod_lshift1_quick(BIGNUM_LCP *r, const BIGNUM_LCP *a, const BIGNUM_LCP *m);
int	BN_LCP_mod_lshift(BIGNUM_LCP *r, const BIGNUM_LCP *a, int n, const BIGNUM_LCP *m, BN_LCP_CTX *ctx);
int	BN_LCP_mod_lshift_quick(BIGNUM_LCP *r, const BIGNUM_LCP *a, int n, const BIGNUM_LCP *m);

BN_LCP_ULONG BN_LCP_mod_word(const BIGNUM_LCP *a, BN_LCP_ULONG w);
BN_LCP_ULONG BN_LCP_div_word(BIGNUM_LCP *a, BN_LCP_ULONG w);
int	BN_LCP_mul_word(BIGNUM_LCP *a, BN_LCP_ULONG w);
int	BN_LCP_add_word(BIGNUM_LCP *a, BN_LCP_ULONG w);
int	BN_LCP_sub_word(BIGNUM_LCP *a, BN_LCP_ULONG w);
int	BN_LCP_set_word(BIGNUM_LCP *a, BN_LCP_ULONG w);
BN_LCP_ULONG BN_LCP_get_word(const BIGNUM_LCP *a);

int	BN_LCP_cmp(const BIGNUM_LCP *a, const BIGNUM_LCP *b);
void	BN_LCP_free(BIGNUM_LCP *a);
int	BN_LCP_is_bit_set(const BIGNUM_LCP *a, int n);
int	BN_LCP_lshift(BIGNUM_LCP *r, const BIGNUM_LCP *a, int n);
int	BN_LCP_lshift1(BIGNUM_LCP *r, const BIGNUM_LCP *a);
int	BN_LCP_exp(BIGNUM_LCP *r, const BIGNUM_LCP *a, const BIGNUM_LCP *p,BN_LCP_CTX *ctx);

int	BN_LCP_mod_exp(BIGNUM_LCP *r, const BIGNUM_LCP *a, const BIGNUM_LCP *p,
	const BIGNUM_LCP *m,BN_LCP_CTX *ctx);
int	BN_LCP_mod_exp_mont(BIGNUM_LCP *r, const BIGNUM_LCP *a, const BIGNUM_LCP *p,
	const BIGNUM_LCP *m, BN_LCP_CTX *ctx, BN_LCP_MONT_CTX *m_ctx);
int BN_LCP_mod_exp_mont_consttime(BIGNUM_LCP *rr, const BIGNUM_LCP *a, const BIGNUM_LCP *p,
	const BIGNUM_LCP *m, BN_LCP_CTX *ctx, BN_LCP_MONT_CTX *in_mont);
int	BN_LCP_mod_exp_mont_word(BIGNUM_LCP *r, BN_LCP_ULONG a, const BIGNUM_LCP *p,
	const BIGNUM_LCP *m, BN_LCP_CTX *ctx, BN_LCP_MONT_CTX *m_ctx);
int	BN_LCP_mod_exp2_mont(BIGNUM_LCP *r, const BIGNUM_LCP *a1, const BIGNUM_LCP *p1,
	const BIGNUM_LCP *a2, const BIGNUM_LCP *p2,const BIGNUM_LCP *m,
	BN_LCP_CTX *ctx,BN_LCP_MONT_CTX *m_ctx);
int	BN_LCP_mod_exp_simple(BIGNUM_LCP *r, const BIGNUM_LCP *a, const BIGNUM_LCP *p,
	const BIGNUM_LCP *m,BN_LCP_CTX *ctx);

int	BN_LCP_mask_bits(BIGNUM_LCP *a,int n);
#ifndef OPENSSL_NO_FP_API
int	BN_LCP_print_fp(FILE *fp, const BIGNUM_LCP *a);
#endif
#ifdef HEADER_BIO_H
int	BN_LCP_print(BIO *fp, const BIGNUM_LCP *a);
#else
int	BN_LCP_print(void *fp, const BIGNUM_LCP *a);
#endif
int	BN_LCP_reciprocal(BIGNUM_LCP *r, const BIGNUM_LCP *m, int len, BN_LCP_CTX *ctx);
int	BN_LCP_rshift(BIGNUM_LCP *r, const BIGNUM_LCP *a, int n);
int	BN_LCP_rshift1(BIGNUM_LCP *r, const BIGNUM_LCP *a);
void	BN_LCP_clear(BIGNUM_LCP *a);
BIGNUM_LCP *BN_LCP_dup(const BIGNUM_LCP *a);
int	BN_LCP_ucmp(const BIGNUM_LCP *a, const BIGNUM_LCP *b);
int	BN_LCP_set_bit(BIGNUM_LCP *a, int n);
int	BN_LCP_clear_bit(BIGNUM_LCP *a, int n);
char *	BN_LCP_bn2hex(const BIGNUM_LCP *a);
char *	BN_LCP_bn2dec(const BIGNUM_LCP *a);
int 	BN_LCP_hex2bn(BIGNUM_LCP **a, const char *str);
int 	BN_LCP_dec2bn(BIGNUM_LCP **a, const char *str);
int	BN_LCP_asc2bn(BIGNUM_LCP **a, const char *str);
int	BN_LCP_gcd(BIGNUM_LCP *r,const BIGNUM_LCP *a,const BIGNUM_LCP *b,BN_LCP_CTX *ctx);
int	BN_LCP_kronecker(const BIGNUM_LCP *a,const BIGNUM_LCP *b,BN_LCP_CTX *ctx); /* returns -2 for error */
BIGNUM_LCP *BN_LCP_mod_inverse(BIGNUM_LCP *ret,
	const BIGNUM_LCP *a, const BIGNUM_LCP *n,BN_LCP_CTX *ctx);
BIGNUM_LCP *BN_LCP_mod_sqrt(BIGNUM_LCP *ret,
	const BIGNUM_LCP *a, const BIGNUM_LCP *n,BN_LCP_CTX *ctx);

void	BN_LCP_consttime_swap(BN_LCP_ULONG swap, BIGNUM_LCP *a, BIGNUM_LCP *b, int nwords);

/* Deprecated versions */
#ifndef OPENSSL_NO_DEPRECATED
BIGNUM_LCP *BN_LCP_generate_prime(BIGNUM_LCP *ret,int bits,int safe,
	const BIGNUM_LCP *add, const BIGNUM_LCP *rem,
	void (*callback)(int,int,void *),void *cb_arg);
int	BN_LCP_is_prime(const BIGNUM_LCP *p,int nchecks,
	void (*callback)(int,int,void *),
	BN_LCP_CTX *ctx,void *cb_arg);
int	BN_LCP_is_prime_fasttest(const BIGNUM_LCP *p,int nchecks,
	void (*callback)(int,int,void *),BN_LCP_CTX *ctx,void *cb_arg,
	int do_trial_division);
#endif /* !defined(OPENSSL_NO_DEPRECATED) */

/* Newer versions */
int	BN_LCP_generate_prime_ex(BIGNUM_LCP *ret,int bits,int safe, const BIGNUM_LCP *add,
		const BIGNUM_LCP *rem, BN_LCP_GENCB *cb);
int	BN_LCP_is_prime_ex(const BIGNUM_LCP *p,int nchecks, BN_LCP_CTX *ctx, BN_LCP_GENCB *cb);
int	BN_LCP_is_prime_fasttest_ex(const BIGNUM_LCP *p,int nchecks, BN_LCP_CTX *ctx,
		int do_trial_division, BN_LCP_GENCB *cb);

int BN_LCP_X931_generate_Xpq(BIGNUM_LCP *Xp, BIGNUM_LCP *Xq, int nbits, BN_LCP_CTX *ctx);

int BN_LCP_X931_derive_prime_ex(BIGNUM_LCP *p, BIGNUM_LCP *p1, BIGNUM_LCP *p2,
			const BIGNUM_LCP *Xp, const BIGNUM_LCP *Xp1, const BIGNUM_LCP *Xp2,
			const BIGNUM_LCP *e, BN_LCP_CTX *ctx, BN_LCP_GENCB *cb);
int BN_LCP_X931_generate_prime_ex(BIGNUM_LCP *p, BIGNUM_LCP *p1, BIGNUM_LCP *p2,
			BIGNUM_LCP *Xp1, BIGNUM_LCP *Xp2,
			const BIGNUM_LCP *Xp,
			const BIGNUM_LCP *e, BN_LCP_CTX *ctx,
			BN_LCP_GENCB *cb);

BN_LCP_MONT_CTX *BN_LCP_MONT_CTX_new(void );
void BN_LCP_MONT_CTX_init(BN_LCP_MONT_CTX *ctx);
int BN_LCP_mod_mul_montgomery(BIGNUM_LCP *r,const BIGNUM_LCP *a,const BIGNUM_LCP *b,
	BN_LCP_MONT_CTX *mont, BN_LCP_CTX *ctx);
#define BN_LCP_to_montgomery(r,a,mont,ctx)	BN_LCP_mod_mul_montgomery(\
	(r),(a),&((mont)->RR),(mont),(ctx))
int BN_LCP_from_montgomery(BIGNUM_LCP *r,const BIGNUM_LCP *a,
	BN_LCP_MONT_CTX *mont, BN_LCP_CTX *ctx);
void BN_LCP_MONT_CTX_free(BN_LCP_MONT_CTX *mont);
int BN_LCP_MONT_CTX_set(BN_LCP_MONT_CTX *mont,const BIGNUM_LCP *mod,BN_LCP_CTX *ctx);
BN_LCP_MONT_CTX *BN_LCP_MONT_CTX_copy(BN_LCP_MONT_CTX *to,BN_LCP_MONT_CTX *from);
BN_LCP_MONT_CTX *BN_LCP_MONT_CTX_set_locked(BN_LCP_MONT_CTX **pmont, int lock,
					const BIGNUM_LCP *mod, BN_LCP_CTX *ctx);

/* BN_LCP_BLINDING flags */
#define	BN_LCP_BLINDING_NO_UPDATE	0x00000001
#define	BN_LCP_BLINDING_NO_RECREATE	0x00000002

BN_LCP_BLINDING *BN_LCP_BLINDING_new(const BIGNUM_LCP *A, const BIGNUM_LCP *Ai, BIGNUM_LCP *mod);
void BN_LCP_BLINDING_free(BN_LCP_BLINDING *b);
int BN_LCP_BLINDING_update(BN_LCP_BLINDING *b,BN_LCP_CTX *ctx);
int BN_LCP_BLINDING_convert(BIGNUM_LCP *n, BN_LCP_BLINDING *b, BN_LCP_CTX *ctx);
int BN_LCP_BLINDING_invert(BIGNUM_LCP *n, BN_LCP_BLINDING *b, BN_LCP_CTX *ctx);
int BN_LCP_BLINDING_convert_ex(BIGNUM_LCP *n, BIGNUM_LCP *r, BN_LCP_BLINDING *b, BN_LCP_CTX *);
int BN_LCP_BLINDING_invert_ex(BIGNUM_LCP *n, const BIGNUM_LCP *r, BN_LCP_BLINDING *b, BN_LCP_CTX *);
#ifndef OPENSSL_NO_DEPRECATED
unsigned long BN_LCP_BLINDING_get_thread_id(const BN_LCP_BLINDING *);
void BN_LCP_BLINDING_set_thread_id(BN_LCP_BLINDING *, unsigned long);
#endif
//CRYPTO_THREADID_LCP *BN_LCP_BLINDING_thread_id(BN_LCP_BLINDING *);
unsigned long BN_LCP_BLINDING_get_flags(const BN_LCP_BLINDING *);
void BN_LCP_BLINDING_set_flags(BN_LCP_BLINDING *, unsigned long);
BN_LCP_BLINDING *BN_LCP_BLINDING_create_param(BN_LCP_BLINDING *b,
	const BIGNUM_LCP *e, BIGNUM_LCP *m, BN_LCP_CTX *ctx,
	int (*bn_lcp_mod_exp)(BIGNUM_LCP *r, const BIGNUM_LCP *a, const BIGNUM_LCP *p,
			  const BIGNUM_LCP *m, BN_LCP_CTX *ctx, BN_LCP_MONT_CTX *m_ctx),
	BN_LCP_MONT_CTX *m_ctx);

#ifndef OPENSSL_NO_DEPRECATED
void BN_LCP_set_params(int mul,int high,int low,int mont);
int BN_LCP_get_params(int which); /* 0, mul, 1 high, 2 low, 3 mont */
#endif

void	BN_LCP_RECP_CTX_init(BN_LCP_RECP_CTX *recp);
BN_LCP_RECP_CTX *BN_LCP_RECP_CTX_new(void);
void	BN_LCP_RECP_CTX_free(BN_LCP_RECP_CTX *recp);
int	BN_LCP_RECP_CTX_set(BN_LCP_RECP_CTX *recp,const BIGNUM_LCP *rdiv,BN_LCP_CTX *ctx);
int	BN_LCP_mod_mul_reciprocal(BIGNUM_LCP *r, const BIGNUM_LCP *x, const BIGNUM_LCP *y,
	BN_LCP_RECP_CTX *recp,BN_LCP_CTX *ctx);
int	BN_LCP_mod_exp_recp(BIGNUM_LCP *r, const BIGNUM_LCP *a, const BIGNUM_LCP *p,
	const BIGNUM_LCP *m, BN_LCP_CTX *ctx);
int	BN_LCP_div_recp(BIGNUM_LCP *dv, BIGNUM_LCP *rem, const BIGNUM_LCP *m,
	BN_LCP_RECP_CTX *recp, BN_LCP_CTX *ctx);

#ifndef OPENSSL_NO_EC2M

/* Functions for arithmetic over binary polynomials represented by BIGNUMs. 
 *
 * The BIGNUM_LCP::neg property of BIGNUMs representing binary polynomials is
 * ignored.
 *
 * Note that input arguments are not const so that their bit arrays can
 * be expanded to the appropriate size if needed.
 */

int	BN_LCP_GF2m_add(BIGNUM_LCP *r, const BIGNUM_LCP *a, const BIGNUM_LCP *b); /*r = a + b*/
#define BN_LCP_GF2m_sub(r, a, b) BN_LCP_GF2m_add(r, a, b)
int	BN_LCP_GF2m_mod(BIGNUM_LCP *r, const BIGNUM_LCP *a, const BIGNUM_LCP *p); /*r=a mod p*/
int	BN_LCP_GF2m_mod_mul(BIGNUM_LCP *r, const BIGNUM_LCP *a, const BIGNUM_LCP *b,
	const BIGNUM_LCP *p, BN_LCP_CTX *ctx); /* r = (a * b) mod p */
int	BN_LCP_GF2m_mod_sqr(BIGNUM_LCP *r, const BIGNUM_LCP *a, const BIGNUM_LCP *p,
	BN_LCP_CTX *ctx); /* r = (a * a) mod p */
int	BN_LCP_GF2m_mod_inv(BIGNUM_LCP *r, const BIGNUM_LCP *b, const BIGNUM_LCP *p,
	BN_LCP_CTX *ctx); /* r = (1 / b) mod p */
int	BN_LCP_GF2m_mod_div(BIGNUM_LCP *r, const BIGNUM_LCP *a, const BIGNUM_LCP *b,
	const BIGNUM_LCP *p, BN_LCP_CTX *ctx); /* r = (a / b) mod p */
int	BN_LCP_GF2m_mod_exp(BIGNUM_LCP *r, const BIGNUM_LCP *a, const BIGNUM_LCP *b,
	const BIGNUM_LCP *p, BN_LCP_CTX *ctx); /* r = (a ^ b) mod p */
int	BN_LCP_GF2m_mod_sqrt(BIGNUM_LCP *r, const BIGNUM_LCP *a, const BIGNUM_LCP *p,
	BN_LCP_CTX *ctx); /* r = sqrt(a) mod p */
int	BN_LCP_GF2m_mod_solve_quad(BIGNUM_LCP *r, const BIGNUM_LCP *a, const BIGNUM_LCP *p,
	BN_LCP_CTX *ctx); /* r^2 + r = a mod p */
#define BN_LCP_GF2m_cmp(a, b) BN_LCP_ucmp((a), (b))
/* Some functions allow for representation of the irreducible polynomials
 * as an unsigned int[], say p.  The irreducible f(t) is then of the form:
 *     t^p[0] + t^p[1] + ... + t^p[k]
 * where m = p[0] > p[1] > ... > p[k] = 0.
 */
int	BN_LCP_GF2m_mod_arr(BIGNUM_LCP *r, const BIGNUM_LCP *a, const int p[]);
	/* r = a mod p */
int	BN_LCP_GF2m_mod_mul_arr(BIGNUM_LCP *r, const BIGNUM_LCP *a, const BIGNUM_LCP *b,
	const int p[], BN_LCP_CTX *ctx); /* r = (a * b) mod p */
int	BN_LCP_GF2m_mod_sqr_arr(BIGNUM_LCP *r, const BIGNUM_LCP *a, const int p[],
	BN_LCP_CTX *ctx); /* r = (a * a) mod p */
int	BN_LCP_GF2m_mod_inv_arr(BIGNUM_LCP *r, const BIGNUM_LCP *b, const int p[],
	BN_LCP_CTX *ctx); /* r = (1 / b) mod p */
int	BN_LCP_GF2m_mod_div_arr(BIGNUM_LCP *r, const BIGNUM_LCP *a, const BIGNUM_LCP *b,
	const int p[], BN_LCP_CTX *ctx); /* r = (a / b) mod p */
int	BN_LCP_GF2m_mod_exp_arr(BIGNUM_LCP *r, const BIGNUM_LCP *a, const BIGNUM_LCP *b,
	const int p[], BN_LCP_CTX *ctx); /* r = (a ^ b) mod p */
int	BN_LCP_GF2m_mod_sqrt_arr(BIGNUM_LCP *r, const BIGNUM_LCP *a,
	const int p[], BN_LCP_CTX *ctx); /* r = sqrt(a) mod p */
int	BN_LCP_GF2m_mod_solve_quad_arr(BIGNUM_LCP *r, const BIGNUM_LCP *a,
	const int p[], BN_LCP_CTX *ctx); /* r^2 + r = a mod p */
int	BN_LCP_GF2m_poly2arr(const BIGNUM_LCP *a, int p[], int max);
int	BN_LCP_GF2m_arr2poly(const int p[], BIGNUM_LCP *a);

#endif

/* faster mod functions for the 'NIST primes' 
 * 0 <= a < p^2 */
int BN_LCP_nist_mod_192(BIGNUM_LCP *r, const BIGNUM_LCP *a, const BIGNUM_LCP *p, BN_LCP_CTX *ctx);
int BN_LCP_nist_mod_224(BIGNUM_LCP *r, const BIGNUM_LCP *a, const BIGNUM_LCP *p, BN_LCP_CTX *ctx);
int BN_LCP_nist_mod_256(BIGNUM_LCP *r, const BIGNUM_LCP *a, const BIGNUM_LCP *p, BN_LCP_CTX *ctx);
int BN_LCP_nist_mod_384(BIGNUM_LCP *r, const BIGNUM_LCP *a, const BIGNUM_LCP *p, BN_LCP_CTX *ctx);
int BN_LCP_nist_mod_521(BIGNUM_LCP *r, const BIGNUM_LCP *a, const BIGNUM_LCP *p, BN_LCP_CTX *ctx);

const BIGNUM_LCP *BN_LCP_get0_nist_prime_192(void);
const BIGNUM_LCP *BN_LCP_get0_nist_prime_224(void);
const BIGNUM_LCP *BN_LCP_get0_nist_prime_256(void);
const BIGNUM_LCP *BN_LCP_get0_nist_prime_384(void);
const BIGNUM_LCP *BN_LCP_get0_nist_prime_521(void);

/* library internal functions */

#define bn_lcp_expand(a,bits) ((((((bits+BN_LCP_BITS2-1))/BN_LCP_BITS2)) <= (a)->dmax)?\
	(a):bn_lcp_expand2((a),(bits+BN_LCP_BITS2-1)/BN_LCP_BITS2))
#define bn_lcp_wexpand(a,words) (((words) <= (a)->dmax)?(a):bn_lcp_expand2((a),(words)))
BIGNUM_LCP *bn_lcp_expand2(BIGNUM_LCP *a, int words);
#ifndef OPENSSL_NO_DEPRECATED
BIGNUM_LCP *bn_lcp_dup_expand(const BIGNUM_LCP *a, int words); /* unused */
#endif

/* Bignum consistency macros
 * There is one "API" macro, bn_lcp_fix_top(), for stripping leading zeroes from
 * bignum data after direct manipulations on the data. There is also an
 * "internal" macro, bn_lcp_check_top(), for verifying that there are no leading
 * zeroes. Unfortunately, some auditing is required due to the fact that
 * bn_lcp_fix_top() has become an overabused duct-tape because bignum data is
 * occasionally passed around in an inconsistent state. So the following
 * changes have been made to sort this out;
 * - bn_lcp_fix_top()s implementation has been moved to bn_lcp_correct_top()
 * - if BN_LCP_DEBUG isn't defined, bn_lcp_fix_top() maps to bn_lcp_correct_top(), and
 *   bn_lcp_check_top() is as before.
 * - if BN_LCP_DEBUG *is* defined;
 *   - bn_lcp_check_top() tries to pollute unused words even if the bignum 'top' is
 *     consistent. (ed: only if BN_LCP_DEBUG_RAND is defined)
 *   - bn_lcp_fix_top() maps to bn_lcp_check_top() rather than "fixing" anything.
 * The idea is to have debug builds flag up inconsistent bignums when they
 * occur. If that occurs in a bn_lcp_fix_top(), we examine the code in question; if
 * the use of bn_lcp_fix_top() was appropriate (ie. it follows directly after code
 * that manipulates the bignum) it is converted to bn_lcp_correct_top(), and if it
 * was not appropriate, we convert it permanently to bn_lcp_check_top() and track
 * down the cause of the bug. Eventually, no internal code should be using the
 * bn_lcp_fix_top() macro. External applications and libraries should try this with
 * their own code too, both in terms of building against the openssl headers
 * with BN_LCP_DEBUG defined *and* linking with a version of OpenSSL built with it
 * defined. This not only improves external code, it provides more test
 * coverage for openssl's own code.
 */

#ifdef BN_LCP_DEBUG

/* We only need assert() when debugging */
#include <assert.h>

#ifdef BN_LCP_DEBUG_RAND
/* To avoid "make update" cvs wars due to BN_LCP_DEBUG, use some tricks */
#ifndef RAND_pseudo_bytes
int RAND_pseudo_bytes(unsigned char *buf,int num);
#define BN_LCP_DEBUG_TRIX
#endif
#define bn_lcp_pollute(a) \
	do { \
		const BIGNUM_LCP *_bnum1 = (a); \
		if(_bnum1->top < _bnum1->dmax) { \
			unsigned char _tmp_char; \
			/* We cast away const without the compiler knowing, any \
			 * *genuinely* constant variables that aren't mutable \
			 * wouldn't be constructed with top!=dmax. */ \
			BN_LCP_ULONG *_not_const; \
			memcpy(&_not_const, &_bnum1->d, sizeof(BN_LCP_ULONG*)); \
			RAND_pseudo_bytes(&_tmp_char, 1); \
			memset((unsigned char *)(_not_const + _bnum1->top), _tmp_char, \
				(_bnum1->dmax - _bnum1->top) * sizeof(BN_LCP_ULONG)); \
		} \
	} while(0)
#ifdef BN_LCP_DEBUG_TRIX
#undef RAND_pseudo_bytes
#endif
#else
#define bn_lcp_pollute(a)
#endif
#define bn_lcp_check_top(a) \
	do { \
		const BIGNUM_LCP *_bnum2 = (a); \
		if (_bnum2 != NULL) { \
			assert((_bnum2->top == 0) || \
				(_bnum2->d[_bnum2->top - 1] != 0)); \
			bn_lcp_pollute(_bnum2); \
		} \
	} while(0)

#define bn_lcp_fix_top(a)		bn_lcp_check_top(a)

#define bn_lcp_check_size(bn, bits) bn_lcp_wcheck_size(bn, ((bits+BN_LCP_BITS2-1))/BN_LCP_BITS2)
#define bn_lcp_wcheck_size(bn, words) \
	do { \
		const BIGNUM_LCP *_bnum2 = (bn); \
		assert(words <= (_bnum2)->dmax && words >= (_bnum2)->top); \
	} while(0)

#else /* !BN_LCP_DEBUG */

#define bn_lcp_pollute(a)
#define bn_lcp_check_top(a)
#define bn_lcp_fix_top(a)		bn_lcp_correct_top(a)
#define bn_lcp_check_size(bn, bits)
#define bn_lcp_wcheck_size(bn, words)

#endif

#define bn_lcp_correct_top(a) \
        { \
        BN_LCP_ULONG *ftl; \
	int tmp_top = (a)->top; \
	if (tmp_top > 0) \
		{ \
		for (ftl= &((a)->d[tmp_top-1]); tmp_top > 0; tmp_top--) \
			if (*(ftl--)) break; \
		(a)->top = tmp_top; \
		} \
	bn_lcp_pollute(a); \
	}

BN_LCP_ULONG bn_lcp_mul_add_words(BN_LCP_ULONG *rp, const BN_LCP_ULONG *ap, int num, BN_LCP_ULONG w);
BN_LCP_ULONG bn_lcp_mul_words(BN_LCP_ULONG *rp, const BN_LCP_ULONG *ap, int num, BN_LCP_ULONG w);
void     bn_lcp_sqr_words(BN_LCP_ULONG *rp, const BN_LCP_ULONG *ap, int num);
BN_LCP_ULONG bn_lcp_div_words(BN_LCP_ULONG h, BN_LCP_ULONG l, BN_LCP_ULONG d);
BN_LCP_ULONG bn_lcp_add_words(BN_LCP_ULONG *rp, const BN_LCP_ULONG *ap, const BN_LCP_ULONG *bp,int num);
BN_LCP_ULONG bn_lcp_sub_words(BN_LCP_ULONG *rp, const BN_LCP_ULONG *ap, const BN_LCP_ULONG *bp,int num);

/* Primes from RFC 2409 */
/*BIGNUM_LCP *get_rfc2409_prime_768(BIGNUM_LCP *bn);
BIGNUM_LCP *get_rfc2409_prime_1024(BIGNUM_LCP *bn);*/

/* Primes from RFC 3526 */
/*BIGNUM_LCP *get_rfc3526_prime_1536(BIGNUM_LCP *bn);
BIGNUM_LCP *get_rfc3526_prime_2048(BIGNUM_LCP *bn);
BIGNUM_LCP *get_rfc3526_prime_3072(BIGNUM_LCP *bn);
BIGNUM_LCP *get_rfc3526_prime_4096(BIGNUM_LCP *bn);
BIGNUM_LCP *get_rfc3526_prime_6144(BIGNUM_LCP *bn);
BIGNUM_LCP *get_rfc3526_prime_8192(BIGNUM_LCP *bn);*/

int BN_LCP_bntest_rand(BIGNUM_LCP *rnd, int bits, int top,int bottom);

/* BEGIN ERROR CODES */
/* The following lines are auto generated by the script mkerr.pl. Any changes
 * made after this point may be overwritten when the script is next run.
 */
void ERR_load_BN_LCP_strings(void);

/* Error codes for the BN functions. */

/* Function codes. */
#define BN_LCP_F_BNRAND					 127
#define BN_LCP_F_BN_LCP_BLINDING_CONVERT_EX			 100
#define BN_LCP_F_BN_LCP_BLINDING_CREATE_PARAM			 128
#define BN_LCP_F_BN_LCP_BLINDING_INVERT_EX			 101
#define BN_LCP_F_BN_LCP_BLINDING_NEW				 102
#define BN_LCP_F_BN_LCP_BLINDING_UPDATE				 103
#define BN_LCP_F_BN_LCP_BN2DEC					 104
#define BN_LCP_F_BN_LCP_BN2HEX					 105
#define BN_LCP_F_BN_LCP_CTX_GET					 116
#define BN_LCP_F_BN_LCP_CTX_NEW					 106
#define BN_LCP_F_BN_LCP_CTX_START				 129
#define BN_LCP_F_BN_LCP_DIV					 107
#define BN_LCP_F_BN_LCP_DIV_NO_BRANCH				 138
#define BN_LCP_F_BN_LCP_DIV_RECP				 130
#define BN_LCP_F_BN_LCP_EXP					 123
#define BN_LCP_F_BN_LCP_EXPAND2					 108
#define BN_LCP_F_BN_LCP_EXPAND_INTERNAL				 120
#define BN_LCP_F_BN_LCP_GF2M_MOD				 131
#define BN_LCP_F_BN_LCP_GF2M_MOD_EXP				 132
#define BN_LCP_F_BN_LCP_GF2M_MOD_MUL				 133
#define BN_LCP_F_BN_LCP_GF2M_MOD_SOLVE_QUAD			 134
#define BN_LCP_F_BN_LCP_GF2M_MOD_SOLVE_QUAD_ARR			 135
#define BN_LCP_F_BN_LCP_GF2M_MOD_SQR				 136
#define BN_LCP_F_BN_LCP_GF2M_MOD_SQRT				 137
#define BN_LCP_F_BN_LCP_MOD_EXP2_MONT				 118
#define BN_LCP_F_BN_LCP_MOD_EXP_MONT				 109
#define BN_LCP_F_BN_LCP_MOD_EXP_MONT_CONSTTIME			 124
#define BN_LCP_F_BN_LCP_MOD_EXP_MONT_WORD			 117
#define BN_LCP_F_BN_LCP_MOD_EXP_RECP				 125
#define BN_LCP_F_BN_LCP_MOD_EXP_SIMPLE				 126
#define BN_LCP_F_BN_LCP_MOD_INVERSE				 110
#define BN_LCP_F_BN_LCP_MOD_INVERSE_NO_BRANCH			 139
#define BN_LCP_F_BN_LCP_MOD_LSHIFT_QUICK			 119
#define BN_LCP_F_BN_LCP_MOD_MUL_RECIPROCAL			 111
#define BN_LCP_F_BN_LCP_MOD_SQRT				 121
#define BN_LCP_F_BN_LCP_MPI2BN					 112
#define BN_LCP_F_BN_LCP_NEW					 113
#define BN_LCP_F_BN_LCP_RAND					 114
#define BN_LCP_F_BN_LCP_RAND_RANGE				 122
#define BN_LCP_F_BN_LCP_USUB					 115

/* Reason codes. */
#define BN_LCP_R_ARG2_LT_ARG3				 100
#define BN_LCP_R_BAD_RECIPROCAL				 101
#define BN_LCP_R_BIGNUM_TOO_LONG				 114
#define BN_LCP_R_CALLED_WITH_EVEN_MODULUS			 102
#define BN_LCP_R_DIV_BY_ZERO				 103
#define BN_LCP_R_ENCODING_ERROR				 104
#define BN_LCP_R_EXPAND_ON_STATIC_BIGNUM_DATA		 105
#define BN_LCP_R_INPUT_NOT_REDUCED				 110
#define BN_LCP_R_INVALID_LENGTH				 106
#define BN_LCP_R_INVALID_RANGE				 115
#define BN_LCP_R_NOT_A_SQUARE				 111
#define BN_LCP_R_NOT_INITIALIZED				 107
#define BN_LCP_R_NO_INVERSE					 108
#define BN_LCP_R_NO_SOLUTION				 116
#define BN_LCP_R_P_IS_NOT_PRIME				 112
#define BN_LCP_R_TOO_MANY_ITERATIONS			 113
#define BN_LCP_R_TOO_MANY_TEMPORARY_VARIABLES		 109

#ifdef  __cplusplus
}
#endif
#endif
