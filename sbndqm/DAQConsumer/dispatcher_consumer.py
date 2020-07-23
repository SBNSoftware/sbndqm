import time
import os
# python 2
try:
    import xmlrpclib as xmlrpc
# python 3
except:
    import xmlrpc.client as xmlrpc

from process import ConsumerProcess, ProcessFhiclException

# global logger
logger = None

class DispatcherConsumer:
    def __init__(self, fhicl_configurations, process_names, log_dir, overwrite_path):
        self.fhicl_configurations = fhicl_configurations
        self.process_names = process_names
        self.log_dir = log_dir
        self.overwrite_path = overwrite_path
        self.PROCESSES = {}
        self.dispatchers = {}

    def run(self):
        this_dispatchers = self.get_dispatchers() 
        new_dispatchers = {}
        for port in this_dispatchers:
            if port in self.dispatchers:
                new_dispatchers[port] = self.dispatchers[port]
            else:
                new_dispatchers[port] = "Starting"
        for port,_ in self.dispatchers.items():
            if port not in new_dispatchers:
                logger.info("Removing dispatcher on port %i" % port)
        self.dispatchers = new_dispatchers
        self.check_dispatchers()

        setters = []
        for port, status in self.dispatchers.items():
            if status == "Initialize":
                logger.info("New dispatcher instance on port: %i" % port)
                # if there are still processes on this port, end them
                if port in self.PROCESSES:
                    for p in self.PROCESSES[port]: 
                        self.end_process(p)
                self.PROCESSES[port] = []

                for config in self.fhicl_configurations:
                    ID = int(time.time())
                    log_file = None if self.log_dir is None else \
                        os.path.join(self.log_dir, os.path.split(config)[-1].split(".")[0] + "_" + str(port) + "_" + str(ID) + ".log")
                    logger.info("Starting process with config %s ID %i on port %i. Output will be logged to %s." % (config, ID, port, log_file))
                    try:
                        process = ConsumerProcess(str(port), ID, config, self.overwrite_path, log_file)
                    except ProcessFhiclException as err:
                        logger.error("Process Fhicl Error for file (%s): %s" % (config, err.message))
                        process = None

                    if process:
                        self.PROCESSES[port].append(process)

                setters.append( (port, "Running") )
        for port, status in setters:
            self.dispatchers[port] = status

    def end_process(self, process):
        logger.info("Ending process with config %s ID %i on port %s" % (process.name, process.ID, process.port))
        retcode = process.force_exit()
        process.cleanup()
        logger.info("Process with config %s ID %i on port %s exited with code %i" % (process.name, process.ID, process.port, retcode)) 

    def unregister(self, port):
        for name in self.process_names:
            logger.info("De-registering old monitoring process with unique_label %s on port %i." % (name, port))
            connect = xmlrpc.ServerProxy('http://localhost:%i' % port)
            connect.daq.unregister_monitor(name) # Added BH to test... Unregister monitor if it's already registered.
            logger.info("Done attempting de-register.")

    def check_dispatchers(self):
        setters = []
        removers = []
        for port, status in self.dispatchers.items():
            if status == "Starting":
                try:
                    self.unregister(port)
                    connect = xmlrpc.ServerProxy('http://localhost:%i' % port)
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
                    connect = xmlrpc.ServerProxy('http://localhost:%i' % port)
                    daq_status = connect.daq.status()
                except:
                    logger.info("Failed to connect to dispatcher XMLRPC server on port %i. Removing." % port)
                    removers.append(port)
                    continue

                if daq_status == "Ready":
                    setters.append( (port, "Starting") )
                    logger.info("Dispatcher on port %i back to 'Ready' state." % port)

        for port, status in setters:
            self.dispatchers[port] = status

        for port in removers:
            del self.dispatchers[port]

    # find all of the instances of the dispatcher running
    def get_dispatchers(self):
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
