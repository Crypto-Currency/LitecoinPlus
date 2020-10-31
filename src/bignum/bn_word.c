/* crypto/bn/bn_word.c */
/* Copyright (C) 1995-1998 Eric Young (eay@cryptsoft.com)
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

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "bn_misc.h"
#include "bn.h"
#include "bn_lcl.h"

BN_LCP_ULONG BN_LCP_mod_word(const BIGNUM_LCP *a, BN_LCP_ULONG w)
	{
#ifndef BN_LCP_LLONG
	BN_LCP_ULONG ret=0;
#else
	BN_LCP_ULLONG ret=0;
#endif
	int i;

	if (w == 0)
		return (BN_LCP_ULONG)-1;

	bn_lcp_check_top(a);
	w&=BN_LCP_MASK2;
	for (i=a->top-1; i>=0; i--)
		{
#ifndef BN_LCP_LLONG
		ret=((ret<<BN_LCP_BITS4)|((a->d[i]>>BN_LCP_BITS4)&BN_LCP_MASK2l))%w;
		ret=((ret<<BN_LCP_BITS4)|(a->d[i]&BN_LCP_MASK2l))%w;
#else
		ret=(BN_LCP_ULLONG)(((ret<<(BN_LCP_ULLONG)BN_LCP_BITS2)|a->d[i])%
			(BN_LCP_ULLONG)w);
#endif
		}
	return((BN_LCP_ULONG)ret);
	}

BN_LCP_ULONG BN_LCP_div_word(BIGNUM_LCP *a, BN_LCP_ULONG w)
	{
	BN_LCP_ULONG ret = 0;
	int i, j;

	bn_lcp_check_top(a);
	w &= BN_LCP_MASK2;

	if (!w)
		/* actually this an error (division by zero) */
		return (BN_LCP_ULONG)-1;
	if (a->top == 0)
		return 0;

	/* normalize input (so bn_lcp_div_words doesn't complain) */
	j = BN_LCP_BITS2 - BN_LCP_num_bits_word(w);
	w <<= j;
	if (!BN_LCP_lshift(a, a, j))
		return (BN_LCP_ULONG)-1;

	for (i=a->top-1; i>=0; i--)
		{
		BN_LCP_ULONG l,d;
		
		l=a->d[i];
		d=bn_lcp_div_words(ret,l,w);
		ret=(l-((d*w)&BN_LCP_MASK2))&BN_LCP_MASK2;
		a->d[i]=d;
		}
	if ((a->top > 0) && (a->d[a->top-1] == 0))
		a->top--;
	ret >>= j;
	bn_lcp_check_top(a);
	return(ret);
	}

int BN_LCP_add_word(BIGNUM_LCP *a, BN_LCP_ULONG w)
	{
	BN_LCP_ULONG l;
	int i;

	bn_lcp_check_top(a);
	w &= BN_LCP_MASK2;

	/* degenerate case: w is zero */
	if (!w) return 1;
	/* degenerate case: a is zero */
	if(BN_LCP_is_zero(a)) return BN_LCP_set_word(a, w);
	/* handle 'a' when negative */
	if (a->neg)
		{
		a->neg=0;
		i=BN_LCP_sub_word(a,w);
		if (!BN_LCP_is_zero(a))
			a->neg=!(a->neg);
		return(i);
		}
	for (i=0;w!=0 && i<a->top;i++)
		{
		a->d[i] = l = (a->d[i]+w)&BN_LCP_MASK2;
		w = (w>l)?1:0;
		}
	if (w && i==a->top)
		{
		if (bn_lcp_wexpand(a,a->top+1) == NULL) return 0;
		a->top++;
		a->d[i]=w;
		}
	bn_lcp_check_top(a);
	return(1);
	}

int BN_LCP_sub_word(BIGNUM_LCP *a, BN_LCP_ULONG w)
	{
	int i;

	bn_lcp_check_top(a);
	w &= BN_LCP_MASK2;

	/* degenerate case: w is zero */
	if (!w) return 1;
	/* degenerate case: a is zero */
	if(BN_LCP_is_zero(a))
		{
		i = BN_LCP_set_word(a,w);
		if (i != 0)
			BN_LCP_set_negative(a, 1);
		return i;
		}
	/* handle 'a' when negative */
	if (a->neg)
		{
		a->neg=0;
		i=BN_LCP_add_word(a,w);
		a->neg=1;
		return(i);
		}

	if ((a->top == 1) && (a->d[0] < w))
		{
		a->d[0]=w-a->d[0];
		a->neg=1;
		return(1);
		}
	i=0;
	for (;;)
		{
		if (a->d[i] >= w)
			{
			a->d[i]-=w;
			break;
			}
		else
			{
			a->d[i]=(a->d[i]-w)&BN_LCP_MASK2;
			i++;
			w=1;
			}
		}
	if ((a->d[i] == 0) && (i == (a->top-1)))
		a->top--;
	bn_lcp_check_top(a);
	return(1);
	}

int BN_LCP_mul_word(BIGNUM_LCP *a, BN_LCP_ULONG w)
	{
	BN_LCP_ULONG ll;

	bn_lcp_check_top(a);
	w&=BN_LCP_MASK2;
	if (a->top)
		{
		if (w == 0)
			BN_LCP_zero(a);
		else
			{
			ll=bn_lcp_mul_words(a->d,a->d,a->top,w);
			if (ll)
				{
				if (bn_lcp_wexpand(a,a->top+1) == NULL) return(0);
				a->d[a->top++]=ll;
				}
			}
		}
	bn_lcp_check_top(a);
	return(1);
	}

