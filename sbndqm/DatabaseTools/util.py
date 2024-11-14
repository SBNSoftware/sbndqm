import redis
import struct

def connect_to_redis_args(args):
    return connect_to_redis(args.server, args.port, args.password, args.passfile)

def connect_to_redis(hostname, port, password=None, passfile=None):
    if passfile is not None:
        with open(passfile) as f:
            password = f.read().rstrip("\n")
    return redis.Redis(hostname, port, password=password)


def add_connection_args(argparser):
    argparser.add_argument("-p", "--port", type=int, default=6379)
    argparser.add_argument("-s", "--server", default="localhost")
    argparser.add_argument("-pw", "--password", default=None)
    argparser.add_argument("-pf", "--passfile", default=None)
    return argparser

def type_to_struct_type(name):
    if name == b'int8_t': return "b"
    if name == b'int16_t': return "h"
    if name == b'int32_t': return "i"
    if name == b'int64_t': return "q"

    if name == b'uint8_t': return "B"
    if name == b'uint16_t': return "H"
    if name == b'uint32_t': return "I"
    if name == b'uint64_t': return "Q"

    if name == b'float': return "f"
    if name == b'double': return "d"

def type_to_size(name):
    if name == b'int8_t': return 1
    if name == b'int16_t': return 2
    if name == b'int32_t': return 4
    if name == b'int64_t': return 8

    if name == b'uint8_t': return 1
    if name == b'uint16_t': return 2
    if name == b'uint32_t': return 4
    if name == b'uint64_t': return 8

    if name == b'float': return 4
    if name == b'double': return 8

def parse_binary(binary, typename):
    size = type_to_size(typename)
    form = type_to_struct_type(typename)
    ret = []
    for i in range(len(binary) // size):
       dat = binary[i*size : (i+1)*size]
       ret.append(struct.unpack(form, dat)[0]) 

    return ret


def read_datum(dat):
    for key, val in dat.items():
        key = key.decode("utf-8")
        if key == "dat": return val
        return parse_binary(val, key)[0]
