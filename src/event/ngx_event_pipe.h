
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_EVENT_PIPE_H_INCLUDED_
#define _NGX_EVENT_PIPE_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_event.h>


typedef struct ngx_event_pipe_s  ngx_event_pipe_t;

/* 处理接收自上游的包体的回调方法原型 */
typedef ngx_int_t (*ngx_event_pipe_input_filter_pt)(ngx_event_pipe_t *p,
                                                    ngx_buf_t *buf);
/* 向下游发送响应的回调的原型 */
typedef ngx_int_t (*ngx_event_pipe_output_filter_pt)(void *data,
                                                     ngx_chain_t *chain);


struct ngx_event_pipe_s {
    ngx_connection_t  *upstream;
    ngx_connection_t  *downstream;

		/* 直接接收自上游的缓冲区链表，这个链表的顺序是逆序的，也就是说，链表前端的ngx_buf_t是后
		 * 接收到的响应，仅在接收响应时使用。
		 */
    ngx_chain_t       *free_raw_bufs;
		/* 表示接收到的上游响应缓冲区，通常in链表是在input_filter方法中设置的 */
    ngx_chain_t       *in;
		/* 表示刚刚接收到的一个缓冲区 */
    ngx_chain_t      **last_in;

		/* 保存着将要发送给客户端的缓冲区链表，在写入临时文件成功时，会把in链表中写入文件的缓冲区添加到out链表 */
    ngx_chain_t       *out;
		/* 等待释放的缓冲区 */
    ngx_chain_t       *free;
		/* 上次调用ngx_http_output_fiter方法发送响应时没有发送完的缓冲区链表，这个链表中的
		 * 缓冲区已经保存到请求的output链表中，busy仅用于记录还有多大的响应正等待发送
		 */
    ngx_chain_t       *busy;

    /*
     * the input filter i.e. that moves HTTP/1.1 chunks
     * from the raw bufs to an incoming chain
     */

		/* 一般使用upstream提供的默认ngx_event_pipe_copy_input_filter */
    ngx_event_pipe_input_filter_pt    input_filter;
		/* 一般设置为ngx_http_request_t的地址 */
    void                             *input_ctx;

    ngx_event_pipe_output_filter_pt   output_filter;
		/* 指向ngx_http_request_t结构体 */
    void                             *output_ctx;

    unsigned           read:1;
    unsigned           cacheable:1;
		/* 为1时表示接收上游响应时一次只能接收一个bgx_bug_t缓冲区 */
    unsigned           single_buf:1;
		/* 为1时一旦不再接收上游 */
    unsigned           free_bufs:1;
    unsigned           upstream_done:1;
		/* 与上游连接出错时，会被置为1 */
    unsigned           upstream_error:1;
		/* 表示与上游的连接状态，当nginx与上游的连接已经关闭是，该标志位为1 */
    unsigned           upstream_eof:1;
		/* 表示暂时阻塞读取上游响应的流程，期待通过向下游发送响应来清理出空闲的缓冲区，再用空闲的缓冲区接收
		 * 响应，也就是说，blocked为1时，，会在ngx_event_pipe_t方法循环中先向downstream发送响应，然后
		 * 再去上游读响应 */
    unsigned           upstream_blocked:1;
    unsigned           downstream_done:1;
    unsigned           downstream_error:1;
		/* 不建议置为1 */
    unsigned           cyclic_temp_file:1;

		/* 已经分配的缓冲区的数目，受bufs.num的限制 */
    ngx_int_t          allocated;
		/* 记录了接收上游响应的内存缓冲区大小，bufs.size表示每个缓冲区的大小，bufs.num表示最的的缓冲区数目 */
    ngx_bufs_t         bufs;
		/* 用于设置、比较缓冲区链表中ngx_buf_t结构体的tag标志位 */
    ngx_buf_tag_t      tag;
    /* busy缓冲区中待发送的响应长度触发值，但达到busy_size长度时，必须等待busy缓冲区发送了足够的内容，
		 * 才能继续发送out和in缓冲区中的内容
		 */
    ssize_t            busy_size;

		/* 已经接收到上游响应的包体长度 */
    off_t              read_length;
    off_t              length;

    off_t              max_temp_file_size;
    ssize_t            temp_file_write_size;

    ngx_msec_t         read_timeout;
    ngx_msec_t         send_timeout;
		/* 向下游发送时，TCP连接中设置的sent_lowat水位 */
    ssize_t            send_lowat;

    ngx_pool_t        *pool;
    ngx_log_t         *log;

		/* 表示在接收上游包头阶段已经读取到的响应包体 */
    ngx_chain_t       *preread_bufs;
    size_t             preread_size;
		/* 仅用于缓存文件的场景 */
    ngx_buf_t         *buf_to_file;

		/* 存放上游响应的临时文件 */
    ngx_temp_file_t   *temp_file;

		/* 已经使用的buf_t缓冲区的数目 */
    /* STUB */ int     num;
};


ngx_int_t ngx_event_pipe(ngx_event_pipe_t *p, ngx_int_t do_write);
ngx_int_t ngx_event_pipe_copy_input_filter(ngx_event_pipe_t *p, ngx_buf_t *buf);
ngx_int_t ngx_event_pipe_add_free_buf(ngx_event_pipe_t *p, ngx_buf_t *b);


#endif /* _NGX_EVENT_PIPE_H_INCLUDED_ */
