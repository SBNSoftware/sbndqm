import argparse
import redis as rd
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

def connect_redis(cfg, args):
    redis = rd.Redis(host=cfg['redis']['hostname'],
                     port=cfg['redis']['port'],
                     password=args.password)
    try:
        redis.ping()
    except:
        logging.info('ERROR: trouble connecting to the Redis database on server (%s) port (%i)' % (cfg['redis']['hostname']))

def ConnectRedis(Config, args):
    r = redis.Redis(host=Config['redis']['hostname'], port=Config['redis']['port'], password=args.password)
    try:
        r.ping()
    except:
        print('Archiver Error: trouble connecting to Redis database on server (%s) port (%i)' % (Config['redis']['hostname'], Config['redis']['port']))
        logging.info('ERROR: trouble connecting to the Redis database on server (%s) port (%i)' % (Config['redis']['hostname'], Config['redis']['port']))
        return None
    logging.info('Connection to the Redis database has been successfully established on server (%s) port (%i)' % (Config['redis']['hostname'], Config['redis']['port']))
    return r

def ConnectPostgreSQL(Config):
    with open(Config['postgresql']['passfile'], 'r') as passfile:
        pwd = passfile.read().replace('\n', '')
    p = psycopg2.connect(host=Config['postgresql']['hostname'], database=Config['postgresql']['database'], user=Config['postgresql']['user'], port=Config['postgresql']['port'], password=pwd)
        
    try:
        cur = p.cursor()
        cur.execute('SELECT 1')
    except psycopg2.OperationalError:
        print('Archiver Error: trouble connecting to PostgreSQL database on server (%s) port (%i)' % (Config['postgresql']['hostname'], Config['postgresql']['port']))
        logging.info('ERROR: trouble connecting to PostgreSQL database on server (%s) port (%i)' % (Config['postgresql']['hostname'], Config['postgresql']['port']))
        return None, None
    logging.info('Connection to the PostgreSQL database has been successfully established on server (%s) port (%i)' % (Config['postgresql']['hostname'], Config['postgresql']['port']))
    
    return p, cur

def LoadConfig(Name):
    with open(Name) as JSONFile:
        Config = json.load(JSONFile)

    mode = 'a'
    if(Config['logging']['overwrite'] == True): mode = 'w'
    logging.basicConfig(filename=Config['logging']['directory'] + Config['logging']['name'], 
                        level=logging.INFO,
                        filemode=mode,
                        format='%(asctime)s - %(message)s',
                        datefmt='%Y-%m-%d %H:%M:%S')
    return Config

def main(args):

    signal.signal(signal.SIGINT, signal_handler)

    #Load configuration information from the configuration file.
    Config = LoadConfig('ArchiverConfig.json')
    logging.info('Configuration loaded. Starting Archiver...')
    
    #Connect to the Redis database.
    r = ConnectRedis(Config, args)
    if r is None:
        sys.exit(0)
    
    #Connect to the PostgreSQL database.
    p, cur = ConnectPostgreSQL(Config)
    if cur is None:
        sys.exit(0)


    #This next bit deletes the record of streams that have been completed. This is for testing purposes only.
    #for key in r.scan_iter('PMT*_LatestCompleted'):
    #    r.delete(key)
    
    #for key in r.scan_iter("*_LatestCompleted"):
        #print(key)
        #if key == 'tpc_channel:1259:rms:archiving':
        #    print(key[:-16], 'Reseting')
        #CurrentTime = int(GetLatest(r,key[:-16]).split('-')[0])
        #print('Current time', CurrentTime)
        #SetLatest(r,key[:-16], '1596208000000')
        #SetLatest(r,key[:-16], '1608237252732')
        #SetLatest(r,key[:-16], '1648073157000')

    #Now we split the job into multiple processes that each handle some portion of the streams.
    try:
        cur.execute('SELECT * FROM ' + Config['postgresql']['metric_config'] + ';')
        StreamConfig = cur.fetchall()
    except Exception as err:
        logging.error('Error while querying metric config table: {}'.format(err))
        logging.error(type(err))
        sys.exit(0)
    ProcessCount = int(args.processes)
    ProcessList = []
    #Start the Monitor process, which monitors CPU usage, memory, and sends a heartbeat to Redis.
    Process(target=Monitor, args=(r, Config)).start()
    for i in range(ProcessCount):
        KeyList = [ x[0] + ':' + str(x[2]) + ':' + x[1] + ':' + 'archiving' for x in StreamConfig ][i::ProcessCount]
        ProcessList.append(Process(target=ProcessStreams, args=(r, p, cur, { x : GetLatest(r, x) for x in KeyList }, { x[0] + ':' + str(x[2]) + ':' + x[1] + ':' + 'archiving' : x for x in StreamConfig[i::ProcessCount]}, args)))
    for i in range(ProcessCount):
        ProcessList[i].start()
    for i in range(ProcessCount):
        ProcessList[i].join()

def ProcessStreams(r, p, cur, StreamDict, Config, args):

    MetricDict = {x : [] for x in StreamDict.keys()}

    logging.info('Archiver is ready to process Redis streams..')
    logging.info('Latest completed for ' + StreamDict.keys()[0] + ': ' + StreamDict[StreamDict.keys()[0]])

    while True:
        TotalSetTime = 0
        TotalSetN = 0
        try:
            st = time.time()
            ReadStream = r.xread(StreamDict, block=0)
            logging.info('XREAD time: {}'.format(time.time()-st))
        except redis.RedisError as err:
            logging.error('Error while reading streams: {}'.format(err))
            logging.error(type(err))
            if ReconnectCount == 4:
                sys.exit(0)
            else:
                ReconnectCount += 1
                logging.error('Reconnecting to Redis (Attempt ' + str(ReconnectCount) + ') ...')
                ArcConfig = LoadConfig('ArchiverConfig.json')
                r = ConnectRedis(Config, args)
                continue

        if len(ReadStream) == 0:
            logging.error('Block timeout. Reconnecting to Redis...')
            ArcConfig = LoadConfig('ArchiverConfig.json')
            r = ConnectRedis(Config, args)
            continue
        
        ReconnectCount = 0

        #Loop over the streams which have entries to be archived.
        for StreamObject in ReadStream:
            #Loop over the individual entries in the stream.
            for DataObject in StreamObject[1]:
                #Check if the latest completed archived entry is '0' (no metrics have been archived yet for the stream).
                if StreamDict[StreamObject[0]] == '0':
                    NewLatest = str( int( DataObject[0].split('-')[0] ) - 1 ) + '-0'
                    TotalSetTime += SetLatest(r, StreamObject[0], NewLatest)
                    TotalSetN += 1
                    ProcessData(MetricDict, DataObject, Config, StreamObject[0])
                    StreamDict[StreamObject[0]] = DataObject[0]
                #Check if the current entry is within the time block (contained in the interval of entries to be archived).
            	elif int( DataObject[0].split('-')[0] ) < int( GetLatest(r, StreamObject[0]).split('-')[0] ) + 1000*Config[StreamObject[0]][5]:
                    ProcessData(MetricDict, DataObject, Config, StreamObject[0])
                    StreamDict[StreamObject[0]] = DataObject[0]
                #Make sure that there are metrics to archive (No metrics in MetricDict, but GetLatest is more than one time block previous to the next entry).
                elif int( DataObject[0].split('-')[0] ) > int( GetLatest(r, StreamObject[0]).split('-')[0] ) + 1000*Config[StreamObject[0]][5] and len(MetricDict[StreamObject[0]]) == 0:
                    NewLatest = str( int( DataObject[0].split('-')[0] ) - 1 ) + '-0'
                    TotalSetTime += SetLatest(r, StreamObject[0], NewLatest)
                    TotalSetN += 1
                    ProcessData(MetricDict, DataObject, Config, StreamObject[0])
                    StreamDict[StreamObject[0]] = DataObject[0]
                #The entry is outside the current time block. Set a new latest completed and write to the database.
            	else:
                    #Case for "mean" averaging.
                    if Config[StreamObject[0]][3] == 0:
                        TotalSetTime += SetLatest(r, StreamObject[0], MetricDict[StreamObject[0]][0])
                        TotalSetN += 1
                        WritePostgres(p, cur, Config[StreamObject[0]][4], Config[StreamObject[0]][2], MetricDict[StreamObject[0]][1], int(MetricDict[StreamObject[0]][0].split('-')[0]))
                        MetricDict[StreamObject[0]] = []
                    #Case for "median" averaging.
                    elif Config[StreamObject[0]][3] == 1:
                        TotalSetTime += SetLatest(r, StreamObject[0], MetricDict[StreamObject[0]][-1][0])
                        TotalSetN += 1
                        MetricList = [ x[1] for x in MetricDict[StreamObject[0]] ]
                        WritePostgres(p, cur, Config[StreamObject[0]][4], Config[StreamObject[0]][2], median(MetricList), int(MetricDict[StreamObject[0]][-1][0].split('-')[0]))
                        MetricDict[StreamObject[0]] = []
                    #Case for "mode" averaging.
                    elif Config[StreamObject[0]][3] == 2:
                        TotalSetTime += SetLatest(r, StreamObject[0], MetricDict[StreamObject[0]][0])
                        TotalSetN += 1
                        WritePostgres(p, cur, Config[StreamObject[0]][4], Config[StreamObject[0]][2], MetricDict[StreamObject[0]][3], int(MetricDict[StreamObject[0]][0].split('-')[0]))
                        MetricDict[StreamObject[0]] = []
                    #Case for "max" averaging.
                    elif Config[StreamObject[0]][3] == 3:
                        TotalSetTime += SetLatest(r, StreamObject[0], MetricDict[StreamObject[0]][0])
                        TotalSetN += 1
                        WritePostgres(p, cur, Config[StreamObject[0]][4], Config[StreamObject[0]][2], MetricDict[StreamObject[0]][1], int(MetricDict[StreamObject[0]][0].split('-')[0]))
                        MetricDict[StreamObject[0]] = []
                    #Case for "min" averaging.
                    elif Config[StreamObject[0]][3] == 4:
                        TotalSetTime += SetLatest(r, StreamObject[0], MetricDict[StreamObject[0]][0])
                        TotalSetN += 1
                        WritePostgres(p, cur, Config[StreamObject[0]][4], Config[StreamObject[0]][2], MetricDict[StreamObject[0]][1], int(MetricDict[StreamObject[0]][0].split('-')[0]))
                        MetricDict[StreamObject[0]] = []
                    #Case for "last" averaging.
                    elif Config[StreamObject[0]][3] == 5:
                        TotalSetTime += SetLatest(r, StreamObject[0], MetricDict[StreamObject[0]][0])
                        WritePostgres(p, cur, Config[StreamObject[0]][4], Config[StreamObject[0]][2], MetricDict[StreamObject[0]][1], int(MetricDict[StreamObject[0]][0].split('-')[0]))
                        MetricDict[StreamObject[0]] = []
                    #After performing the archiving on the previous block of data, we can now process the current entry.
                    ProcessData(MetricDict, DataObject, Config, StreamObject[0])
                    StreamDict[StreamObject[0]] = DataObject[0]
                    #Check to see if there is a gap in the data stream (the current object is more than one time interval from the latest completed).
                    if int( DataObject[0].split('-')[0] ) > int( GetLatest(r, StreamObject[0]).split('-')[0] ) + 1000*Config[StreamObject[0]][5]:
                        NewLatest = str( int( DataObject[0].split('-')[0] ) - 1 ) + '-0'
                        TotalSetTime += SetLatest(r, StreamObject[0], NewLatest)
                        TotalSetN += 1
        logging.info('Total set time: {}'.format(TotalSetTime))
        logging.info('Average set time: {}'.format(float(TotalSetTime)/float(TotalSetN)))

def ProcessData(Metrics, Datum, Config, StreamName):
    Value = float(ReadDatum(Datum[1]))
    if not math.isnan(Value):
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
    try:
        Latest = Database.get(StreamName + '_LatestCompleted')
    except redis.RedisError as err:
            logging.error(str('Error while getting latest for stream ' + StreamName + ': {}').format(err))
            logging.error(type(err))
            sys.exit(0)
    if Latest is not None: return Latest
    else:
        Database.set(StreamName + '_LatestCompleted','0')
        return '0'

def SetLatest(Database, StreamName, Latest):
    s = time.time()
    try:
        Database.set(StreamName + '_LatestCompleted', Latest)
    except redis.RedisError as err:
            logging.error(str('Error while setting latest for stream ' + StreamName + ' and timestamp ' + Latest + ': {}').format(err))
            logging.error(type(err))
            sys.exit(0)
    return time.time() - s

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
    Attempt = 0
    if math.isnan(Value):
        Value = 0;
    Command = 'INSERT INTO runcon_prd.' + Table + ' (CHANNEL_ID, SMPL_TIME, SMPL_VALUE) VALUES (' + str(Channel) + ',to_timestamp(' + str(Timestamp) + '),' + str(Value) + ');'
    while Attempt < 5:
        try:
            Attempt += 1
            cur.execute(Command)
            p.commit()
        except psycopg2.OperationalError as err:
            time.sleep(60)
            logging.error(str('Attempt ' + str(Attempt) + ': Possible PostgreSQL connection issue. Re-connecting...'))
            Config = LoadConfig('ArchiverConfig.json')
            p, cur = ConnectPostgreSQL(Config)
            if cur is None:
                sys.exit(0)
        except Exception as err:
            logging.error(str('Attempt ' + str(Attempt) + ': Error while archiving to table ' + Table + ' for channel ' + str(Channel) + ' and value ' + str(Value) + 'with timestamp ' + str(Time) + ': {}').format(err))
            logging.error(type(err))
            time.sleep(60)
            #sys.exit(0)
        else: break
    else:
        logging.error('Failed reattempts; exiting...')
        sys.exit(0)

def Monitor(r, Config):
    logging.info('Monitor is running.')
    ErrorCount = 0
    while ErrorCount < 4:
        pipeline = r.pipeline()
        #pipeline.xadd('archiver_heartbeat', {'float': struct.pack('f', 0)}, maxlen=1)
        #CPUs = psutil.cpu_percent(interval=Config['redis']['monitor_time'], percpu=True)
        #CPUAverage = psutil.cpu_percent(interval=Config['redis']['monitor_time'])
        #Mem = psutil.virtual_memory()[2]
        #try:
        #    for i in range(len(CPUs)): pipeline.xadd('archiver_cpu'+str(i), {'float': struct.pack('f', CPUs[i])}, maxlen=10)
        #    pipeline.xadd('archiver_cpu_average', {'float': struct.pack('f', CPUAverage)}, maxlen=10)
        #    pipeline.xadd('archiver_mem', {'float': struct.pack('f', Mem)}, maxlen=10)
        #    [ _ for _ in pipeline.execute()]
        #except redis.RedisError as err:
        #    logging.error(str('Error while executing pipeline for archiver metrics: {}').format(err))
        #    sys.exit(0)
        #time.sleep(300)
        try:
            pipeline.xadd('archiver_heartbeat', {'float': struct.pack('f', 0)}, maxlen=1)
            [ _ for _ in pipeline.execute() ]
        except redis.RedisError as err:
            logging.error(str('Error while executing pipeline for archiver heartbeat: {}').format(err))
            #sys.exit(0)
            ErrorCount += 1
        time.sleep(60)

def signal_handler(sig, frame):
    print('Keyboard Interrupt')
    logging.info('Keyboard interrupt. Exiting Archiver.')
    sys.exit(0)

if __name__ == "__main__":
    args = argparse.ArgumentParser()
    args.add_argument("-pw", "--password", default=None)
    args.add_argument("-pr", "--processes", default=1)
    main(args.parse_args())
