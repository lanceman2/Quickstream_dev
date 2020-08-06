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


def dspew():
    global count
    count += 1
    print("         ++++++++++ Python module " + __file__ + ":" +
            inspect.stack()[1][3] + "() count=" +
            repr(count))



def getParameterCallback(value, stream, fName, pName, data):
    dspew()
    print(stream, fName, pName, value, data)
    print("\n\n", value, "\n\n")
    # Looks like this must have a return int value or this function will not
    # get called.  If the number of arguments are not consistent with the
    # controller.getParameter() call, this function will not get called.
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
        print("===========================================================================================================parameter ", p, "userData=", userData)
        controller.getParameter(stream, Filter, p, getParameterCallback,
                userData)
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

