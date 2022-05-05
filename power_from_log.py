INPUT_FILE = "power.testlog"

# From TelosB and CC2420 datasheet 
CURRENT_MA = {
    "CPU" : 1.8,
    "LPM" : 0.0051,
    "Deep LPM" : 0, # not used
    "Radio Rx" : 23,
    "Radio Tx" : 17.4, # P = 0 dBm | 17.4 mA, P = −5 | dBm 14 mA, P = -10 | 11 mA, P = -15 dBm | 9.9 mA,  P = -25 dBm | 8.5 mA
}

STATES = list(CURRENT_MA.keys())

VOLTAGE = 0 # taked from GetSensorBattery()
RTIMER_ARCH_SECOND = 32768

def main():
    node_ticks = {}
    node_total_ticks = {}
    with open(INPUT_FILE, "r") as f:
        
        for line in f:

            if "INFO: Client" in line and "Battery Value" in line:
                fields = line.split()
                VOLTAGE= float(fields [5])
                print ("Voltage has bee set to", VOLTAGE)
            if "INFO: Energest" not in line:
                continue
            line = "1 1 " + line
            fields = line.split()

            try:
                node = int(fields[1])
            except:
                continue
            node = 1
            if node not in node_ticks:
                # initialize to zero
                node_ticks[node] = { u : 0  for u in STATES }
                node_total_ticks[node] = 0

            try:
                state_index = 5
                state = fields[state_index]
                tick_index = state_index + 2
                if state not in STATES:
                    state = fields[state_index] + " " + fields[state_index+1]
                    tick_index += 1
                    if state not in STATES:
                        # add to the total time
                        if state == "Total time":
                            node_total_ticks[node] += int(fields[tick_index])
                        continue
                # add to the time spent in specific state
                ticks = int(fields[tick_index][:-1])
                node_ticks[node][state] += ticks
            except Exception as ex:
                print("Failed to process line '{}': {}".format(line, ex))

    nodes = sorted(node_ticks.keys())
    for node in nodes:
        total_avg_current_mA = 0
        period_ticks = node_total_ticks[node]
        period_seconds = period_ticks / RTIMER_ARCH_SECOND
        for state in STATES:
            ticks = node_ticks[node].get(state, 0)
            current_mA = CURRENT_MA[state]
            state_avg_current_mA = ticks * current_mA / period_ticks
            total_avg_current_mA += state_avg_current_mA
        total_charge_mC = period_ticks * total_avg_current_mA / RTIMER_ARCH_SECOND
        total_energy_mJ = total_charge_mC * VOLTAGE
        total_energy_mW = total_avg_current_mA * VOLTAGE
        print("Node {}: {:.2f} mC ({:.3f} mAh) charge consumption, {:.2f} mJ energy consumption or {:.2f} mW in {:.2f} seconds".format(
            node, total_charge_mC, total_charge_mC / 3600.0, total_energy_mJ, total_energy_mW, period_seconds))

if __name__ == "__main__":
    main()
