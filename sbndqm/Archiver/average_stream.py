import argparse
import redis
import json
#import numpy as np

def main(args):
    with open('ArchiverConfig.json') as json_file:
        Config = json.load(json_file)
    r = redis.Redis(host=Config['config']['hostname'], port=Config['config']['port'], password=args.password)
    StreamDict = { x : Config['streams'][x]['latest'] for x in Config['streams'].keys()}
    MetricDict = { x : [Config['streams'][x]['latest'], [] ] for x in Config['streams'].keys()}
    print(StreamDict)
    while True:
        ReadStream = r.xread(StreamDict)
        if len(ReadStream) !=0:
            for StreamObject in ReadStream:
                for DataObject in StreamObject[1]:
                    if len(MetricDict[StreamObject[0]][1]) < Config['streams'][StreamObject[0]]:
                        MetricDict[StreamObject[0]][1].append(float(DataObject[1]['dat']))
                        MetricDict[StreamObject[0]][0] = DataObject[0]
                    else:
                        Config['streams'][StreamObject[0]]['latest'] = MetricDict[StreamObject[0]][0]
                        StreamDict[StreamObject[0]] = MetricDict[StreamObject[0]][0]
                        with open('ArchiverConfig.json', 'w') as OutputConfig:
                            json.dump(Config, OutputConfig, indent=4, sort_keys=True)
                        if Config['streams'][StreamObject[0]]['type'] == 'mean':
                            print(str(StreamObject[0]) + "Mean: " + str(mean(MetricDict[StreamObject[0]][1])))
                            MetricDict[StreamObject[0]][1] = []
                        #elif Config['streams'][StreamObject[0]]['type'] == 'mode':
                        #    print("Mode: " + str(np.mode(MetricDict[StreamObject[0]][1])))
                        #    MetricDict[StreamObject[0]][1] = []
                        #elif Config['streams'][StreamObject[0]]['type'] == 'median':
                        #    print("Median: " + str(np.median(MetricDict[StreamObject[0]][1])))
                        #    MetricDict[StreamObject[0]][1] = []
                        MetricDict[StreamObject[0]].append(float(DataObject[1]['data']))
                        MetricDict[StreamObject[0]][0] = DataObject[0]

def mean(l):
    return sum(l)/len(l)

if __name__ == "__main__":
    args = argparse.ArgumentParser()
    args.add_argument("-pw", "--password", default=None)
    main(args.parse_args())
