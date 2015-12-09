# vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4
import paramiko
import os
import sys
import time
import inspect
import logging
import argparse
import build

dir = os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe())))
pdir = os.path.dirname(dir)

import settings
import cmd
import ssh

logging.config.dictConfig(settings.LOGGING)
log = logging.getLogger('main')

ch = logging.StreamHandler()
ch.setLevel(logging.DEBUG)
formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
ch.setFormatter(formatter)
log.addHandler(ch)

def rebuild(target):
    build.rebuild(log, target)

def test_run(target, host, bsod = False, test = False):
    login = "Administrator"
    passwd = "1q2w3eQAZ"
    sh = ssh.SshUser(log, host, login, passwd = passwd)
    client = "FBackupCtl.exe"
    driver = "FBackup.sys"
    files = [client, driver, "FBackupCtl.pdb", "FBackup.pdb"]
    r_path = "c:\\FBackupTest"
    b_path = os.path.join(os.path.join(pdir, "build"), target)
    try:
        sh.cmd("rmdir /s /q " + r_path, throw = False)
        sh.cmd("mkdir " + r_path)
        for f in files:
            sh.file_put(os.path.join(b_path, f), os.path.join(r_path, f))
        sh.cmd(os.path.join(r_path, client) + " load " + os.path.join(r_path, driver))
        sh.cmd(os.path.join(r_path, client) + " fltstart")
        if test:
            sh.cmd(os.path.join(r_path, client) + " test")
        time.sleep(5)
        if bsod:
            sh.cmd(os.path.join(r_path, client) + " bugcheck")
        sh.cmd(os.path.join(r_path, client) + " fltstop")
    except Exception as e:
        log.error('Exception=' + str(e))
    finally:
        try:
            sh.cmd(os.path.join(r_path, client) + " unload")
        except:
            pass

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument("-b", "--bsod", action="store_true", help="bsod the node")
    parser.add_argument("-t", "--test", action="store_true", help="test drv")
    parser.add_argument("-r", "--rebuild", action="store_true", help="rebuild project")
    parser.add_argument("target", type=str, help="target")
    parser.add_argument("host", type=str, help="host ip")
    args = parser.parse_args()
    if args.rebuild:
        rebuild(args.target)
    test_run(args.target, args.host, args.bsod, args.test)