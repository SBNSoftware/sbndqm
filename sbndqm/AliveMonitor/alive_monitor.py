import logging
import argparse
import redis
from redis import RedisError
from subprocess import Popen
import time

def main(args):
    redis = connect_to_redis_args(args)
    process = Popen(args.command, shell=True) 
    last_connection_was_error = False
    while process.poll() is None:
        # attempt to run the query
        try:
            redis.xadd(args.key, {"dat": "alive"}, maxlen=1)
        except RedisError as e: # catches all errors related to executing redis
            # Redis is sometimes busy executing a script -- don't print these
            # errors to the screen to avoid polluting the terminal
            is_busy_error = 'busy redis is busy' in ''.join(e.args).lower()
            if not is_busy_error:
                print("ERROR: Unable to to communicate to redis. Error text:")
                print(e)
                last_connection_was_error = True
        else: # runs if xadd worked
            if last_connection_was_error:
                 print("Able to communicate with Redis! All copacetic now.")
            last_connection_was_error = False

        time.sleep(args.sleep)
    return process.returncode

def connect_to_redis_args(args):
    return connect_to_redis(args.server, args.port, args.password, args.passfile)

def connect_to_redis(hostname, port, password=None, passfile=None):
    if passfile is not None:
        with open(passfile) as f:
            password = f.read().rstrip("\n")
    return redis.Redis(hostname, port, password=password)

if __name__ == "__main__":
    argparser = argparse.ArgumentParser()
    argparser.add_argument("-p", "--port", type=int, default=6379, help="Port of redis server to connect to. Default is 6379.", metavar="<port number>")
    argparser.add_argument("-s", "--server", default="localhost",  help="Host name of redis server to connect to. Default is localhost", metavar="<hostname>")
    argparser.add_argument("-pw", "--password", default=None, help="Password used to connect to redis server. Default is no password", metavar="<password>")
    argparser.add_argument("-pf", "--passfile", default=None, help="Location of file containing password to connect to redis server. Default is no password", metavar="</path/to/password/file>")
    argparser.add_argument("-S", "--sleep", type=float, default=5, help="Time to sleep in between checking process and sending message to redis in seconds. Default is 5s.", metavar="<sleep time (s)>")
    argparser.add_argument("-k", "--key", required=True, help="REQUIRED. Key name to store in redis.", metavar="<keyname>")

    argparser.add_argument("-c", "--command", required=True, help="REQUIRED. Shell command to run/monitor. Use quotes for any spaces", metavar='<"shell command">')

    args = argparser.parse_args()
    #res = main(args)
    while main(args)==0:
        time.sleep(5.0)

