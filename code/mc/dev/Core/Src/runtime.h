#include "stm32h725xx.h"

#define ARM_CM_DWT_CYCCNT (*(volatile uint32_t *)0xE0001004)

__STATIC_INLINE void enable_cycle_counter(void) {
  SET_BIT(DWT->CTRL, DWT_CTRL_CYCCNTENA_Msk);
}

__STATIC_INLINE uint32_t get_cycle_count(void) { return ARM_CM_DWT_CYCCNT; }

__STATIC_INLINE uint32_t MAX(uint32_t a, uint32_t b) {
  return ((a) > (b) ? a : b);
}
__STATIC_INLINE uint32_t MIN(uint32_t a, uint32_t b) {
  return ((a) < (b) ? a : b);
}