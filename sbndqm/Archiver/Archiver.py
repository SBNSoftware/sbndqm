import argparse
import redis
import json
import time

def main(args):

    start = time.time()
    #Load configuration information from the configuration file.
    with open('ArchiverConfig.json') as JSONFile:
        Config = json.load(JSONFile)

    #Connect to the Redis database.
    r = redis.Redis(host=Config['config']['hostname'], port=Config['config']['port'], password=args.password)
    
    #Create two dictionary items. StreamDict is used in queries to specify both the stream name and the how
    #far backwards in time to look for entries. MetricDict is meant to contain the metrics which have been read
    #from the database, but not averaged (not enough have been read to do the archiving).
    StreamDict = { x : GetLatest(r, x) for x in Config['streams'].keys()}
    MetricDict = { x : [] for x in Config['streams'].keys()}

    #We now begin to continuously query these streams for new metrics, archiving them when possible.
    ControlVar = True
    while ControlVar:
        #ReadStream is a list of stream object which have new entries that have yet to be archived.
        ReadStream = r.xread(StreamDict, block=0)
        #Loop over the streams which have entries to be archived.
        for StreamObject in ReadStream:
            #Loop over the individual entries in the stream.
            for DataObject in StreamObject[1]:
                #Check if the latest completed archived entry is '0' (no metrics have been archived for the stream).
                if StreamDict[StreamObject[0]] == '0':
                    NewLatest = str( int( DataObject[0].split('-')[0] ) - 1 ) + '-0'
                    SetLatest(r, StreamObject[0], NewLatest)
                    MetricDict[StreamObject[0]].append((DataObject[0], float(DataObject[1]['dat'])))
                    StreamDict[StreamObject[0]] = DataObject[0]
                #Check if the current entry is within the time block (contained in the interval of entries to be archived).
                elif int( DataObject[0].split('-')[0] ) < int( GetLatest(r, StreamObject[0]).split('-')[0] ) + Config['streams'][StreamObject[0]]['time']:
                    MetricDict[StreamObject[0]].append((DataObject[0], float(DataObject[1]['dat'])))                            
                    StreamDict[StreamObject[0]] = DataObject[0]
                #The entry is outside the current time block. Do the appropriate averaging of the accumulated entries.
                else:
                    SetLatest(r, StreamObject[0], MetricDict[StreamObject[0]][-1][0])
                    MetricList = [ x[1] for x in MetricDict[StreamObject[0]] ]
                    if Config['streams'][StreamObject[0]]['type'] == 'mean':
                        WritePostgres(StreamObject[0], mean(MetricList))
                        MetricDict[StreamObject[0]] = []
                    elif Config['streams'][StreamObject[0]]['type'] == 'mode':
                        WritePostgres(StreamObject[0], mode(MetricList))
                        MetricDict[StreamObject[0]] = []
                    elif Config['streams'][StreamObject[0]]['type'] == 'median':
                        WritePostgres(StreamObject[0], median(MetricList))
                        MetricDict[StreamObject[0]] = []
                    elif Config['streams'][StreamObject[0]]['type'] == 'max':
                        WritePostgres(StreamObject[0], max(MetricList))
                        MetricDict[StreamObject[0]] = []
                    MetricDict[StreamObject[0]].append((DataObject[0], float(DataObject[1]['dat'])))
                    StreamDict[StreamObject[0]] = DataObject[0]
                    #Check to see if there is a gap in the data stream (the current object is more than one time interval from the latest completed).
                    if int( DataObject[0].split('-')[0] ) > int( GetLatest(r, StreamObject[0]).split('-')[0] ) + Config['streams'][StreamObject[0]]['time']:
                        NewLatest = str( int( DataObject[0].split('-')[0] ) - 1 ) + '-0'
                        SetLatest(r, StreamObject[0], NewLatest)
        ControlVar = False #Dummy loop control for testing purposes.
    end = time.time()
    print(end-start)
    for key in r.scan_iter("*_LatestCompleted"):
        r.delete(key)

def GetLatest(Database, StreamName):
    Latest = Database.get(StreamName + '_LatestCompleted')
    if Latest is not None: return Latest
    else:
        Database.set(StreamName + '_LatestCompleted','0')
        return '0'

def SetLatest(Database, StreamName, Latest):
    response = Database.set(StreamName + '_LatestCompleted', Latest)
    
def mean(l):
    return sum(l)/len(l)

def mode(l):
    ModeDictionary = {}
    CurrentMaxCount = 0
    CurrentMaxValue = 0
    for Datum in l:
        if Datum not in ModeDictionary:
            ModeDictionary[Datum] = 1
            if ModeDictionary[Datum] > CurrentMaxCount:
                CurrentMaxCount = ModeDictionary[Datum]
                CurrentMaxValue = Datum
        else:
            ModeDictionary[Datum] += 1
            if ModeDictionary[Datum] > CurrentMaxCount:
                CurrentMaxCount = ModeDictionary[Datum]
                CurrentMaxValue = Datum
    return CurrentMaxValue

def median(l):
    l = sorted(l)
    Length = len(l)
    if Length < 1:
        return None
    if Length % 2 == 0:
        return ( l[(Length-1)/2] + l[(Length+1)/2] ) / 2.0
    else:
        return l[(Length-1)/2]

def WritePostgres(MetricName, MetricValue):
    StringToPrint = MetricName + ': ' + str(MetricValue)
    print(StringToPrint)
    #Do stuff with it...

if __name__ == "__main__":
    args = argparse.ArgumentParser()
    args.add_argument("-pw", "--password", default=None)
    main(args.parse_args())
