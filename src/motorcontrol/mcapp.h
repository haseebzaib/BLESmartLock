#ifndef _MCAPP_H_
#define _MCAPP_H_

#include "app_defines.h"


enum mcapp_dir{
 mcapp_forward = 0,
 mcapp_backward = 1
};


extern void mcapp_init();
extern void mcapp_speedDirection(enum mcapp_dir direction,uint16_t speed);



#endif

