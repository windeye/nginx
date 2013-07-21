
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_CONNECTION_H_INCLUDED_
#define _NGX_CONNECTION_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>


typedef struct ngx_listening_s  ngx_listening_t;

struct ngx_listening_s {
	  /* socket套接字句柄 */
    ngx_socket_t        fd;

		/* 监听的sockaddr地址 */
    struct sockaddr    *sockaddr;
    socklen_t           socklen;    /* size of sockaddr */
    size_t              addr_text_max_len;
		/* 以字符串形式存储IP地址 */
    ngx_str_t           addr_text;

    int                 type;

    int                 backlog;
    int                 rcvbuf;
    int                 sndbuf;
#if (NGX_HAVE_KEEPALIVE_TUNABLE)
    int                 keepidle;
    int                 keepintvl;
    int                 keepcnt;
#endif

    /* handler of accepted connection,tcp连接建立成功后的处理方法 */
    ngx_connection_handler_pt   handler;

		/* 目前主要用于http和mail模块，保存当前监听端口对应着的所有主机名 */
    void               *servers;  /* array of ngx_http_in_addr_t, for example */

    ngx_log_t           log;
    ngx_log_t          *logp;

		/* 如果为新的TCP连接创建内存池，则内存池的大小应该是pool_size */
    size_t              pool_size;
    /* should be here because of the AcceptEx() preread */
    size_t              post_accept_buffer_size;
    /* should be here because of the deferred accept */
		/* TCP_DEFER_ACCETP设置后，如果经过post_accept_timeout秒后仍然没接收到用户
		 * 的数据，则内核直接丢弃连接
		 */
    ngx_msec_t          post_accept_timeout;

		/* 用于组成ngx_listening_t链表的指针 */
    ngx_listening_t    *previous;
		/* 当前监听句柄对应的ngx_connection_t结构体 */
    ngx_connection_t   *connection;

		/* 为1时表示当前监听句柄有效，且执行ngx_init_cycle时不关闭监听端口，为0时则正常关闭
		 * 该标志为框架代码会自动设置 */
    unsigned            open:1;
    unsigned            remain:1;
		/* 为1时表示跳过设置当前ngx_listening_t结构体中的套接字，为0则正常初始化套接字 */
    unsigned            ignore:1;

		/* 目前还没有使用这个标志为 */
    unsigned            bound:1;       /* already bound */
		/* 表示当前监听套接字是否来自前一个进程(如nginx升级) ，为1表示来自前一个
		 * 进程，一般会保留之前已经设置好的套节字，不做改变 */
    unsigned            inherited:1;   /* inherited from previous process */
		/* 目前未使用 */
    unsigned            nonblocking_accept:1;
		/* 表示当前结构体对应的套接字是否已经监听 */
    unsigned            listen:1;
		/* 下面两个标志为目前没有意义 */
    unsigned            nonblocking:1;
    unsigned            shared:1;    /* shared between threads or processes */
		/* 为1时表示nginx会将网络地址转变为字符串形式的地址 */
    unsigned            addr_ntop:1;

#if (NGX_HAVE_INET6 && defined IPV6_V6ONLY)
    unsigned            ipv6only:1;
#endif
    unsigned            keepalive:2;

#if (NGX_HAVE_DEFERRED_ACCEPT)
    unsigned            deferred_accept:1;
    unsigned            delete_deferred:1;
    unsigned            add_deferred:1;
#ifdef SO_ACCEPTFILTER
    char               *accept_filter;
#endif
#endif
#if (NGX_HAVE_SETFIB)
    int                 setfib;
#endif

};


typedef enum {
     NGX_ERROR_ALERT = 0,
     NGX_ERROR_ERR,
     NGX_ERROR_INFO,
     NGX_ERROR_IGNORE_ECONNRESET,
     NGX_ERROR_IGNORE_EINVAL
} ngx_connection_log_error_e;


typedef enum {
     NGX_TCP_NODELAY_UNSET = 0,
     NGX_TCP_NODELAY_SET,
     NGX_TCP_NODELAY_DISABLED
} ngx_connection_tcp_nodelay_e;


typedef enum {
     NGX_TCP_NOPUSH_UNSET = 0,
     NGX_TCP_NOPUSH_SET,
     NGX_TCP_NOPUSH_DISABLED
} ngx_connection_tcp_nopush_e;


#define NGX_LOWLEVEL_BUFFERED  0x0f
#define NGX_SSL_BUFFERED       0x01


struct ngx_connection_s {
	  /* 连接未使用时，充当连接池中空闲链表的next指针，被使用时意义不缺定，
		 * 在http框架中，data指向ngx_http_request_t请求 */
    void               *data;
		/* 连接对应的读写事件 */
    ngx_event_t        *read;
    ngx_event_t        *write;

    ngx_socket_t        fd;

		/* 接收和发送字符流的方法 */
    ngx_recv_pt         recv;
    ngx_send_pt         send;
		/* 以ngx_chain_t为参数接收字符流的方法 */
    ngx_recv_chain_pt   recv_chain;
    ngx_send_chain_pt   send_chain;

		/* 连接对应的监听对象 */
    ngx_listening_t    *listening;

		/*连接上已经发送的字节数 */
    off_t               sent;

    ngx_log_t          *log;

    ngx_pool_t         *pool;

		/* 连接客户端的sockaddr结构体， */
    struct sockaddr    *sockaddr;
    socklen_t           socklen;
		/* 连接客户端的字符串形式的IP地址 */
    ngx_str_t           addr_text;

#if (NGX_SSL)
    ngx_ssl_connection_t  *ssl;
#endif

		/* 本机的监听端口对应的sockaddr结构体，也就是listening监听对象中的sockaddr成员 */
    struct sockaddr    *local_sockaddr;

		/* 接收、缓存客户端发来的字节流，每个事件消费模块可自由决定从连接池中分配多大
		 * 的空间，http模块分配的大小是client_header_buffer_size */
    ngx_buf_t          *buffer;

		/* 将当前连接以双向链表元素的形式添加到ngx_cycle_t的reuseable_connections_queue
		 * 中，表示可以重用的连接 */
    ngx_queue_t         queue;

		/* 连接使用次数 */
    ngx_atomic_uint_t   number;

		/* 处理的请求次数 */
    ngx_uint_t          requests;

    unsigned            buffered:8;

		/* 该连接的日志级别 */
    unsigned            log_error:3;     /* ngx_connection_log_error_e */

		/* 目前无意义 */
    unsigned            unexpected_eof:1;
		/* 为1表示连接已经超时 */
    unsigned            timedout:1;
    unsigned            error:1;
		/* 为1表示对应的TCP连接已经销毁 */
    unsigned            destroyed:1;

    unsigned            idle:1;
		/* 表示可重用，与上面的queue字段是对应使用的 */
    unsigned            reusable:1;
    unsigned            close:1;

		/* 表示正在将文件中的数据发往另一端 */
    unsigned            sendfile:1;
    unsigned            sndlowat:1;
    unsigned            tcp_nodelay:2;   /* ngx_connection_tcp_nodelay_e */
    unsigned            tcp_nopush:2;    /* ngx_connection_tcp_nopush_e */

#if (NGX_HAVE_IOCP)
    unsigned            accept_context_updated:1;
#endif

#if (NGX_HAVE_AIO_SENDFILE)
    unsigned            aio_sendfile:1;
    ngx_buf_t          *busy_sendfile;
#endif

#if (NGX_THREADS)
    ngx_atomic_t        lock;
#endif
};


ngx_listening_t *ngx_create_listening(ngx_conf_t *cf, void *sockaddr,
    socklen_t socklen);
ngx_int_t ngx_set_inherited_sockets(ngx_cycle_t *cycle);
ngx_int_t ngx_open_listening_sockets(ngx_cycle_t *cycle);
void ngx_configure_listening_sockets(ngx_cycle_t *cycle);
void ngx_close_listening_sockets(ngx_cycle_t *cycle);
void ngx_close_connection(ngx_connection_t *c);
ngx_int_t ngx_connection_local_sockaddr(ngx_connection_t *c, ngx_str_t *s,
    ngx_uint_t port);
ngx_int_t ngx_connection_error(ngx_connection_t *c, ngx_err_t err, char *text);

ngx_connection_t *ngx_get_connection(ngx_socket_t s, ngx_log_t *log);
void ngx_free_connection(ngx_connection_t *c);

void ngx_reusable_connection(ngx_connection_t *c, ngx_uint_t reusable);

#endif /* _NGX_CONNECTION_H_INCLUDED_ */
