import time
import inspect
import pyQsController

controller = pyQsController.Controller()

count = 0

print(controller)


userData = {
    "type": "Ford",
    "mass": 2040,
    "year": 2008
    }

print(controller.getParameter(2,3, "hi", userData))


def dspew():
    global count
    count += 1
    print("         ++++++++++ Python module " + __file__ + ":" +
            inspect.stack()[1][3] + "() count=" +
            repr(count))



def getParameterCallback(value, stream, fName, pName, data):
    dspew()
    print(stream, fName, pName, value, data)
    return 0


parameters = None


def construct(args):

    global parameters
    dspew()
    print(args)
    parameters = args
    return 0 # success


def preStart(stream, Filter, numIn, numOut):
    dspew()
    print(stream, Filter, numIn, numOut)
    return 1


def postStart(stream, Filter, numIn, numOut):
    for p in parameters:
        print("===========parameter ", p, "userData=", userData)
        #controller.getParameter(Filter, p, getParameterCallback)
    dspew()
    print(pyQsController)
    print(pyQsController.getVersion, pyQsController.getVersion())
    return 0

def preStop(stream, Filter, numIn, numOut):
    dspew()
    return 0

def postStop(stream, Filter, numIn, numOut):
    dspew()
    return 0


def destroy():
    dspew()
    return 0 # success

