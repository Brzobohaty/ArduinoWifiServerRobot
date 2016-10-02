from marvelmind import MarvelmindHedge
from time import sleep
import sys
sys.path.insert(0, '/usr/lib/python2.7/bridge/')
from bridgeclient import BridgeClient as bridgeclient


def main():
    portNumber = 0
    hedge = MarvelmindHedge(tty="/dev/ttyACM" + `portNumber`)
    hedge.start()
    bridge = bridgeclient()
    lastMeasureTime = 0
    while True:
        try:
            sleep(0.2)
            if hedge.getError() == "NO_ERROR":
                position = hedge.position()
                measureTime = position[3]
                if measureTime == lastMeasureTime:
                    bridge.put("error", ("E"))
                    hedge.stop()
                    hedge=MarvelmindHedge(tty="/dev/ttyACM"+`portNumber%10`)
                    hedge.start()
                else:
                    lastMeasureTime = measureTime
                    bridge.put("error", ("N"))
                    bridge.put("x", (str(position[0])))
                    bridge.put("y", (str(position[1])))
                    #print(str(position[0]),str(position[1]))
            else:
                bridge.put("error", ("S"))
                portNumber += 1
                hedge.stop()
                hedge=MarvelmindHedge(tty="/dev/ttyACM" + `portNumber%10`)
                hedge.start()
        except KeyboardInterrupt:
            hedge.stop()  # stop and close serial port
            sys.exit()
        except Exception:
            bridge = bridgeclient()
            continue
main()