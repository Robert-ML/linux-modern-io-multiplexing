from datamodel_program import ProgramRunner


CLIENT_ELF_NAME = "client"
CLIENT_PATH = "../code/c/client/"
CLIENT_RUNNER = ProgramRunner(
    executable_name=CLIENT_ELF_NAME,
    path=CLIENT_PATH,
    args=[]
)

EPOLL_ELF_NAME = "epoll_server"
EPOLL_SERVER_PATH = "../code/c/server/epoll/"
EPOLL_SERVER_RUNNER = ProgramRunner(
    executable_name=EPOLL_ELF_NAME,
    path=EPOLL_SERVER_PATH,
    args=[]
)
