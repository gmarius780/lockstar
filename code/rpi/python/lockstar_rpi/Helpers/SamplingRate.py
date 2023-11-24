import numpy as np
from math import ceil

from lockstar_rpi.BackendSettings import BackendSettings

class SamplingRate:
    @staticmethod
    def calculate_prescaler_counter(sampling_rate):
        """Calculate prescaler and counter for a given sampling_rate. The MC realizes a sampling rate by scaling down 
        the internal clock_frequency (BackendSettings.mc_sampling_clock_rate) with the prescaler and then counting up to
        <counter> --> sampling_rate = internal_clock_rate / prescaler / counter"""
        rate = BackendSettings.mc_sampling_clock_rate / sampling_rate
        possible_counters = np.flip(np.arange(ceil(rate/BackendSettings.mc_max_counter), BackendSettings.mc_max_counter))
        possible_prescalers = rate/possible_counters
        best_prescaler = int(possible_prescalers[np.abs(possible_prescalers - possible_prescalers.astype(int)).argmin()])
        best_counter = int(rate/best_prescaler)
        return best_prescaler - 1  , best_counter - 1