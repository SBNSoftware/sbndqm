import redis

def connect_to_redis_args(args):
    return connect_to_redis(args.server, args.port, args.password, args.passfile)

def connect_to_redis(hostname, port, password=None, passfile=None):
    if passfile is not None:
        with open(passfile) as f:
            password = f.read()
    return redis.Redis(hostname, port, password=password)


def add_connection_args(argparser):
    argparser.add_argument("-p", "--port", type=int, default=6379)
    argparser.add_argument("-s", "--server", default="localhost")
    argparser.add_argument("-pw", "--password", default=None)
    argparser.add_argument("-pf", "--passfile", default=None)
    return argparser

