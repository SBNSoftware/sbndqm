import argparse
import util
import json


def main(args):
    redis = util.connect_to_redis_args(args)
    try:
        redis.ping()
    except:
        print "Error: trouble connecting to redis database on server (%s) port (%i)" % (args.server, args.port)
        return
    try:
        data = redis.hgetall(args.key)
    except:
        print "Error: unable to get key (%s)" % args.key
        return 

    for k, v in data.items():
        if k == "Data":
            print k
        else:
            print "%s: %s" % (k, v)

if __name__ == "__main__":
    args = argparse.ArgumentParser()
    args = util.add_connection_args(args)
    args.add_argument("-k", "--key", required=True)
    main(args.parse_args())
