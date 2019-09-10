import argparse
import util
import json
import ROOT
from array import array


def main(args):
    redis = util.connect_to_redis_args(args)
    try:
        redis.ping()
    except:
        print "Error: trouble connecting to redis database on server (%s) port (%i)" % (args.server, args.port)
        return
    try:
        waveform = redis.lrange(args.key)
    except:
        print "ERROR: key (%s) is misformed" % (args.key)
        return 
    if len(waveform) == 0:
        print "ERROR: Waveform is empty -- the specified key is not setup in redis."
    else:
        print "Non empty waveform found."
    xs = array('d', range(len(waveform)))
    try:
        ys = array('d', waveform)
    except:
        print "ERROR: Bad waveform value in list"
        return
    graph(xs.size(), xs, ys)
    graph.Draw("ALP")

if __name__ == "__main__":
    print "NOTE: you must have graphics forwarding enabled to use this script."
    args = argparse.ArgumentParser()
    args = util.add_connection_args(args)
    args.add_argument("-k", "--key", required=True)
    main(args.parse_args())
