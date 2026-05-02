import asyncio
import signal
from datetime import timedelta

async def run_command(command: str, raise_on_exit_code: bool = True) -> None:
    process: asyncio.subprocess.Process = await asyncio.create_subprocess_shell(
        command,
        stdout=asyncio.subprocess.DEVNULL,
        stderr=asyncio.subprocess.DEVNULL
    )

    process_exit_code: int = await process.wait()

    if raise_on_exit_code == True and process_exit_code != 0:
        print(f"Command failed with return code {process_exit_code}")
        raise RuntimeError(f"Command failed: {command}")


async def run_command_and_signal(command: str, signal_time: timedelta, signal_to_send: signal.Signals) -> None:
    process: asyncio.subprocess.Process = await asyncio.create_subprocess_exec(
        command,
        stdout=asyncio.subprocess.DEVNULL,
        stderr=asyncio.subprocess.DEVNULL
    )
    print(f"\tStarted process with PID {process.pid}")

    await asyncio.sleep(signal_time.total_seconds())

    print(f"\tSending signal {signal_to_send} to process with PID {process.pid}")
    process.send_signal(signal_to_send)

    print(f"\tWaiting for process with PID {process.pid} to finish")
    process_exit_code: int = await process.wait()

    if process_exit_code != 0:
        # print the stdout of the process for debugging
        print(f"\tCommand failed with return code {process_exit_code}")
        raise RuntimeError(f"Command failed: {command}")
