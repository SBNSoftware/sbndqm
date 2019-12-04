import argparse
import redis
import json
import time
import signal
import sys
import psutil
import struct
import psycopg2
import logging
from multiprocessing import Process
import math

def main(args):

    signal.signal(signal.SIGINT, signal_handler)

    #Load configuration information from the configuration file.
    with open('ArchiverConfig.json') as JSONFile:
        Config = json.load(JSONFile)

    mode = 'a'
    if(Config['logging']['overwrite'] == True): mode = 'w'
    logging.basicConfig(filename=Config['logging']['directory'] + Config['logging']['name'], 
                        level=logging.INFO,
                        filemode=mode,
                        format='%(asctime)s - %(message)s',
                        datefmt='%Y-%m-%d %H:%M:%S')
    logging.info('Configuration loaded. Starting Archiver...')
    
    #Connect to the Redis database.
    r = redis.Redis(host=Config['redis']['hostname'], port=Config['redis']['port'], password=args.password)
    try:
        r.ping()
    except:
        print('Archiver Error: trouble connecting to Redis database on server (%s) port (%i)' % (Config['redis']['hostname'], Config['redis']['port']))
        logging.info('ERROR: trouble connecting to the Redis database on server (%s) port (%i)' % (Config['redis']['hostname'], Config['redis']['port']))
        return
    logging.info('Connection to the Redis database has been successfully established on server (%s) port (%i)' % (Config['redis']['hostname'], Config['redis']['port']))
    
    #Connect to the PostgreSQL database.
    p = psycopg2.connect(host=Config['postgresql']['hostname'], database=Config['postgresql']['database'], user=Config['postgresql']['user'], port=Config['postgresql']['port'])
    try:
        cur = p.cursor()
        cur.execute('SELECT 1')
    except psycopg2.OperationalError:
        print('Archiver Error: trouble connecting to PostgreSQL database on server (%s) port (%i)' % (Config['postgresql']['hostname'], Config['postgresql']['port']))
        logging.info('ERROR: trouble connecting to PostgreSQL database on server (%s) port (%i)' % (Config['postgresql']['hostname'], Config['postgresql']['port']))
        return
    logging.info('Connection to the PostgreSQL database has been successfully established on server (%s) port (%i)' % (Config['postgresql']['hostname'], Config['postgresql']['port']))

    #This next bit deletes the record of streams that have been completed. This is for testing purposes only.
    for key in r.scan_iter("*_LatestCompleted"):
        r.delete(key)

    #Now we split the job into multiple processes that each handle some portion of the streams.
    cur.execute('SELECT * FROM ' + Config['postgresql']['metric_config'] + ';')
    StreamConfig = cur.fetchall()
    ProcessCount = int(args.processes)
    ProcessList = []
    #Start the Monitor process, which monitors CPU usage, memory, and sends a heartbeat to Redis.
    Process(target=Monitor, args=(r, Config)).start()
    for i in range(ProcessCount):
        KeyList = [ x[0] + ':' + str(x[2]) + ':' + x[1] + ':' + 'archiving' for x in StreamConfig ][i::ProcessCount]
        ProcessList.append(Process(target=ProcessStreams, args=(r, p, cur, { x : GetLatest(r, x) for x in KeyList }, { x[0] + ':' + str(x[2]) + ':' + x[1] + ':' + 'archiving' : x for x in StreamConfig[i::ProcessCount]})))
    for i in range(ProcessCount):
        ProcessList[i].start()
    for i in range(ProcessCount):
        ProcessList[i].join()

def ProcessStreams(r, p, cur, StreamDict, Config):
    MetricDict = { x : [] for x in StreamDict.keys()}

    logging.info('Archiver in ProcessStreams.')
    #We now begin to continuously query these streams for new metrics, archiving them when possible.
    while True:
        #ReadStream is a list of stream object which have new entries that have yet to be archived.
        ReadStream = r.xread(StreamDict, block=0)
        #Loop over the streams which have entries to be archived.
        for StreamObject in ReadStream:
            #Loop over the individual entries in the stream.
            for DataObject in StreamObject[1]:
                #Check if the latest completed archived entry is '0' (no metrics have been archived yet for the stream).
                if StreamDict[StreamObject[0]] == '0':
                    NewLatest = str( int( DataObject[0].split('-')[0] ) - 1 ) + '-0'
                    SetLatest(r, StreamObject[0], NewLatest)
                    ProcessData(MetricDict, DataObject, Config, StreamObject[0])
                    StreamDict[StreamObject[0]] = DataObject[0]
                #Check if the current entry is within the time block (contained in the interval of entries to be archived).
            	elif int( DataObject[0].split('-')[0] ) < int( GetLatest(r, StreamObject[0]).split('-')[0] ) + 1000*Config[StreamObject[0]][3]:
                    ProcessData(MetricDict, DataObject, Config, StreamObject[0])
                    StreamDict[StreamObject[0]] = DataObject[0]
                #The entry is outside the current time block. Set a new latest completed and write to the database.
            	else:
                    #Case for "mean" averaging.
                    if Config[StreamObject[0]][3] == 0:
                        SetLatest(r, StreamObject[0], MetricDict[StreamObject[0]][0])
                        WritePostgres(p, cur, Config[StreamObject[0]][4], Config[StreamObject[0]][2], MetricDict[StreamObject[0]][1], int(MetricDict[StreamObject[0]][0].split('-')[0]))
                        MetricDict[StreamObject[0]] = []
                    #Case for "median" averaging.
                    elif Config[StreamObject[0]][3] == 1:
                        SetLatest(r, StreamObject[0], MetricDict[StreamObject[0]][-1][0])
                        MetricList = [ x[1] for x in MetricDict[StreamObject[0]] ]
                        WritePostgres(p, cur, Config[StreamObject[0]][4], Config[StreamObject[0]][2], median(MetricList), int(MetricDict[StreamObject[0]][0].split('-')[0]))
                        MetricDict[StreamObject[0]] = []
                    #Case for "mode" averaging.
                    elif Config[StreamObject[0]][3] == 2:
                        SetLatest(r, StreamObject[0], MetricDict[StreamObject[0]][0])
                        WritePostgres(p, cur, Config[StreamObject[0]][4], Config[StreamObject[0]][2], MetricDict[StreamObject[0]][3], int(MetricDict[StreamObject[0]][0].split('-')[0]))
                        MetricDict[StreamObject[0]] = []
                    #Case for "max" averaging.
                    elif Config[StreamObject[0]][3] == 3:
                        SetLatest(r, StreamObject[0], MetricDict[StreamObject[0]][0])
                        WritePostgres(p, cur, Config[StreamObject[0]][4], Config[StreamObject[0]][2], MetricDict[StreamObject[0]][1], int(MetricDict[StreamObject[0]][0].split('-')[0]))
                        MetricDict[StreamObject[0]] = []
                    #Case for "min" averaging.
                    elif Config[StreamObject[0]][3] == 4:
                        SetLatest(r, StreamObject[0], MetricDict[StreamObject[0]][0])
                        WritePostgres(p, cur, Config[StreamObject[0]][4], Config[StreamObject[0]][2], MetricDict[StreamObject[0]][1], int(MetricDict[StreamObject[0]][0].split('-')[0]))
                        MetricDict[StreamObject[0]] = []
                    #Case for "last" averaging.
                    elif Config[StreamObject[0]][3] == 5:
                        SetLatest(r, StreamObject[0], MetricDict[StreamObject[0]][0])
                        WritePostgres(p, cur, Config[StreamObject[0]][4], Config[StreamObject[0]][2], MetricDict[StreamObject[0]][1], int(MetricDict[StreamObject[0]][0].split('-')[0]))
                        MetricDict[StreamObject[0]] = []
                    #After performing the archiving on the previous block of data, we can now process the current entry.
                    ProcessData(MetricDict, DataObject, Config, StreamObject[0])
                    StreamDict[StreamObject[0]] = DataObject[0]
                    #Check to see if there is a gap in the data stream (the current object is more than one time interval from the latest completed).
                    if int( DataObject[0].split('-')[0] ) > int( GetLatest(r, StreamObject[0]).split('-')[0] ) + 1000*Config[StreamObject[0]][3]:
                        NewLatest = str( int( DataObject[0].split('-')[0] ) - 1 ) + '-0'
                        SetLatest(r, StreamObject[0], NewLatest)

def ProcessData(Metrics, Datum, Config, StreamName):
    Value = float(ReadDatum(Datum[1]))
    if Config[StreamName][3] == 0:
        if len(Metrics[StreamName]) == 0: 
            Metrics[StreamName] = [Datum[0], Value, 1]
        else:
            Metrics[StreamName][0] = Datum[0]
            Metrics[StreamName][1] = (Metrics[StreamName][1] * Metrics[StreamName][2] + Value) / (Metrics[StreamName][2] + 1)
            Metrics[StreamName][2] += 1
    elif Config[StreamName][3] == 1: #Keeps all data for now.
        Metrics[StreamName].append( (Datum[0], Value) )
    elif Config[StreamName][3] == 2:
        if len(Metrics[StreamName]) == 0:
            Metrics[StreamName] = [Datum[0], {Value : 1}, 1, Value]
        else:
            Metrics[StreamName][0] = Datum[0]
            if Value not in Metrics[StreamName][1]:
                Metrics[StreamName][1][Value] = 1
            else:
                Metrics[StreamName][1][Value] += 1
                if Metrics[StreamName][1][Value] > Metrics[StreamName][2]:
                    Metrics[StreamName][2] = Metrics[StreamName][1][Value]
                    Metrics[StreamName][3] = Value
    elif Config[StreamName][3] == 3:
        if len(Metrics[StreamName]) == 0:
            Metrics[StreamName] = [Datum[0], Value]
        else:
            Metrics[StreamName][0] = Datum[0]
            if Metrics[StreamName][1] < Value: Metrics[StreamName][1] = Value
    elif Config[StreamName][3] == 4:
        if len(Metrics[StreamName]) == 0:
            Metrics[StreamName] = [Datum[0], Value]
        else:
            Metrics[StreamName][0] = Datum[0]
            if Metrics[StreamName][1] > Value: Metrics[StreamName][1] = Value
    elif Config[StreamName][3] == 5:
        if len(Metrics[StreamName]) == 0:
            Metrics[StreamName] = [Datum[0], Value]
        else:
            Metrics[StreamName][0] = Datum[0]
            Metrics[StreamName][1] = Value

def TypeToStructType(Name):
    if Name == "int8_t": return "b"
    if Name == "int16_t": return "h"
    if Name == "int32_t": return "i"
    if Name == "int64_t": return "q"
    if Name == "uint8_t": return "B"
    if Name == "uint16_t": return "H"
    if Name == "uint32_t": return "I"
    if Name == "uint64_t": return "Q"
    if Name == "float": return "f"
    if Name == "double": return "d"

def TypeToSize(Name):
    if Name == "int8_t": return 1
    if Name == "int16_t": return 2
    if Name == "int32_t": return 4
    if Name == "int64_t": return 8
    if Name == "uint8_t": return 1
    if Name == "uint16_t": return 2
    if Name == "uint32_t": return 4
    if Name == "uint64_t": return 8
    if Name == "float": return 4
    if Name == "double": return 8

def ParseBinary(Binary, TypeName):
    Size = TypeToSize(TypeName)
    Form = TypeToStructType(TypeName)
    ret = []
    for i in range(len(Binary) / Size):
        dat = Binary[i*Size : (i+1)*Size]
        ret.append(struct.unpack(Form, dat)[0])
    return ret

def ReadDatum(dat):
    for key, val in dat.items():
        if key == 'val' or key == 'dat': return val
        return ParseBinary(val,key)[0]
        
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

def WritePostgres(p, cur, Table, Channel, Value, Time):
    Timestamp = int(Time/1000)
    Command = 'INSERT INTO ' + Table + ' (CHANNEL_ID, SMPL_TIME, SMPL_VALUE) VALUES (' + str(Channel) + ',to_timestamp(' + str(Timestamp) + '),' + str(Value) + ');'
    cur.execute(Command)
    p.commit()

def Monitor(r, Config):
    logging.info('Monitor is running.')
    Time = time.time()
    while True:
        pipeline = r.pipeline()
        if time.time() >= Time + 60:
            pipeline.xadd('archiver_heartbeat', {'float': struct.pack('f', 0)}, maxlen=1)
            Time = time.time()
        CPUs = psutil.cpu_percent(interval=Config['redis']['monitor_time'], percpu=True)
        CPUAverage = psutil.cpu_percent(interval=Config['redis']['monitor_time'])
        Mem = psutil.virtual_memory()[2]
        for i in range(len(CPUs)): pipeline.xadd('archiver_cpu'+str(i), {'float': struct.pack('f', CPUs[i])})
        pipeline.xadd('archiver_cpu_average', {'float': struct.pack('f', CPUAverage)})
        pipeline.xadd('archiver_mem', {'float': struct.pack('f', Mem)})
        [ _ for _ in pipeline.execute()]

def signal_handler(sig, frame):
    print('Keyboard Interrupt')
    logging.info('Keyboard interrupt. Exiting Archiver.')
    sys.exit(0)

if __name__ == "__main__":
    args = argparse.ArgumentParser()
    args.add_argument("-pw", "--password", default=None)
    args.add_argument("-pr", "--processes", default=1)
    main(args.parse_args())
