/*
 * Copyright (c) 2021, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS CT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES BE LIABLE FOR ANY DIRE(INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
PB4 PB1 R
PB6 PB7 L
mpu6050 scl PA12 sda PA13
oled scl PB2 sda PB31
uart TX PA28 RX PA31
encoder R PA24 PA17
encoder L PA8 PA26

*/



#include "ti_msp_dl_config.h"
#include "Delay.h"
#include "stdio.h"
#include "MPU6050.h"
#include "Motor.h"
#include "Encoder.h"
#include "string.h"
#include "UI.h"
#include "switch.h"
#include "ssd1306.h"
#include "IR.h"
#include "counter.h"
#include "beep.h"
#include "PID.h"
volatile unsigned int delay_times = 0;
uint32_t time_system; 
IR_t Front_IR;
IR_t Back_IR;
float front_ir_pos;
PID_Base turn_pid;
PID_Base left_pid;
PID_Base right_pid;
Motor motor_L;
Motor motor_R;
float base_setpoint = 30;
float left_setpoint = 0;
float right_setpoint = 0;
void Delay_Systick_ms(unsigned int ms) 
{
    delay_times = ms;
    while( delay_times != 0 );
}      
 
//�δ���ʱ���ж�
void SysTick_Handler(void)
{
    if( delay_times != 0 )
    {
        delay_times--;
    }
}


uint8_t str[50],str2[50];
int speed_L=0,speed_R=0,speed=1000,angel_error=0;
uint32_t uptime = 0;

int main(void)
{
    SYSCFG_DL_init();
    turn_pid = PID_Base_Init(-10, 0, -1, 800, -800, 1, 0, 0, 0);
    left_pid = PID_Base_Init(15, 0.25, 1, 900, -900, 1, 1, 0.33f, 0);
    right_pid = PID_Base_Init(15, 0.25, 1, 900, -900, 1, 1, 0.33f, 0);
	ssd1306_Init();
    UI_init();
    MPU6050_Init();
	NVIC_ClearPendingIRQ(UART_0_INST_INT_IRQN);
	NVIC_EnableIRQ(UART_0_INST_INT_IRQN);
    NVIC_EnableIRQ(GPIO_MULTIPLE_GPIOA_INT_IRQN);
	DL_Timer_startCounter(TIMER_Encoder_Read_INST);
    NVIC_EnableIRQ(TIMER_Encoder_Read_INST_INT_IRQN);
    motor_L = motor_init(PWM_Motor_R_INST, DL_TIMER_CC_0_INDEX, DL_TIMER_CC_1_INDEX);//B4 B1
    motor_R = motor_init(PWM_Motor_L_INST, DL_TIMER_CC_0_INDEX, DL_TIMER_CC_1_INDEX);//B6 B7

    IR_Init(&Front_IR,IRfront_SF1_PORT,IRfront_SF1_PIN,IRfront_SF2_PORT,IRfront_SF2_PIN,IRfront_SF3_PORT,IRfront_SF3_PIN,IRfront_SF4_PORT,IRfront_SF4_PIN);
    IR_Init(&Back_IR,IRback_SB1_PORT,IRback_SB1_PIN,IRback_SB2_PORT,IRback_SB2_PIN,IRback_SB3_PORT,IRback_SB3_PIN,IRback_SB4_PORT,IRback_SB4_PIN);


    beep_ms(20);
    counter.led_ms = 10000;

    while (1) 
	{
        UI_show();
        UI_key_process();
        MPU6050_Read_All(&mpu6050);
//        delay_ms(20);


    }
}

void counter_process(){
    if(counter.beep_ms>0){
        counter.beep_ms-=5;
        beep_on();
    }else{
        beep_off();
    }

    if(counter.led_ms % 501 > 250){
        DL_GPIO_setPins(LED_PORT, LED_led1_PIN);
        DL_GPIO_clearPins(LED_PORT, LED_led2_PIN);
    }else{
        DL_GPIO_clearPins(LED_PORT, LED_led1_PIN);
        DL_GPIO_setPins(LED_PORT, LED_led2_PIN);
    }
    if(counter.led_ms > 0){
        counter.led_ms-=5;
    }
}

void TIMER_Encoder_Read_INST_IRQHandler(void){
    uptime += 5;
    speed_L=left_count;
    speed_R=right_count;
    left_count_sum+=left_count;
    right_count_sum+=right_count;
    left_count=0;
    right_count=0;
    IR_Read(&Front_IR);
    IR_Read(&Back_IR);
    front_ir_pos = IR_get_pos(&Front_IR);
    float turn_out = PID_Base_Calc(&turn_pid, front_ir_pos, 0);
    if(turn_out > 0){
        left_setpoint = base_setpoint;
        right_setpoint = base_setpoint - turn_out;
    } else {
        right_setpoint = base_setpoint;
        left_setpoint = base_setpoint + turn_out;
    }
    motor_set_speed(&motor_L, (int) PID_Base_Calc(&left_pid, speed_L, left_setpoint));
    motor_set_speed(&motor_R, (int) PID_Base_Calc(&right_pid, speed_R, right_setpoint));
    counter_process();
}
