import time
import inspect
import pyQsController

count = 0

def dspew():
    global count
    count += 1
    print("         ++++++++++ Python module " + __file__ + ":" +
            inspect.stack()[1][3] + "() count=" +
            repr(count))


def help():
    dspew()


def construct(args):
    dspew()
    return 0 # success


def preStart():
    dspew()
    return 0

def postStart():
    dspew()
    print(pyQsController.getVersion())
    return 0

def preStop():
    dspew()
    return 0

def postStop():
    dspew()
    return 0


def destroy():
    dspew()
    return 0 # success

