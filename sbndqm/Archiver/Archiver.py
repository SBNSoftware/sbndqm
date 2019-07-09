import argparse
import redis
import json
import time
from multiprocessing import Process

def main(args):

    start = time.time()
    #Load configuration information from the configuration file.
    with open('ArchiverConfig.json') as JSONFile:
        Config = json.load(JSONFile)

    #Connect to the Redis database.
    r = redis.Redis(host=Config['config']['hostname'], port=Config['config']['port'], password=args.password)
    
    #Now we split the job into multiple processes that each handle some portion of the streams.

    ProcessCount = int(args.processes)
    ProcessList = []
    for i in range(ProcessCount):
        KeyList = Config['streams'].keys()[i::ProcessCount]
        ProcessList.append(Process(target=ProcessStreams, args=(r,{ x : GetLatest(r, x) for x in KeyList},{x : Config['streams'][x] for x in KeyList})))
    for i in range(ProcessCount):
        ProcessList[i].start()
    for i in range(ProcessCount):
        ProcessList[i].join()

    #KeyList = Config['streams'].keys()
    #ProcessStreams(r,{ x : GetLatest(r, x) for x in KeyList},{x : Config['streams'][x] for x in KeyList})
    
    end = time.time()
    print(end-start)

    #This next bit deletes the record of streams that have been completed. This is for testing purposes only.
    for key in r.scan_iter("*_LatestCompleted"):
        r.delete(key)

def ProcessStreams(r, StreamDict, Config):
    MetricDict = { x : [] for x in StreamDict.keys()}

    #We now begin to continuously query these streams for new metrics, archiving them when possible.
    ControlVar = True
    while ControlVar:
        #ReadStream is a list of stream object which have new entries that have yet to be archived.
        #BeforeStream = time.time()
        ReadStream = r.xread(StreamDict, block=0)
        #AfterStream = time.time()
        #print('XREAD time',AfterStream-BeforeStream)
        #Loop over the streams which have entries to be archived.
        for StreamObject in ReadStream:
            #Loop over the individual entries in the stream.
            for DataObject in StreamObject[1]:
                #Check if the latest completed archived entry is '0' (no metrics have been archived for the stream).
                if StreamDict[StreamObject[0]] == '0':
                    NewLatest = str( int( DataObject[0].split('-')[0] ) - 1 ) + '-0'
                    SetLatest(r, StreamObject[0], NewLatest)
                    ProcessData(MetricDict, DataObject, Config, StreamObject[0])
                    StreamDict[StreamObject[0]] = DataObject[0]
                #Check if the current entry is within the time block (contained in the interval of entries to be archived).
                elif int( DataObject[0].split('-')[0] ) < int( GetLatest(r, StreamObject[0]).split('-')[0] ) + Config[StreamObject[0]]['time']:
                    ProcessData(MetricDict, DataObject, Config, StreamObject[0])
                    StreamDict[StreamObject[0]] = DataObject[0]
                #The entry is outside the current time block. Set a new latest completed and write to the database.
            else:
                    if Config[StreamObject[0]]['type'] == 'mean':
                        SetLatest(r, StreamObject[0], MetricDict[StreamObject[0]][0])
                        WritePostgres(StreamObject[0], MetricDict[StreamObject[0]][1])
                        MetricDict[StreamObject[0]] = []
                    elif Config[StreamObject[0]]['type'] == 'mode':
                        SetLatest(r, StreamObject[0], MetricDict[StreamObject[0]][0])
                        WritePostgres(StreamObject[0], MetricDict[StreamObject[0]][3])
                        MetricDict[StreamObject[0]] = []
                    elif Config[StreamObject[0]]['type'] == 'median':
                        SetLatest(r, StreamObject[0], MetricDict[StreamObject[0]][-1][0])
                        MetricList = [ x[1] for x in MetricDict[StreamObject[0]] ]
                        WritePostgres(StreamObject[0], median(MetricList))
                        MetricDict[StreamObject[0]] = []
                    elif Config[StreamObject[0]]['type'] == 'max':
                        SetLatest(r, StreamObject[0], MetricDict[StreamObject[0]][0])
                        WritePostgres(StreamObject[0], MetricDict[StreamObject[0]][1])
                        MetricDict[StreamObject[0]] = []
                    ProcessData(MetricDict, DataObject, Config, StreamObject[0])
                    StreamDict[StreamObject[0]] = DataObject[0]
                    #Check to see if there is a gap in the data stream (the current object is more than one time interval from the latest completed).
                    if int( DataObject[0].split('-')[0] ) > int( GetLatest(r, StreamObject[0]).split('-')[0] ) + Config[StreamObject[0]]['time']:
                        NewLatest = str( int( DataObject[0].split('-')[0] ) - 1 ) + '-0'
                        SetLatest(r, StreamObject[0], NewLatest)
        ControlVar = False #Dummy loop control for testing purposes.

def ProcessData(Metrics, Datum, Config, StreamName):
    if Config[StreamName]['type'] == 'mean':
        if len(Metrics[StreamName]) == 0: 
            Metrics[StreamName] = [Datum[0], float(Datum[1]['dat']), 1]
        else:
            Metrics[StreamName][0] = Datum[0]
            Metrics[StreamName][1] = (Metrics[StreamName][1] * Metrics[StreamName][2] + float(Datum[1]['dat'])) / (Metrics[StreamName][2] + 1)
            Metrics[StreamName][2] += 1
    elif Config[StreamName]['type'] == 'mode':
        if len(Metrics[StreamName]) == 0:
            Metrics[StreamName] = [Datum[0], {Datum[1]['dat'] : 1}, 1, float(Datum[1]['dat'])]
        else:
            Metrics[StreamName][0] = Datum[0]
            if Datum[1]['dat'] not in Metrics[StreamName][1]:
                Metrics[StreamName][1][Datum[1]['dat']] = 1
            else:
                Metrics[StreamName][1][Datum[1]['dat']] += 1
                if Metrics[StreamName][1][Datum[1]['dat']] > Metrics[StreamName][2]:
                    Metrics[StreamName][2] = Metrics[StreamName][1][Datum[1]['dat']]
                    Metrics[StreamName][3] = float(Datum[1]['dat'])
    elif Config[StreamName]['type'] == 'median': #Keeps all data for now.
        Metrics[StreamName].append( (Datum[0], float(Datum[1]['dat'])) )
    elif Config[StreamName]['type'] == 'max':
        if len(Metrics[StreamName]) == 0:
            Metrics[StreamName] = [Datum[0], float(Datum[1]['dat'])]
        else:
            Metrics[StreamName][0] = Datum[0]
            if Metrics[StreamName][1] < float(Datum[1]['dat']): Metrics[StreamName][1] = float(Datum[1]['dat'])

        
def GetLatest(Database, StreamName):
    Latest = Database.get(StreamName + '_LatestCompleted')
    if Latest is not None: return Latest
    else:
        Database.set(StreamName + '_LatestCompleted','0')
        return '0'

def SetLatest(Database, StreamName, Latest):
    response = Database.set(StreamName + '_LatestCompleted', Latest)

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
    #print(StringToPrint)
    #Do stuff with it...

if __name__ == "__main__":
    args = argparse.ArgumentParser()
    args.add_argument("-pw", "--password", default=None)
    args.add_argument("-pr", "--processes", default=1)
    main(args.parse_args())
