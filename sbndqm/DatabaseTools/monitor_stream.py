import argparse
import util
from datetime import datetime
import signal
import sys

def handle_user_stop(signum, frame):
    print("User Ctrl-C, shutting down...")
    sys.exit(0)

def value_display(datum):
    timestamp = int(datum[0].decode("utf-8").split("-")[0]) / 1000 # ms -> s
    timestr = datetime.fromtimestamp(timestamp).strftime("%Y-%m-%d %H:%M:%S")
    value = util.read_datum(datum[1])
    return "%s at time %s" % (value, timestr)

def main(args):
    redis = util.connect_to_redis_args(args)
    try:
        redis.ping()
    except:
        print("Error: trouble connecting to redis database on server (%s) port (%i)") % (args.server, args.port)
        return

    # build the list of keys
    # case 1 -- use supplied a key -- use it
    if args.key is not None:
        keys = [args.key]
    # case 2 -- build key from user supplied arguments
    else:
        keys = redis.keys("%s:%s:%s:%s" % (args.group, args.instance, args.metric, args.stream))
        if len(keys) == 0:
            keys = ["%s:%s:%s:%s" % (args.group, args.instance, args.metric, args.stream)]
        keys = [k.decode("utf-8") for k in keys]

    # read the initial value for each key
    last_seen_ids = {}
    for key in keys:
        # ignore archiver metadata
        if key.endswith("_LatestCompleted"): continue

        try:
            lastval = redis.xrevrange(key, count=1)
        except:
            print("Error: key (%s) has mis-formed value" % key)
            return
        if len(lastval) == 0:
            print("Key (%s) has no prior values" % key)
            last_seen_ids[key] = "0-0"
        else:
            print( ("Key (%s) last value: " % key) + value_display(lastval[0]) )
            last_seen_ids[key] = lastval[0][0]

    print("Listening for new values")
    # setup to exit nice on Ctrl-C
    signal.signal(signal.SIGINT, handle_user_stop)
    # now listen for new values
    while True:
        value = redis.xread(last_seen_ids)
        for v in value:
            key = v[0]
            data = v[1]
            for d in data:
                print( ("Key (%s) updated: " % key) + value_display(d))
            last_seen_ids[key] = data[-1][0]

if __name__ == "__main__":
    args = argparse.ArgumentParser()
    args = util.add_connection_args(args)
    args.add_argument("-k", "--key", default=None)
    args.add_argument("-g", "--group", default="*")
    args.add_argument("-i", "--instance", default="*")
    args.add_argument("-m", "--metric", default="*")
    args.add_argument("-st", "--stream", default="*")
    main(args.parse_args())
