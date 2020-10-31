/* crypto/bn/bn_blind.c */
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

#define BN_LCP_BLINDING_COUNTER	32

BN_LCP_BLINDING *BN_LCP_BLINDING_new(const BIGNUM_LCP *A, const BIGNUM_LCP *Ai, BIGNUM_LCP *mod)
	{
	BN_LCP_BLINDING *ret=NULL;

	bn_lcp_check_top(mod);

	if ((ret=(BN_LCP_BLINDING *)OPENSSL_malloc(sizeof(BN_LCP_BLINDING))) == NULL)
		{
		BNLCPerr(BN_LCP_F_BN_LCP_BLINDING_NEW,ERR_R_MALLOC_FAILURE);
		return(NULL);
		}
	memset(ret,0,sizeof(BN_LCP_BLINDING));
	if (A != NULL)
		{
		if ((ret->A  = BN_LCP_dup(A))  == NULL) goto err;
		}
	if (Ai != NULL)
		{
		if ((ret->Ai = BN_LCP_dup(Ai)) == NULL) goto err;
		}

	/* save a copy of mod in the BN_LCP_BLINDING structure */
	if ((ret->mod = BN_LCP_dup(mod)) == NULL) goto err;
	if (BN_LCP_get_flags(mod, BN_LCP_FLG_CONSTTIME) != 0)
		BN_LCP_set_flags(ret->mod, BN_LCP_FLG_CONSTTIME);

	/* Set the counter to the special value -1
	 * to indicate that this is never-used fresh blinding
	 * that does not need updating before first use. */
	ret->counter = -1;
	CRYPTO_THREADID_LCP_current(&ret->tid);
	return(ret);
err:
	if (ret != NULL) BN_LCP_BLINDING_free(ret);
	return(NULL);
	}

void BN_LCP_BLINDING_free(BN_LCP_BLINDING *r)
	{
	if(r == NULL)
	    return;

	if (r->A  != NULL) BN_LCP_free(r->A );
	if (r->Ai != NULL) BN_LCP_free(r->Ai);
	if (r->e  != NULL) BN_LCP_free(r->e );
	if (r->mod != NULL) BN_LCP_free(r->mod); 
	OPENSSL_free(r);
	}

int BN_LCP_BLINDING_update(BN_LCP_BLINDING *b, BN_LCP_CTX *ctx)
	{
	int ret=0;

	if ((b->A == NULL) || (b->Ai == NULL))
		{
		BNLCPerr(BN_LCP_F_BN_LCP_BLINDING_UPDATE,BN_LCP_R_NOT_INITIALIZED);
		goto err;
		}

	if (b->counter == -1)
		b->counter = 0;

	if (++b->counter == BN_LCP_BLINDING_COUNTER && b->e != NULL &&
		!(b->flags & BN_LCP_BLINDING_NO_RECREATE))
		{
		/* re-create blinding parameters */
		if (!BN_LCP_BLINDING_create_param(b, NULL, NULL, ctx, NULL, NULL))
			goto err;
		}
	else if (!(b->flags & BN_LCP_BLINDING_NO_UPDATE))
		{
		if (!BN_LCP_mod_mul(b->A,b->A,b->A,b->mod,ctx)) goto err;
		if (!BN_LCP_mod_mul(b->Ai,b->Ai,b->Ai,b->mod,ctx)) goto err;
		}

	ret=1;
err:
	if (b->counter == BN_LCP_BLINDING_COUNTER)
		b->counter = 0;
	return(ret);
	}

int BN_LCP_BLINDING_convert(BIGNUM_LCP *n, BN_LCP_BLINDING *b, BN_LCP_CTX *ctx)
	{
	return BN_LCP_BLINDING_convert_ex(n, NULL, b, ctx);
	}

int BN_LCP_BLINDING_convert_ex(BIGNUM_LCP *n, BIGNUM_LCP *r, BN_LCP_BLINDING *b, BN_LCP_CTX *ctx)
	{
	int ret = 1;

	bn_lcp_check_top(n);

	if ((b->A == NULL) || (b->Ai == NULL))
		{
		BNLCPerr(BN_LCP_F_BN_LCP_BLINDING_CONVERT_EX,BN_LCP_R_NOT_INITIALIZED);
		return(0);
		}

	if (b->counter == -1)
		/* Fresh blinding, doesn't need updating. */
		b->counter = 0;
	else if (!BN_LCP_BLINDING_update(b,ctx))
		return(0);

	if (r != NULL)
		{
		if (!BN_LCP_copy(r, b->Ai)) ret=0;
		}

	if (!BN_LCP_mod_mul(n,n,b->A,b->mod,ctx)) ret=0;
	
	return ret;
	}

int BN_LCP_BLINDING_invert(BIGNUM_LCP *n, BN_LCP_BLINDING *b, BN_LCP_CTX *ctx)
	{
	return BN_LCP_BLINDING_invert_ex(n, NULL, b, ctx);
	}

int BN_LCP_BLINDING_invert_ex(BIGNUM_LCP *n, const BIGNUM_LCP *r, BN_LCP_BLINDING *b, BN_LCP_CTX *ctx)
	{
	int ret;

	bn_lcp_check_top(n);

	if (r != NULL)
		ret = BN_LCP_mod_mul(n, n, r, b->mod, ctx);
	else
		{
		if (b->Ai == NULL)
			{
			BNLCPerr(BN_LCP_F_BN_LCP_BLINDING_INVERT_EX,BN_LCP_R_NOT_INITIALIZED);
			return(0);
			}
		ret = BN_LCP_mod_mul(n, n, b->Ai, b->mod, ctx);
		}

	bn_lcp_check_top(n);
	return(ret);
	}

#ifndef OPENSSL_NO_DEPRECATED
unsigned long BN_LCP_BLINDING_get_thread_id(const BN_LCP_BLINDING *b)
	{
	return b->thread_id;
	}

void BN_LCP_BLINDING_set_thread_id(BN_LCP_BLINDING *b, unsigned long n)
	{
	b->thread_id = n;
	}
#endif

CRYPTO_THREADID_LCP *BN_LCP_BLINDING_thread_id(BN_LCP_BLINDING *b)
	{
	return &b->tid;
	}

unsigned long BN_LCP_BLINDING_get_flags(const BN_LCP_BLINDING *b)
	{
	return b->flags;
	}

void BN_LCP_BLINDING_set_flags(BN_LCP_BLINDING *b, unsigned long flags)
	{
	b->flags = flags;
	}

BN_LCP_BLINDING *BN_LCP_BLINDING_create_param(BN_LCP_BLINDING *b,
	const BIGNUM_LCP *e, BIGNUM_LCP *m, BN_LCP_CTX *ctx,
	int (*bn_lcp_mod_exp)(BIGNUM_LCP *r, const BIGNUM_LCP *a, const BIGNUM_LCP *p,
			  const BIGNUM_LCP *m, BN_LCP_CTX *ctx, BN_LCP_MONT_CTX *m_ctx),
	BN_LCP_MONT_CTX *m_ctx)
{
	int    retry_counter = 32;
	BN_LCP_BLINDING *ret = NULL;

	if (b == NULL)
		ret = BN_LCP_BLINDING_new(NULL, NULL, m);
	else
		ret = b;

	if (ret == NULL)
		goto err;

	if (ret->A  == NULL && (ret->A  = BN_LCP_new()) == NULL)
		goto err;
	if (ret->Ai == NULL && (ret->Ai	= BN_LCP_new()) == NULL)
		goto err;

	if (e != NULL)
		{
		if (ret->e != NULL)
			BN_LCP_free(ret->e);
		ret->e = BN_LCP_dup(e);
		}
	if (ret->e == NULL)
		goto err;

	if (bn_lcp_mod_exp != NULL)
		ret->bn_lcp_mod_exp = bn_lcp_mod_exp;
	if (m_ctx != NULL)
		ret->m_ctx = m_ctx;

	do {
		if (!BN_LCP_rand_range(ret->A, ret->mod)) goto err;
		if (BN_LCP_mod_inverse(ret->Ai, ret->A, ret->mod, ctx) == NULL)
			{
			/* this should almost never happen for good RSA keys */
			unsigned long error = ERR_LCP_peek_last_error();
			if (ERR_GET_REASON(error) == BN_LCP_R_NO_INVERSE)
				{
				if (retry_counter-- == 0)
				{
					BNLCPerr(BN_LCP_F_BN_LCP_BLINDING_CREATE_PARAM,
						BN_LCP_R_TOO_MANY_ITERATIONS);
					goto err;
				}
				ERR_LCP_clear_error();
				}
			else
				goto err;
			}
		else
			break;
	} while (1);

	if (ret->bn_lcp_mod_exp != NULL && ret->m_ctx != NULL)
		{
		if (!ret->bn_lcp_mod_exp(ret->A, ret->A, ret->e, ret->mod, ctx, ret->m_ctx))
			goto err;
		}
	else
		{
		if (!BN_LCP_mod_exp(ret->A, ret->A, ret->e, ret->mod, ctx))
			goto err;
		}

	return ret;
err:
	if (b == NULL && ret != NULL)
		{
		BN_LCP_BLINDING_free(ret);
		ret = NULL;
		}

	return ret;
}
