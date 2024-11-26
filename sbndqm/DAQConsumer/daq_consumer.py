import logging
import time
import sys
import argparse
import os
import signal
import sys
import socket
import fhicl

from process import ConsumerProcess, ProcessFhiclException

import dispatcher_consumer 
import file_consumer

logger = None # set in main()

def clean_exit(consumer, signum, frame):
    logger.info("Received SIGINT.")
    do_clean_exit(consumer)

def do_clean_exit(consumer):
    logger.info("Cleaning up.")
    for _, process_list in consumer.PROCESSES.items(): 
        for process in process_list:
            retcode = process.force_exit()
            process.cleanup()
            logger.info("Process with config %s ID %i on port %s exited with code %i" % (process.name, process.ID, process.port, retcode)) 
    logger.info("Finished cleaning up subprocesses. Exiting.")
    sys.exit(0)

def walk_fhicl_path(f, path):
    for name in path.split("."):
        f = f[name]
    return f

def load_fhicl_file(fname):
    try:
        return fhicl.make_pset(fname)
    except fhicl.CetException as err:
        # parse error
        if err.message[:10] == "---- Parse":
            raise err
        # file not found error
        elif err.message[:16] == "---- search_path":
            err = Exception("Unable to find file %s on path FHICL_FILE_PATH. Try adding the file location to FHICL_FILE_PATH. If the file is located in an installations/ sub-directory, try running 'mrbslp'" % fname)
            raise err
        # unknown error
        else:
            raise err
        
def main(args):
    # configure logging
    if args.log_dir is None:
        # use stdout by default
        logging.basicConfig(
	    stream=sys.stdout,
	    level=logging.INFO,
	    format='%(asctime)s - %(message)s',
	    datefmt='%Y-%m-%d %H:%M:%S')
    else:
        logging.basicConfig(
            filename=os.path.join(args.log_dir, "daq_consumer_master.log"),
	    level=logging.INFO,
	    format='%(asctime)s - %(message)s',
	    datefmt='%Y-%m-%d %H:%M:%S')

    global logger
    logger = logging.getLogger("DAQConsumerMaster")
    dispatcher_consumer.logger = logger
    file_consumer.logger = logger

    # if configured, log to both stdout and a file
    if args.log_dir is not None and args.log_stdout:
        console = logging.StreamHandler()
        console.setLevel(logging.DEBUG)
        formatter = logging.Formatter('%(asctime)s - %(message)s')
        console.setFormatter(formatter)
        logger.addHandler(console)

    hostname = socket.gethostname()
    logger.info("New DAQ Consumer Master starting on hostname: %s." % hostname)

    fhicls = [load_fhicl_file(os.path.basename(f)) for f in args.fhicl_configuration]
    process_names = [walk_fhicl_path(f, args.process_name_path) for f in fhicls]

    # get the consumer
    if args.consumer == "dispatcher":
        consumer = dispatcher_consumer.DispatcherConsumer(args.fhicl_configuration, process_names, args.log_dir, args.overwrite_path)
    elif args.consumer == "file":
        consumer = file_consumer.FileConsumer(args.file_glob, args.parallel_process, args.fhicl_configuration, process_names, args.log_dir, args.overwrite_path)

    # cleanup on sigint
    signal.signal(signal.SIGINT, lambda signum, frame: clean_exit(consumer, signum, frame))

    # start the consumer
    while True:
        consumer.run()
        consumer.PROCESSES = manage_processes(args, consumer.PROCESSES)
        time.sleep(args.sleep)

def manage_processes(args, processes):
    toremove = []
    for port in processes.keys():
        processes[port] = [p for p in processes[port] if check_process(args, p)]
        if len(processes[port]) == 0:
            logger.info("Consumer at (%s) no longer has any processes -- removing." % str(port))
            toremove.append(port)
    for port in toremove:
        del processes[port]
            
    return processes

def check_process(args, process):
    retcode = process.check_exit()
    if retcode is not None:
        process.cleanup()
        logger.info("Process with config %s ID %i on port %s exited with code %i" % (process.name, process.ID, process.port, retcode)) 
        if (process.n_restart < args.restart or args.restart < 0):
            logger.info("Restarting process with config %s ID %i on port %s. Process has been restarted %i times." % (process.name, process.ID, process.port, process.n_restart))
            process.restart()
        else:
            logger.info("Removing process with config %s ID %i on port %s." % (process.name, process.ID, process.port))
            return False
    return True

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("-l", "--log_dir", default=None, help="Log directory location. Defaults to stdout (very cluttered).", metavar="<directory>")
    parser.add_argument("-f", "--fhicl_configuration", action="append", default=[],
        help="Fhicl files to run. Must be a full-path to the file. Use multiple times to specify multiple files.", metavar="</path/to/fhicl-filename>")
    parser.add_argument("-s", "--sleep", type=int, default=5, help="Sleep time between checks for new dispatchers [seconds]. Defaults to 5s.", metavar="<seconds>") 
    parser.add_argument("-r", "--restart", type=int, default=0, help="Number of times to restart failed process. Set to '-1' to always restart. Defaults to 0.", metavar="<ntimes>")
    parser.add_argument("-p", "--process_name_path", default=None, help="Fhicl path to unique_label name. Defaults to 'source.transfer_plugin.unique_label' for dispatcher consumer and 'process_name' for file consumer.", metavar="<fhicl path>")
    parser.add_argument("-lo", "--log_stdout", action="store_true", help="Force logging to stdout.")
    parser.add_argument("-c", "--consumer", choices=["file", "dispatcher"], default="dispatcher", help="Which consumer to run. Defaults to '%(default)s'.", metavar="file|dispatcher")
    parser.add_argument("-o", "--overwrite_path", default=None, help="Fhicl path to overwrite dispatcher port/file name. Defaults to 'source.dispatcherPort' for dispatcher consumer and 'source.fileNames' for file consumer.", metavar="<fhicl path>")
    parser.add_argument("-g", "--file_glob", default=None, help="Glob path for input files. Used only by the file consumer. Defaults to 'None'", metavar="<glob>")
    parser.add_argument("-pp", "--parallel_process", type=int, default=1, help="Number of processes to run in parallel. Only used by the file consumer.", metavar="<num>")
    args = parser.parse_args()

    if args.consumer == "file" and args.overwrite_path is None:
        args.overwrite_path = "source.fileNames"
    if args.consumer == "dispatcher" and args.overwrite_path is None:
        args.overwrite_path = "source.dispatcherPort"
    if args.consumer == "file" and args.process_name_path is None:
        args.process_name_path = "process_name"
    if args.consumer == "dispatcher" and args.process_name_path is None:
        args.process_name_path = "source.transfer_plugin.unique_label"
  

    main(args)
