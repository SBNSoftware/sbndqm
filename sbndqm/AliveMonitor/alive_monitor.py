import logging
import argparse
import redis
from subprocess import Popen
import time

def main(args):
    redis = connect_to_redis_args(args)
    process = Popen(args.command, shell=True) 
    while process.poll() is None:
        redis.xadd(args.key, {"dat": "alive"}, maxlen=1)
        time.sleep(args.sleep)

def connect_to_redis_args(args):
    return connect_to_redis(args.server, args.port, args.password, args.passfile)

def connect_to_redis(hostname, port, password=None, passfile=None):
    if passfile is not None:
        with open(passfile) as f:
            password = f.read().rstrip("\n")
    return redis.Redis(hostname, port, password=password)

if __name__ == "__main__":
    argparser = argparse.ArgumentParser()
    argparser.add_argument("-p", "--port", type=int, default=6379)
    argparser.add_argument("-s", "--server", default="localhost")
    argparser.add_argument("-pw", "--password", default=None)
    argparser.add_argument("-pf", "--passfile", default=None)
    argparser.add_argument("-S", "--sleep", type=float, default=5)
    argparser.add_argument("-k", "--key", required=True)

    argparser.add_argument("-c", "--command", required=True)

    args = argparser.parse_args()
    main(args)
