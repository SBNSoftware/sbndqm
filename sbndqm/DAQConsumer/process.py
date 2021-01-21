import os
import subprocess
import tempfile
import signal
import select
import threading
import sys

class ProcessFhiclException(Exception):
    pass

class ConsumerProcess(object):
    def __init__(self, port, ID, config_file_path, overwrite_path, output_file=None):
        self.config_file_path = config_file_path
        self.name = os.path.split(config_file_path)[-1]
        self.overwrite_path = overwrite_path
        self.port = port
        self.n_restart = 0
        self.ID = ID
        self.output_file = output_file
        self.start()

    def start(self):
        try:
            with open(self.config_file_path) as f:
                text = f.read()
        except:
            raise ProcessFhiclException("Couldn't read fhicl file (%s)" % self.config_file_path)
        self.temp_file = tempfile.NamedTemporaryFile() 
        self.temp_file.write(text.encode())
        self.temp_file.write(("\n\n%s: %s" % (self.overwrite_path, self.port)).encode())
        self.temp_file.flush()

        self.process = None
        command = ["lar", "-c", self.temp_file.name] 
        # command = ["while", "sleep 1;", "ls", "-ltr"]
        # command = ["run_temp.sh"]
        output_file = sys.stdout.fileno() if self.output_file is None else open(self.output_file, "w+")
        try:
            self.process = subprocess.Popen(command, stdout=output_file, stderr=subprocess.STDOUT)
        except subprocess.CalledProcessError as err:
            self.retcode = err.code
        if self.output_file is not None:
            output_file.close()

    def cleanup(self):
        pass
        #if self.read_thread is not None:
        #    self.read_thread.join()

    def check_exit(self):
        if self.process is None: return self.retcode
        retcode = self.process.poll()
        if retcode is not None:
            self.process = None
            self.retcode = retcode
            return retcode
        return None

    def force_exit(self):
        if self.process is None: return self.retcode
        self.process.send_signal(signal.SIGINT)
        return self.wait_exit()

    def wait_exit(self):
        if self.process is None: return self.retcode
        self.retcode = self.process.wait()
        self.process = None
        return self.retcode

    def restart(self,reset=False):
        if reset:
            self.n_restart = 0
        else:
            self.n_restart += 1
        self.start()
