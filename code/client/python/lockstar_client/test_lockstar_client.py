from BackendDataPackages.BackendDP import BackendDP
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
    dp = BackendDP(int(0).to_bytes(2, 'big'), int(3).to_bytes(2, 'big'), int(10).to_bytes(4, 'big'), int(1000).to_bytes(10, 'big'))
    asyncio.run(call_backend(dp))
