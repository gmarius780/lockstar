import asyncio
import logging
from lockstar_client.BufferBaseClient_ import BufferBaseClient_

class AWGClient(BufferBaseClient_):
    def __init__(self, lockstar_ip, lockstar_port, client_id) -> None:
        super().__init__(lockstar_ip, lockstar_port, client_id, 'AWGModule')


if __name__ == "__main__":
    import numpy as np
    from os.path import join, dirname
    logging.basicConfig(
        level=logging.DEBUG,
        format="%(asctime)s [%(levelname)s] %(message)s",
        handlers=[
            # logging.FileHandler("./debug.log"),
            logging.StreamHandler()
        ]
    )
    client = AWGClient('192.168.88.13', 10780, 1234)

    
    if client.register_client_id():
        logging.info(f'Successfully initialized AWG module')
        
        linearization_file = join(dirname(__file__), 'test_linearization.json')
        linearization_length = 2000

        sampling_rate = 1000
        
        ch_one_chunks = [499]
        ch_one_buffer = np.linspace(0, 10, num=500).tolist()
        ch_two_chunks = [0]
        ch_two_buffer = [0.]
        print(client.initialize_buffers(len(ch_one_buffer), 1, len(ch_one_chunks), 1, sampling_rate))
        print(client.set_ch_one_output_limits(0, 10))
        print(client.set_ch_two_output_limits(0, 10))
        print(client.set_ch_one_chunks(ch_one_chunks))
        print(client.set_ch_two_chunks(ch_two_chunks))
        print(client.set_ch_one_buffer(ch_one_buffer))
        print(client.set_ch_two_buffer(ch_two_buffer))

        print(client.set_linearization_length_one(linearization_length))
        print(client.set_linearization_one_from_file(linearization_file))
        #print(client.disable_linearization_one())

        # ch_one_chunks = [999, 1999, 2999, 3999, 4999]
        # ch_two_chunks = [1999, 2999]
        # # ch_one_buffer = np.concatenate((np.sin(np.linspace(0, 2*np.pi, num=1000)),
        # #                                 np.linspace(0, 1, num=1000),
        # #                                 np.cos(np.linspace(0, 6*np.pi, num=1000)),
        # #                                 np.linspace(1, -3, num=1000),
        # #                                 np.linspace(-3, 0, num=1000))).tolist()
        # ch_one_buffer = np.concatenate((np.sin(np.linspace(0, 1*2*np.pi, num=1000)),
        #                                 np.sin(np.linspace(0, 1*2*np.pi, num=1000)),
        #                                 np.sin(np.linspace(0, 1*2*np.pi, num=1000)),
        #                                 np.sin(np.linspace(0, 1*2*np.pi, num=1000)),
        #                                 np.sin(np.linspace(0, 1*2*np.pi, num=1000)))).tolist()

        # ch_two_buffer = np.concatenate((np.cos(np.linspace(0, 50*4*np.pi, num=2000)),
        #                                 np.linspace(1, 4, num=500), np.linspace(4, 1, num=500))).tolist()
        

        # # ch_one_chunks = [1999]
        # # ch_two_chunks = [1999]
        
        # # ch_one_buffer = np.sin(np.linspace(0, 50*2*np.pi, num=2000)).tolist()
        # # ch_two_buffer = np.sin(np.linspace(0, 50*2*np.pi, num=2000)).tolist()


        # # print(client.set_ch_one_output_limits(0, 1))
        # print(client.initialize_buffers(len(ch_one_buffer), len(ch_two_buffer), len(ch_one_chunks), 
        #                                 len(ch_two_chunks), sampling_rate))
        # print(client.set_ch_one_output_limits(-5, 5))
        # print(client.set_ch_two_output_limits(-5, 5))
        # print(client.set_ch_one_chunks(ch_one_chunks))
        # print(client.set_ch_two_chunks(ch_two_chunks))
        # print(client.set_ch_one_buffer(ch_one_buffer))
        # print(client.set_ch_two_buffer(ch_two_buffer))
        # client.output_ttl()

