import logging
import time
import sys
import argparse
import os
import signal
import sys
import xmlrpclib
import socket
import fhicl

from process import ConsumerProcess, ProcessFhiclException

PROCESSES = {}
ALL_DISPATCHERS = {}

logger = None # set in main()

def clean_exit(signum, frame):
    logger.info("Received SIGINT.")
    do_clean_exit()

def do_clean_exit():
    logger.info("Cleaning up.")
    for _, process_list in PROCESSES.items(): 
        for process in process_list:
            retcode = process.force_exit()
            process.cleanup()
            logger.info("Process with config %s ID %i on port %i exited with code %i" % (process.name, process.ID, process.port, retcode)) 
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

    # cleanup on sigint
    signal.signal(signal.SIGINT, clean_exit)
    dispatchers = {}
    while True:
        this_dispatchers = get_dispatchers() 
        new_dispatchers = {}
        for port in this_dispatchers:
            if port in dispatchers:
                new_dispatchers[port] = dispatchers[port]
            else:
                new_dispatchers[port] = "Starting"
        for port,_ in dispatchers.items():
            if port not in new_dispatchers:
                logger.info("Removing dispatcher on port %i" % port)
        dispatchers = new_dispatchers
        check_dispatchers(dispatchers, process_names)

        setters = []
        for port, status in dispatchers.items():
            if status == "Initialize":
                logger.info("New dispatcher instance on port: %i" % port)
                # if there are still processes on this port, end them
                if port in PROCESSES:
                    for p in PROCESSES[port]: end_process(p)

                PROCESSES[port] = []
                for config in args.fhicl_configuration:
                    ID = int(time.time())
                    log_file = None if args.log_dir is None else \
                        os.path.join(args.log_dir, os.path.split(config)[-1].split(".")[0] + "_" + str(port) + "_" + str(ID) + ".log")
                    logger.info("Starting process with config %s ID %i on port %i. Output will be logged to %s." % (config, ID, port, log_file))
                    try:
                        process = ConsumerProcess(port, ID, config, args.overwrite_path, log_file)
                    except ProcessFhiclException as err:
                        logger.error("Process Fhicl Error for file (%s): %s" % (config, err.message))
                        do_clean_exit()
                    PROCESSES[port].append(process)
                setters.append( (port, "Running") )
        for port, status in setters:
            dispatchers[port] = status
        manage_processes(args)
        time.sleep(args.sleep)

def manage_processes(args):
    for port in PROCESSES.keys():
        PROCESSES[port] = [p for p in PROCESSES[port] if check_process(p, args)]

def end_process(process):
    logger.info("Ending process with config %s ID %i on port %i" % (process.name, process.ID, process.port))
    retcode = process.force_exit()
    process.cleanup()
    logger.info("Process with config %s ID %i on port %i exited with code %i" % (process.name, process.ID, process.port, retcode)) 

def check_process(process, args):
    retcode = process.check_exit()
    if retcode is not None:
        process.cleanup()
        logger.info("Process with config %s ID %i on port %i exited with code %i" % (process.name, process.ID, process.port, retcode)) 
	if retcode != 0 and (process.n_restart < args.restart or args.restart < 0):
            logger.info("Restarting process with config %s ID %i on port %i. Process has been restarted %i times." % (process.name, process.ID, process.port, process.n_restart))
	    process.restart()
	else:
            logger.info("Removing process with config %s ID %i on port %i." % (process.name, process.ID, process.port))
            return False
    return True

def unregister(port, process_names):
    for name in process_names:
	logger.info("De-registering old monitoring process with unique_label %s on port %i." % (name, port))
	connect = xmlrpclib.ServerProxy('http://localhost:%i' % port)
	connect.daq.unregister_monitor(name) # Added BH to test... Unregister monitor if it's already registered.
	logger.info("Done attempting de-register.")

def check_dispatchers(dispatchers, process_names):
    setters = []
    removers = []
    for port, status in dispatchers.items():
        if status == "Starting":
            try:
                unregister(port, process_names)
	        connect = xmlrpclib.ServerProxy('http://localhost:%i' % port)
	        daq_status = connect.daq.status()
            # connection failed -- remove the dispatcher
            except:
                logger.info("Failed to connect to dispatcher XMLRPC server on port %i. Removing." % port)
                removers.append(port)
                continue
                
            logger.info("Checking dispatcher port %i. Reported status: %s." % (port, daq_status))
            if daq_status == "Running":
                logger.info("Dispatcher port %i ready for connections." % port)
                setters.append( (port, "Initialize") )
        elif status == "Running":
            try:
                connect = xmlrpclib.ServerProxy('http://localhost:%i' % port)
                daq_status = connect.daq.status()
            except:
                logger.info("Failed to connect to dispatcher XMLRPC server on port %i. Removing." % port)
                removers.append(port)
                continue

            if daq_status == "Ready":
                setters.append( (port, "Starting") )
                logger.info("Dispatcher on port %i back to 'Ready' state." % port)

    for port, status in setters:
        dispatchers[port] = status

    for port in removers:
        del dispatchers[port]

# find all of the instances of the dispatcher running
def get_dispatchers():
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
                        logger.error("Error parsing command line arguments for dispatcher instance: %s" % " ".join(cmdline))
                    break
            else:
                logger.error("Error: Bad command line arguments for dispatcher instance: %s" % " ".join(cmdline))
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
    parser.add_argument("-p", "--process_name_path", default="source.transfer_plugin.unique_label", help="Fhicl path to unique_label name. Defaults to 'source.transfer_plugin.unique_label'.", metavar="<fhicl path>")
    parser.add_argument("-lo", "--log_stdout", action="store_true", help="Force logging to stdout.")
    args = parser.parse_args()
    main(args)
