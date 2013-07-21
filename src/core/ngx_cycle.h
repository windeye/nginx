
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_CYCLE_H_INCLUDED_
#define _NGX_CYCLE_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>


#ifndef NGX_CYCLE_POOL_SIZE
#define NGX_CYCLE_POOL_SIZE     NGX_DEFAULT_POOL_SIZE
#endif


#define NGX_DEBUG_POINTS_STOP   1
#define NGX_DEBUG_POINTS_ABORT  2


typedef struct ngx_shm_zone_s  ngx_shm_zone_t;

typedef ngx_int_t (*ngx_shm_zone_init_pt) (ngx_shm_zone_t *zone, void *data);

struct ngx_shm_zone_s {
    void                     *data;
    ngx_shm_t                 shm;
    ngx_shm_zone_init_pt      init;
    void                     *tag;
};


struct ngx_cycle_s {
	  /* 第7个指针由ngx_http_module使用,index为6,这个指针设置为指向
		 * 解析http{}是生成的ngx_http_conf_ctx结构体,保存着所有模块存储配置项的结构体的
		 * 指针，首先它是一个数组，数组里的元素是指针，指针指向另一个存储着指针的数组，
		 * 因此是void****。
		 */
    void                  ****conf_ctx;
    ngx_pool_t               *pool;

		/* 这个log对象是在还没有执行ngx_init_cycle前使用的，会把日志输出到屏幕，在执行
		 * init函数后，将会根据配置文件中的配置项，构造出正确的日志文件，再对log重新赋值 */
    ngx_log_t                *log;
    ngx_log_t                 new_log;

    ngx_connection_t        **files;
		/* 可用连接池，与free_connection_n配合使用 */
    ngx_connection_t         *free_connections;
		/* 可用连接池中连接的总数 */
    ngx_uint_t                free_connection_n;

		/* 双向链表容器，元素是ngx_connection_t，表示可重复使用的连接队列 */
    ngx_queue_t               reusable_connections_queue;

		/* 动态数组，里面存储着ngx_listening_t成员，表示监听端口及相关参数 */
    ngx_array_t               listening;
    /* 保存着nginx所要操作的所有目录，如果目录不存在，就会试着创建，而创建目录失败就会
		 * 导致nginx启动失败。*/
    ngx_array_t               paths;
		/* 链表，里面是open_file_t元素，表示nginx已经打开的所有文件，实时上，nginx框架
		 * 不会向这个链表中添加文件，而是由对此感兴趣的模块向其中添加文件路径名，nginx
		 * 会在ngx_init_cycle方法中打开这些文件 */
    ngx_list_t                open_files;
		/* 共享内存链表 */
    ngx_list_t                shared_memory;
    /* 当前进程中所有连接对象的总数，与下面的connections成员配合使用 */
    ngx_uint_t                connection_n;
    ngx_uint_t                files_n;

		/* 当前进程中的所有连接对象 */
    ngx_connection_t         *connections;
		/* 指向当前进程的所有读事件对象，connection_n同时表示读事件的总数 */
    ngx_event_t              *read_events;
		/* 指向当前进程的所有写事件对象，connection_n同时表示写事件的总数 */
    ngx_event_t              *write_events;
    ngx_cycle_t              *old_cycle;

		/* 配置文件相对于安装目录的路径名称: conf/nginx.conf */
    ngx_str_t                 conf_file;
		/* nginx处理配置文件时需要特殊处理的在命令行携带的参数，一般是-g携带的参数 */
    ngx_str_t                 conf_param;
		/* 配置文件所在目录的路径 */
    ngx_str_t                 conf_prefix;
		/* Nginx安装目录的路径 */
    ngx_str_t                 prefix;
		/* 用于进程间同步文件的文件锁名 */
    ngx_str_t                 lock_file;
		/* 使用gethostname获得的主机名 */
    ngx_str_t                 hostname;
};


typedef struct {
     ngx_flag_t               daemon;
     ngx_flag_t               master;

     ngx_msec_t               timer_resolution;

     ngx_int_t                worker_processes;
     ngx_int_t                debug_points;

     ngx_int_t                rlimit_nofile;
     ngx_int_t                rlimit_sigpending;
     off_t                    rlimit_core;

     int                      priority;

     ngx_uint_t               cpu_affinity_n;
     uint64_t                *cpu_affinity;

     char                    *username;
     ngx_uid_t                user;
     ngx_gid_t                group;

     ngx_str_t                working_directory;
     ngx_str_t                lock_file;

     ngx_str_t                pid;
     ngx_str_t                oldpid;

     ngx_array_t              env;
     char                   **environment;

#if (NGX_THREADS)
     ngx_int_t                worker_threads;
     size_t                   thread_stack_size;
#endif

} ngx_core_conf_t;


typedef struct {
     ngx_pool_t              *pool;   /* pcre's malloc() pool */
} ngx_core_tls_t;


#define ngx_is_init_cycle(cycle)  (cycle->conf_ctx == NULL)


ngx_cycle_t *ngx_init_cycle(ngx_cycle_t *old_cycle);
ngx_int_t ngx_create_pidfile(ngx_str_t *name, ngx_log_t *log);
void ngx_delete_pidfile(ngx_cycle_t *cycle);
ngx_int_t ngx_signal_process(ngx_cycle_t *cycle, char *sig);
void ngx_reopen_files(ngx_cycle_t *cycle, ngx_uid_t user);
char **ngx_set_environment(ngx_cycle_t *cycle, ngx_uint_t *last);
ngx_pid_t ngx_exec_new_binary(ngx_cycle_t *cycle, char *const *argv);
uint64_t ngx_get_cpu_affinity(ngx_uint_t n);
ngx_shm_zone_t *ngx_shared_memory_add(ngx_conf_t *cf, ngx_str_t *name,
    size_t size, void *tag);


extern volatile ngx_cycle_t  *ngx_cycle;
extern ngx_array_t            ngx_old_cycles;
extern ngx_module_t           ngx_core_module;
extern ngx_uint_t             ngx_test_config;
extern ngx_uint_t             ngx_quiet_mode;
#if (NGX_THREADS)
extern ngx_tls_key_t          ngx_core_tls_key;
#endif


#endif /* _NGX_CYCLE_H_INCLUDED_ */
