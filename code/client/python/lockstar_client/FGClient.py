import asyncio
import logging
from lockstar_general.backend.BackendResponse import BackendResponse
from lockstar_general.backend.BackendCall import BackendCall
from lockstar_client.BufferBaseClient_ import BufferBaseClient_
import numpy as np


class FGClient(BufferBaseClient_):
    """Basic Module which implements a simple PID controller by using input_1 as error signal,
    input_2 as setpoint and output 1 for the control signal"""

    def __init__(self, lockstar_ip, lockstar_port, client_id) -> None:
        super().__init__(lockstar_ip, lockstar_port, client_id, "FGModule")

    async def set_cfunction(self, func: str, scale: str):
        bc = BackendCall(
            self.client_id,
            "FGModule",
            "set_cfunction",
            args={"func": func, "scale": scale},
        )
        return await self._call_lockstar(bc)

    async def start_ccalculation(
        self
    ):
        bc = BackendCall(
            self.client_id,
            "FGModule",
            "start_ccalculation",
            args={},
        )
        return await self._call_lockstar(bc)

    async def start_output(self):
        bc = BackendCall(self.client_id, "FGModule", "start_output", args={})
        return await self._call_lockstar(bc)


client = None


def to_q31(x):
    return int(x * 2**31)


def ramp_gen(sampling_freq, flat_scale, ramp_time, amplitude, offset, inverse=False):
    cordic_scaling_factor = 6

    num_samples = int(ramp_time * sampling_freq)
    ramp_begin = -flat_scale
    ramp_end = flat_scale
    inverse = False
    start_value = to_q31(ramp_begin)
    end_value = to_q31(ramp_end)
    step_size = (ramp_end - ramp_begin) / num_samples
    step_size = to_q31(step_size)

    scaling = (2**cordic_scaling_factor) / (
        np.arctan(ramp_begin * (2**cordic_scaling_factor)) / np.pi
    )
    scaling = -scaling if inverse else scaling
    total_scaling = scaling * amplitude
    print(total_scaling, offset, num_samples, start_value, step_size)
    return total_scaling, offset, num_samples, start_value, step_size


def sin_gen(sample_freq, wave_freq, amplitude, offset):
    num_samples = int(sample_freq / wave_freq)
    start_value = 0
    step_size = 0xFFFFFFFF / num_samples
    print(amplitude, offset, num_samples, start_value, step_size)
    print(
        "Amplitude: ",
        amplitude,
        "Offset: ",
        offset,
        "Num Samples: ",
        num_samples,
        "Start Value: ",
        start_value,
        "Step Size: ",
        step_size,
    )
    return amplitude, offset, num_samples, start_value, step_size


if __name__ == "__main__":
    from os.path import join, dirname
    from time import sleep

    client = FGClient("192.168.137.2", 10780, 1234)

if asyncio.run(client.register_client_id()):
    logging.info(f"Successfully initialized AWG module")

    linearization_file = join(dirname(__file__), "test_linearization.json")
    linearization_length = 2000

    sampling_rate = 350000

    ramp1 = ramp_gen(
        sampling_rate, flat_scale=0.1, ramp_time=0.01, amplitude=4, offset=4
    )
    sin1 = sin_gen(sampling_rate, 50, 4, 0)

    func_buffer = [
        # {
        #     "ll_func": 0x00000001,
        #     "ll_scaling": 0x00000000,
        #     "start_value": 0,
        #     "step_size": 0,
        #     "num_samples": 0,
        #     "total_scaling": 0,
        #     "offset": 0,
        #     "n_periods": 4,
        #     "time_start": 2600,
        # },
        {
            "ll_func": 0x00000001,
            "ll_scaling": 0x00000000,
            "start_value": 0,
            "step_size": 17179869,
            "num_samples": 250,
            "total_scaling": 4.0,
            "offset": 0,
            "n_periods": 4,
            "time_start": 4000,
        },
        {
            "ll_func": 0x00000004,
            "ll_scaling": 0x00000600,
            "start_value": -214748364,
            "step_size": 85899,
            "num_samples": 5000,
            "total_scaling": -568.0519530539988,
            "offset": 4,
            "n_periods": 2,
            "time_start": 700,
        },
        {
            "ll_func": 0x00000001,
            "ll_scaling": 0x00000000,
            "start_value": 0,
            "step_size": 17179869,
            "num_samples": 250,
            "total_scaling": 4.0,
            "offset": 0,
            "n_periods": 5,
            "time_start": 8000,
        },
        {
            "ll_func": 0x00000004,
            "ll_scaling": 0x00000600,
            "start_value": -214748364,
            "step_size": 85899,
            "num_samples": 5000,
            "total_scaling": -568.0519530539988,
            "offset": 4,
            "n_periods": 5,
            "time_start": 1000,
        },
        {
            "ll_func": 0x00000001,
            "ll_scaling": 0x00000000,
            "start_value": 0,
            "step_size": 17179869,
            "num_samples": 250,
            "total_scaling": 4.0,
            "offset": 0,
            "n_periods": 5,
            "time_start": 2,
        },
        {
            "ll_func": 0x00000001,
            "ll_scaling": 0x00000000,
            "start_value": 0,
            "step_size": 17179869,
            
            "num_samples": 250,
            "total_scaling": 4.0,
            "offset": 0,
            "n_periods": 5,
            "time_start": 2,
        },
        {
            "ll_func": 0x00000001,
            "ll_scaling": 0x00000000,
            "start_value": 0,
            "step_size": 17179869,
            "num_samples": 250,
            "total_scaling": 4.0,
            "offset": 0,
            "n_periods": 5,
            "time_start": 2,
        },        
        {
            "ll_func": 0x00000001,
            "ll_scaling": 0x00000000,
            "start_value": 0,
            "step_size": 17179869,
            "num_samples": 250,
            "total_scaling": 4.0,
            "offset": 0,
            "n_periods": 50,
            "time_start": 800,
        },
        {
            "ll_func": 0x00000001,
            "ll_scaling": 0x00000000,
            "start_value": 0,
            "step_size": 17179869,
            "num_samples": 250,
            "total_scaling": 4.0,
            "offset": 0,
            "n_periods": 5,
            "time_start": 300,
        },
        {
            "ll_func": 0x00000004,
            "ll_scaling": 0x00000600,
            "start_value": -214748364,
            "step_size": 85899,
            "num_samples": 5000,
            "total_scaling": -568.0519530539988,
            "offset": 4,
            "n_periods": 5,
            "time_start": 500,
        }
    ]
    # ch_one_chunks = [999, 1999, 2999, 3999, 4999]
    # ch_two_chunks = [1999, 2999]
    # ch_one_buffer = np.concatenate((np.cos(np.linspace(0, 2*np.pi, num=1000)),
    #                                 np.linspace(0, 1, num=1000),
    #                                 np.cos(np.linspace(0, 6*np.pi, num=1000)),
    #                                 np.linspace(1, -1, num=1000),
    #                                 np.linspace(-1, 0, num=1000)))

    # ch_two_buffer = np.concatenate((np.cos(np.linspace(0, 50*4*np.pi, num=2000)),
    #                                 np.linspace(1, 2, num=500), np.linspace(2, 1, num=500)))

    # ch_one_buffer = (ch_one_buffer * 5).tolist()
    # ch_two_buffer = (ch_two_buffer * 5).tolist()

    # print(asyncio.run(client.initialize_buffers(len(ch_one_buffer), len(ch_two_buffer), len(ch_one_chunks),
    #                                 len(ch_two_chunks), sampling_rate)))
    print(asyncio.run(client.set_sampling_rate(sampling_rate)))
    print(asyncio.run(client.set_ch_one_output_limits(-10, 10)))
    print(asyncio.run(client.set_ch_two_output_limits(-10, 10)))
    print(asyncio.run(client.set_ch_func_buffer(func_buffer, buffer_one=True)))
    print(asyncio.run(client.set_ch_func_buffer(func_buffer, buffer_one=False)))
    # print(asyncio.run(client.set_cfunction("arctan", "LL_CORDIC_SCALE_6")))
    print(asyncio.run(client.start_ccalculation()))
    sleep(0.001)
    # print(asyncio.run(client.set_ch_func_buffer(func_buffer, buffer_one=True)))
    # print(asyncio.run(client.set_ch_func_buffer(func_buffer, buffer_one=False)))
    print(asyncio.run(client.start_output()))
    # sleep(3)
    # print(asyncio.run(client.set_ch_func_buffer(func_buffer, buffer_one=True)))
    # print(asyncio.run(client.set_cfunction("sin", "LL_CORDIC_SCALE_0")))
    # print(
    #     asyncio.run(
    #         client.start_ccalculation(
    #             *sin1
    #         )
    #     )
    # )
    # print(asyncio.run(client.start_output()))
