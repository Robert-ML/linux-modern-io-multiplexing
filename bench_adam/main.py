import asyncio
from datetime import timedelta

from datamodel_program import (
    MakeBuilder,
    ProgramDataModel,
    ProgramRunner,
    CDefineType,
    MakeVarArg
)
from datamodel_bencher import BencherDataModel


test_config: BencherDataModel = BencherDataModel(
    name="epoll_1000c_4096B_0rr",
    program_info=ProgramDataModel(
        builder=MakeBuilder(
                r_path_server="../code/c/server/epoll/",
                r_path_client="../code/c/client/",
                builder_args=[
                    MakeVarArg(name="CDEFINES", value=f""
                        f" {CDefineType(name="DEFAULT_BUFFER_SIZE", value="4096U")}"
                        f" {CDefineType(name="CLIENT_MODE", value="0")}"
                    )
                ],
            ),
        server_runner=ProgramRunner(
            executable_name="epoll_server",
            path="../code/c/server/epoll/",
            args=[]
        ),
        client_runner=ProgramRunner(
            executable_name="client",
            path="../code/c/client/",
            args=[]
        ),
    ),
    no_clients=1000,
)


async def main():
    await run_bench(test_config)

async def run_bench(bench: BencherDataModel):
    print(f"Running bench: {bench.name}")

    # clean up in case of previous build artifacts
    await bench.program_info.clean()

    # build the server and client programs
    await bench.program_info.build()

    # run the bench
    await bench.benchmark()

    # clean up after the bench
    await bench.program_info.clean()


if __name__ == "__main__":
    asyncio.run(main())
