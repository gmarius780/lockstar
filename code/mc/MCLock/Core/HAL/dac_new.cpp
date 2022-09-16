/*
 * dac_new.cpp
 *
 *  Created on: 28.07.2022
 *      Author: sjele
 */

#include "dac_new.hpp"

DAC_Device::DAC_Device(uint8_t spi_lane, uint8_t dma_stream_out, uint8_t dma_channel_out, GPIO_TypeDef* sync_port, uint16_t sync_pin, GPIO_TypeDef* clear_port, uint16_t clear_pin) {
    
    inv_step_size 	= 0;
    step_size 		= 0;
    zero_voltage 	= 0;
    full_range 		= 0;

    max_output 	= 0;
    min_output 	= 0;
    busy 		= false;
    invert 		= false;
    dma_buffer 	= new uint8_t[3]();

    this->sync_port 	= sync_port;
    this->sync_pin 		= sync_pin;
    this->clear_port 	= clear_port;
    this->clear_pin 	= clear_pin;

    spi_handler = new SPI(spi_lane);

    uint8_t dma_prio = 2;
    dma_config.priority = dma_prio;
    dma_config.CR = 0;
    // reset 3 bits that define channel
    dma_config.CR &= ~(DMA_SxCR_CHSEL);
    // set channel via 3 control bits
    dma_config.CR |= dma_channel_out * DMA_SxCR_CHSEL_0;
    // set stream priority from very low (00) to very high (11)
    dma_config.CR &= ~(DMA_SxCR_PL); 
    // reset 2 bits that define priority
    dma_config.CR |= dma_prio * DMA_SxCR_PL_0; // set priority via 2 control bits
    // increment the memory address with each transfer
    dma_config.CR |= DMA_SxCR_MINC;
    // do not increment peripheral address
    dma_config.CR &= ~DMA_SxCR_PINC;
    // set direction of transfer to "memory to peripheral"
    dma_config.CR &= ~DMA_SxCR_DIR_1;
    dma_config.CR |= DMA_SxCR_DIR_0;
    // Clear DBM bit
    dma_config.CR &= (uint32_t)(~DMA_SxCR_DBM);
    // Program transmission-complete interrupt
    dma_config.CR  |= DMA_SxCR_TCIE;

    dma_config.stream      = dma_stream_out;
    dma_config.channel     = dma_channel_out;
    dma_config.PAR         = (uint32_t)spi_handler->getDRAddress();
    dma_config.M0AR        = (uint32_t)dma_buffer;
    dma_config.M1AR        = 0;
    dma_config.NDTR        = 0;
    dma_output_handler     = new DMA(dma_config);

    old_dma = new SPI_DMA_Handler(spi_lane, NONE, NONE, dma_stream_out, dma_channel_out, 2);

    spi_handler->enableSPI();

    // Disable Clear-bit from start
	HAL_GPIO_WritePin(clear_port, clear_pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(sync_port, sync_pin, GPIO_PIN_SET);
}

void DAC_Device::write(float output) {
    if(busy)
        return;

    busy = true;

    output = std::min(max_output, std::max(output, min_output));

    int32_t int_output = (int32_t)((output-zero_voltage) * inv_step_size - 0.5f);
    
    // Bring SYNC line low to prepare DAC
    sync_port->BSRR = (uint32_t)sync_pin << 16U;

    if(invert)
        int_output = -int_output;

    // Doesn't check that value is 20bit, may get unexpected results
    dma_buffer[0] = 0b00010000 + ((int_output>>16) & 0x0f);
    dma_buffer[1] = (int_output>>8) & 0xff;
    dma_buffer[2] = int_output & 0xff;

    arm_dma();
}

void DAC_Device::dma_transmission_callback() {
    spi_handler->disable_spi_tx_dma();

    dma_output_handler->resetTransferCompleteInterruptFlag();

     /*
     * bring SYNC line up to finish DA conversion
     * (The DA conversion is completed automatically with the 24th transmitted bit.
     * The SYNC line has to go high at least 20ish ns before the next data package, so it
     * could also be done at a later point, if more convenient / faster.)
     */
    sync_port->BSRR = (uint32_t)sync_pin;

    busy = false;
}

void DAC_Device::config_output(ADC_HandleTypeDef* hadc, uint32_t ADC_SENL, uint32_t ADC_SENH) {
    /*
    * The STM32 on-chip ADC (not the ADC_Device) is used to detect jumper configuration.
    * Based on this the output range of the DAC_Device is set
    */

    // read ADC value of lower voltage
    ADC_ChannelConfTypeDef adc_config   = {0};
    adc_config.Channel                  = ADC_SENL;
    adc_config.Rank                     = 1;
    adc_config.SamplingTime             = ADC_SAMPLETIME_3CYCLES;

    HAL_ADC_ConfigChannel(hadc, &adc_config);
    HAL_ADC_Start(hadc);
    HAL_ADC_PollForConversion(hadc, 1);

    uint32_t adc_result = HAL_ADC_GetValue(hadc);

    float low = 3.3 * adc_result / 4096.0f;
    if (low>=2.6)
        min_hardware_output = 0.0f;
    	min_output = 0.0f;
    if (low<2.6 && low>=1.0)
        min_hardware_output = -5.0f;
    	min_output = -5.0f;
    if (low < 1.0)
        min_hardware_output = -10.0f;
    	min_output = -10.0f;

    // read ADC value of upper voltage
    adc_config.Channel = ADC_SENH;
    HAL_ADC_ConfigChannel(hadc, &adc_config);
    HAL_ADC_Start(hadc);
    HAL_ADC_PollForConversion(hadc, 1);

    adc_result = HAL_ADC_GetValue(hadc);

    float high = 3.3 * adc_result / 4096.0f;
    if (high>=2.4)
        max_hardware_output = 10.0f;
    	max_output = 10.0f;
    if (high < 2.4)
        max_hardware_output = 5.0f;
    	max_output = 5.0f;

    full_range    		= max_output - min_output;
    zero_voltage        = (max_output + min_output)/2.0f;
    step_size           = full_range / 0xfffff;   // full_range / (2^20-1)
    inv_step_size       = 1 / step_size;
    invert              = false;

    send_output_range();
}

void DAC_Device::send_output_range() {
    busy = true;

    // bring SYNC line low to prepare DAC
    sync_port->BSRR = (uint32_t)sync_pin << 16U;

    // depending on the output range, the DAC applies a correction to improve linear behavior
    uint8_t comp = 0b0000;
    if(full_range<10.0f)
        comp = 0b0000;
    else if(full_range<12.0f)
        comp = 0b1001;
    else if(full_range<16.0f)
        comp = 0b1010;
    else if(full_range<19.0f)
        comp = 0b1011;
    else if(full_range>=19.0f)
        comp = 0b1100;

    // construct control register, see datasheet
    bool RBUF = true;
    bool OPGND = false;
    bool DACTRI = false;
    bool NOT2C = false;
    bool SDODIS = false;
    volatile uint8_t control_reg = (RBUF<<1)+(OPGND<<2)+(DACTRI<<3)+(NOT2C<<4)+(SDODIS<<5);
    dma_buffer[0] = 0b00100000;
    dma_buffer[1] = comp>>2;
    dma_buffer[2] = ((comp & 0b11)<<6)+control_reg;

    arm_dma();

    // Wait till configuration is sent
    while(is_busy());
}

void DAC_Device::arm_dma() {
    dma_output_handler->disableDMA();

    // TODO: create an arm_dma method in the DMA class
    dma_output_handler->setMemory0Address(dma_buffer);
    dma_output_handler->setNumberOfData(3);

    dma_output_handler->enableDMA();

    spi_handler->enable_spi_tx_dma();
}
