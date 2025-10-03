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

def ConnectRedis(Config, args):
    r = redis.Redis(host=Config['redis']['hostname'], port=Config['redis']['port'], password=args.password)
    try:
        r.ping()
    except Exception:
        print('Archiver Error: trouble connecting to Redis database on server (%s) port (%i)' % (Config['redis']['hostname'], Config['redis']['port']))
        logging.info('ERROR: trouble connecting to the Redis database on server (%s) port (%i)' % (Config['redis']['hostname'], Config['redis']['port']))
        return None
    logging.info('Connection to the Redis database has been successfully established on server (%s) port (%i)' % (Config['redis']['hostname'], Config['redis']['port']))
    return r

def ConnectPostgreSQL(Config):
    with open(Config['postgresql']['passfile'], 'r') as passfile:
        pwd = passfile.read().replace('\n', '')
    p = psycopg2.connect(host=Config['postgresql']['hostname'], database=Config['postgresql']['database'], user=Config['postgresql']['user'], port=Config['postgresql']['port'], password=pwd)
        #host=Config['postgresql']['hostname'], database=Config['postgresql']['database'], user='mueller', port=Config['postgresql']['port'])
        
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
    if Config['logging'].get('overwrite') == True:
        mode = 'w'
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
        #SetLatest(r,key[:-16], '1666210000000')
        #SetLatest(r,key[:-16], '1713834881000')

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
        stream_last = { x : GetLatest(r, x) for x in KeyList }
        per_stream_cfg = { x[0] + ':' + str(x[2]) + ':' + x[1] + ':' + 'archiving' : x for x in StreamConfig[i::ProcessCount]}
        ProcessList.append(Process(target=ProcessStreams, args=(r, p, cur, stream_last, per_stream_cfg, args)))

    for i in range(ProcessCount):
        ProcessList[i].start()
    for i in range(ProcessCount):
        ProcessList[i].join()

def ProcessStreams(r, p, cur, StreamDict, Config, args):

    MetricDict = { x : [] for x in StreamDict.keys()}

    logging.info('Archiver in ProcessStreams.')
    first_key = next(iter(StreamDict)) if StreamDict else None
    if first_key is not None:
        logging.info('LatestCompleted for ' + first_key + ': ' + StreamDict[first_key])
    else:
        logging.info('No streams assigned to this process.')
        return

    ReconnectCount = 0

    #We now begin to continuously query these streams for new metrics, archiving them when possible.
    while True:
        TotalSetTime = 0.0
        TotalSetN = 0
        #ReadStream is a list of stream object which have new entries that have yet to be archived.
        try:
            st = time.time()
            try:
                ReadStream = r.xread(StreamDict, block=0)
            except Exception as e:
                logging.error(e)
            logging.info('XREAD time: {}'.format(time.time()-st))
            logging.info('Redis READ completed.')
        except redis.RedisError as err:
            logging.error('Error while reading streams: {}'.format(err))
            logging.error(type(err))
            if ReconnectCount == 4:
                sys.exit(0)
            else:
                ReconnectCount += 1
                logging.error('Reconnecting to Redis (Attempt ' + str(ReconnectCount) + ') ...')
                ArcConfig = LoadConfig('ArchiverConfig.json')
                r = ConnectRedis(ArcConfig, args)
                continue

        if len(ReadStream) == 0:
            logging.error('Block timeout. Reconnecting to Redis...')
            ArcConfig = LoadConfig('ArchiverConfig.json')
            r = ConnectRedis(ArcConfig, args)
            continue
        
        ReconnectCount = 0

        #Loop over the streams which have entries to be archived.
        countStreams = 0
        totalStreams = len(ReadStream)
        logging.info("Streams to be archived: {}".format(totalStreams))

        for StreamObject in ReadStream:
            # stream_name_b = StreamObject[0]
            # print("DEBUG: stream_name_b =", stream_name_b, "type =", type(stream_name_b))
            # stream_name = stream_name_b.decode('utf-8')
            stream_name = StreamObject[0]
            entries = StreamObject[1]
            logging.debug("Stream: {}".format(stream_name))
            
            #Loop over the individual entries in the stream.
            for DataObject in entries:
                entry_id_b, entry_fields = DataObject
                try: 
                    entry_id = entry_id_b.decode('utf-8')
            
                    logging.debug("Entry: {}, fields: {}".format(entry_id,entry_fields))
                    
                    #Check if the latest completed archived entry is '0' (no metrics have been archived yet for the stream).
                    if StreamDict[stream_name] == '0':
                        NewLatest = str( int( entry_id.split('-')[0] ) - 1 ) + '-0'
                        TotalSetTime += SetLatest(r, stream_name, NewLatest)
                        TotalSetN += 1
                        ProcessData(MetricDict, (entry_id, entry_fields), Config, stream_name)
                        StreamDict[stream_name] = entry_id
                    #Check if the current entry is within the time block (contained in the interval of entries to be archived).
                    elif int( entry_id.split('-')[0] ) < int( GetLatest(r, stream_name).split('-')[0] ) + 1000*Config[stream_name][5]:
                        ProcessData(MetricDict, (entry_id, entry_fields), Config, stream_name)
                        StreamDict[stream_name] = entry_id
                    #Make sure that there are metrics to archive (No metrics in MetricDict, but GetLatest is more than one time block previous to the next entry).
                    elif int( entry_id.split('-')[0] ) > int( GetLatest(r, stream_name).split('-')[0] ) + 1000*Config[stream_name][5] and len(MetricDict[stream_name]) == 0:
                        NewLatest = str( int( entry_id.split('-')[0] ) - 1 ) + '-0'
                        TotalSetTime += SetLatest(r, stream_name, NewLatest)
                        TotalSetN += 1
                        ProcessData(MetricDict, (entry_id, entry_fields), Config, stream_name)
                        StreamDict[stream_name] = entry_id
                    #The entry is outside the current time block. Set a new latest completed and write to the database.
                    else:
                        #Case for "mean" averaging.
                        if Config[stream_name][3] == 0:
                            TotalSetTime += SetLatest(r, stream_name, MetricDict[stream_name][0])
                            TotalSetN += 1
                            WritePostgres(p, cur, Config[stream_name][4], Config[stream_name][2], MetricDict[stream_name][1], int(MetricDict[stream_name][0].split('-')[0]))
                            MetricDict[stream_name] = []
                        #Case for "median" averaging.
                        elif Config[stream_name][3] == 1:
                            TotalSetTime += SetLatest(r, stream_name, MetricDict[stream_name][-1][0])
                            TotalSetN += 1
                            MetricList = [ x[1] for x in MetricDict[stream_name] ]
                            WritePostgres(p, cur, Config[stream_name][4], Config[stream_name][2], median(MetricList), int(MetricDict[stream_name][-1][0].split('-')[0]))
                            MetricDict[stream_name] = []
                        #Case for "mode" averaging.
                        elif Config[stream_name][3] == 2:
                            TotalSetTime += SetLatest(r, stream_name, MetricDict[stream_name][0])
                            TotalSetN += 1
                            WritePostgres(p, cur, Config[stream_name][4], Config[stream_name][2], MetricDict[stream_name][3], int(MetricDict[stream_name][0].split('-')[0]))
                            MetricDict[stream_name] = []
                        #Case for "max" averaging.
                        elif Config[stream_name][3] == 3:
                            TotalSetTime += SetLatest(r, stream_name, MetricDict[stream_name][0])
                            TotalSetN += 1
                            WritePostgres(p, cur, Config[stream_name][4], Config[stream_name][2], MetricDict[stream_name][1], int(MetricDict[stream_name][0].split('-')[0]))
                            MetricDict[stream_name] = []
                        #Case for "min" averaging.
                        elif Config[stream_name][3] == 4:
                            TotalSetTime += SetLatest(r, stream_name, MetricDict[stream_name][0])
                            TotalSetN += 1
                            WritePostgres(p, cur, Config[stream_name][4], Config[stream_name][2], MetricDict[stream_name][1], int(MetricDict[stream_name][0].split('-')[0]))
                            MetricDict[stream_name] = []
                        #Case for "last" averaging.
                        elif Config[stream_name][3] == 5:
                            TotalSetTime += SetLatest(r, stream_name, MetricDict[stream_name][0])
                            WritePostgres(p, cur, Config[stream_name][4], Config[stream_name][2], MetricDict[stream_name][1], int(MetricDict[stream_name][0].split('-')[0]))
                            MetricDict[stream_name] = []
                        #After performing the archiving on the previous block of data, we can now process the current entry.
                        ProcessData(MetricDict, (entry_id, entry_fields), Config, stream_name)
                        StreamDict[stream_name] = entry_id
                        #Check to see if there is a gap in the data stream (the current object is more than one time interval from the latest completed).
                        if int( entry_id.split('-')[0] ) > int( GetLatest(r, stream_name).split('-')[0] ) + 1000*Config[stream_name][5]:
                            NewLatest = str( int( entry_id.split('-')[0] ) - 1 ) + '-0'
                            TotalSetTime += SetLatest(r, stream_name, NewLatest)
                            TotalSetN += 1 

                except Exception as e:
                    logging.exception(e)

            if countStreams % 5000 == 0 or countStreams == totalStreams:
                percentDone = (countStreams / totalStreams) * 100
                logging.info("Processed {}/{} streams ({:.2f}%)".format(countStreams,totalStreams,percentDone))
            countStreams += 1

        logging.info('Total set time: {}'.format(TotalSetTime))
        logging.info('Average set time: {}'.format(float(TotalSetTime)/float(TotalSetN) if TotalSetN else 0.0))

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
    for i in range(len(Binary) // Size):
        dat = Binary[i*Size : (i+1)*Size]
        ret.append(struct.unpack(Form, dat)[0])
    return ret

def ReadDatum(dat):
    for key, val in dat.items():
        if key == b'val' or key == b'dat': 
            return val 
        type_name = key.decode('utf-8')
        return ParseBinary(val, type_name)[0]
        
def GetLatest(Database, StreamName):
    try:
        Latest = Database.get(StreamName + '_LatestCompleted')
    except redis.RedisError as err:
            logging.error(str('Error while getting latest for stream ' + StreamName + ': {}').format(err))
            logging.error(type(err))
            sys.exit(0)
    if Latest is not None: return Latest.decode('utf-8')
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
        return ( l[(Length-1)//2] + l[(Length+1)//2] ) / 2.0
    else:
        return l[(Length-1)//2]

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
            pipeline.xadd('archiver_heartbeat', {'float': struct.pack('f', 0.0)}, maxlen=1)
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
    parser = argparse.ArgumentParser()
    parser.add_argument("-pw", "--password", default=None)
    parser.add_argument("-pr", "--processes", default=1)
    try:
        main(parser.parse_args())
    except Exception as e:
        logging.exception("Uncaught exception in main()")
        logging.exception(f"Archiver crashed with exception: {e}")
        sys.exit(1)
