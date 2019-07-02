#import redis
import json
import argparse

def main(args):
    
    #r = redis.Redis(host='localhost', port=6379)
    
    Config = {}
    Config['config'] = { "timeout":1, "hostname":"localhost", "port":6379 }
    StreamDict = {}
    for i in range(int(args.quantity)):
        StreamName = 'test' + str(i)
        StreamDict[StreamName] = {"type":"mean", "time":59000}
    Config['streams']=StreamDict
    with open('ArchiverConfig.json', 'w') as OutputConfig:
        json.dump(Config, OutputConfig, indent=4, sort_keys=True)


if __name__ == "__main__":
    args = argparse.ArgumentParser()
    args.add_argument("-q", "--quantity", default=10)
    main(args.parse_args())
