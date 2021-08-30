
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "bn_misc.h"
#include "bn.h"
#include "bn_lcl.h"


void LCP_CRYPTO_lock(int mode, int type, const char *file, int line)
{
	fprintf(stderr, "LCP_CRYPTO_lock, damn it...\n");
}

void ERR_LCP_put_error(int lib, int func, int reason, const char *file, int line)
{
	fprintf(stderr, "ERR_LCP_put_error, damn it...\n");
}

void CRYPTO_THREADID_LCP_current(CRYPTO_THREADID_LCP *id)
{
	fprintf(stderr, "CRYPTO_THREADID_LCP_current, damn it...\n");
}

void ERR_LCP_clear_error()
{
	fprintf(stderr, "ERR_LCP_clear_error, damn it...\n");
}

unsigned long ERR_LCP_peek_last_error(void)
{
	fprintf(stderr, "ERR_LCP_peek_last_error, damn it...\n");
	return(0);
}

extern void OPENSSL_cleanse(void *ptr, size_t len);
void LCP_OPENSSL_cleanse(void *ptr, size_t len)
{
	OPENSSL_cleanse(ptr, len);		// call original hook
}

int LCP_BIO_snprintf(char *buf, size_t n, const char *format, ...)
{
	fprintf(stderr, "LCP_BIO_snprintf, damn it...\n");
	return(0);
}

int LCP_BIO_vsnprintf(char *buf, size_t n, const char *format, va_list args)
{
	fprintf(stderr, "LCP_BIO_vsnprintf, damn it...\n");
	return(0);
}

void LCP_ERR_load_strings(int lib,LCP_ERR_STRING_DATA str[])
{
	fprintf(stderr, "LCP_ERR_load_strings, damn it...\n");
}

const char *LCP_ERR_func_error_string(unsigned long e)
{
	// this function is called in the daemon process, need probably to verify why, for now output is commented.
	
	// fprintf(stderr, "LCP_ERR_func_error_string, damn it...\n");
	return(NULL);
}

