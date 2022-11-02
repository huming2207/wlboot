#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void NMI_Handler(void);
void HardFault_Handler(void);
void MemManage_Handler(void);
void BusFault_Handler(void);
void UsageFault_Handler(void);
void SVC_Handler(void);
void DebugMon_Handler(void);
void PendSV_Handler(void);
void SysTick_Handler(void);
void I2C2_EV_IRQHandler(void);
void I2C2_ER_IRQHandler(void);
void USART1_IRQHandler(void);
void LPUART1_IRQHandler(void);
void SUBGHZ_Radio_IRQHandler(void);

#ifdef __cplusplus
}
#endif
