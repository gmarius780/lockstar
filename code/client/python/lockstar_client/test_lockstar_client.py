from lockstar_general.communication.BackendDP import BackendDP
from lockstar_general.communication.SinglePIDMCDP import SinglePIDMCDP
import asyncio

async def call_backend(dp):
    reader, writer = await asyncio.open_connection(
        '127.0.0.1', 10780)

    for d in dp.byte_array():
        writer.write(d)

    await writer.drain()

    data = await reader.read(100)
    print(f'Received: {data.decode()!r}')

    print('Close the connection')
    writer.close()
    await writer.wait_closed()

if __name__ == "__main__":
    dp = BackendDP.for_init_hardware(1234)
    
    asyncio.run(call_backend(dp))
