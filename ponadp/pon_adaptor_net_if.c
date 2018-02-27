/**************************************************************
* 文件名称: 
* 文件描述:
* 版           本: 
* 修改历史:
*     <修改者>     <时间>      <版本 >     <描述>

**************************************************************/

#include "pon_adaptor_net_if.h"

#if DEFUNC("网络通信接口")

/*
存入分段数据
*/
static unsigned 
int cdt_pon_net_msg_push(CDT_NET_MULTI_MSG *pmsg, void *pdata, unsigned int len)
{
    if ((NULL == pdata)|| (NULL == pmsg))
    {
        printf("Parameter is null !\n");
        return CDT_MDW_NET_RC_PARAM_NULL;
    }

    if (pmsg->count >= IO_MAXSIZE)
    {
        printf("The parameter value is out of range !\n");
        return CDT_MDW_NET_RC_PARAM_OUT_OF_RANGE;
    }

    pmsg->io[pmsg->count].iov_base = pdata;
    pmsg->io[pmsg->count].iov_len = len;
    pmsg->count++;
    return 0;
}

/*
收包
*/
static unsigned int 
cdt_pon_net_rec_packet(int fd, void *pdata, unsigned int data_len, unsigned int *prec_len)
{
    int recv_num = 0;

    if (!prec_len)
    {
        printf("Parameter is null !\n");
        return CDT_MDW_NET_RC_PARAM_NULL;
    }

    if (!pdata || 0 == data_len)
    {
        //没有需要接收的数据
        return 0;
    }

    recv_num = recv(fd, pdata, data_len, 0);
    if(recv_num <  0)
    {
      switch(errno)
      {
        case EINTR:
        case EAGAIN:
            //try again
            errno = 0;
            break;
        case ETIMEDOUT:
            //keepallive error
            break;
        default: 
                printf("AF_INET server bad,ERRNO:%d,errno description:%s\n",errno,strerror(errno));
                return CDT_MDW_NET_RC_REC_PACKET;
      }

    }
    else
    if (0 == recv_num)
    {
        printf("remote disconnected !\n");  
        return CDT_MDW_NET_RC_REMOTE_DISCONNECT;
    }
    else
    {
      *prec_len = recv_num;
    }
    
    return 0;
}

/*
接收固定长度包
*/
unsigned int 
cdt_pon_net_rec_packet_fix(int fd, void *pdata, unsigned int data_len)
{
    unsigned int read_len = 0;
    unsigned int rc;
    unsigned int len;
  
    do 
    {
        len = 0;
        rc = cdt_pon_net_rec_packet(fd, ((char*)pdata + read_len), (data_len - read_len), &len);
        if(rc)  
        {          
            printf("read error !\n");
            return rc;  
        }
        
        read_len += len;
    }while(data_len > read_len);

    return 0;
}

static unsigned int 
cdt_pon_net_rec_packet_peek(int fd, void *pdata, unsigned int data_len, unsigned int *prec_len)
{
    int recv_num = 0;

    if (!prec_len)
    {
        printf("Parameter is null !\n");
        return CDT_MDW_NET_RC_PARAM_NULL;
    }

    if (!pdata || 0 == data_len)
    {
        //没有需要接收的数据
        return 0;
    }

    recv_num = recv(fd, pdata, data_len, MSG_PEEK);
    if(recv_num <  0)
    {
      switch(errno)
      {
        case EINTR:
        case EAGAIN:
                //try again
                break;
        case ETIMEDOUT:
                //keepallive error
                break;
        default: 
                printf("AF_INET server bad,ERRNO:%d,errno description:%s\n",errno,strerror(errno));
                return CDT_MDW_NET_RC_REC_PACKET;
      }

    }
    else
    if (0 == recv_num)
    {
        printf("remote disconnected !\n");  
        return CDT_MDW_NET_RC_REMOTE_DISCONNECT;
    }
    else
    {
      *prec_len = recv_num;
    }
    
    return 0;
}

unsigned int 
cdt_pon_net_rec_packet_fix_peek(int fd, void *pdata, unsigned int data_len)
{
    unsigned int read_len = 0;
    unsigned int rc;
    unsigned int len;
  
    do 
    {
        len = 0;
        rc = cdt_pon_net_rec_packet_peek(fd, ((char*)pdata + read_len), (data_len - read_len), &len);
        if(rc)  
        {          
            printf("read error !\n");
            return rc;  
        }
        
        read_len += len;
    }while(data_len > read_len);

    return 0;
}

/*
基于端口的发包函数
*/
static unsigned int 
cdt_pon_net_send_msg(CDT_NET_PARA* pnet_io, CDT_NET_MULTI_MSG *pmsg)
{
    int             send_size;
    struct msghdr   tmp_msg;

    if ((NULL == pnet_io) || (NULL == pmsg))
    {
        printf("Parameter is null !\n");
        return CDT_MDW_NET_RC_PARAM_NULL;
    }

    bzero((void*)&tmp_msg,sizeof(struct msghdr));
    tmp_msg.msg_name = &(pnet_io->remote_addr);
    tmp_msg.msg_namelen = sizeof(struct sockaddr_in);
    tmp_msg.msg_iov = pmsg->io;
    tmp_msg.msg_iovlen = pmsg->count;

again:
    send_size = sendmsg(pnet_io->fd, &tmp_msg, 0);
    if (send_size < 0)
    {
        if (errno == EINTR)
            goto again;
        else
        {
            printf("sendmsg err %d, %s\n", errno, strerror(errno));
            
            return errno;
        }
    }

    return 0;
}

//发送消息
unsigned int 
cdt_pon_net_send_packet(CDT_NET_PARA *pnet_io, CDT_NET_MSG *pnet_msg)
{
    int ret;
    CDT_NET_MULTI_MSG msg;

    if ((NULL == pnet_io) || (NULL == pnet_msg))
    {
        printf("Parameter is null !\n");
        return CDT_MDW_NET_RC_PARAM_NULL;
    }
    
    bzero((void*)&msg,sizeof(msg));

    if ((ret = cdt_pon_net_msg_push(&msg, pnet_msg, pnet_msg->len)))
    {
        printf("data_push error !\n");
        return ret;
    }

    ret = cdt_pon_net_send_msg(pnet_io, &msg);
    if(ret)
    {
        printf("AF_INET sendto error !\n");
        return ret;
    } 

    return 0;
}

unsigned int 
cdt_pon_net_send_alone_packet(int fd, CDT_NET_MSG *pnet_msg)
{
    int send_size;

    if (NULL == pnet_msg)
    {
        printf("Parameter is null !\n");
        return CDT_MDW_NET_RC_PARAM_NULL;
    }
    
again:
    send_size = send(fd, pnet_msg, pnet_msg->len, 0);
    if (send_size < 0)
    {
        if (errno == EINTR)
            goto again;
        else
        {
            printf("send err %d, %s\n", errno, strerror(errno));

            return errno;
        }
    }
    
    return 0;
}

static unsigned int 
cdt_pon_net_opt_init(int fd)
{
    int ret;
    int reuse = 1;
    struct linger so_linger;

    //设置地址可重用
    ret = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (void *)&reuse, sizeof(reuse));
    if (ret < 0) 
    {
        goto fail;
    }

    //设置板间数据连接延迟关闭
    so_linger.l_onoff = 1;
    so_linger.l_linger = 1;
    ret = setsockopt(fd, SOL_SOCKET, SO_LINGER, &so_linger, sizeof(so_linger));
    if (ret < 0) 
    {
        goto fail;
    }

    return 0;

fail:
    printf("%s %s %d error %d\n", __FILE__, __FUNCTION__, __LINE__, errno);
    return CDT_MDW_NET_RC_SETSOCKOPT;
}

unsigned int 
cdt_pon_net_trbuf_get(int fd, int *sdb_size, int *rdb_size)
{
    int ret;
    int result = 0;
    int result_len = sizeof(result);

    //获取发送和接收缓冲区大小
    ret = getsockopt(fd, SOL_SOCKET, SO_SNDBUF, (void*)&result, (socklen_t*)&result_len);
    if (ret < 0) 
    {
        goto fail;
    }

    *sdb_size = result;

    ret = getsockopt(fd, SOL_SOCKET, SO_RCVBUF, (void *)&result, (socklen_t*)&result_len);
    if (ret < 0) 
    {
        goto fail;
    }

    *rdb_size = result;

    return 0;

    fail:
    printf("%s %s %d errorno %d\n", __FILE__, __FUNCTION__, __LINE__, errno);
    return CDT_MDW_NET_RC_GETSOCKOPT;
}

unsigned int 
cdt_pon_net_trbuf_set(int fd, int sdb_size, int rdb_size)
{
    int ret;
    int result;

    //获取发送和接收缓冲区大小
    ret = setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (void *)&sdb_size, sizeof(sdb_size));
    if (ret < 0) 
    {
        goto fail;
    }

    ret = setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (void *)&rdb_size, sizeof(rdb_size));
    if (ret < 0) 
    {
        goto fail;
    }

    return 0;

fail:
    printf("%s %s %d errorno %d\n", __FILE__, __FUNCTION__, __LINE__, errno);
    return CDT_MDW_NET_RC_SETSOCKOPT;
}

// 开启keepalive属性
unsigned int 
cdt_pon_net_keepalive_set(int fd, int keepalive, int idle, int interval, int count)
{
    int ret;

    //开启与关闭keepalive
    ret = setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepalive, sizeof(keepalive));
    if (ret < 0) 
    {
        goto fail;
    }

    if (0 == keepalive)
    {
        return 0;
    }

    // 如该板间连接在idle秒内没有任何数据往来,则进行探测
    ret= setsockopt(fd, SOL_TCP, TCP_KEEPIDLE, (void*)&idle, sizeof(idle));
    if (ret < 0) 
    {
        goto fail;
    }
    // 探测时发包的时间间隔为interval秒
    ret = setsockopt(fd, SOL_TCP, TCP_KEEPINTVL, (void *)&interval, sizeof(interval));
    if (ret < 0) 
    {
        goto fail;
    }
    // 探测尝试的次数.如果第1次探测包就收到响应了,则后2次的不再发
    ret = setsockopt(fd, SOL_TCP, TCP_KEEPCNT, (void *)&count, sizeof(count));
    if (ret < 0) 
    {
        goto fail;
    }

    return 0;

fail:
    printf("%s %s %d errorno %d\n", __FILE__, __FUNCTION__, __LINE__, errno);
    return CDT_MDW_NET_RC_SETSOCKOPT;
}

unsigned int 
cdt_pon_net_setnonblock(int fd)
{
    long flags;

    flags = fcntl(fd, F_GETFL);
    if (flags < 0) 
    {
        goto fail;
    }

    flags |= O_NONBLOCK;
    if (fcntl(fd, F_SETFL, flags) < 0) 
    {
        goto fail;
    }

    return 0;
    
fail:
    printf("%s %s %d errorno %d\n", __FILE__, __FUNCTION__, __LINE__, errno);
    return CDT_MDW_NET_RC_FCNTL;
}

unsigned int 
cdt_pon_net_init(struct sockaddr_in *psin, int *p_fd)
{
    int fd;
    struct ifreq ifr_ip;
    char ipaddr[20];

    if ((NULL == psin) || (NULL == p_fd))
    {
        printf("Parameter is null !\n");
        return CDT_MDW_NET_RC_PARAM_NULL;
    }

    if( (fd = socket(AF_INET, SOCK_STREAM, 0)) < 0 )  
    {  
        printf("%s %s %d error %d\n", __FILE__, __FUNCTION__, __LINE__, errno);
        goto fail;
    }

    if (0 != cdt_pon_net_opt_init(fd))
    {
        close(fd);
        printf("%s %s %d error %d\n", __FILE__, __FUNCTION__, __LINE__, errno);
        goto fail;
    }

    //获取本板lo地址
    bzero(&ifr_ip, sizeof(struct ifreq));     
    strncpy(ifr_ip.ifr_name, "lo", sizeof(ifr_ip.ifr_newname) - 1);
    if(ioctl(fd, SIOCGIFADDR, &ifr_ip) < 0)     
    {     
        close(fd);
        printf("%s %s %d error %d\n", __FILE__, __FUNCTION__, __LINE__, errno);
        goto fail;
    }  
             
    memcpy(psin, &ifr_ip.ifr_ifru.ifru_addr, sizeof(struct sockaddr));
    
    strncpy(ipaddr, (char*)inet_ntoa(psin->sin_addr), sizeof(ipaddr));
    printf("local ip %s\n", ipaddr);

    *p_fd = fd;

    return 0;

fail:
    return CDT_MDW_NET_RC_INIT;
}

//校验消息
unsigned int 
cdt_pon_net_msg_verify(CDT_NET_MSG *pmsg)
{
    if (NULL == pmsg)
    {
        printf("Parameter is null !\n");
        return CDT_MDW_NET_RC_PARAM_NULL;
    }

    //msg base
    if ((pmsg->msg_type >= NET_MSG_TYPE_NUM_OF) || (pmsg->syn_flag >= NET_MSG_SYN_NUM_OF) || (pmsg->direction >= NET_MSG_DRICTION_NUM_OF))
    {
        printf("%s %s %d msg base type error\n", __FILE__, __FUNCTION__, __LINE__);
        return CDT_MDW_NET_RC_MSG_HEADER_VERIFY;
    }

    //msg len
    if (pmsg->len < NET_MSG_HEAD_LEN)
    {
        printf("%s %s %d msg len error\n", __FILE__, __FUNCTION__, __LINE__);
        return CDT_MDW_NET_RC_MSG_HEADER_VERIFY;
    }
   
    return 0;
}

/*
    封装消息,申请内存由外部释放，
    pvar_len 用于发送消息的内存，
    param_len用于构造回传消息的内存，
    对于ctrl需要满足param_len >= pvar_len
    对于drv需要满足param_len == pvar_len
*/
//申请内存使用系统接口
CDT_NET_MSG *
cdt_pon_net_msg_pack(   CDT_NET_BLK_POOL           *p_mem_pool,
                        const unsigned long long    vif_info,
                        const CDT_NET_MSG_CMD       cmd_info, 
                        const void                  *pvar, 
                        const unsigned int          pvar_len,
                        const unsigned int          param_len)
{
    unsigned int out_len = pvar_len;
    
    //para check pvar 与pvar_len 对应
    if ((NULL == pvar && pvar_len) || (pvar && 0 == pvar_len))
    {
        return NULL;  
    }

    if (param_len < pvar_len)
    {
        return NULL;
    }

    //对于同步消息结构其总内存: header + body预留空间
    int         msg_size = NET_MSG_HEAD_LEN + param_len;
    //tx消息有效内存, 需要发送消息的真实长度
    int         msg_len  = NET_MSG_HEAD_LEN + pvar_len;

    CDT_NET_MSG *msg = cdt_pon_net_malloc(p_mem_pool, msg_size);

    if (!msg)
    {
        return NULL;  
    }

    msg->len       = msg_len;
    msg->vif_info  = vif_info;
    msg->corr_tag  = 0;         //魔术字默认0
    msg->state     = 0;         //sdk默认返回0
    msg->body_size = param_len;
    msg->compress  = 0;         //默认不压缩

    memcpy(&msg->cmd_info, &cmd_info, sizeof(CDT_NET_MSG_CMD));
    
    if (pvar && pvar_len)
    {
        //尝试压缩数据
        if (!cdt_pon_net_msg_compress_process(msg, pvar, pvar_len, &out_len))
        {
            //corrected parameter
            msg->uncmp_len  = pvar_len;
            msg->len        = out_len + NET_MSG_HEAD_LEN;
            msg->compress   = 1;
        }
        else
        {
            //使用非压缩消息
            memcpy(msg->body, pvar, pvar_len);
        }
    }

    return msg;
}

unsigned long long 
cdt_pon_net_htonll(unsigned long long x)
{
    unsigned int tmp1, tmp2;

    tmp1 = htonl(x&0xFFFFFFFF);
    tmp2 = htonl((x>>32) & 0xFFFFFFFF);
    x = (unsigned long long)tmp1 << 32;
    x |= tmp2;
    
    return (unsigned long long)tmp1 << 32 | tmp2;;
}

unsigned long long 
cdt_pon_net_ntohll(unsigned long long x)
{
    unsigned int tmp1, tmp2;

    tmp1 = ntohl(x&0xFFFFFFFF);
    tmp2 = ntohl((x>>32) & 0xFFFFFFFF);

    return (unsigned long long)tmp1 << 32 | tmp2;
}

//消息发送前消息头部处理
unsigned int
cdt_pon_net_msg_header_tx_process(int *p_conn_id, CDT_NET_MSG *p_msg)
{
    if (!p_conn_id || !p_msg)
    {
        printf("Parameter is null !\n");
        return CDT_MDW_NET_RC_PARAM_NULL;
    }

    //获取发送对端
    *p_conn_id = MSG2SLOT(p_msg);
    
    //msg 头部字节序适配网络序
    p_msg->msg_type           = htonl(p_msg->msg_type);
    p_msg->cmd_info.cmd_id    = htonl(p_msg->cmd_info.cmd_id);
    p_msg->cmd_info.timeout   = htonl(p_msg->cmd_info.timeout);
    p_msg->cmd_info.order     = htonl(p_msg->cmd_info.order);
    p_msg->vif_info           = cdt_pon_net_htonll(p_msg->vif_info);
    p_msg->syn_flag           = htonl(p_msg->syn_flag);
    p_msg->corr_tag           = htonl(p_msg->corr_tag);
    p_msg->direction          = htonl(p_msg->direction);
    p_msg->state              = htonl(p_msg->state);
    p_msg->len                = htonl(p_msg->len);
    p_msg->body_size          = htonl(p_msg->body_size);
    
    return 0;
}

#endif

#if DEFUNC("同步连接")

static CDT_NET_CONN_MSG *
cdt_pon_net_conn_msg_syn_varify(const CDT_NET_CONN *pconn, const CDT_NET_MSG *pmsg)
{
    CDT_NET_CONN_MSG *pcmsg;
    CDT_NET_MSG      *ptmsg;

    TAILQ_FOREACH(pcmsg, &pconn->msg_list, l)
    {
        ptmsg = (CDT_NET_MSG*)(pcmsg->data);
        if (ptmsg->corr_tag == pmsg->corr_tag)
            break;
    }
    
    return pcmsg;
}

static void 
cdt_pon_net_conn_msg_syn_ready(CDT_NET_CONN_MSG *pcmsg)
{
    cdt_pon_net_sem_post(&pcmsg->sem);
}

static inline void 
cdt_pon_net_conn_msg_syn_response(CDT_NET_CONN *pconn, CDT_NET_CONN_MSG *pcmsg)
{
    cdt_pon_net_mutex_unlock(&pconn->mutex);

    cdt_pon_net_conn_msg_syn_ready(pcmsg);
}

unsigned int 
cdt_pon_net_conn_msg_syn_ack_process(CDT_NET_CONN *pconn, CDT_NET_MSG *pmsg)
{  
    CDT_NET_CONN_MSG    *pcmsg;
    int                 max_len;

    cdt_pon_net_mutex_lock(&pconn->mutex);
    
    pcmsg = cdt_pon_net_conn_msg_syn_varify(pconn, pmsg);
    if (!pcmsg)
    {
        DRV_ERROR("cdt_pon_net_conn_msg_syn_varify error!!!\n");
        cdt_pon_net_mutex_unlock(&pconn->mutex);
        return CDT_APP_PONOLT_RC_MSG_SYN_VARIFY;
    }

    //AR msg copy 到syn send 接口申请的内存中
    //正常的消息请求与响应需满足msg body size >= len - header_len
    max_len = ((CDT_NET_MSG*)(pcmsg->data))->body_size + NET_MSG_HEAD_LEN;
    memcpy(pcmsg->data, pmsg, (pmsg->len > max_len ? max_len : pmsg->len));

    TAILQ_REMOVE(&pconn->msg_list, pcmsg, l);

    cdt_pon_net_conn_msg_syn_response(pconn, pcmsg);

    return 0;
}

//获取fifo中的conn msg ，锁在外部加上
CDT_NET_CONN_MSG* 
cdt_pon_net_conn_msg_get_free(pon_net_conn_list *plist)
{
    CDT_NET_CONN_MSG *pcmsg = TAILQ_FIRST(plist);
    
    if (pcmsg)
        TAILQ_REMOVE(plist, pcmsg, l);
        
    return pcmsg;
}

//conn msg入空闲队列,锁在外部加上
void 
cdt_pon_net_conn_msg_free(  CDT_NET_CONN_MSG  *pcmsg, 
                            pon_net_conn_list *plist)
{
    pcmsg->data = NULL;

    TAILQ_INSERT_TAIL(plist, pcmsg, l);
}

#endif

#if DEFUNC("数据解压缩")

static unsigned int
cdt_pon_net_msg_need_compress(CDT_NET_MSG* p_msg)
{
    //不使用压缩，直接返回失败，使用非压缩数据
#ifndef CDT_NET_USE_COMPRESS
        return CDT_MDW_NET_RC_COMPRESS_NO_NEED;
#endif
    
    //不使用压缩，直接返回失败，使用非压缩数据
    if (p_msg->len < NET_MSG_HEAD_LEN + CDT_NET_COMPRESS_MIN_LEN)
        return CDT_MDW_NET_RC_COMPRESS_NO_NEED;

    return 0;
}

//消息压缩
unsigned int
cdt_pon_net_msg_compress_process(
                                CDT_NET_MSG*            p_msg,
                                const void*             p_var, 
                                const unsigned int      var_len,
                                unsigned int*           p_out_len)
{
    unsigned rc;
    unsigned level = 1;

    if (!p_msg || !p_var || !p_out_len)
    {
        return CDT_MDW_NET_RC_PARAM_NULL;
    }

    if (!var_len)
    {
        return CDT_MDW_NET_RC_PARAM_OUT_OF_RANGE;
    }

    if ((rc = cdt_pon_net_msg_need_compress(p_msg)))
    {
        return rc;
    }
    //压缩级别1-9， 越高越耗时
    rc = cdt_pon_net_data_compress(p_var, (const unsigned long)var_len, p_msg->body, (unsigned long*)p_out_len, level);
    
    return rc; 
}

unsigned int
cdt_pon_net_msg_need_uncompress(CDT_NET_MSG* p_msg)
{
    //不使用压缩，直接返回处理未压缩消息
#ifndef CDT_NET_USE_COMPRESS
        return CDT_MDW_NET_RC_UNCOMPRESS_NO_NEED;
#endif
    
    //不使用压缩，直接返回处理未压缩消息
    if (!p_msg->compress)
        return CDT_MDW_NET_RC_UNCOMPRESS_NO_NEED;

    return 0;
}

//消息解压缩处理
unsigned int
cdt_pon_net_msg_uncompress_process(CDT_NET_BLK_POOL* p_mem_pool, CDT_NET_MSG** p_msg)
{
    unsigned int    rc;
    CDT_NET_MSG     *p_in = *p_msg;
    CDT_NET_MSG     *p_out;
    unsigned int    out_len = p_in->uncmp_len;
    
    if (!p_mem_pool || !p_msg || !p_in)
        return CDT_MDW_NET_RC_PARAM_NULL;

    if ((rc = cdt_pon_net_msg_need_uncompress(p_in)))
    {
        return 0;   
    }

    //最大消息结构作为body的缓存
    p_out = cdt_pon_net_malloc(p_mem_pool, NET_BUFFER_LEN);
    if (!p_out)
        return CDT_MDW_NET_RC_MEM_ALLOCATE;
        
    memcpy(p_out, p_in, NET_MSG_HEAD_LEN);
    
    rc = cdt_pon_net_data_uncompress((void*)p_in->body, p_in->len-NET_MSG_HEAD_LEN, (void*)p_out->body, (unsigned long*)&out_len);
    if (rc)
    {
        cdt_pon_net_safe_free(p_out);
        return rc;
    }
    
    p_out->len = out_len + NET_MSG_HEAD_LEN;
    *p_msg = p_out;
    cdt_pon_net_safe_free(p_in);

    return 0;
}

#endif

#if DEFUNC("接口参数转换")

//所有id 以0为其实索引，比如gpon_ni 0~31 0x00~0x1f
unsigned int 
cdt_pon_net_vif2phy(unsigned long long ulong, CDT_NET_PHY_INFO *p_phy)
{
    cdt_rc_info     rc_info = {.obj_id = CDT_OBJ_MDW_NET_RC_OFFSET};
    unsigned    int rc;

    if (!p_phy)
    {
        CDT_PON_ERRNO_INFO2RC(rc_info, rc, CDT_MDW_NET_RC_PARAM_NULL);
        return rc; 
    }
    
    p_phy->key_type = ulong >> KEY_TYPE_MOVE_BITS;
    
    switch (p_phy->key_type) 
    {
        case PON_NI:
            p_phy->phy_oscs.slot_id = (ulong & 0xff);
            p_phy->phy_oscs.pon_ni  = ((ulong >> SLOT_ID_HOLD_BITS) & 0xff);
        break;

        case PON_ONU:
            p_phy->phy_oscs.slot_id = (ulong & 0xff);
            p_phy->phy_oscs.pon_ni  = ((ulong >> SLOT_ID_HOLD_BITS) & 0xff);
            p_phy->phy_oscs.onu_id  = ((ulong >> (SLOT_ID_HOLD_BITS + PON_NI_HOLD_BITS)) & 0xff);
        break;

        case PON_ALLOC:
            p_phy->phy_oscs.slot_id  = (ulong & 0xff);
            p_phy->phy_oscs.pon_ni   = ((ulong >> SLOT_ID_HOLD_BITS) & 0xff);
            p_phy->phy_oscs.alloc_id = ((ulong >> (SLOT_ID_HOLD_BITS + PON_NI_HOLD_BITS)) & 0xffff);
        break;

        case PON_GEMPORT:
            p_phy->phy_oscs.slot_id    = (ulong & 0xff);
            p_phy->phy_oscs.pon_ni     = ((ulong >> SLOT_ID_HOLD_BITS) & 0xff);
            p_phy->phy_oscs.gemport_id = ((ulong >> (SLOT_ID_HOLD_BITS + PON_NI_HOLD_BITS)) & 0xffff);
        break;

        case PON_VLAN:
            p_phy->phy_oscs.slot_id = (ulong & 0xff);
            p_phy->phy_oscs.pon_ni  = ((ulong >> SLOT_ID_HOLD_BITS) & 0xff);
            p_phy->phy_oscs.vlan_id = ((ulong >> (SLOT_ID_HOLD_BITS + PON_NI_HOLD_BITS)) & 0xffff);
        break;

        case PON_FLOW:
            p_phy->phy_oscs.slot_id = (ulong & 0xff);
            p_phy->phy_oscs.pon_ni  = ((ulong >> SLOT_ID_HOLD_BITS) & 0xff);
            p_phy->phy_oscs.flow_id = ((ulong >> (SLOT_ID_HOLD_BITS + PON_NI_HOLD_BITS)) & 0xffff);
        break;

        case PON_GPIO:
            p_phy->phy_oscs.slot_id = (ulong & 0xff);
            p_phy->phy_oscs.pon_ni  =  ((ulong >> SLOT_ID_HOLD_BITS) & 0xff);
            p_phy->phy_oscs.gpio_id = ((ulong >> (SLOT_ID_HOLD_BITS + PON_NI_HOLD_BITS)) & 0xffff);
        break;

        case PON_BOARD:
             p_phy->phy_oscs.slot_id = (ulong & 0xff);
        break;
        
        default:
            CDT_PON_ERRNO_INFO2RC(rc_info, rc, CDT_MDW_NET_RC_PARAM_OUT_OF_RANGE);
            return rc;
    }

    return 0;    
}

unsigned int 
cdt_pon_net_phy2vif(CDT_NET_PHY_INFO *p_phy, unsigned long long *p_ulong)
{
    cdt_rc_info     rc_info = {.obj_id = CDT_OBJ_MDW_NET_RC_OFFSET};
    unsigned    int rc;
    
    if (!p_phy || !p_ulong)
    {
        CDT_PON_ERRNO_INFO2RC(rc_info, rc, CDT_MDW_NET_RC_PARAM_NULL);
        return rc;
    }
    
    #if 0
    printf("inner phy info:\n"
           "key_type: %d\n"
           "slot_id: %d\n"
           "pon_ni: %d\n"
           "onu_id: %d\n"
           "alloc_id: %d\n"
           "gemport_id: %d\n"
           "vlan_id: %d\n"
           "flow_id: %d\n"
           "gpio_id: %d\n",
           p_phy->key_type, p_phy->phy_oscs.slot_id, p_phy->phy_oscs.pon_ni, 
           p_phy->phy_oscs.onu_id, p_phy->phy_oscs.alloc_id, 
           p_phy->phy_oscs.gemport_id, p_phy->phy_oscs.vlan_id, 
           p_phy->phy_oscs.flow_id, p_phy->phy_oscs.gpio_id);
    #endif
    
    *p_ulong = (unsigned long long)p_phy->key_type << KEY_TYPE_MOVE_BITS;

    switch (p_phy->key_type) {
        case PON_NI:
            *p_ulong |= p_phy->phy_oscs.slot_id; 
            *p_ulong |= ((unsigned long long)p_phy->phy_oscs.pon_ni << SLOT_ID_HOLD_BITS);
        break;

        case PON_ONU:
            *p_ulong |= p_phy->phy_oscs.slot_id; 
            *p_ulong |= ((unsigned long long)p_phy->phy_oscs.pon_ni << SLOT_ID_HOLD_BITS);
            *p_ulong |= ((unsigned long long)p_phy->phy_oscs.onu_id << (SLOT_ID_HOLD_BITS + PON_NI_HOLD_BITS));
        break;

        case PON_ALLOC:
            *p_ulong |= p_phy->phy_oscs.slot_id; 
            *p_ulong |= ((unsigned long long)p_phy->phy_oscs.pon_ni << SLOT_ID_HOLD_BITS);
            *p_ulong |= ((unsigned long long)p_phy->phy_oscs.alloc_id << (SLOT_ID_HOLD_BITS + PON_NI_HOLD_BITS));
        break;

        case PON_GEMPORT:
            *p_ulong |= p_phy->phy_oscs.slot_id; 
            *p_ulong |= ((unsigned long long)p_phy->phy_oscs.pon_ni << SLOT_ID_HOLD_BITS);
            *p_ulong |= ((unsigned long long)p_phy->phy_oscs.gemport_id << (SLOT_ID_HOLD_BITS + PON_NI_HOLD_BITS));
        break;

        case PON_VLAN:
            *p_ulong |= p_phy->phy_oscs.slot_id; 
            *p_ulong |= ((unsigned long long)p_phy->phy_oscs.pon_ni << SLOT_ID_HOLD_BITS);
            *p_ulong |= ((unsigned long long)p_phy->phy_oscs.vlan_id << (SLOT_ID_HOLD_BITS + PON_NI_HOLD_BITS));
        break;

        case PON_FLOW:
            *p_ulong |= p_phy->phy_oscs.slot_id; 
            *p_ulong |= ((unsigned long long)p_phy->phy_oscs.pon_ni << SLOT_ID_HOLD_BITS);
            *p_ulong |= ((unsigned long long)p_phy->phy_oscs.flow_id << (SLOT_ID_HOLD_BITS + PON_NI_HOLD_BITS));
        break;

        case PON_GPIO:
            *p_ulong |= p_phy->phy_oscs.slot_id; 
            *p_ulong |= ((unsigned long long)p_phy->phy_oscs.pon_ni << SLOT_ID_HOLD_BITS);
            *p_ulong |= ((unsigned long long)p_phy->phy_oscs.gpio_id << (SLOT_ID_HOLD_BITS + PON_NI_HOLD_BITS));
        break;

        case PON_BOARD:
            *p_ulong |= p_phy->phy_oscs.slot_id; 
        break;

        default:
            CDT_PON_ERRNO_INFO2RC(rc_info, rc, CDT_MDW_NET_RC_PARAM_OUT_OF_RANGE);
            return rc;
    }

    return 0;
}

//PON_NI if param convert
unsigned int 
cdt_pon_net_pon_ni2vif_ullong(  
                                unsigned int        slot_id, 
                                unsigned int        pon_ni,
                                unsigned long long  *p_ullong)
{
    CDT_NET_PHY_INFO olt_phy_info;

    olt_phy_info.key_type         = PON_NI;
    olt_phy_info.phy_oscs.slot_id = slot_id;
    olt_phy_info.phy_oscs.pon_ni  = pon_ni;

    return cdt_pon_net_phy2vif(&olt_phy_info, p_ullong);
}

//PON_ONU if param convert
unsigned int 
cdt_pon_net_pon_onu2vif_ullong( 
                                unsigned int       slot_id, 
                                unsigned int       pon_ni ,
                                unsigned int       onu_id,
                                unsigned long long *p_ullong)
{
    CDT_NET_PHY_INFO olt_phy_info = {0};

    olt_phy_info.key_type         = PON_ONU;
    olt_phy_info.phy_oscs.slot_id = slot_id;
    olt_phy_info.phy_oscs.pon_ni  = pon_ni;
    olt_phy_info.phy_oscs.onu_id  = onu_id;

    return cdt_pon_net_phy2vif(&olt_phy_info, p_ullong);
}

//PON_ALLOC if param convert
unsigned int 
cdt_pon_net_pon_alloc2vif_ullong( 
                                unsigned int       slot_id, 
                                unsigned int       pon_ni ,
                                unsigned int       alloc_id,
                                unsigned long long *p_ullong)
{
    CDT_NET_PHY_INFO olt_phy_info = {0};

    olt_phy_info.key_type           = PON_ALLOC;
    olt_phy_info.phy_oscs.slot_id   = slot_id;
    olt_phy_info.phy_oscs.pon_ni    = pon_ni;
    olt_phy_info.phy_oscs.alloc_id  = alloc_id;

    return cdt_pon_net_phy2vif(&olt_phy_info, p_ullong);
}

//PON_GEMPORT if param convert
unsigned int 
cdt_pon_net_pon_gemport2vif_ullong( 
                                    unsigned int       slot_id, 
                                    unsigned int       pon_ni ,
                                    unsigned int       gemport_id,
                                    unsigned long long *p_ullong)
{
    CDT_NET_PHY_INFO olt_phy_info = {0};

    olt_phy_info.key_type             = PON_GEMPORT;
    olt_phy_info.phy_oscs.slot_id     = slot_id;
    olt_phy_info.phy_oscs.pon_ni      = pon_ni;
    olt_phy_info.phy_oscs.gemport_id  = gemport_id;

    return cdt_pon_net_phy2vif(&olt_phy_info, p_ullong);	
}

//PON_GEMPORT if param convert
unsigned int 
cdt_pon_net_pon_vlan2vif_ullong( 
                                unsigned int       slot_id, 
                                unsigned int       pon_ni ,
                                unsigned int       vlan_id,
                                unsigned long long *p_ullong)
{
    CDT_NET_PHY_INFO olt_phy_info = {0};

    olt_phy_info.key_type             = PON_VLAN;
    olt_phy_info.phy_oscs.slot_id     = slot_id;
    olt_phy_info.phy_oscs.pon_ni      = pon_ni;
    olt_phy_info.phy_oscs.vlan_id     = vlan_id;

    return cdt_pon_net_phy2vif(&olt_phy_info, p_ullong);
}

//PON_FLOW if param convert
unsigned int 
cdt_pon_net_pon_flow2vif_ullong( 
                                unsigned int       slot_id, 
                                unsigned int       pon_ni ,
                                unsigned int       flow_id,
                                unsigned long long *p_ullong)
{
    CDT_NET_PHY_INFO olt_phy_info = {0};

    olt_phy_info.key_type             = PON_FLOW;
    olt_phy_info.phy_oscs.slot_id     = slot_id;
    olt_phy_info.phy_oscs.pon_ni      = pon_ni;
    olt_phy_info.phy_oscs.flow_id     = flow_id;

    return cdt_pon_net_phy2vif(&olt_phy_info, p_ullong);
}

//PON_GPIO if param convert
unsigned int 
cdt_pon_net_pon_gpio2vif_ullong( 
                                unsigned int       slot_id, 
                                unsigned int       pon_ni ,
                                unsigned int       gpio_id,
                                unsigned long long *p_ullong)
{
    CDT_NET_PHY_INFO olt_phy_info = {0};

    olt_phy_info.key_type             = PON_FLOW;
    olt_phy_info.phy_oscs.slot_id     = slot_id;
    olt_phy_info.phy_oscs.pon_ni      = pon_ni;
    olt_phy_info.phy_oscs.gpio_id     = gpio_id;

    return cdt_pon_net_phy2vif(&olt_phy_info, p_ullong);
}

//PON_BOARD if param convert
unsigned int 
cdt_pon_net_pon_board2vif_ullong( 
                                unsigned int       slot_id, 
                                unsigned long long *p_ullong)
{
    CDT_NET_PHY_INFO olt_phy_info = {0};

    olt_phy_info.key_type             = PON_BOARD;
    olt_phy_info.phy_oscs.slot_id     = slot_id;

    return cdt_pon_net_phy2vif(&olt_phy_info, p_ullong);
}

#endif

