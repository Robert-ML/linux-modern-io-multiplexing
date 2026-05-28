from datamodel_program import (
    MakeBuilder,
    ProgramDataModel,
    CDefineType,
    ChainerCDefineType,
    MakeVarArg
)
from datamodel_bencher import BencherDataModel

from constants import (
    CLIENT_PATH,
    CLIENT_RUNNER,
    EPOLL_SERVER_PATH,
    EPOLL_SERVER_RUNNER,
)


# epoll method,    1 clients,   512B buffer size, 0 request-response mode
# epoll method,   10 clients,   512B buffer size, 0 request-response mode
# epoll method,  100 clients,   512B buffer size, 0 request-response mode
# epoll method, 1000 clients,   512B buffer size, 0 request-response mode

# epoll method,    1 clients,  4096B buffer size, 0 request-response mode
# epoll method,   10 clients,  4096B buffer size, 0 request-response mode
# epoll method,  100 clients,  4096B buffer size, 0 request-response mode
# epoll method, 1000 clients,  4096B buffer size, 0 request-response mode

# epoll method,    1 clients, 16384B buffer size, 0 request-response mode
# epoll method,   10 clients, 16384B buffer size, 0 request-response mode
# epoll method,  100 clients, 16384B buffer size, 0 request-response mode
# epoll method, 1000 clients, 16384B buffer size, 0 request-response mode

# epoll method,    1 clients, 65536B buffer size, 0 request-response mode
# epoll method,   10 clients, 65536B buffer size, 0 request-response mode
# epoll method,  100 clients, 65536B buffer size, 0 request-response mode
# epoll method, 1000 clients, 65536B buffer size, 0 request-response mode
epoll_bench_0rr_mode_config: list[BencherDataModel] = [
    # 512B buffer size
    BencherDataModel(
        name="epoll_1c_512B_0rr",
        program_info=ProgramDataModel(
            builder=MakeBuilder(
                    r_path_server=EPOLL_SERVER_PATH,
                    r_path_client=CLIENT_PATH,
                    builder_args=[
                        MakeVarArg(name="CDEFINES", value=ChainerCDefineType([
                            CDefineType(name="DEFAULT_BUFFER_SIZE", value="512U"),
                            CDefineType(name="CLIENT_MODE", value="0"),
                        ])
                        )
                    ],
                ),
            server_runner=EPOLL_SERVER_RUNNER,
            client_runner=CLIENT_RUNNER,
        ),
        no_clients=1,
    ),
    BencherDataModel(
        name="epoll_10c_512B_0rr",
        program_info=ProgramDataModel(
            builder=MakeBuilder(
                    r_path_server=EPOLL_SERVER_PATH,
                    r_path_client=CLIENT_PATH,
                    builder_args=[
                        MakeVarArg(name="CDEFINES", value=ChainerCDefineType([
                            CDefineType(name="DEFAULT_BUFFER_SIZE", value="512U"),
                            CDefineType(name="CLIENT_MODE", value="0"),
                        ])
                        )
                    ],
                ),
            server_runner=EPOLL_SERVER_RUNNER,
            client_runner=CLIENT_RUNNER,
        ),
        no_clients=10,
    ),
    BencherDataModel(
        name="epoll_100c_512B_0rr",
        program_info=ProgramDataModel(
            builder=MakeBuilder(
                    r_path_server=EPOLL_SERVER_PATH,
                    r_path_client=CLIENT_PATH,
                    builder_args=[
                        MakeVarArg(name="CDEFINES", value=ChainerCDefineType([
                            CDefineType(name="DEFAULT_BUFFER_SIZE", value="512U"),
                            CDefineType(name="CLIENT_MODE", value="0"),
                        ])
                        )
                    ],
                ),
            server_runner=EPOLL_SERVER_RUNNER,
            client_runner=CLIENT_RUNNER,
        ),
        no_clients=100,
    ),
    BencherDataModel(
        name="epoll_1000c_512B_0rr",
        program_info=ProgramDataModel(
            builder=MakeBuilder(
                    r_path_server=EPOLL_SERVER_PATH,
                    r_path_client=CLIENT_PATH,
                    builder_args=[
                        MakeVarArg(name="CDEFINES", value=ChainerCDefineType([
                            CDefineType(name="DEFAULT_BUFFER_SIZE", value="512U"),
                            CDefineType(name="CLIENT_MODE", value="0"),
                        ])
                        )
                    ],
                ),
            server_runner=EPOLL_SERVER_RUNNER,
            client_runner=CLIENT_RUNNER,
        ),
        no_clients=1000,
    ),

    # 4096B buffer size
    BencherDataModel(
        name="epoll_1c_4096B_0rr",
        program_info=ProgramDataModel(
            builder=MakeBuilder(
                    r_path_server=EPOLL_SERVER_PATH,
                    r_path_client=CLIENT_PATH,
                    builder_args=[
                        MakeVarArg(name="CDEFINES", value=ChainerCDefineType([
                            CDefineType(name="DEFAULT_BUFFER_SIZE", value="4096U"),
                            CDefineType(name="CLIENT_MODE", value="0"),
                        ])
                        )
                    ],
                ),
            server_runner=EPOLL_SERVER_RUNNER,
            client_runner=CLIENT_RUNNER,
        ),
        no_clients=1,
    ),
    BencherDataModel(
        name="epoll_10c_4096B_0rr",
        program_info=ProgramDataModel(
            builder=MakeBuilder(
                    r_path_server=EPOLL_SERVER_PATH,
                    r_path_client=CLIENT_PATH,
                    builder_args=[
                        MakeVarArg(name="CDEFINES", value=ChainerCDefineType([
                            CDefineType(name="DEFAULT_BUFFER_SIZE", value="4096U"),
                            CDefineType(name="CLIENT_MODE", value="0"),
                        ])
                        )
                    ],
                ),
            server_runner=EPOLL_SERVER_RUNNER,
            client_runner=CLIENT_RUNNER,
        ),
        no_clients=10,
    ),
    BencherDataModel(
        name="epoll_100c_4096B_0rr",
        program_info=ProgramDataModel(
            builder=MakeBuilder(
                    r_path_server=EPOLL_SERVER_PATH,
                    r_path_client=CLIENT_PATH,
                    builder_args=[
                        MakeVarArg(name="CDEFINES", value=ChainerCDefineType([
                            CDefineType(name="DEFAULT_BUFFER_SIZE", value="4096U"),
                            CDefineType(name="CLIENT_MODE", value="0"),
                        ])
                        )
                    ],
                ),
            server_runner=EPOLL_SERVER_RUNNER,
            client_runner=CLIENT_RUNNER,
        ),
        no_clients=100,
    ),
    BencherDataModel(
        name="epoll_1000c_4096B_0rr",
        program_info=ProgramDataModel(
            builder=MakeBuilder(
                    r_path_server=EPOLL_SERVER_PATH,
                    r_path_client=CLIENT_PATH,
                    builder_args=[
                        MakeVarArg(name="CDEFINES", value=ChainerCDefineType([
                            CDefineType(name="DEFAULT_BUFFER_SIZE", value="4096U"),
                            CDefineType(name="CLIENT_MODE", value="0"),
                        ])
                        )
                    ],
                ),
            server_runner=EPOLL_SERVER_RUNNER,
            client_runner=CLIENT_RUNNER,
        ),
        no_clients=1000,
    ),

    # 16384B buffer size
    BencherDataModel(
        name="epoll_1c_16384B_0rr",
        program_info=ProgramDataModel(
            builder=MakeBuilder(
                    r_path_server=EPOLL_SERVER_PATH,
                    r_path_client=CLIENT_PATH,
                    builder_args=[
                        MakeVarArg(name="CDEFINES", value=ChainerCDefineType([
                            CDefineType(name="DEFAULT_BUFFER_SIZE", value="16384U"),
                            CDefineType(name="CLIENT_MODE", value="0"),
                        ])
                        )
                    ],
                ),
            server_runner=EPOLL_SERVER_RUNNER,
            client_runner=CLIENT_RUNNER,
        ),
        no_clients=1,
    ),
    BencherDataModel(
        name="epoll_10c_16384B_0rr",
        program_info=ProgramDataModel(
            builder=MakeBuilder(
                    r_path_server=EPOLL_SERVER_PATH,
                    r_path_client=CLIENT_PATH,
                    builder_args=[
                        MakeVarArg(name="CDEFINES", value=ChainerCDefineType([
                            CDefineType(name="DEFAULT_BUFFER_SIZE", value="16384U"),
                            CDefineType(name="CLIENT_MODE", value="0"),
                        ])
                        )
                    ],
                ),
            server_runner=EPOLL_SERVER_RUNNER,
            client_runner=CLIENT_RUNNER,
        ),
        no_clients=10,
    ),
    BencherDataModel(
        name="epoll_100c_16384B_0rr",
        program_info=ProgramDataModel(
            builder=MakeBuilder(
                    r_path_server=EPOLL_SERVER_PATH,
                    r_path_client=CLIENT_PATH,
                    builder_args=[
                        MakeVarArg(name="CDEFINES", value=ChainerCDefineType([
                            CDefineType(name="DEFAULT_BUFFER_SIZE", value="16384U"),
                            CDefineType(name="CLIENT_MODE", value="0"),
                        ])
                        )
                    ],
                ),
            server_runner=EPOLL_SERVER_RUNNER,
            client_runner=CLIENT_RUNNER,
        ),
        no_clients=100,
    ),
    BencherDataModel(
        name="epoll_1000c_16384B_0rr",
        program_info=ProgramDataModel(
            builder=MakeBuilder(
                    r_path_server=EPOLL_SERVER_PATH,
                    r_path_client=CLIENT_PATH,
                    builder_args=[
                        MakeVarArg(name="CDEFINES", value=ChainerCDefineType([
                            CDefineType(name="DEFAULT_BUFFER_SIZE", value="16384U"),
                            CDefineType(name="CLIENT_MODE", value="0"),
                        ])
                        )
                    ],
                ),
            server_runner=EPOLL_SERVER_RUNNER,
            client_runner=CLIENT_RUNNER,
        ),
        no_clients=1000,
    ),

    # 65536B buffer size
    BencherDataModel(
        name="epoll_1c_65536B_0rr",
        program_info=ProgramDataModel(
            builder=MakeBuilder(
                    r_path_server=EPOLL_SERVER_PATH,
                    r_path_client=CLIENT_PATH,
                    builder_args=[
                        MakeVarArg(name="CDEFINES", value=ChainerCDefineType([
                            CDefineType(name="DEFAULT_BUFFER_SIZE", value="65536U"),
                            CDefineType(name="CLIENT_MODE", value="0"),
                        ])
                        )
                    ],
                ),
            server_runner=EPOLL_SERVER_RUNNER,
            client_runner=CLIENT_RUNNER,
        ),
        no_clients=1,
    ),
    BencherDataModel(
        name="epoll_10c_65536B_0rr",
        program_info=ProgramDataModel(
            builder=MakeBuilder(
                    r_path_server=EPOLL_SERVER_PATH,
                    r_path_client=CLIENT_PATH,
                    builder_args=[
                        MakeVarArg(name="CDEFINES", value=ChainerCDefineType([
                            CDefineType(name="DEFAULT_BUFFER_SIZE", value="65536U"),
                            CDefineType(name="CLIENT_MODE", value="0"),
                        ])
                        )
                    ],
                ),
            server_runner=EPOLL_SERVER_RUNNER,
            client_runner=CLIENT_RUNNER,
        ),
        no_clients=10,
    ),
    BencherDataModel(
        name="epoll_100c_65536B_0rr",
        program_info=ProgramDataModel(
            builder=MakeBuilder(
                    r_path_server=EPOLL_SERVER_PATH,
                    r_path_client=CLIENT_PATH,
                    builder_args=[
                        MakeVarArg(name="CDEFINES", value=ChainerCDefineType([
                            CDefineType(name="DEFAULT_BUFFER_SIZE", value="65536U"),
                            CDefineType(name="CLIENT_MODE", value="0"),
                        ])
                        )
                    ],
                ),
            server_runner=EPOLL_SERVER_RUNNER,
            client_runner=CLIENT_RUNNER,
        ),
        no_clients=100,
    ),
    BencherDataModel(
        name="epoll_1000c_65536B_0rr",
        program_info=ProgramDataModel(
            builder=MakeBuilder(
                    r_path_server=EPOLL_SERVER_PATH,
                    r_path_client=CLIENT_PATH,
                    builder_args=[
                        MakeVarArg(name="CDEFINES", value=ChainerCDefineType([
                            CDefineType(name="DEFAULT_BUFFER_SIZE", value="65536U"),
                            CDefineType(name="CLIENT_MODE", value="0"),
                        ])
                        )
                    ],
                ),
            server_runner=EPOLL_SERVER_RUNNER,
            client_runner=CLIENT_RUNNER,
        ),
        no_clients=1000,
    ),
]


# epoll method,    1 clients,   512B buffer size, 1 fire-hose mode
# epoll method,   10 clients,   512B buffer size, 1 fire-hose mode
# epoll method,  100 clients,   512B buffer size, 1 fire-hose mode
# epoll method, 1000 clients,   512B buffer size, 1 fire-hose mode

# epoll method,    1 clients,  4096B buffer size, 1 fire-hose mode
# epoll method,   10 clients,  4096B buffer size, 1 fire-hose mode
# epoll method,  100 clients,  4096B buffer size, 1 fire-hose mode
# epoll method, 1000 clients,  4096B buffer size, 1 fire-hose mode

# epoll method,    1 clients, 16384B buffer size, 1 fire-hose mode
# epoll method,   10 clients, 16384B buffer size, 1 fire-hose mode
# epoll method,  100 clients, 16384B buffer size, 1 fire-hose mode
# epoll method, 1000 clients, 16384B buffer size, 1 fire-hose mode

# epoll method,    1 clients, 65536B buffer size, 1 fire-hose mode
# epoll method,   10 clients, 65536B buffer size, 1 fire-hose mode
# epoll method,  100 clients, 65536B buffer size, 1 fire-hose mode
# epoll method, 1000 clients, 65536B buffer size, 1 fire-hose mode
epoll_bench_1fh_mode_config: list[BencherDataModel] = [
    # 512B buffer size
    BencherDataModel(
        name="epoll_1c_512B_1fh",
        program_info=ProgramDataModel(
            builder=MakeBuilder(
                    r_path_server=EPOLL_SERVER_PATH,
                    r_path_client=CLIENT_PATH,
                    builder_args=[
                        MakeVarArg(name="CDEFINES", value=ChainerCDefineType([
                            CDefineType(name="DEFAULT_BUFFER_SIZE", value="512U"),
                            CDefineType(name="CLIENT_MODE", value="1"),
                        ])
                        )
                    ],
                ),
            server_runner=EPOLL_SERVER_RUNNER,
            client_runner=CLIENT_RUNNER,
        ),
        no_clients=1,
    ),
    BencherDataModel(
        name="epoll_10c_512B_1fh",
        program_info=ProgramDataModel(
            builder=MakeBuilder(
                    r_path_server=EPOLL_SERVER_PATH,
                    r_path_client=CLIENT_PATH,
                    builder_args=[
                        MakeVarArg(name="CDEFINES", value=ChainerCDefineType([
                            CDefineType(name="DEFAULT_BUFFER_SIZE", value="512U"),
                            CDefineType(name="CLIENT_MODE", value="1"),
                        ])
                        )
                    ],
                ),
            server_runner=EPOLL_SERVER_RUNNER,
            client_runner=CLIENT_RUNNER,
        ),
        no_clients=10,
    ),
    BencherDataModel(
        name="epoll_100c_512B_1fh",
        program_info=ProgramDataModel(
            builder=MakeBuilder(
                    r_path_server=EPOLL_SERVER_PATH,
                    r_path_client=CLIENT_PATH,
                    builder_args=[
                        MakeVarArg(name="CDEFINES", value=ChainerCDefineType([
                            CDefineType(name="DEFAULT_BUFFER_SIZE", value="512U"),
                            CDefineType(name="CLIENT_MODE", value="1"),
                        ])
                        )
                    ],
                ),
            server_runner=EPOLL_SERVER_RUNNER,
            client_runner=CLIENT_RUNNER,
        ),
        no_clients=100,
    ),
    BencherDataModel(
        name="epoll_1000c_512B_1fh",
        program_info=ProgramDataModel(
            builder=MakeBuilder(
                    r_path_server=EPOLL_SERVER_PATH,
                    r_path_client=CLIENT_PATH,
                    builder_args=[
                        MakeVarArg(name="CDEFINES", value=ChainerCDefineType([
                            CDefineType(name="DEFAULT_BUFFER_SIZE", value="512U"),
                            CDefineType(name="CLIENT_MODE", value="1"),
                        ])
                        )
                    ],
                ),
            server_runner=EPOLL_SERVER_RUNNER,
            client_runner=CLIENT_RUNNER,
        ),
        no_clients=1000,
    ),

    # 4096B buffer size
    BencherDataModel(
        name="epoll_1c_4096B_1fh",
        program_info=ProgramDataModel(
            builder=MakeBuilder(
                    r_path_server=EPOLL_SERVER_PATH,
                    r_path_client=CLIENT_PATH,
                    builder_args=[
                        MakeVarArg(name="CDEFINES", value=ChainerCDefineType([
                            CDefineType(name="DEFAULT_BUFFER_SIZE", value="4096U"),
                            CDefineType(name="CLIENT_MODE", value="1"),
                        ])
                        )
                    ],
                ),
            server_runner=EPOLL_SERVER_RUNNER,
            client_runner=CLIENT_RUNNER,
        ),
        no_clients=1,
    ),
    BencherDataModel(
        name="epoll_10c_4096B_1fh",
        program_info=ProgramDataModel(
            builder=MakeBuilder(
                    r_path_server=EPOLL_SERVER_PATH,
                    r_path_client=CLIENT_PATH,
                    builder_args=[
                        MakeVarArg(name="CDEFINES", value=ChainerCDefineType([
                            CDefineType(name="DEFAULT_BUFFER_SIZE", value="4096U"),
                            CDefineType(name="CLIENT_MODE", value="1"),
                        ])
                        )
                    ],
                ),
            server_runner=EPOLL_SERVER_RUNNER,
            client_runner=CLIENT_RUNNER,
        ),
        no_clients=10,
    ),
    BencherDataModel(
        name="epoll_100c_4096B_1fh",
        program_info=ProgramDataModel(
            builder=MakeBuilder(
                    r_path_server=EPOLL_SERVER_PATH,
                    r_path_client=CLIENT_PATH,
                    builder_args=[
                        MakeVarArg(name="CDEFINES", value=ChainerCDefineType([
                            CDefineType(name="DEFAULT_BUFFER_SIZE", value="4096U"),
                            CDefineType(name="CLIENT_MODE", value="1"),
                        ])
                        )
                    ],
                ),
            server_runner=EPOLL_SERVER_RUNNER,
            client_runner=CLIENT_RUNNER,
        ),
        no_clients=100,
    ),
    BencherDataModel(
        name="epoll_1000c_4096B_1fh",
        program_info=ProgramDataModel(
            builder=MakeBuilder(
                    r_path_server=EPOLL_SERVER_PATH,
                    r_path_client=CLIENT_PATH,
                    builder_args=[
                        MakeVarArg(name="CDEFINES", value=ChainerCDefineType([
                            CDefineType(name="DEFAULT_BUFFER_SIZE", value="4096U"),
                            CDefineType(name="CLIENT_MODE", value="1"),
                        ])
                        )
                    ],
                ),
            server_runner=EPOLL_SERVER_RUNNER,
            client_runner=CLIENT_RUNNER,
        ),
        no_clients=1000,
    ),

    # 16384B buffer size
    BencherDataModel(
        name="epoll_1c_16384B_1fh",
        program_info=ProgramDataModel(
            builder=MakeBuilder(
                    r_path_server=EPOLL_SERVER_PATH,
                    r_path_client=CLIENT_PATH,
                    builder_args=[
                        MakeVarArg(name="CDEFINES", value=ChainerCDefineType([
                            CDefineType(name="DEFAULT_BUFFER_SIZE", value="16384U"),
                            CDefineType(name="CLIENT_MODE", value="1"),
                        ])
                        )
                    ],
                ),
            server_runner=EPOLL_SERVER_RUNNER,
            client_runner=CLIENT_RUNNER,
        ),
        no_clients=1,
    ),
    BencherDataModel(
        name="epoll_10c_16384B_1fh",
        program_info=ProgramDataModel(
            builder=MakeBuilder(
                    r_path_server=EPOLL_SERVER_PATH,
                    r_path_client=CLIENT_PATH,
                    builder_args=[
                        MakeVarArg(name="CDEFINES", value=ChainerCDefineType([
                            CDefineType(name="DEFAULT_BUFFER_SIZE", value="16384U"),
                            CDefineType(name="CLIENT_MODE", value="1"),
                        ])
                        )
                    ],
                ),
            server_runner=EPOLL_SERVER_RUNNER,
            client_runner=CLIENT_RUNNER,
        ),
        no_clients=10,
    ),
    BencherDataModel(
        name="epoll_100c_16384B_1fh",
        program_info=ProgramDataModel(
            builder=MakeBuilder(
                    r_path_server=EPOLL_SERVER_PATH,
                    r_path_client=CLIENT_PATH,
                    builder_args=[
                        MakeVarArg(name="CDEFINES", value=ChainerCDefineType([
                            CDefineType(name="DEFAULT_BUFFER_SIZE", value="16384U"),
                            CDefineType(name="CLIENT_MODE", value="1"),
                        ])
                        )
                    ],
                ),
            server_runner=EPOLL_SERVER_RUNNER,
            client_runner=CLIENT_RUNNER,
        ),
        no_clients=100,
    ),
    BencherDataModel(
        name="epoll_1000c_16384B_1fh",
        program_info=ProgramDataModel(
            builder=MakeBuilder(
                    r_path_server=EPOLL_SERVER_PATH,
                    r_path_client=CLIENT_PATH,
                    builder_args=[
                        MakeVarArg(name="CDEFINES", value=ChainerCDefineType([
                            CDefineType(name="DEFAULT_BUFFER_SIZE", value="16384U"),
                            CDefineType(name="CLIENT_MODE", value="1"),
                        ])
                        )
                    ],
                ),
            server_runner=EPOLL_SERVER_RUNNER,
            client_runner=CLIENT_RUNNER,
        ),
        no_clients=1000,
    ),

    # 65536B buffer size
    BencherDataModel(
        name="epoll_1c_65536B_1fh",
        program_info=ProgramDataModel(
            builder=MakeBuilder(
                    r_path_server=EPOLL_SERVER_PATH,
                    r_path_client=CLIENT_PATH,
                    builder_args=[
                        MakeVarArg(name="CDEFINES", value=ChainerCDefineType([
                            CDefineType(name="DEFAULT_BUFFER_SIZE", value="65536U"),
                            CDefineType(name="CLIENT_MODE", value="1"),
                        ])
                        )
                    ],
                ),
            server_runner=EPOLL_SERVER_RUNNER,
            client_runner=CLIENT_RUNNER,
        ),
        no_clients=1,
    ),
    BencherDataModel(
        name="epoll_10c_65536B_1fh",
        program_info=ProgramDataModel(
            builder=MakeBuilder(
                    r_path_server=EPOLL_SERVER_PATH,
                    r_path_client=CLIENT_PATH,
                    builder_args=[
                        MakeVarArg(name="CDEFINES", value=ChainerCDefineType([
                            CDefineType(name="DEFAULT_BUFFER_SIZE", value="65536U"),
                            CDefineType(name="CLIENT_MODE", value="1"),
                        ])
                        )
                    ],
                ),
            server_runner=EPOLL_SERVER_RUNNER,
            client_runner=CLIENT_RUNNER,
        ),
        no_clients=10,
    ),
    BencherDataModel(
        name="epoll_100c_65536B_1fh",
        program_info=ProgramDataModel(
            builder=MakeBuilder(
                    r_path_server=EPOLL_SERVER_PATH,
                    r_path_client=CLIENT_PATH,
                    builder_args=[
                        MakeVarArg(name="CDEFINES", value=ChainerCDefineType([
                            CDefineType(name="DEFAULT_BUFFER_SIZE", value="65536U"),
                            CDefineType(name="CLIENT_MODE", value="1"),
                        ])
                        )
                    ],
                ),
            server_runner=EPOLL_SERVER_RUNNER,
            client_runner=CLIENT_RUNNER,
        ),
        no_clients=100,
    ),
    BencherDataModel(
        name="epoll_1000c_65536B_1fh",
        program_info=ProgramDataModel(
            builder=MakeBuilder(
                    r_path_server=EPOLL_SERVER_PATH,
                    r_path_client=CLIENT_PATH,
                    builder_args=[
                        MakeVarArg(name="CDEFINES", value=ChainerCDefineType([
                            CDefineType(name="DEFAULT_BUFFER_SIZE", value="65536U"),
                            CDefineType(name="CLIENT_MODE", value="1"),
                        ])
                        )
                    ],
                ),
            server_runner=EPOLL_SERVER_RUNNER,
            client_runner=CLIENT_RUNNER,
        ),
        no_clients=1000,
    ),
]