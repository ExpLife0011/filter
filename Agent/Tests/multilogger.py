# vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4
import httplib
import json
import subprocess
import os
from datetime import datetime
import logging.config
import logging
import uuid
import threading
import copy
import time
import sys
import inspect
import re

class MultiLogger():
    def __init__(self, logs = []):
        self.logs = logs
    
    def append(self, l):
        self.logs.append(l)

    def error(self, msg, *args, **kwargs):
        for l in self.logs:
            l.error(msg, args, **kwargs)

    def info(self, msg, *args, **kwargs):
        for l in self.logs:
            l.info(msg, *args, **kwargs)

    def debug(self, msg, *args, **kwargs):
        for l in self.logs:
            l.debug(msg, *args, **kwargs)

    def critical(self, msg, *args, **kwargs):
        for l in self.logs:
            l.critical(msg, *args, **kwargs)

    def exception(self, msg, *args):
        for l in self.logs:
            l.exception(msg, *args)

