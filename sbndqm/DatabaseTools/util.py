import redis
import struct

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


def read_datum(dat):
    for key, val in dat.items():
        if key == "val": return val
        return parse_binary(val, key)[0]
