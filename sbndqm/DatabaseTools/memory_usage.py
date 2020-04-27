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

    keys = redis.keys()
    pipeline = redis.pipeline()
    for k in keys:
      pipeline.memory_usage(k)

    usage = pipeline.execute()
    usage, keys = zip(*sorted(zip(usage, keys), reverse=True))

    for k, b in zip(keys, usage):
        if b > 1024 * 1024:
            print "%s %.1fMB" % (k, b / 1024. / 1024)
        elif b > 1024:
            print "%s %.1fKB" % (k, b / 1024.)
        else:
            print "%s %.1fB" % (k, float(b))

if __name__ == "__main__":
    args = argparse.ArgumentParser()
    args = util.add_connection_args(args)
    main(args.parse_args())
