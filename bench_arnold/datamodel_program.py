from __future__ import annotations

import asyncio
import pathlib
from dataclasses import dataclass
from enum import Enum
from abc import abstractmethod
from typing import override, Any

from subprocess_kit import run_command


@dataclass
class BuilderArg:
    @abstractmethod
    def __str__(self) -> str:
        raise NotImplementedError("Subclasses must implement this method.")

@dataclass
class CDefineType(BuilderArg):
    name: str
    value: str | None

    @override
    def __str__(self) -> str:
        if self.value is None:
            return f"-D{self.name}"
        return f"-D{self.name}={self.value}"

    @staticmethod
    def chain(*args: CDefineType) -> str:
        return ' '.join(str(arg) for arg in args)

@dataclass
class ChainerCDefineType(BuilderArg):
    defines: list[CDefineType] = []

    @override
    def __str__(self) -> str:
        return ' '.join(str(define) for define in self.defines)

    def add(self, define: CDefineType) -> None:
        self.defines.append(define)

@dataclass
class MakeVarArg(BuilderArg):
    name: str
    value: BuilderArg

    @override
    def __str__(self) -> str:
        return f"{self.name}=\"{self.value}\""


@dataclass
class ProgramRunner:
    executable_name: str
    path: str
    args: list[str]

    def get_run_command(self) -> str:
        args_str = ' '.join(self.args)
        return f"cd {self.path} && ./{self.executable_name} {args_str}"

    def get_executable_path(self) -> str:
        path = pathlib.Path(self.path) / self.executable_name
        return str(path.resolve())

@dataclass
class ProgramBuilder:
    async def build(self) -> None:
        print("No build needed for this program.")

    async def clean(self) -> None:
        print("No clean needed for this program.")

@dataclass
class MakeBuilder(ProgramBuilder):
    r_path_server: str
    r_path_client: str
    builder_args: list[BuilderArg]

    @override
    async def build(self) -> None:
        args_str = ' '.join(str(arg) for arg in self.builder_args)
        server_command = f"make -C {self.r_path_server} {args_str}"
        client_command = f"make -C {self.r_path_client} {args_str}"

        print(f"Running build command for server: {server_command}")
        print(f"Running build command for client: {client_command}")
        await asyncio.gather(run_command(server_command), run_command(client_command))
        print("Build completed.", end="\n\n")

    @override
    async def clean(self) -> None:
        server_command = f"make -C {self.r_path_server} clean"
        client_command = f"make -C {self.r_path_client} clean"

        print(f"Running clean command for server: {server_command}")
        print(f"Running clean command for client: {client_command}")
        await asyncio.gather(run_command(server_command), run_command(client_command))
        print("Clean completed.", end="\n\n")

class NoneBuilder(ProgramBuilder):
    def __init__(self) -> None:
        super().__init__()


class ProgramDataModel:
    def __init__(self, builder: ProgramBuilder, server_runner: ProgramRunner, client_runner: ProgramRunner):
        self.builder = builder
        self.server_runner = server_runner
        self.client_runner = client_runner

    async def build(self) -> None:
        await self.builder.build()

    async def clean(self) -> None:
        await self.builder.clean()
