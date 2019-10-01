import argparse
import util
import json
import matplotlib.pyplot as plt
from array import array
import struct

# print struct.unpack("i", data)

def type_to_struct_type(name):
    if name == "int8_t": return "b"
    if name == "int16_t": return "h"
    if name == "int32_t": return "i"
    if name == "int64_t": return "q"

    if name == "uint8_t": return "B"
    if name == "uint16_t": return "H"
    if name == "uint32_t": return "I"
    if name == "uint64_t": return "Q"

    if name == "float": return "f"
    if name == "double": return "d"

def type_to_size(name):
    if name == "int8_t": return 1
    if name == "int16_t": return 2
    if name == "int32_t": return 4
    if name == "int64_t": return 8

    if name == "uint8_t": return 1
    if name == "uint16_t": return 2
    if name == "uint32_t": return 4
    if name == "uint64_t": return 8

    if name == "float": return 4
    if name == "double": return 8


def parse_binary(binary, typename):
    size = type_to_size(typename)
    form = type_to_struct_type(typename)
    ret = []
    for i in range(len(binary) / size):
       dat = binary[i*size : (i+1)*size]
       ret.append(struct.unpack(form, dat)[0]) 

    return ret

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

    data = parse_binary(data, data_type)
    if sizes:
        sizes = parse_binary(sizes, size_type)
    else:
       sizes = [len(data)]
    if offsets:
        offsets = parse_binary(offsets, offset_type)
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
