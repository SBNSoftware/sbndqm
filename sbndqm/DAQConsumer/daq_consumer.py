import logging
import time
import sys
import argparse
import os
import signal
import sys

from process import ConsumerProcess, ProcessFhiclException

PROCESSES = {}

logger = None # set in main()

def clean_exit(signum, frame):
    do_clean_exit()

def do_clean_exit():
    logger.info("Received SIGINT. Attempting to shutdown subprocesses cleanly.")
    for _, process_list in PROCESSES.items(): 
        for process in process_list:
            retcode = process.force_exit()
            process.cleanup()
            logger.info("Process with config %s on port %i exited with code %i" % (process.config_file_path, process.port, retcode)) 
    logger.info("Finished cleaning up subprocesses. Exiting.")
    sys.exit(0)

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

    logger.info("New DAQ Consumer Master starting.")

    # cleanup on sigint
    signal.signal(signal.SIGINT, clean_exit)
    old_dispatchers = set()
    while True:
        dispatchers = get_dispatchers() 
        for port in dispatchers:
            if port not in old_dispatchers:
                PROCESSES[port] = []
                for config in args.fhicl_configuration:
                    log_file = None if args.log_dir is None else os.path.join(args.log_dir, config.split(".")[0] + "_" + str(port) + "_" + str(int(time.time())) + ".log")
                    try:
                        process = ConsumerProcess(port, config, args.overwrite_path, log_file)
                    except ProcessFhiclException as err:
                        logger.error("Process Fhicl Error for file (%s): %s" % (config, err.message))
                        do_clean_exit()
                    PROCESSES[port].append(process)
        old_dispatchers = dispatchers
        manage_processes(args)
        time.sleep(args.sleep)

def manage_processes(args):
    for port in PROCESSES.keys():
        PROCESSES[port] = [p for p in PROCESSES[port] if check_process(p, args)]

def check_process(process, args):
    retcode = process.check_exit()
    if retcode is not None:
        process.cleanup()
        logger.info("Process with config %s on port %i exited with code %i" % (process.config_file_path, process.port, retcode)) 
	if retcode != 0 and (process.n_restart < args.restart or args.restart < 0):
            logger.info("Restarting process with config %s on port %i. Process has been restarted %i times." % (process.config_file_path, process.port, process.n_restart))
	    process.restart()
	else:
            logger.info("Removing process with config %s on port %i." % (process.config_file_path, process.port))
            return False
    return True

# find all of the instances of the dispatcher running
def get_dispatchers():
    return set([5])
    pids = [pid for pid in os.listdir('/proc') if pid.isdigit()]
    dispatcher_ports = set()
    for pid in pids:
        port = None
        try: 
            with open(os.path.join('/proc', pid, 'cmdline')) as f:
                cmdline = f.read().split('\0')
        except: # may happen if a given process no longer exists
            continue
        if len(cmdline) and cmdline[0] == "dispatcher":
            for cmd in cmdline:
                if cmd.startswith("id:"):
                    try:
                        port = int(cmd.split(" ")[1])
                    except:
                        logger.error("Error parsing command line arguments for dispatcher instnace: %s" % " ".join(cmdline))
                    break
            else:
                logger.error("Error: Bad command line arguments for dispatcher instnace: %s" % " ".join(cmdline))
        if port in dispatcher_ports:
            logger.error("Error: Duplicate dispatcher id (%i) for dispatcher instance: %s" % (port, " ".join(cmdline)))
        if port:
            dispatcher_ports.add(port)
    return dispatcher_ports

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("-l", "--log_dir", default=None, help="Log directory location. Defaults to stdout (very cluttered).", metavar="<directory>")
    parser.add_argument("-f", "--fhicl_configuration", action="append", default=[],
        help="Fhicl files to run. Must be a full-path to the file. Use multiple times to specify multiple files.", metavar="</path/to/fhicl-filename>")
    parser.add_argument("-s", "--sleep", type=int, default=5, help="Sleep time between checks for new dispatchers [seconds]. Defaults to 5s.", metavar="<seconds>") 
    parser.add_argument("-r", "--restart", type=int, default=0, help="Number of times to restart failed process. Set to '-1' to always restart. Defaults to 0.", metavar="<ntimes>")
    parser.add_argument("-o", "--overwrite_path", default="source.dispatcherPort", help="Fhicl path to overwrite dispatcher port. Defaults to 'source.dispatcherPort'.", metavar="<fhicl path>")
    args = parser.parse_args()
    main(args)
