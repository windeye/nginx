
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_HTTP_UPSTREAM_H_INCLUDED_
#define _NGX_HTTP_UPSTREAM_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_event.h>
#include <ngx_event_connect.h>
#include <ngx_event_pipe.h>
#include <ngx_http.h>


#define NGX_HTTP_UPSTREAM_FT_ERROR           0x00000002
#define NGX_HTTP_UPSTREAM_FT_TIMEOUT         0x00000004
#define NGX_HTTP_UPSTREAM_FT_INVALID_HEADER  0x00000008
#define NGX_HTTP_UPSTREAM_FT_HTTP_500        0x00000010
#define NGX_HTTP_UPSTREAM_FT_HTTP_502        0x00000020
#define NGX_HTTP_UPSTREAM_FT_HTTP_503        0x00000040
#define NGX_HTTP_UPSTREAM_FT_HTTP_504        0x00000080
#define NGX_HTTP_UPSTREAM_FT_HTTP_403        0x00000100
#define NGX_HTTP_UPSTREAM_FT_HTTP_404        0x00000200
#define NGX_HTTP_UPSTREAM_FT_UPDATING        0x00000400
#define NGX_HTTP_UPSTREAM_FT_BUSY_LOCK       0x00000800
#define NGX_HTTP_UPSTREAM_FT_MAX_WAITING     0x00001000
#define NGX_HTTP_UPSTREAM_FT_NOLIVE          0x40000000
#define NGX_HTTP_UPSTREAM_FT_OFF             0x80000000

#define NGX_HTTP_UPSTREAM_FT_STATUS          (NGX_HTTP_UPSTREAM_FT_HTTP_500  \
                                             |NGX_HTTP_UPSTREAM_FT_HTTP_502  \
                                             |NGX_HTTP_UPSTREAM_FT_HTTP_503  \
                                             |NGX_HTTP_UPSTREAM_FT_HTTP_504  \
                                             |NGX_HTTP_UPSTREAM_FT_HTTP_403  \
                                             |NGX_HTTP_UPSTREAM_FT_HTTP_404)

#define NGX_HTTP_UPSTREAM_INVALID_HEADER     40


#define NGX_HTTP_UPSTREAM_IGN_XA_REDIRECT    0x00000002
#define NGX_HTTP_UPSTREAM_IGN_XA_EXPIRES     0x00000004
#define NGX_HTTP_UPSTREAM_IGN_EXPIRES        0x00000008
#define NGX_HTTP_UPSTREAM_IGN_CACHE_CONTROL  0x00000010
#define NGX_HTTP_UPSTREAM_IGN_SET_COOKIE     0x00000020
#define NGX_HTTP_UPSTREAM_IGN_XA_LIMIT_RATE  0x00000040
#define NGX_HTTP_UPSTREAM_IGN_XA_BUFFERING   0x00000080
#define NGX_HTTP_UPSTREAM_IGN_XA_CHARSET     0x00000100


typedef struct {
    ngx_msec_t                       bl_time;
    ngx_uint_t                       bl_state;

    ngx_uint_t                       status;
    time_t                           response_sec;
    ngx_uint_t                       response_msec;
    off_t                            response_length;

    ngx_str_t                       *peer;
} ngx_http_upstream_state_t;


typedef struct {
    ngx_hash_t                       headers_in_hash;
    ngx_array_t                      upstreams;
                                             /* ngx_http_upstream_srv_conf_t */
} ngx_http_upstream_main_conf_t;

typedef struct ngx_http_upstream_srv_conf_s  ngx_http_upstream_srv_conf_t;

typedef ngx_int_t (*ngx_http_upstream_init_pt)(ngx_conf_t *cf,
    ngx_http_upstream_srv_conf_t *us);
typedef ngx_int_t (*ngx_http_upstream_init_peer_pt)(ngx_http_request_t *r,
    ngx_http_upstream_srv_conf_t *us);


typedef struct {
    ngx_http_upstream_init_pt        init_upstream;
    ngx_http_upstream_init_peer_pt   init;
    void                            *data;
} ngx_http_upstream_peer_t;


typedef struct {
    ngx_addr_t                      *addrs;
    ngx_uint_t                       naddrs;
    ngx_uint_t                       weight;
    ngx_uint_t                       max_fails;
    time_t                           fail_timeout;

    unsigned                         down:1;
    unsigned                         backup:1;
} ngx_http_upstream_server_t;


#define NGX_HTTP_UPSTREAM_CREATE        0x0001
#define NGX_HTTP_UPSTREAM_WEIGHT        0x0002
#define NGX_HTTP_UPSTREAM_MAX_FAILS     0x0004
#define NGX_HTTP_UPSTREAM_FAIL_TIMEOUT  0x0008
#define NGX_HTTP_UPSTREAM_DOWN          0x0010
#define NGX_HTTP_UPSTREAM_BACKUP        0x0020


struct ngx_http_upstream_srv_conf_s {
    ngx_http_upstream_peer_t         peer;
    void                           **srv_conf;

    ngx_array_t                     *servers;  /* ngx_http_upstream_server_t */

    ngx_uint_t                       flags;
    ngx_str_t                        host;
    u_char                          *file_name;
    ngx_uint_t                       line;
    in_port_t                        port;
    in_port_t                        default_port;
    ngx_uint_t                       no_port;  /* unsigned no_port:1 */
};


typedef struct {
    ngx_addr_t                      *addr;
    ngx_http_complex_value_t        *value;
} ngx_http_upstream_local_t;


typedef struct {
	  /* 当ngx_http_upstream_t中没有实现resolved成员时，这个结构体才会生效，
		 * 他会定义上游服务器的配置
		 */
    ngx_http_upstream_srv_conf_t    *upstream;

		/* 与上游服务器的连接，发送，接收超时时间 */
    ngx_msec_t                       connect_timeout
    ngx_msec_t                       send_timeout;
    ngx_msec_t                       read_timeout;
		/* 现在没有用到 */
    ngx_msec_t                       timeout;

		/* 发送缓冲区的下限，TCP的SO_SNOLOWAT */
    size_t                           send_lowat;
		/* 接收头部的缓冲区的大小(ngx_http_upstream_t的buffer)，当不转发响应或者
		 * buffering为0的情况下转发响应时，它同样表示接收包体的缓冲区大小。
		 */
    size_t                           buffer_size;

		/*  仅当buffering为1，且想下游转发响应时生效，会被设置到ngx_event_pipe_t的busy_size*/
    size_t                           busy_buffers_size;
		/* 缓存上游响应的临时文件的最大大小 */
    size_t                           max_temp_file_size;
		/* 向缓存文件中一次写入的最大长度 */
    size_t                           temp_file_write_size;

		/* 这三个目前没有意义 */
    size_t                           busy_buffers_size_conf;
    size_t                           max_temp_file_size_conf;
    size_t                           temp_file_write_size_conf;

		/* 以缓存的方式转发上游包体时使用的内存大小 */
    ngx_bufs_t                       bufs;

		/* 转发包头时，可以在转发包头时跳过upstream_t的headers_in（保存着解析完的包头）的
		 * 某些包头，nginx目前仅提供8个位用于位操作
		 */
    ngx_uint_t                       ignore_headers;
    ngx_uint_t                       next_upstream;
		/* store为1时，表示创建的文件/目录的权限 */
    ngx_uint_t                       store_access;
		/* 决定转发响应方式的标志位 */
    ngx_flag_t                       buffering;
		/* 这俩参数目前无意义 */
    ngx_flag_t                       pass_request_headers;
    ngx_flag_t                       pass_request_body;

		/* 为1时，nginx与upstream server交互时不检查nginx与client的连接。 */
    ngx_flag_t                       ignore_client_abort;
		/* 如果解析到的上游包头中，错误码大于400,则会试图与error_page中指定的错误码匹配，
		 * 能陪陪上，则发送匹配上的error_page中指定的响应，否则继续返回上游服务器的状态码
		 */
    ngx_flag_t                       intercept_errors;
		/* 不要设置为1，否则会试图复用临时文件中已经使用过的空间 */
    ngx_flag_t                       cyclic_temp_file;

		/* 临时文件的路径 */
    ngx_path_t                      *temp_path;

		/* 不转发的头部，实际是通过ngx_http_upstream_hide_headers_hash方法，根据hide_headers和
		 * pass_headers数组构造出的需要隐藏的HTTP头散列表。
		 */
    ngx_hash_t                       hide_headers_hash;
		/* 不希望转发给客户端的http头部 */
    ngx_array_t                     *hide_headers;
		/* 转发上游响应时，upstream默认不会转发Date,Server等header，如需要转发则设置到pass_headers */
    ngx_array_t                     *pass_headers;

		/* 连接上游服务器时使用的本机地址 */
    ngx_http_upstream_local_t       *local;

#if (NGX_HTTP_CACHE)
    ngx_shm_zone_t                  *cache;

    ngx_uint_t                       cache_min_uses;
    ngx_uint_t                       cache_use_stale;
    ngx_uint_t                       cache_methods;

    ngx_flag_t                       cache_lock;
    ngx_msec_t                       cache_lock_timeout;

    ngx_array_t                     *cache_valid;
    ngx_array_t                     *cache_bypass;
    ngx_array_t                     *no_cache;
#endif

		/* store为1时，如果需要将上游响应存到文件中，lengths表示存放路径长度，values表示存放路径 */
    ngx_array_t                     *store_lengths;
    ngx_array_t                     *store_values;

		/*  与ngx_http_upstream_t中的store相同 */
    signed                           store:2;
		/* 上面的intercept_errors的例外，如果此值为1，当上游返回404时，
		 * 则直接转发这个错误码给下游，不会与error_page比较。
		 */
    unsigned                         intercept_404:1;
		/* 为1时，会根据headers_in中的X-Accel-Buffering来改变buffering的值，
		 * 因此，此值为1时可以根据上游返回的头部，动态决定上游网速优先还是下游优先
		 */
    unsigned                         change_buffering:1;

#if (NGX_HTTP_SSL)
    ngx_ssl_t                       *ssl;
    ngx_flag_t                       ssl_session_reuse;
#endif

		/* 使用upstream的模块的名字 */
    ngx_str_t                        module;
} ngx_http_upstream_conf_t;


typedef struct {
    ngx_str_t                        name;
    ngx_http_header_handler_pt       handler;
    ngx_uint_t                       offset;
    ngx_http_header_handler_pt       copy_handler;
    ngx_uint_t                       conf;
    ngx_uint_t                       redirect;  /* unsigned   redirect:1; */
} ngx_http_upstream_header_t;


typedef struct {
    ngx_list_t                       headers;

    ngx_uint_t                       status_n;
    ngx_str_t                        status_line;

    ngx_table_elt_t                 *status;
    ngx_table_elt_t                 *date;
    ngx_table_elt_t                 *server;
    ngx_table_elt_t                 *connection;

    ngx_table_elt_t                 *expires;
    ngx_table_elt_t                 *etag;
    ngx_table_elt_t                 *x_accel_expires;
    ngx_table_elt_t                 *x_accel_redirect;
    ngx_table_elt_t                 *x_accel_limit_rate;

    ngx_table_elt_t                 *content_type;
    ngx_table_elt_t                 *content_length;

    ngx_table_elt_t                 *last_modified;
    ngx_table_elt_t                 *location;
    ngx_table_elt_t                 *accept_ranges;
    ngx_table_elt_t                 *www_authenticate;
    ngx_table_elt_t                 *transfer_encoding;

#if (NGX_HTTP_GZIP)
    ngx_table_elt_t                 *content_encoding;
#endif

    off_t                            content_length_n;

    ngx_array_t                      cache_control;

    unsigned                         connection_close:1;
    unsigned                         chunked:1;
} ngx_http_upstream_headers_in_t;


typedef struct {
    ngx_str_t                        host;
    in_port_t                        port;
    ngx_uint_t                       no_port; /* unsigned no_port:1 */

		/* 地址个数 */
    ngx_uint_t                       naddrs;
    in_addr_t                       *addrs;

		/* 设置上游服务器地址 */
    struct sockaddr                 *sockaddr;
    socklen_t                        socklen;

    ngx_resolver_ctx_t              *ctx;
} ngx_http_upstream_resolved_t;


typedef void (*ngx_http_upstream_handler_pt)(ngx_http_request_t *r,
    ngx_http_upstream_t *u);


struct ngx_http_upstream_s {
	  /* 处理读写事件的回调方法，每个阶段该方法是不同的 */
    ngx_http_upstream_handler_pt     read_event_handler;
    ngx_http_upstream_handler_pt     write_event_handler;

		/* 主动向上游发起的连接 */
    ngx_peer_connection_t            peer;

		/* 当向下游转发响应时(ngx_http__request_t中的subrequest_in_memory为0)，如果打开啦缓存
		 * 且认为上游网速更快(buffering为1),这时会使用pipe来转发响应，使用这种机制的话，必须
		 * 由http模块在使用upstream前构造pipe结构体，否则会coredump。*/
    ngx_event_pipe_t                *pipe;

		/* 在实现create_request方法时需要设置它，该参数决定发送什么样的请求给上游服务器 
		 * request_bufs以链表的方式把ngx_buf_t缓冲区链接起来，表示所有需要发送到
		 * 上游的请求内容，所以http模块实现的回调方法就在于构造request_bufs链表
		 */
    ngx_chain_t                     *request_bufs;

		/* 定义啦向下游发送响应的方式,都有啥方式? */
    ngx_output_chain_ctx_t           output;
    ngx_chain_writer_ctx_t           writer;

		/* upstream访问时的所有配置参数 */
    ngx_http_upstream_conf_t        *conf;

    ngx_http_upstream_headers_in_t   headers_in;

		/*  上游服务器地址 */
    ngx_http_upstream_resolved_t    *resolved;

    ngx_buf_t                        from_client;

		/* buffer存储接收自上游服务器发送来的响应，由于它会被复用，所以有以下多种含义：
		 * 1）在process_header解析上游响应包头时，buffer保存完整的响应包头，
		 * 2）当下面的buffering为1时，且此时upstream是向下游转发上游的包体时，buffer
		 *    没有意义
		 * 3）当buffering为0时，buffer会用于反复的接收上游的包体，进而向下游转发
		 * 4）当upstream不用于转发上游包体时，buffer会被用来反复的接收上游的包体，HTTP
		 *    模块实现的input_filter方法需要关注它
		 */
    ngx_buf_t                        buffer;
		/*  来自上游服务器的响应包体的长度 */
    off_t                            length;

		/* out_bufs在两种场景下有不同的意义：
		 * 1) 当不需要转发包体，且使用默认的input_filter方法(也就是ngx_http_upstream_non
		 *    _buffered_filter)处理包体时，out_bufs将会指向响应包体，事实上，out_bufs链表
		 *    中会产生多个ngx_buf_t缓冲区，每个缓冲区都指向buffer缓存的一部分，而这里的一
		 *    部分就是调用recv方法接收到的一段TCP流。
		 * 2) 当需要转发包体到下游时(buffering为0，即以下游网速优先)，这个链表指向上一次
		 *    向下游转发响应到现在这段时间内接收自上游的缓存响应。
		 */
    ngx_chain_t                     *out_bufs;
		/* 上一次向下游转发响应时没有发送完的内容，用于buffering=0时 */
    ngx_chain_t                     *busy_bufs;
		/*  同样用于buffering=0时，用于回收out_bufs中已经发送给下游的ngx_buf_t。*/
    ngx_chain_t                     *free_bufs;

    ngx_int_t                      (*input_filter_init)(void *data);
    ngx_int_t                      (*input_filter)(void *data, ssize_t bytes);
		/* 用于传递http模块自定义的数据结构，就是上面两个函数的data参数 */
    void                            *input_filter_ctx;

#if (NGX_HTTP_CACHE)
    ngx_int_t                      (*create_key)(ngx_http_request_t *r);
#endif
		/* 构造发往上游服务器的请求内容 */
    ngx_int_t                      (*create_request)(ngx_http_request_t *r);
    ngx_int_t                      (*reinit_request)(ngx_http_request_t *r);
		/* 收到上游服务器的响应后就会回调process_header函数，如果它返回NGX_AGAIN，
		 * 说明还没接收到完整的包头，对于本次uostream来说，再次接收到上游服务器发
		 * 来的TCP流时，要继续调用该好书，知道process_header返回非NGX_AGAIN为止。
		 */
    ngx_int_t                      (*process_header)(ngx_http_request_t *r);
    void                           (*abort_request)(ngx_http_request_t *r);
		/* 销毁请求时调用 */
    void                           (*finalize_request)(ngx_http_request_t *r,
                                         ngx_int_t rc);
		/* 在上游返回的响应出现Location或者Refresh头部表示重定向时，会通过
		 * ngx_http_upstream_process_headers方法调用到由http模块实现的rewrite_redirect方法
		 */
    ngx_int_t                      (*rewrite_redirect)(ngx_http_request_t *r,
                                         ngx_table_elt_t *h, size_t prefix);
    ngx_int_t                      (*rewrite_cookie)(ngx_http_request_t *r,
                                         ngx_table_elt_t *h);

    ngx_msec_t                       timeout;

		/* 上游返回的状态码，包体长度等信息 */
    ngx_http_upstream_state_t       *state;

		/* 使用文件缓存时才有意义 */
    ngx_str_t                        method;
		/* 下面两个成员变量仅在记录日志会用到，没有其他用处 */
    ngx_str_t                        schema;
    ngx_str_t                        uri;

    ngx_http_cleanup_pt             *cleanup;

		/* 是否指定文件缓存路径的标志位， */
    unsigned                         store:1;
		/* 是否启用文件缓存 */
    unsigned                         cacheable:1;
    unsigned                         accel:1;
    unsigned                         ssl:1;
#if (NGX_HTTP_CACHE)
    unsigned                         cache_status:3;
#endif

		/*  在向客户端转发上游服务器的包体时才有用，为1时，表示使用多个缓冲区以及
		 *  磁盘文件来转发上游的响应包体，当nginx与上游的网速远大于nginx与下游客户
		 *  端的网速时，让nginx开辟更多的内存甚至磁盘文件来缓存上游的包体，这可以
		 *  减轻上游服务器并发压力，为0时，表示只使用上面的一个固定buffer转发响应。
		 */
    unsigned                         buffering:1;
    unsigned                         keepalive:1;
    unsigned                         upgrade:1;

		/* 表示是否已经向上游发送了请求，为1表示已经发送完或者发送啦一部分，实际上只是
		 * h为了使用ngx_output_chain发送请求，该方法发送请求时会自动记录未发送完的
		 * request_bufs链表，为防止重复发送请求，必须有这样一个标志为记录是否调用啦该方法
		 */
    unsigned                         request_sent:1;
		/* 直接把响应转发给客户端时，表示包体是否发送，如果不转发响应到客户端则无意义 */
    unsigned                         header_sent:1;
};


typedef struct {
    ngx_uint_t                      status;
    ngx_uint_t                      mask;
} ngx_http_upstream_next_t;


typedef struct {
    ngx_str_t   key;
    ngx_str_t   value;
    ngx_uint_t  skip_empty;
} ngx_http_upstream_param_t;


ngx_int_t ngx_http_upstream_header_variable(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);

ngx_int_t ngx_http_upstream_create(ngx_http_request_t *r);
void ngx_http_upstream_init(ngx_http_request_t *r);
ngx_http_upstream_srv_conf_t *ngx_http_upstream_add(ngx_conf_t *cf,
    ngx_url_t *u, ngx_uint_t flags);
char *ngx_http_upstream_bind_set_slot(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf);
char *ngx_http_upstream_param_set_slot(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf);
ngx_int_t ngx_http_upstream_hide_headers_hash(ngx_conf_t *cf,
    ngx_http_upstream_conf_t *conf, ngx_http_upstream_conf_t *prev,
    ngx_str_t *default_hide_headers, ngx_hash_init_t *hash);


#define ngx_http_conf_upstream_srv_conf(uscf, module)                         \
    uscf->srv_conf[module.ctx_index]


extern ngx_module_t        ngx_http_upstream_module;
extern ngx_conf_bitmask_t  ngx_http_upstream_cache_method_mask[];
extern ngx_conf_bitmask_t  ngx_http_upstream_ignore_headers_masks[];


#endif /* _NGX_HTTP_UPSTREAM_H_INCLUDED_ */
