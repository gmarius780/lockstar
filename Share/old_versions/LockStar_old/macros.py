# -*- coding: utf-8 -*-
"""
Created on Tue Mar 10 16:34:45 2020

@author: qo
"""

# Communication Structure
SEND_BYTES = 20
READ_FLOATS = 1000

Command_SetIO = 16
Command_SetScopeSkips = 17
Command_SetPIDParameters = 18
Command_MeasureAOMResponse = 19
Command_RecordTrace = 20
Command_MoveTo = 21
Command_GotoLock = 22
Command_StopLock = 23
Command_DACLog = 24
Command_DetailedTrace = 27

# ADC Settings
ADC_BIPOLAR_10V		= 0
ADC_BIPOLAR_5V		= 1
ADC_UNIPOLAR_10V	= 2
ADC_UNIPOLAR_5V		= 3
ADC_OFF				  = 4
ADC_CONFIG       = {'+/- 10V' : ADC_BIPOLAR_10V, '+/- 5V' : ADC_BIPOLAR_10V, '0-10V' : ADC_UNIPOLAR_10V, '0-5V' : ADC_UNIPOLAR_5V, 'OFF' : ADC_OFF}
ADC_RANGES       = {'+/- 10V' : [-10,10], '+/- 5V' : [-5,5], '0-10V' : [0,10], '0-5V' : [0,5]}

# DAC Settings
DAC_BIPOLAR_10V		= 0
DAC_BIPOLAR_5V		= 1
DAC_UNIPOLAR_10V	= 2
DAC_UNIPOLAR_5V		= 3
DAC_CONFIG       = {'+/- 10V' : DAC_BIPOLAR_10V, '+/- 5V' : DAC_BIPOLAR_10V, '0-10V' : DAC_UNIPOLAR_10V, '0-5V' : DAC_UNIPOLAR_5V}
DAC_RANGES       = {'+/- 10V' : [-10,10], '+/- 5V' : [-5,5], '0-10V' : [0,10], '0-5V' : [0,5]}

# AOM Linearization
LIN_NUMBER_PIVOTS   = 100

FLANK_FALLING       = -1
FLANK_RISING        = +1
FLANK               = {'FALLING': FLANK_FALLING, 'RISING': FLANK_RISING}
