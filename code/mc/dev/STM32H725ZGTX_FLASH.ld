/*
******************************************************************************
**
**  File        : LinkerScript.ld
**
**  Author      : STM32CubeIDE
**
**  Abstract    : Linker script for STM32H7 series
**                1024Kbytes FLASH and 560Kbytes RAM
**
**                Set heap size, stack size and stack location according
**                to application requirements.
**
**                Set memory bank area and size if external memory is used.
**
**  Target      : STMicroelectronics STM32
**
**  Distribution: The file is distributed as is, without any warranty
**                of any kind.
**
*****************************************************************************
** @attention
**
** Copyright (c) 2023 STMicroelectronics.
** All rights reserved.
**
** This software is licensed under terms that can be found in the LICENSE file
** in the root directory of this software component.
** If no LICENSE file comes with this software, it is provided AS-IS.
**
****************************************************************************
*/

/* Entry Point */
ENTRY(Reset_Handler)

/* Highest address of the user mode stack */
_estack = ORIGIN(RAM_D1) + LENGTH(RAM_D1);    /* end of RAM */
/* Generate a link error if heap and stack don't fit into RAM */
_Min_Heap_Size = 0x200;      /* required amount of heap  */
_Min_Stack_Size = 0x400; /* required amount of stack */

/* Specify the memory areas */
MEMORY
{
  ITCMRAM (xrw)    : ORIGIN = 0x00000000,   LENGTH = 64K
  DTCMRAM (xrw)    : ORIGIN = 0x20000000,   LENGTH = 128K
  FLASH    (rx)    : ORIGIN = 0x08000000,   LENGTH = 1024K
  RAM_D1  (xrw)    : ORIGIN = 0x24000000,   LENGTH = 320K
  RAM_D2  (xrw)    : ORIGIN = 0x30000000,   LENGTH = 32K
  RAM_D3  (xrw)    : ORIGIN = 0x38000000,   LENGTH = 16K
}

/* Define output sections */
SECTIONS
{
  _sivector = LOADADDR(.isr_vector);
  /* The startup code goes first into FLASH */
  .isr_vector :
  {
    . = ALIGN(4);
    _svector = .;
    KEEP(*(.isr_vector)) /* Startup code */
    . = ALIGN(4);
    _evector = .;
  } >RAM_D1 AT> FLASH

 /*--- New ITCMRAM linker section definition ---*/
 _siitcmram = LOADADDR(.itcmram);
 /* ITCMRAM section */
 .itcmram :
 {
 . = ALIGN(4);
 _sitcmram = .; /* define a global symbols at ccmitcmramram start */
 *(.itcmram) 
 *(.itcmram*)
 . = ALIGN(4);
 _eitcmram = .; /* define a global symbols at itcmram end */
 } >ITCMRAM AT> FLASH
 /*--- End of ITCMRAM linker section definition ---*/

  /*--- New DTCMRAM linker section definition ---*/
 _sidtcmram = LOADADDR(.dtcmram);
 /* DTCMRAM section */
 .dtcmram :
 {
 . = ALIGN(4);
 _sdtcmram = .; /* define a global symbols at dtcmram start */
 *(.dtcmram) 
 *(.dtcmram*)
 . = ALIGN(4);
 _edtcmram = .; /* define a global symbols at dtcmram end */
 } >DTCMRAM AT> FLASH
 /*--- End of DTCMRAM linker section definition ---*/


.BDMABlock : 
{
  . = ALIGN(32);
  *(.BDMA_Access)
  . = ALIGN(32);
} >RAM_D3

.DMA_D1 : 
{
  *(.DMA_Access)
  KEEP(*(.DMA_Access)) 
} >RAM_D1

.ADC_D1 : 
{
  *(.ADC_Access)
  KEEP(*(.ADC_Access)) 
} >RAM_D1

.RPI_W_D1 :
{
  *(.RPI_W_Access)
  KEEP(*(.RPI_W_Access))
} >RAM_D1

.RPI_R_D1 :
{
  *(.RPI_R_Access)
  KEEP(*(.RPI_R_Access))
} >RAM_D1


  /* The program code and other data goes into FLASH */
  .text :
  {
    . = ALIGN(4);
    *(.text)           /* .text sections (code) */
    *(.text*)          /* .text* sections (code) */
    *(.glue_7)         /* glue arm to thumb code */
    *(.glue_7t)        /* glue thumb to arm code */
    *(.eh_frame)

    KEEP (*(.init))
    KEEP (*(.fini))

    . = ALIGN(4);
    _etext = .;        /* define a global symbols at end of code */
  } >FLASH
  
  .sram_func : 
  {
    . = ALIGN(4);
    __sram_func_start = .;
    *(.sram_func)
    *(.sram_func*)
    __sram_func_end = .;
  } >FLASH 
  __sram_func_loadaddr = LOADADDR(.sram_func);
  
  /* Constant data goes into FLASH */
  .rodata :
  {
    . = ALIGN(4);
    *(.rodata)         /* .rodata sections (constants, strings, etc.) */
    *(.rodata*)        /* .rodata* sections (constants, strings, etc.) */
    . = ALIGN(4);
  } >FLASH

  .ARM.extab   : { *(.ARM.extab* .gnu.linkonce.armextab.*) } >FLASH
  .ARM : {
    __exidx_start = .;
    *(.ARM.exidx*)
    __exidx_end = .;
  } >FLASH

  .preinit_array     :
  {
    PROVIDE_HIDDEN (__preinit_array_start = .);
    KEEP (*(.preinit_array*))
    PROVIDE_HIDDEN (__preinit_array_end = .);
  } >FLASH

  .init_array :
  {
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP (*(SORT(.init_array.*)))
    KEEP (*(.init_array*))
    PROVIDE_HIDDEN (__init_array_end = .);
  } >FLASH

  .fini_array :
  {
    PROVIDE_HIDDEN (__fini_array_start = .);
    KEEP (*(SORT(.fini_array.*)))
    KEEP (*(.fini_array*))
    PROVIDE_HIDDEN (__fini_array_end = .);
  } >FLASH

  /* used by the startup to initialize data */
  _sidata = LOADADDR(.data);

  /* Initialized data sections goes into RAM, load LMA copy after code */
  .data :
  {
    . = ALIGN(4);
    _sdata = .;        /* create a global symbol at data start */
    *(.data)           /* .data sections */
    *(.data*)          /* .data* sections */
    *(.RamFunc)        /* .RamFunc sections */
    *(.RamFunc*)       /* .RamFunc* sections */

    . = ALIGN(4);
    _edata = .;        /* define a global symbol at data end */
  } >RAM_D1 AT> FLASH

  /* Uninitialized data section */
  . = ALIGN(4);
  .bss :
  {
    /* This is used by the startup in order to initialize the .bss section */
    _sbss = .;         /* define a global symbol at bss start */
    __bss_start__ = _sbss;
    *(.bss)
    *(.bss*)
    *(COMMON)

    . = ALIGN(4);
    _ebss = .;         /* define a global symbol at bss end */
    __bss_end__ = _ebss;
  } >RAM_D1

  /* User_heap_stack section, used to check that there is enough RAM left */
  ._user_heap_stack :
  {
    . = ALIGN(8);
    PROVIDE ( end = . );
    PROVIDE ( _end = . );
    . = . + _Min_Heap_Size;
    . = . + _Min_Stack_Size;
    . = ALIGN(8);
  } >RAM_D1

  /* Remove information from the standard libraries */
  /DISCARD/ :
  {
    libc.a ( * )
    libm.a ( * )
    libgcc.a ( * )
  }

  .ARM.attributes 0 : { *(.ARM.attributes) }
}
