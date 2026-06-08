import asyncio
import sys


async def listen(path="/var/lib/ns/ns-serial-mux.sock"):

    socket_path = path
    reader, writer = await asyncio.open_unix_connection(socket_path)

    while True:
        yield reader.readline()

    writer.close()

    await writer.wait_closed()

    return "".join(responses)


async def read_socket(reader, cmd, timeout=1):

    rsps = []

    try:
        async with asyncio.timeout(timeout):
            while True:
                line = await reader.readline()

                if not line:
                    break

                line = line.decode("utf-8")

                if line.startswith(cmd):
                    rsps.append(line)
                    break

    except TimeoutError:

        pass

    return rsps


async def read_write_socket(cmd: str) -> str:

    socket_path = "/var/lib/ns/ns-serial-mux.sock"
    reader, writer = await asyncio.open_unix_connection(socket_path)

    read_task = asyncio.create_task(read_socket(reader, cmd))

    writer.write((cmd + "\r\n").encode())
    await writer.drain()

    responses = await read_task

    writer.close()

    await writer.wait_closed()

    return "".join(responses)


if len(sys.argv) > 1:

    rsp = asyncio.run(read_write_socket("$" + sys.argv[1]))

    if rsp:
        print(rsp.strip("\r\n"))
    else:
        print("no rsp or no cmd")
else:
    print("run: python write.py <command w/o $>")
