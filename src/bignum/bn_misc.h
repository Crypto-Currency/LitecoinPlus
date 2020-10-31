#ifndef HEADER_BN_MISC_H
#define HEADER_BN_MISC_H

#ifdef  __cplusplus
extern "C" {
#endif


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

#define OPENSSL_LCP_VERSION_TEXT	"LCP subset of BIGNUM"
#define OPENSSL_LCP_VERSION_PTEXT	" part of " OPENSSL_LCP_VERSION_TEXT

#ifdef NO_ASN1_TYPEDEFS
#define ASN1_INTEGER		ASN1_STRING
#define ASN1_ENUMERATED		ASN1_STRING
#define ASN1_BIT_STRING		ASN1_STRING
#define ASN1_OCTET_STRING	ASN1_STRING
#define ASN1_PRINTABLESTRING	ASN1_STRING
#define ASN1_T61STRING		ASN1_STRING
#define ASN1_IA5STRING		ASN1_STRING
#define ASN1_UTCTIME		ASN1_STRING
#define ASN1_GENERALIZEDTIME	ASN1_STRING
#define ASN1_TIME		ASN1_STRING
#define ASN1_GENERALSTRING	ASN1_STRING
#define ASN1_UNIVERSALSTRING	ASN1_STRING
#define ASN1_BMPSTRING		ASN1_STRING
#define ASN1_VISIBLESTRING	ASN1_STRING
#define ASN1_UTF8STRING		ASN1_STRING
#define ASN1_BOOLEAN		int
#define ASN1_NULL		int
#else
typedef struct asn1_string_st ASN1_INTEGER;
typedef struct asn1_string_st ASN1_ENUMERATED;
typedef struct asn1_string_st ASN1_BIT_STRING;
typedef struct asn1_string_st ASN1_OCTET_STRING;
typedef struct asn1_string_st ASN1_PRINTABLESTRING;
typedef struct asn1_string_st ASN1_T61STRING;
typedef struct asn1_string_st ASN1_IA5STRING;
typedef struct asn1_string_st ASN1_GENERALSTRING;
typedef struct asn1_string_st ASN1_UNIVERSALSTRING;
typedef struct asn1_string_st ASN1_BMPSTRING;
typedef struct asn1_string_st ASN1_UTCTIME;
typedef struct asn1_string_st ASN1_TIME;
typedef struct asn1_string_st ASN1_GENERALIZEDTIME;
typedef struct asn1_string_st ASN1_VISIBLESTRING;
typedef struct asn1_string_st ASN1_UTF8STRING;
typedef struct asn1_string_st ASN1_STRING;
typedef int ASN1_BOOLEAN;
typedef int ASN1_NULL;
#endif

typedef struct ASN1_ITEM_st ASN1_ITEM;
typedef struct asn1_pctx_st ASN1_PCTX;

#ifdef OPENSSL_SYS_WIN32
#undef X509_NAME
#undef X509_EXTENSIONS
#undef X509_CERT_PAIR
#undef PKCS7_ISSUER_AND_SERIAL
#undef OCSP_REQUEST
#undef OCSP_RESPONSE
#endif


typedef struct buf_mem_st BUF_MEM;

typedef struct evp_cipher_st EVP_CIPHER;
typedef struct evp_cipher_ctx_st EVP_CIPHER_CTX;
typedef struct evp_pkey_st EVP_PKEY;

typedef struct evp_pkey_asn1_method_st EVP_PKEY_ASN1_METHOD;

typedef struct evp_pkey_method_st EVP_PKEY_METHOD;
typedef struct evp_pkey_ctx_st EVP_PKEY_CTX;

typedef struct dh_st DH;
typedef struct dh_method DH_METHOD;

typedef struct dsa_st DSA;
typedef struct dsa_method DSA_METHOD;

typedef struct rsa_st RSA;
typedef struct rsa_meth_st RSA_METHOD;

typedef struct rand_meth_st RAND_METHOD;

typedef struct ecdh_method ECDH_METHOD;
typedef struct ecdsa_method ECDSA_METHOD;

typedef struct x509_st X509;
typedef struct X509_algor_st X509_ALGOR;
typedef struct X509_crl_st X509_CRL;
typedef struct x509_crl_method_st X509_CRL_METHOD;
typedef struct x509_revoked_st X509_REVOKED;
typedef struct X509_name_st X509_NAME;
typedef struct X509_pubkey_st X509_PUBKEY;
typedef struct x509_store_st X509_STORE;
typedef struct x509_store_ctx_st X509_STORE_CTX;

typedef struct pkcs8_priv_key_info_st PKCS8_PRIV_KEY_INFO;

typedef struct v3_ext_ctx X509V3_CTX;
typedef struct conf_st CONF;

typedef struct store_st STORE;
typedef struct store_method_st STORE_METHOD;

typedef struct ui_st UI;
typedef struct ui_method_st UI_METHOD;

typedef struct st_ERR_FNS ERR_FNS;

typedef struct engine_st ENGINE;
typedef struct ssl_st SSL;
typedef struct ssl_ctx_st SSL_CTX;

typedef struct X509_POLICY_NODE_st X509_POLICY_NODE;
typedef struct X509_POLICY_LEVEL_st X509_POLICY_LEVEL;
typedef struct X509_POLICY_TREE_st X509_POLICY_TREE;
typedef struct X509_POLICY_CACHE_st X509_POLICY_CACHE;

typedef struct AUTHORITY_KEYID_st AUTHORITY_KEYID;
typedef struct DIST_POINT_st DIST_POINT;
typedef struct ISSUING_DIST_POINT_st ISSUING_DIST_POINT;
typedef struct NAME_CONSTRAINTS_st NAME_CONSTRAINTS;

  /* If placed in pkcs12.h, we end up with a circular depency with pkcs7.h */
#define DECLARE_PKCS12_STACK_OF(type) /* Nothing */
#define IMPLEMENT_PKCS12_STACK_OF(type) /* Nothing */

typedef struct crypto_ex_data_st CRYPTO_EX_DATA;
/* Callback types for crypto.h */
typedef void CRYPTO_EX_free(void *parent, void *ptr, CRYPTO_EX_DATA *ad,
					int idx, long argl, void *argp);

typedef struct ocsp_req_ctx_st OCSP_REQ_CTX;
typedef struct ocsp_response_st OCSP_RESPONSE;
typedef struct ocsp_responder_id_st OCSP_RESPID;


const char *LCP_ERR_func_error_string(unsigned long e);

/* CRYPTO_set_mem_functions includes CRYPTO_set_locked_mem_functions --
 * call the latter last if you need different functions */
int CRYPTO_set_mem_functions(void *(*m)(size_t),void *(*r)(void *,size_t), void (*f)(void *));
int CRYPTO_set_locked_mem_functions(void *(*m)(size_t), void (*free_func)(void *));
int CRYPTO_set_mem_ex_functions(void *(*m)(size_t,const char *,int),
                                void *(*r)(void *,size_t,const char *,int),
                                void (*f)(void *));
int CRYPTO_set_locked_mem_ex_functions(void *(*m)(size_t,const char *,int),
                                       void (*free_func)(void *));
int CRYPTO_set_mem_debug_functions(void (*m)(void *,int,const char *,int,int),
				   void (*r)(void *,void *,int,const char *,int,int),
				   void (*f)(void *,int),
				   void (*so)(long),
				   long (*go)(void));
void CRYPTO_get_mem_functions(void *(**m)(size_t),void *(**r)(void *, size_t), void (**f)(void *));
void CRYPTO_get_locked_mem_functions(void *(**m)(size_t), void (**f)(void *));
void CRYPTO_get_mem_ex_functions(void *(**m)(size_t,const char *,int),
                                 void *(**r)(void *, size_t,const char *,int),
                                 void (**f)(void *));
void CRYPTO_get_locked_mem_ex_functions(void *(**m)(size_t,const char *,int),
                                        void (**f)(void *));
void CRYPTO_get_mem_debug_functions(void (**m)(void *,int,const char *,int,int),
				    void (**r)(void *,void *,int,const char *,int,int),
				    void (**f)(void *,int),
				    void (**so)(long),
				    long (**go)(void));

#define CRYPTO_malloc_init()	CRYPTO_set_mem_functions(\
	malloc, realloc, free)

void *CRYPTO_malloc_locked(int num, const char *file, int line);
void CRYPTO_free_locked(void *ptr);
void *CRYPTO_malloc(int num, const char *file, int line);
//char *CRYPTO_strdup(const char *str, const char *file, int line);
void CRYPTO_free(void *ptr);
void *CRYPTO_realloc(void *addr,int num, const char *file, int line);
void *CRYPTO_realloc_clean(void *addr,int old_num,int num,const char *file,
			   int line);
void *CRYPTO_remalloc(void *addr,int num, const char *file, int line);

void LCP_OPENSSL_cleanse(void *ptr, size_t len);

void CRYPTO_set_mem_debug_options(long bits);
long CRYPTO_get_mem_debug_options(void);

#define CRYPTO_push_info(info) \
        CRYPTO_push_info_(info, __FILE__, __LINE__);
int CRYPTO_push_info_(const char *info, const char *file, int line);
int CRYPTO_pop_info(void);
int CRYPTO_remove_all_info(void);

/* for library-internal use */
#define MemCheck_on()	CRYPTO_mem_ctrl(CRYPTO_MEM_CHECK_ENABLE)
#define MemCheck_off()	CRYPTO_mem_ctrl(CRYPTO_MEM_CHECK_DISABLE)
#define is_MemCheck_on() CRYPTO_is_mem_check_on()

#ifndef OPENSSL_malloc
#define OPENSSL_malloc(num)	CRYPTO_malloc((int)num,__FILE__,__LINE__)
//#define OPENSSL_strdup(str)	CRYPTO_strdup((str),__FILE__,__LINE__)
#define OPENSSL_realloc(addr,num) \
	CRYPTO_realloc((char *)addr,(int)num,__FILE__,__LINE__)
#define OPENSSL_realloc_clean(addr,old_num,num) \
	CRYPTO_realloc_clean(addr,old_num,num,__FILE__,__LINE__)
#define OPENSSL_remalloc(addr,num) \
	CRYPTO_remalloc((char **)addr,(int)num,__FILE__,__LINE__)
#define OPENSSL_freeFunc	CRYPTO_free
#define OPENSSL_free(addr)	CRYPTO_free(addr)

#define OPENSSL_malloc_locked(num) \
	CRYPTO_malloc_locked((int)num,__FILE__,__LINE__)
#define OPENSSL_free_locked(addr) CRYPTO_free_locked(addr)
#endif


/* When changing the CRYPTO_LOCK_* list, be sure to maintin the text lock
 * names in cryptlib.c
 */
void LCP_CRYPTO_lock(int mode, int type,const char *file,int line);
#define	CRYPTO_LOCK_ERR			1
#define	CRYPTO_LOCK_EX_DATA		2
#define	CRYPTO_LOCK_X509		3
#define	CRYPTO_LOCK_X509_INFO		4
#define	CRYPTO_LOCK_X509_PKEY		5
#define CRYPTO_LOCK_X509_CRL		6
#define CRYPTO_LOCK_X509_REQ		7
#define CRYPTO_LOCK_DSA			8
#define CRYPTO_LOCK_RSA			9
#define CRYPTO_LOCK_EVP_PKEY		10
#define CRYPTO_LOCK_X509_STORE		11
#define CRYPTO_LOCK_SSL_CTX		12
#define CRYPTO_LOCK_SSL_CERT		13
#define CRYPTO_LOCK_SSL_SESSION		14
#define CRYPTO_LOCK_SSL_SESS_CERT	15
#define CRYPTO_LOCK_SSL			16
#define CRYPTO_LOCK_SSL_METHOD		17
#define CRYPTO_LOCK_RAND		18
#define CRYPTO_LOCK_RAND2		19
#define CRYPTO_LOCK_MALLOC		20
#define CRYPTO_LOCK_BIO			21
#define CRYPTO_LOCK_GETHOSTBYNAME	22
#define CRYPTO_LOCK_GETSERVBYNAME	23
#define CRYPTO_LOCK_READDIR		24
#define CRYPTO_LOCK_RSA_BLINDING	25
#define CRYPTO_LOCK_DH			26
#define CRYPTO_LOCK_MALLOC2		27
#define CRYPTO_LOCK_DSO			28
#define CRYPTO_LOCK_DYNLOCK		29
#define CRYPTO_LOCK_ENGINE		30
#define CRYPTO_LOCK_UI			31
#define CRYPTO_LOCK_ECDSA               32
#define CRYPTO_LOCK_EC			33
#define CRYPTO_LOCK_ECDH		34
#define CRYPTO_LOCK_BN  		35
#define CRYPTO_LOCK_EC_PRE_COMP		36
#define CRYPTO_LOCK_STORE		37
#define CRYPTO_LOCK_COMP		38
#define CRYPTO_LOCK_FIPS		39
#define CRYPTO_LOCK_FIPS2		40
#define CRYPTO_NUM_LOCKS		41

#define CRYPTO_LOCK		1
#define CRYPTO_UNLOCK		2
#define CRYPTO_READ		4
#define CRYPTO_WRITE		8

#ifndef OPENSSL_NO_LOCKING
#ifndef CRYPTO_w_lock
#define CRYPTO_w_lock(type)	\
	LCP_CRYPTO_lock(CRYPTO_LOCK|CRYPTO_WRITE,type,__FILE__,__LINE__)
#define CRYPTO_w_unlock(type)	\
	LCP_CRYPTO_lock(CRYPTO_UNLOCK|CRYPTO_WRITE,type,__FILE__,__LINE__)
#define CRYPTO_r_lock(type)	\
	LCP_CRYPTO_lock(CRYPTO_LOCK|CRYPTO_READ,type,__FILE__,__LINE__)
#define CRYPTO_r_unlock(type)	\
	LCP_CRYPTO_lock(CRYPTO_UNLOCK|CRYPTO_READ,type,__FILE__,__LINE__)
#define CRYPTO_add(addr,amount,type)	\
	CRYPTO_add_lock(addr,amount,type,__FILE__,__LINE__)
#endif
#else
#define CRYPTO_w_lock(a)
#define CRYPTO_w_unlock(a)
#define CRYPTO_r_lock(a)
#define CRYPTO_r_unlock(a)
#define CRYPTO_add(a,b,c)	((*(a))+=(b))
#endif

/* Some applications as well as some parts of OpenSSL need to allocate
   and deallocate locks in a dynamic fashion.  The following typedef
   makes this possible in a type-safe manner.  */
/* struct CRYPTO_dynlock_value has to be defined by the application. */
/*typedef struct
	{
	int references;
	struct CRYPTO_dynlock_value *data;
	} CRYPTO_dynlock;*/


/* Borland C seems too stupid to be able to shift and do longs in
 * the pre-processor :-( */
#define ERR_PACK(l,f,r)		(((((unsigned long)l)&0xffL)*0x1000000)| \
				((((unsigned long)f)&0xfffL)*0x1000)| \
				((((unsigned long)r)&0xfffL)))
#define ERR_GET_LIB(l)		(int)((((unsigned long)l)>>24L)&0xffL)
#define ERR_GET_FUNC(l)		(int)((((unsigned long)l)>>12L)&0xfffL)
#define ERR_GET_REASON(l)	(int)((l)&0xfffL)
#define ERR_FATAL_ERROR(l)	(int)((l)&ERR_R_FATAL)

/* fatal error */
#define ERR_R_FATAL				64
#define	ERR_R_MALLOC_FAILURE			(1|ERR_R_FATAL)
#define	ERR_R_SHOULD_NOT_HAVE_BEEN_CALLED	(2|ERR_R_FATAL)
#define	ERR_R_PASSED_NULL_PARAMETER		(3|ERR_R_FATAL)
#define	ERR_R_INTERNAL_ERROR			(4|ERR_R_FATAL)
#define	ERR_R_DISABLED				(5|ERR_R_FATAL)

#define ERR_LCP_LIB_BN		3

void ERR_LCP_put_error(int lib, int func,int reason,const char *file,int line);

#define ERR_LCP_R_BN_LIB	ERR_LCP_LIB_BN        /* 3 */

#ifndef OPENSSL_NO_ERR
#define ERR_LCP_PUT_error(a,b,c,d,e)	ERR_LCP_put_error(a,b,c,d,e)
#else
#define ERR_LCP_PUT_error(a,b,c,d,e)	ERR_LCP_put_error(a,b,c,NULL,0)
#endif

#define BNLCPerr(f,r)   ERR_LCP_PUT_error(ERR_LCP_LIB_BN,(f),(r),__FILE__,__LINE__)


typedef struct LCP_ERR_string_data_st
	{
	unsigned long error;
	const char *string;
	} LCP_ERR_STRING_DATA;


void LCP_ERR_load_strings(int lib,LCP_ERR_STRING_DATA str[]);


#ifdef __GNUC__
#  define __bio_h__attr__ __attribute__
#else
#  define __bio_h__attr__(x)
#endif
int LCP_BIO_snprintf(char *buf, size_t n, const char *format, ...)
	__bio_h__attr__((__format__(__printf__,3,4)));
int LCP_BIO_vsnprintf(char *buf, size_t n, const char *format, va_list args)
	__bio_h__attr__((__format__(__printf__,3,0)));
#undef __bio_h__attr__

// some compat defines
//#define OPENSSL_NO_ASM
//#define BN_LCP_UMULT_HIGH
#define OPENSSL_NO_BIO

unsigned long ERR_LCP_peek_last_error(void);
void ERR_LCP_clear_error(void );

#ifdef  __cplusplus
}
#endif


#endif
