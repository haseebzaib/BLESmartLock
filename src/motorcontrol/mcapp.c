#include "mcapp.h"
#include <zephyr/drivers/pwm.h>

LOG_MODULE_REGISTER(mcapp, LOG_LEVEL_DBG);

#define motor_lhs  DT_NODELABEL(motor_lhs)
#define motor_rhs  DT_NODELABEL(motor_rhs)

static const struct pwm_dt_spec pwm_motor_lhs = PWM_DT_SPEC_GET(motor_lhs);
static const struct pwm_dt_spec pwm_motor_rhs = PWM_DT_SPEC_GET(motor_rhs);

GLOBAL void mcapp_init()
{

    if (!pwm_is_ready_dt(&pwm_motor_lhs)) {
        LOG_ERR("Error: PWM device %s is not ready", pwm_motor_lhs.dev->name);
        return;
	}

	    if (!pwm_is_ready_dt(&pwm_motor_rhs)) {
        LOG_ERR("Error: PWM device %s is not ready", pwm_motor_rhs.dev->name);
        return;
	}

    pwm_set_dt(&pwm_motor_lhs, PWM_USEC(1000),PWM_USEC(0) );
	pwm_set_dt(&pwm_motor_rhs, PWM_USEC(1000),PWM_USEC(0) );


    LOG_INF("Initialized");

}



GLOBAL void mcapp_speedDirection(enum mcapp_dir direction,uint16_t speed)
{

    switch(direction)
    {
        case mcapp_forward:
        {
	        pwm_set_dt(&pwm_motor_lhs, PWM_USEC(1000),PWM_USEC(0) );
	        pwm_set_dt(&pwm_motor_rhs, PWM_USEC(1000),PWM_USEC(speed) );
            break;
        }

        case mcapp_backward:
        {

	        pwm_set_dt(&pwm_motor_lhs, PWM_USEC(1000),PWM_USEC(speed) );
	        pwm_set_dt(&pwm_motor_rhs, PWM_USEC(1000),PWM_USEC(0)  );
            break;
        }

    }

}