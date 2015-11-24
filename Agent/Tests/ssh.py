# vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4
import paramiko
import os
import sys
import time
import inspect
import logging

currentdir = os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe())))

import settings
from cmd import StdFp, StdThread

logging.config.dictConfig(settings.LOGGING)

log = logging.getLogger('main')

def ssh_cmd(ssh, cmd, throw = False, log = None):
    log.info("SSH:" + cmd)
    chan = None
    out_lines = []
    err_lines = []
    rc = 777
    try:
        chan = ssh.get_transport().open_session()
        chan.exec_command(cmd)
    
        stdout = StdFp(chan.makefile(), "SSH:stdout", elog = log)
        stderr = StdFp(chan.makefile_stderr(), "SSH:stderr", elog = log)

        stderr_t = StdThread(stderr)
        stdout_t = StdThread(stdout)

        stdout_t.start()
        stderr_t.start()
        
        rc = chan.recv_exit_status()
        stdout_t.join()
        stderr_t.join()
        out_lines = stdout.lines
        err_lines = stderr.lines

        if rc == 0:
            log.info("SSH:" + cmd + ":rc:" + str(rc))
        else:
            log.error("SSH:" + cmd + ":rc:" + str(rc))
    except Exception as e:
        log.exception(str(e))
    finally:
        if chan != None:
            try:
                chan.shutdown(2)
            except Exception as e:
                log.exception(str(e))
            try:
                chan.close()
            except Exception as e:
                log.exception(str(e))
    if rc != 0 and throw:
        raise Exception("SSH:" + cmd + ":rc:" + str(rc))

    return rc, out_lines, err_lines

class SshUser:
    def __init__(self, log, host, user, passwd = None, key_file = None, ftp = False):
        self.log = log
        self.host = host
        self.user = user
        self.passwd = passwd
        self.key_file = key_file
        self.ftp = ftp
    def get_ssh(self, ftp = False):
        if ftp or self.ftp:
            ftp = True
        return SshExec(self.log, self.host, self.user, passwd = self.passwd, key_file = self.key_file, ftp = ftp)
    def cmd(self, cmd, throw = True):
        s = self.get_ssh()
        s.cmd(cmd, throw = throw)
        s.close()
    def file_get(self, remote_file, local_file):
        s = self.get_ssh(ftp = True)
        s.file_get(remote_file, local_file)
        s.close()
    def file_put(self, local_file, remote_file):
        s = self.get_ssh(ftp = True)
        s.file_put(local_file, remote_file)
        s.close()

class SshExec:
    def __init__(self, log, host, user, passwd = None, key_file = None, ftp = False):
        self.host = host
        self.user = user
        self.passwd = passwd
        self.key_file = key_file
        self.log = log
        if self.key_file != None:
            self.pkey = paramiko.RSAKey.from_private_key_file(self.key_file)
        else:
            self.pkey = None
        self.ssh = paramiko.SSHClient()
        self.ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
        self.ssh.connect(self.host, username=self.user, password = self.passwd, pkey=self.pkey)
        if ftp:
            self.ftp = self.ssh.open_sftp()
        else:
            self.ftp = None
        self.log.info('SSH:opened to host ' + self.host)
    def cmd(self, cmd, throw = True):
        return ssh_cmd(self.ssh, cmd, throw = throw, log = self.log)
    def file_get(self, remote_file, local_file):
        self.log.info("SSH:getfile:" + str(self.host) + " remote:" + remote_file + " local:" + local_file)
        self.ftp.get(remote_file, local_file)
        self.log.info("SSH:getfile:completed")
    def file_put(self, local_file, remote_file):
        self.log.info("SSH:putfile:" + str(self.host) + " local:" + local_file + " remote:" + remote_file)
        self.ftp.put(local_file, remote_file)
        self.log.info("SSH:putfile:completed")
    def file_getcwd(self):
        return self.ftp.getcwd()
    def file_chdir(self, path):
        return self.ftp.chdir(path)
    def close(self):
        if self.ftp:
            self.ftp.close()
        self.ssh.close()

