/**************************************************************
* 文件名称: 
* 文件描述:
* 版           本: 
* 修改历史:
*     <修改者>     <时间>      <版本 >     <描述>

**************************************************************/

#include "pon_adaptor_errno.h"

#if DEFUNC("全局错误码转换接口")

unsigned int 
cdt_pon_rc2info_convert(unsigned int rc_val, cdt_rc_info *p_rc_info)
{
    if (!p_rc_info)
    {
        return CDT_MDW_NET_RC_PARAM_NULL;
    }
    
    if (rc_val>=CDT_RETURNCODE_MAX_VALUE)
    {
        return CDT_MDW_NET_RC_PARAM_OUT_OF_RANGE;
    }
    
    p_rc_info->obj_id = (rc_val/CDT_RETURNCODE_BASE)/CDT_RETURNCODE_BASE;

    switch(p_rc_info->obj_id)
    {
        case OBJ_DRV_PON_SDK:case OBJ_DRV_PON_NW:case OBJ_MDW_LIBEV:case OBJ_MDW_SQL:
        case OBJ_MDW_NET:
        case OBJ_APP_PON_STAT:case OBJ_APP_PON_ALARM:
            {
                //暂未使用 
            }
            break;

        case OBJ_DRV_PON_OLT:case OBJ_APP_PON_PONOLT:
            {
                p_rc_info->sub_obj_id.olt_id = (rc_val/CDT_RETURNCODE_BASE)%CDT_RETURNCODE_BASE;
            }
            break;

        case OBJ_DRV_PON_NI:case OBJ_APP_PON_PONCFG:
            {
                p_rc_info->sub_obj_id.pon_id = (rc_val/CDT_RETURNCODE_BASE)%CDT_RETURNCODE_BASE;
            }
            break;

        case OBJ_DRV_PON_ONU:case OBJ_APP_PON_ONUCFG:case OBJ_APP_PON_ONURA:
            {
                p_rc_info->sub_obj_id.onu_id = (rc_val/CDT_RETURNCODE_BASE)%CDT_RETURNCODE_BASE;
            }
            break;

        case OBJ_APP_PON_PROFILE:
            {
                p_rc_info->sub_obj_id.profile_id = (rc_val/CDT_RETURNCODE_BASE)%CDT_RETURNCODE_BASE;
            }
            break;

        default:
            printf("obj id: %d not in range!!!\n", p_rc_info->obj_id);
            return CDT_MDW_NET_RC_PARAM_OUT_OF_RANGE;
    }

    p_rc_info->err_id = rc_val%CDT_RETURNCODE_BASE;
  
    return 0;
}

unsigned int 
cdt_pon_info2rc_convert(unsigned int *p_rc_val, cdt_rc_info *p_rc_info)
{
    if (!p_rc_val || !p_rc_info)
    {
        return CDT_MDW_NET_RC_PARAM_NULL;
    }

    if (p_rc_info->obj_id >= CDT_RETURNCODE_BASE || p_rc_info->err_id >= CDT_RETURNCODE_BASE || 
        *(unsigned int*)&p_rc_info->sub_obj_id >= CDT_RETURNCODE_BASE)
    {
        return CDT_MDW_NET_RC_PARAM_OUT_OF_RANGE;
    }

    *p_rc_val = p_rc_info->err_id;
    *p_rc_val += p_rc_info->obj_id*CDT_RETURNCODE_BASE*CDT_RETURNCODE_BASE;
    
    switch(p_rc_info->obj_id)
    {
        case OBJ_DRV_PON_SDK:case OBJ_DRV_PON_NW:case OBJ_MDW_LIBEV:case OBJ_MDW_SQL:
        case OBJ_MDW_NET:
        case OBJ_APP_PON_STAT:case OBJ_APP_PON_ALARM:
            {
                //暂未使用 
            }
            break;
        
        case OBJ_DRV_PON_OLT:case OBJ_APP_PON_PONOLT:
           {
               *p_rc_val += p_rc_info->sub_obj_id.olt_id * CDT_RETURNCODE_BASE;
           }
           break;
        
        case OBJ_DRV_PON_NI:case OBJ_APP_PON_PONCFG:
           {
               *p_rc_val += p_rc_info->sub_obj_id.pon_id * CDT_RETURNCODE_BASE;
           }
           break;
        
        case OBJ_DRV_PON_ONU:case OBJ_APP_PON_ONUCFG:case OBJ_APP_PON_ONURA:
           {
               *p_rc_val += p_rc_info->sub_obj_id.onu_id * CDT_RETURNCODE_BASE;
           }
           break;
        
        case OBJ_APP_PON_PROFILE:
           {
               *p_rc_val += p_rc_info->sub_obj_id.profile_id * CDT_RETURNCODE_BASE;
           }
           break;
        
           default:
               printf("obj id: %d not in range!!!\n", p_rc_info->obj_id);
               return CDT_MDW_NET_RC_PARAM_OUT_OF_RANGE;
    }

    return 0;
}

#endif

