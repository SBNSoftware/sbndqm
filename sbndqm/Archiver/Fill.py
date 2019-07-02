import argparse
import redis
import json
import random
import time

def main(args):
    with open('ArchiverConfig.json') as JSONFile:
        Config = json.load(JSONFile)
    r = redis.Redis(host=Config['config']['hostname'], port=Config['config']['port'], password=args.password)
    for key in r.scan_iter("test*"):
        r.delete(key)
    Count = 0
    while Count < 60:
        pipeline = r.pipeline()
        for stream in Config['streams'].keys():
            pipeline.xadd(stream, {"dat": random.random()})
        [_ for _ in pipeline.execute()]
        Count += 1
        time.sleep(Config['config']['timeout'])

if __name__ == "__main__":
    args = argparse.ArgumentParser()
    args.add_argument("-pw", "--password", default=None)
    main(args.parse_args())

