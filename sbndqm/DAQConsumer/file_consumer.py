import logging
import time
import sys
import argparse
import os
import signal
import sys
import socket
import fhicl
import glob

from process import ConsumerProcess, ProcessFhiclException

logger = None

class FileConsumer:
    def __init__(self, search_glob, parallel_process, fhicl_configurations, process_names, log_dir, overwrite_path):
        self.search_glob = search_glob
        self.parallel_process = parallel_process
        self.fhicl_configurations = fhicl_configurations
        self.process_names = process_names
        self.log_dir = log_dir
        self.overwrite_path = overwrite_path
        self.wait_time = 10 # [s]
        self.PROCESSES = {}
        self.files = {}
        self.initial_files = set([])
        self.initial_files = self.get_files()

    def run(self):
        # get any new files 
        this_files = self.get_files()
        now = int(time.time())
        # update the files we have 
        for f in this_files:
            if f not in self.files or self.files[f] != "Ready":
                self.files[f] = os.path.getmtime(f)
                logger.info("Detected file (%s) last edited at (%i). Current time is (%i)." % (os.path.basename(f), self.files[f], now))

        for f, _ in self.files.items():
            if f not in this_files:
                logger.info("File (%s) has been removed." % f)
                del self.files[f]

        make_ready = []
        for fname, ftime in self.files.items():
            # if the time is marked as ready, this file has already been handled
            if ftime == "Ready":
                continue 

            # if the file has been edited recently, wait more
            if now - ftime < self.wait_time:
                continue 

            # a new file is ready! 
            make_ready.append(fname)
            basef = os.path.basename(fname)
            logger.info("New file (%s) ready for processing." % basef)

            self.PROCESSES[basef] = []
            for config in self.fhicl_configurations:
                 n_active = sum([config in [p.config_file_path for p in procs] for _, procs in self.PROCESSES.items()])
                 # see if we are ready to start a new process
                 if n_active >= self.parallel_process:
                     logger.info("For config (%s): Currently processing %i files (max is %i). Skipping this one." % (config, n_active, self.parallel_process))
                     continue

                 ID = int(time.time())
                 log_file = None if self.log_dir is None else \
                     os.path.join(self.log_dir, os.path.split(config)[-1].split(".")[0] + "_" + basef + "_" + str(ID) + ".log")
                 logger.info("Starting process with config %s ID %i on file %s. Output will be logged to %s." % (config, ID, basef, log_file))
                 try:
                     process = ConsumerProcess('["%s"]' % fname, ID, config, self.overwrite_path, log_file)
                 except ProcessFhiclException as err:
                     logger.error("Process Fhicl Error for file (%s): %s" % (config, err.message))
                     process = None
 
                 if process:
                    self.PROCESSES[basef].append(process)
        for fname in make_ready:
            self.files[fname] = "Ready"

           
    def get_files(self):
        if self.search_glob:
            return set(glob.glob(self.search_glob)) - self.initial_files
        else:
            return set()
