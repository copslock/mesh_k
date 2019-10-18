#ifndef __APP_UPLOAD_H__
#define __APP_UPLOAD_H__

#include "stdio.h"

#include <netdb.h>
#ifdef INET6
/* #define S_FAMILY __s_family */
#define SOCKADDR sockaddr_storage
#define SERVER_PF PF_INET6
#define S_FAMILY sin6_family
#ifndef NI_MAXHOST
#error NI_MAXHOST undefined!!
#endif /* ifndef NI_MAXHOST */
#define BOA_NI_MAXHOST NI_MAXHOST
#else /* ifdef INET6 */
#define SOCKADDR sockaddr_in
#define SERVER_PF PF_INET
#define S_FAMILY sin_family
#define BOA_NI_MAXHOST 20
#endif /* ifdef INET6 */


#define CGI_VERSION				"CGI/1.1"

#ifdef USE_NCSA_CGI_ENV
#define COMMON_CGI_COUNT 8
#else
#define COMMON_CGI_COUNT 6
#endif

#define CGI_ENV_MAX     100
#define CGI_ARGC_MAX 128

#define SERVER_METHOD "http"
#ifdef SUPPORT_ASP
#define REBOOT			10
#endif

/***** Various stuff that you may want to tweak, but probably shouldn't *****/

#define SOCKETBUF_SIZE                          32768
#ifdef HTTP_FILE_SERVER_SUPPORTED
#define CLIENT_STREAM_SIZE                      (64*1024)
#define BUFFER_SIZE                             CLIENT_STREAM_SIZE
#else
#define CLIENT_STREAM_SIZE                      8192
#define BUFFER_SIZE                             8192
#endif
#ifdef SUPPORT_ASP
#define MAX_HEADER_LENGTH			2048
#else
#define MAX_HEADER_LENGTH			1024
#endif

//--------------------------------------------------------------------
//#define CHECK_BURN_SERIAL 1

#if 0
  #define BDBG_BSN(format, arg...) 	  \
		  printf(format , ## arg)
#else
  #define BDBG_BSN(format, arg...)
#endif

#define FW_SIGNATURE			((char *)"cs6c")	// fw signature
#define FW_SIGNATURE_WITH_ROOT	((char *)"cr6c")	// fw signature with root fs
#define ROOT_SIGNATURE			((char *)"r6cr")

#define SQSH_SIGNATURE			((char *)"sqsh")
#define SQSH_SIGNATURE_LE       ((char *)"hsqs")

#define SIG_LEN			4

#define SQUASHFS_MAGIC			0x73717368
#define SQUASHFS_MAGIC_SWAP 	0x68737173

#define SIZE_OF_SQFS_SUPER_BLOCK 640
#define SIZE_OF_CHECKSUM 2
#define OFFSET_OF_LEN 2


/********************** METHODS **********************/
enum HTTP_METHOD { M_GET = 1, M_HEAD, M_PUT, M_POST,
    M_DELETE, M_LINK, M_UNLINK, M_MOVE, M_TRACE
};

/******************* HTTP VERSIONS *******************/
enum HTTP_VERSION { HTTP09=1, HTTP10, HTTP11 };

/************** REQUEST STATUS (req->status) ***************/
enum REQ_STATUS { READ_HEADER, ONE_CR, ONE_LF, TWO_CR,
    BODY_READ, BODY_WRITE,
    WRITE,
    PIPE_READ, PIPE_WRITE,
    IOSHUFFLE,
    DONE,
    TIMED_OUT,
    DEAD
};

/******************* RESPONSE CODES ******************/
enum RESPONSE_CODE { R_CONTINUE = 100,
                     R_REQUEST_OK = 200,
                     R_CREATED,
                     R_ACCEPTED,
                     R_PROVISIONAL,
                     R_NO_CONTENT,
                     R_205,
                     R_PARTIAL_CONTENT,
                     R_MULTIPLE = 300,
                     R_MOVED_PERM,
                     R_MOVED_TEMP,
                     R_303,
                     R_NOT_MODIFIED,
                     R_BAD_REQUEST = 400,
                     R_UNAUTHORIZED,
                     R_PAYMENT,
                     R_FORBIDDEN,
                     R_NOT_FOUND,
                     R_METHOD_NA, /* method not allowed */
                     R_NON_ACC,   /* non acceptable */
                     R_PROXY,     /* proxy auth required */
                     R_REQUEST_TO, /* request timeout */
                     R_CONFLICT,
                     R_GONE,
                     R_LENGTH_REQUIRED,
                     R_PRECONDITION_FAILED,
                     R_REQUEST_URI_TOO_LONG = 414,
                     R_INVALID_RANGE = 416,
                     R_ERROR = 500,
                     R_NOT_IMP,
                     R_BAD_GATEWAY,
                     R_SERVICE_UNAV,
                     R_GATEWAY_TO, /* gateway timeout */
                     R_BAD_VERSION };

/************* ALIAS TYPES (aliasp->type) ***************/
enum ALIAS { ALIAS, SCRIPTALIAS, REDIRECT };

/*********** KEEPALIVE CONSTANTS (req->keepalive) *******/
enum KA_STATUS { KA_INACTIVE, KA_ACTIVE, KA_STOPPED };

/********* CGI STATUS CONSTANTS (req->cgi_status) *******/
enum CGI_STATUS { CGI_PARSE, CGI_BUFFER, CGI_DONE };

/************** CGI TYPE (req->is_cgi) ******************/
enum CGI_TYPE { NPH = 1, CGI, ASP };

/**************** STRUCTURES ****************************/
struct range {
#if defined(ENABLE_LFS)
	off64_t start;
    	off64_t stop;
#else	
    unsigned long start;
    unsigned long stop;
#endif    
    struct range *next;
};

typedef struct range Range;


struct request {                /* pending requests */
    enum REQ_STATUS status;
    enum KA_STATUS keepalive;   /* keepalive status */
    enum HTTP_VERSION http_version;
    enum HTTP_METHOD method;    /* M_GET, M_POST, etc. */
    enum RESPONSE_CODE response_status; /* R_NOT_FOUND, etc.. */

    enum CGI_TYPE cgi_type;
    enum CGI_STATUS cgi_status;

    /* should pollfd_id be zeroable or no ? */
#ifdef HAVE_POLL
    int pollfd_id;
#endif

    char *pathname;             /* pathname of requested file */

    Range *ranges;              /* our Ranges */
    int numranges;

    int data_fd;                /* fd of data */

#if defined(ENABLE_LFS)    
    off64_t filesize;     /* filesize */
    off64_t filepos;      /* position in file */
    off64_t bytes_written; /* total bytes written (sans header) */
#else
    unsigned long filesize;     /* filesize */
    unsigned long filepos;      /* position in file */
    unsigned long bytes_written; /* total bytes written (sans header) */
#endif		
    
    char *data_mem;             /* mmapped/malloced char array */

    char *logline;              /* line to log file */

    char *header_line;          /* beginning of un or incompletely processed header line */
    char *header_end;           /* last known end of header, or end of processed data */
    int parse_pos;              /* how much have we parsed */

    int buffer_start;           /* where the buffer starts */
    int buffer_end;             /* where the buffer ends */

    char *if_modified_since;    /* If-Modified-Since */
    time_t last_modified;       /* Last-modified: */

    /* CGI vars */
    int cgi_env_index;          /* index into array */

    /* Agent and referer for logfiles */
    char *header_host;
    char *header_user_agent;
    char *header_referer;
    char *header_ifrange;
    char *host;                 /* what we end up using for 'host', no matter the contents of header_host */

// davidhsu ------------------------
#ifndef NEW_POST
    int post_data_fd;           /* fd for post data tmpfile */
#else
#if defined(BOA_CGI_SUPPORT)
    int post_data_fd;
#endif
    char *post_data;
    int post_data_len;
    int post_data_idx;
#endif	
//---------------------------------

    char *path_info;            /* env variable */
    char *path_translated;      /* env variable */
    char *script_name;          /* env variable */
    char *query_string;         /* env variable */
    char *content_type;         /* env variable */
    char *content_length;       /* env variable */

// davidhsu ---------	
	unsigned char *upload_data;
	int upload_len;
	int daemon_killed;
//------------------

    struct mmap_entry *mmap_entry_var;

#ifdef USE_AUTH
	char *userName, *password;
	int auth_flag;	// 1: superUser , 2: secondUser
#endif
#ifdef DIGEST_AUTHENTICATION
	char *authorization_nonce;     //ponit to a random number 'nonce' generated by server
	char *authorization_uri;       //point to uri value of authentication field
	char *authorization_response;  //ponit to response value of authorization field
#endif

    /* everything **above** this line is zeroed in sanitize_request */
    /* this may include 'fd' */
    /* in sanitize_request with the 'new' parameter set to 1,
     * kacount is set to ka_max and client_stream_pos is also zeroed.
     * Also, time_last is set to 'NOW'
     */
    int fd;                     /* client's socket fd */
    time_t time_last;           /* time of last succ. op. */
    char local_ip_addr[BOA_NI_MAXHOST]; /* for virtualhost */
    char remote_ip_addr[BOA_NI_MAXHOST]; /* after inet_ntoa */
    unsigned int remote_port;            /* could be used for ident */

    unsigned int kacount;                /* keepalive count */
    int client_stream_pos;      /* how much have we read... */

    /* everything below this line is kept regardless */
#ifdef SUPPORT_ASP		
//	char *post_file_name;		/* only used processing POST */
	unsigned int max_buffer_size;
	char *buffer;	
#else
    char buffer[BUFFER_SIZE + 1]; /* generic I/O buffer */
#endif
    char request_uri[MAX_HEADER_LENGTH + 1]; /* uri */
    char client_stream[CLIENT_STREAM_SIZE]; /* data from client - fit or be hosed */
    char *cgi_env[CGI_ENV_MAX + 4]; /* CGI environment */

#ifdef ACCEPT_ON
    char accept[MAX_ACCEPT_LENGTH]; /* Accept: fields */
#endif

	char *multipart_boundary;
#ifdef HTTP_FILE_SERVER_SUPPORTED
	int req_sort_type;
	int req_sort_order;
#if defined(ENABLE_LFS)	
	off64_t lenFileData;        /* upload file data length */
	off64_t TotalContentLen;
#else	
	int lenFileData;        /* upload file data length */
	int TotalContentLen;
#endif	
	int re_set_req_timeout;
	char *destpath;		/* dsetpath for file upload*/
	FILE *destfile;
	int FileUploadAct;      /* upload file action */
	int req_timeout_count;
	int req_error_state;
	char MagicKey[32];
	char 	UserBrowser[32];
#if defined(ENABLE_LFS)		
	off64_t clen;
#else	
	int clen;
#endif

#endif

    struct request *next;       /* next */
    struct request *prev;       /* previous */
#ifdef BOA_WITH_OPENSSL
    void *ssl;
#elif defined(BOA_WITH_MBEDTLS)
    mbedtls_net_context			mbedtls_client_fd;
	mbedtls_ssl_context      	mbedtls_ssl_ctx;
#endif
};

typedef struct request request;



#endif
