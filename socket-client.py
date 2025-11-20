import socket
import sys
import asyncio

socket_path = "/tmp/serial.sock"
client = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)


async def rx():
    while True:
        # client.send(bytes("0x000", 'utf-8'))
        response = client.recv(2048)
        sys.stdout.write(response.decode())  # No extra newline
        sys.stdout.flush()
        await asyncio.sleep(0)


async def tx():

    while True:
        inp = "$VER?\r\n"
        client.send(bytes(inp, 'utf-8'))
        await asyncio.sleep(1)


async def main():

    client.connect(socket_path)

    await asyncio.gather(rx(), tx())

    client.close()


if __name__ == "__main__":
    asyncio.run(main())
