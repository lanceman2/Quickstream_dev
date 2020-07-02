import time

count = 0

#def dprint(func, args, ...):
#    print(__file__ + func + "()" + args)


def help():
    t = 1
    while(True):
        if(t > 6):
            break
        t += 1
        print(" ++++++++++ Hello Python module Help" + repr(t))


def construct(args):
    global count
    count += 1
    print("construct(" + ",".join(args) + ") count=" + repr(count))
    return 0 # success


def preStart():
    global count
    count += 1
    print("+++++++++++++++++++preStart() count=" + repr(count))
    return 0


def destroy():
    global count
    count += 1
    print("destroy() count=" + repr(count))
    return 0 # success

