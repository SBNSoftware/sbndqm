import argparse
import util
import json
import matplotlib.pyplot as plt
from array import array

# print struct.unpack("i", data)

def main(args):
    redis = util.connect_to_redis_args(args)
    key = args.key
    try:
        redis.ping()
    except:
        print "Error: trouble connecting to redis database on server (%s) port (%i)" % (args.server, args.port)
        return
    try:
        data_type = redis.hget(key, "DataType")
        offset_type = redis.hget(key, "OffsetType")
        size_type = redis.hget(key, "SizeType")
        period = float(redis.hget(key, "TickPeriod"))
        data = redis.hget(key, "Data")
        sizes = redis.hget(key, "Sizes")
        offsets = redis.hget(key, "Offsets")
    except:
         print "ERROR: key (%s) is misformed" % (args.key)
         return

    data = util.parse_binary(data, data_type)
    if sizes:
        sizes = util.parse_binary(sizes, size_type)
    else:
       sizes = [len(data)]
    if offsets:
        offsets = util.parse_binary(offsets, offset_type)
    else:
        offsets = [0]

    xs = []
    total_offset = 0
    for size, offset in zip(sizes, offsets):
        total_offset += offset
        xs += [i * period + total_offset for i in range(size)]
        total_offset += size * period

    plt.plot(xs, data)
    plt.title(args.key)
    plt.show()

if __name__ == "__main__":
    print "NOTE: you must have graphics forwarding enabled to use this script."
    args = argparse.ArgumentParser()
    args = util.add_connection_args(args)
    args.add_argument("-k", "--key", required=True)
    main(args.parse_args())
