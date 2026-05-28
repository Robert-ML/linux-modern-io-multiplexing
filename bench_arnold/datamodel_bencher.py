import asyncio
import signal
from datetime import timedelta
from dataclasses import dataclass

from datamodel_program import ProgramDataModel, ProgramRunner
from subprocess_kit import run_command, run_command_and_signal


@dataclass
class BencherDataModel:
    name: str
    program_info: ProgramDataModel
    no_clients: int
    bench_time: timedelta = timedelta(seconds=30)
    server_warmup_time: timedelta = timedelta(seconds=10)


    async def benchmark(self) -> None:
        # first start the server
        print(f"Starting the server with command: {self.program_info.server_runner.get_executable_path()}")
        server_task = asyncio.create_task(
            run_command_and_signal(
                command=self.program_info.server_runner.get_executable_path(),
                signal_time=self.bench_time,
                signal_to_send=signal.SIGUSR1
            )
        )

        # wait for the server to start up before starting the clients
        await asyncio.sleep(self.server_warmup_time.total_seconds())

        # start the no. of clients in parallel (clients are allowed to fail for now)
        client_tasks = []
        print(f"Starting the clients with command: {self.program_info.client_runner.get_run_command()}")
        for _ in range(self.no_clients):
            client_tasks.append(
                asyncio.create_task(
                    run_command(self.program_info.client_runner.get_run_command(), raise_on_exit_code=False)
                )
            )

        # after the `bench_time`, the server will send the signal `SIGUSR1` to server to close
        print(f"Waiting for the server to finish")
        await server_task
        print(f"Server finished")
        # wait for all clients to finish
        print(f"Waiting for the clients to finish")
        await asyncio.gather(*client_tasks)
        print(f"All clients finished")

        # rename and move the "bench_file.json" to "results/{self.name}.json"
        print(f"Moving the bench results to results/{self.name}.json")
        await run_command(f"mv ./bench_file.json ./results/{self.name}.json")
