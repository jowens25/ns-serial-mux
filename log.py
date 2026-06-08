import asyncio
from datetime import datetime
import os


async def main():

    socket_path = "/var/lib/ns/ns-serial-mux.sock"
    reader, writer = await asyncio.open_unix_connection(socket_path)

    f_name = f"{datetime.now().strftime("%d-%m-%Y")}_log.txt"

    base, ext = os.path.splitext(f_name)
    counter = 1

    while os.path.exists(f_name):
        f_name = f"{base}({counter}){ext}"
        counter += 1

    while True:
        line = await reader.readline()
        if not line:
            break
        line = line.decode("utf-8", errors="ignore")

        with open(f_name, "a") as f:
            f.writelines(line)
            f.close()


asyncio.run(main())
