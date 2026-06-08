from datetime import datetime, timedelta
import pandas as pd
import sys
import plotly.express as px


selected_string = sys.argv[1]


col_labels = {
    7: [
        "Identifier",
        "String ID",
        "Time",
        "Date",
        "GPS Lock",
        "# of Sats in View",
        "Error Byte",
        "Freq Diff",
        "PPS Diff",
        "Freq Correction Slice",
        "DAC Value",
        "Power Supply1",
        "Power Supply2",
    ],
    9: [
        "Identifier",
        "String ID",
        "Frequency (Loop Period)",
        "DAC Voltage (Double)",
        "Frequency (per second)",
        "Loop Period",
        "Antenna Monitor",
        "Sine Output RMS",
        "Battery Voltage",
        "Temperature",
        "Reserved1",
        "Reserved2",
        "Reserved3",
    ],
    10: [
        "Identifier",
        "String ID",
        "PPS Stability Enabled",
        "PPS Disciplining to GPS",
        "PPS Output Type",
        "PPS Difference",
        "PPS Avg Difference",
        "PPS Avg Count",
        "PPS Synch Threshold",
        "PPS pull Cal Factor",
        "PPS active Time Cal Factor",
        "Frequency Variance",
        "Frequency Var Threshold",
        "PPS Stabile Mode Post-Warm up",
        "PPS Slope Indicator",
        "PPS Slope Cal Factor",
        "PPS Slope Distance",
        "Phase pulse to wave",
        "Pulse Aligned",
        "Target Pulse alignment",
        "Reserved1",
        "Reserved2",
        "Reserved3",
        "Reserved4",
        "Reserved5",
    ],
    30: ["id", "string id", "freq", "num1", "num2", "num3", "checksum"],
    70: [
        "Identifier",  #                   $GPNVS",,
        "String ID",  # 	70,
        "Clock status",  # 	0-7, (bit 0: enable, bit 1: in-sync, bit 3: in-holdover)
        "Clock source",  # 	Source
        "Clock seconds",  # Seconds….
        "Clock nanoseconds",  # 	Nanoseconds, (0 – 999,999,999)
        "TOD status",  # 0-3, (bit 0: enabled, bit 1: input-ok)
        "PPS status",  # 0-3, (bit 0: enabled, bit 1: input-ok)
        "PTP enable",  # 0-1,
        "PTP state",  # STATE, - String
        "NTP enable",  # 0-1,
        "NTP responses",
    ],
}


strings = []


with open(sys.argv[2], "r") as f:

    content = f.readlines()

    for line in content:

        if line.startswith("$GPNVS"):

            line = line.split("*")[0]

            parts = line.split(",")

            if parts[1] == selected_string:

                strings.append(parts)

        # print(f"{entry["__REALTIME_TIMESTAMP"]}: {entry["MESSAGE"]}")


df = pd.DataFrame(strings)
df.columns = col_labels[int(selected_string)]


fig = px.line(df, x=df.index, y=df.columns)
fig.update_layout(title_text=f"String: {selected_string}")

fig.show()
