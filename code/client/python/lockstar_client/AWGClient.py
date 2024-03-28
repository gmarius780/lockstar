import asyncio
import logging
from lockstar_client.BufferBaseClient_ import BufferBaseClient_
from time import sleep
import matplotlib.pyplot as plt
import numpy as np

class AWGClient(BufferBaseClient_):
    def __init__(self, lockstar_ip, lockstar_port, client_id) -> None:
        super().__init__(lockstar_ip, lockstar_port, client_id, 'AWGModule')

def scope_test(client):
    scope_sampling_rate = 1000
    scope_buffer_length = 1000
    print(asyncio.run(client.setup_scope(
        sampling_rate=scope_sampling_rate,
        sample_in_one=True,
        sample_in_two=True,
        sample_out_one=True,
        sample_out_two=True,
        buffer_length=scope_buffer_length,
        adc_active_mode=True
    )))
    print(asyncio.run(client.enable_scope()))
    scope_time_axis = np.arange(scope_buffer_length)*1/scope_sampling_rate

    for _ in range(10):
        asyncio.run(client.output_on())
        sleep(1)
        scope_data = asyncio.run(client.get_scope_data())
        if scope_data != False:
            plt.figure()
            for trace_name in scope_data.keys():
                trace = scope_data[trace_name]
                if len(trace) > 0:
                    plt.plot(scope_time_axis, trace, label=trace_name)
            
            plt.legend()
            plt.show()

if __name__ == "__main__":
    from os.path import join, dirname
    # logging.basicConfig(
    #     level=logging.DEBUG,
    #     format="%(asctime)s [%(levelname)s] %(message)s",
    #     handlers=[
    #         # logging.FileHandler("./debug.log"),
    #         logging.StreamHandler()
    #     ]
    # )
    client = AWGClient('192.168.137.2', 10780, 1234)

    
    if asyncio.run(client.register_client_id()):
        logging.info(f'Successfully initialized AWG module')
        
        linearization_file = join(dirname(__file__), 'test_linearization.json')
        linearization_length = 2000

        sampling_rate = 500000
        
        print(asyncio.run(client.set_ch_one_output_limits(-10, 10)))
        print(asyncio.run(client.set_ch_two_output_limits(-10, 10)))
        print(asyncio.run(client.unclamp_output()))
        # ch_one_chunks = [1999]
        # ch_one_buffer = np.linspace(0, 10, num=2000).tolist()
        # ch_two_chunks = [0]
        # ch_two_buffer = [0.]
        # print(asyncio.run(client.initialize_buffers(len(ch_one_buffer), 1, len(ch_one_chunks), 1, sampling_rate)))
        # print(asyncio.run(client.set_ch_one_output_limits(0, 10)))
        # print(asyncio.run(client.set_ch_two_output_limits(0, 10)))
        # print(asyncio.run(client.set_ch_one_chunks(ch_one_chunks)))
        # print(asyncio.run(client.set_ch_two_chunks(ch_two_chunks)))
        # print(asyncio.run(client.set_ch_one_buffer(ch_one_buffer)))
        # print(asyncio.run(client.set_ch_two_buffer(ch_two_buffer)))

        # print(asyncio.run(client.set_linearization_length_one(linearization_length)))
        # print(asyncio.run(client.set_linearization_one_from_file(linearization_file)))
        #print(await client.disable_linearization_one())

        ch_one_chunks = [999, 1999, 2999, 3999, 4999]
        ch_two_chunks = [1999, 2999]
        ch_one_buffer = np.concatenate((np.cos(np.linspace(0, 2*np.pi, num=1000)),
                                        np.linspace(0, 1, num=1000),
                                        np.cos(np.linspace(0, 6*np.pi, num=1000)),
                                        np.linspace(1, -1, num=1000),
                                        np.linspace(-1, 0, num=1000)))
        # ch_one_buffer = np.concatenate((np.sin(np.linspace(0, 1*2*np.pi, num=1000)),
        #                                 np.sin(np.linspace(0, 1*2*np.pi, num=1000)),
        #                                 np.sin(np.linspace(0, 1*2*np.pi, num=1000)),
        #                                 np.sin(np.linspace(0, 1*2*np.pi, num=1000)),
        #                                 np.sin(np.linspace(0, 1*2*np.pi, num=1000)))).tolist()

        ch_two_buffer = np.concatenate((np.cos(np.linspace(0, 50*4*np.pi, num=2000)),
                                        np.linspace(1, 2, num=500), np.linspace(2, 1, num=500)))

        ch_one_buffer = (ch_one_buffer*7).tolist()
        ch_two_buffer = (ch_two_buffer).tolist()
        

        # ch_one_chunks = [1999]
        # ch_two_chunks = [1999]
        
        # ch_one_buffer = np.sin(np.linspace(0, 50*2*np.pi, num=2000)).tolist()
        # ch_two_buffer = np.sin(np.linspace(0, 50*2*np.pi, num=2000)).tolist()


        # print(await lient.set_ch_one_output_limits(0, 1))
        print(asyncio.run(client.initialize_buffers(len(ch_one_buffer), len(ch_two_buffer), len(ch_one_chunks), 
                                        len(ch_two_chunks), sampling_rate)))
        # print(asyncio.run(client.set_ch_one_output_limits(-10, 10)))
        # print(asyncio.run(client.set_ch_two_output_limits(-10, 10)))
        print(asyncio.run(client.set_ch_one_chunks(ch_one_chunks)))
        print(asyncio.run(client.set_ch_two_chunks(ch_two_chunks)))
        print(asyncio.run(client.set_ch_one_buffer(ch_one_buffer)))
        print(asyncio.run(client.set_ch_two_buffer(ch_two_buffer)))
        print(asyncio.run(client.output_on()))