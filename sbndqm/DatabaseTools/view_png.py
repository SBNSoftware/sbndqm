import argparse
import util
from PIL import Image
import io

def wait():
    raw_input("Press Enter to continue...")

def main(args):
    redis = util.connect_to_redis_args(args)
    try:
        redis.ping()
    except:
        print "Error: trouble connecting to redis database on server (%s) port (%i)" % (args.server, args.port)
        return
    try:
        binary = redis.get(args.key)
    except:
        print "ERROR: key (%s) is misformed" % (args.key)
        return 

    image = Image.open(io.BytesIO(binary))
    image.show()
    wait()

if __name__ == "__main__":
    print "NOTE: you must have graphics forwarding enabled to use this script."
    args = argparse.ArgumentParser()
    args = util.add_connection_args(args)
    args.add_argument("-k", "--key", required=True)
    main(args.parse_args())
