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
            sleep(1)
            if hedge.getError() == "NO_ERROR":
                position = hedge.position()
                measureTime = position[3]
                if measureTime == lastMeasureTime:
                    # Nedošlo k aktualizaci polohy majáku -> zkusit znovu
                    # inicializovat maják
                    bridge.put("error", ("E"))
                    hedge.stop()
                    hedge=MarvelmindHedge(tty="/dev/ttyACM"+`portNumber%10`)
                    hedge.start()
                else:
                    # Došlo k aktualizaci polohy majáku -> odeslat polohu
                    # do mikrokontroleru
                    lastMeasureTime = measureTime
                    bridge.put("error", ("N"))
                    bridge.put("x", (str(position[0])))
                    bridge.put("y", (str(position[1])))
            else:
                # Došlo k chybě v připojení k majáku
                # (špatný port | nepřipojený maják | nehlášení polohy | ...)
                # -> zkusit postupně inicializovat maják na jiných portech
                bridge.put("error", ("S"))
                portNumber += 1
                hedge.stop()
                hedge=MarvelmindHedge(tty="/dev/ttyACM" + `portNumber%10`)
                hedge.start()
        except Exception:
            # Není vytvořeno připojení přes bridge k mikrokontroleru
            bridge = bridgeclient()
            continue
main()