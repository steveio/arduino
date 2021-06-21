###
### Python Pandas / Mathplotlib DataVis: GPS data from a short walk
###
###
###

import matplotlib as mpl
import matplotlib.pyplot as plt
import pandas as pd

### Speed (kmph)
strlist = """
0.52
1.19
0.399
0.28
0.13
0.17
0.04
0.15
0.28
1.35
8.96
3.48
2.32
0.70
14.65
6.59
5.17
5.28
4.65
5.70
4.39
6.63
6.00
6.240
5.830
6.150
6.020
0.090
6.9390
6.5690
6.200
5.410
4.440
6.800
6.070
6.440
5.890
4.48
0.59
1.87
0.00
"""

lst = strlist.split("\n")

df = pd.DataFrame(lst)

### rename column
df.columns = ['kmph']

### remove null & empty rows
nan_value = float("NaN")
df.replace("", nan_value, inplace=True)
df1 = df.dropna()

df1 = df1.sort_values(by='kmph',ascending=True)

### convert string values to float
df1['kmph'] = pd.to_numeric(df1['kmph'], downcast="float")

print df1['kmph']

### histogram
ax = df1.hist(column='kmph', bins=7, grid=False, figsize=(12,8), color='#86bf91', zorder=2, rwidth=0.9)

ax = ax[0]
for x in ax:

    # Despine
    x.spines['right'].set_visible(False)
    x.spines['top'].set_visible(False)
    x.spines['left'].set_visible(False)

    # Switch off ticks
    x.tick_params(axis="both", which="both", bottom="off", top="off", labelbottom="on", left="off", right="off", labelleft="on")

    # Draw horizontal axis lines
    vals = x.get_yticks()
    for tick in vals:
        x.axhline(y=tick, linestyle='dashed', alpha=0.4, color='#eeeeee', zorder=1)

    # Remove title
    x.set_title("")

    # Set x-axis label
    x.set_xlabel("Speed (kmph)", labelpad=20, weight='bold', size=12)

    # Set y-axis label
    x.set_ylabel("Freq", labelpad=20, weight='bold', size=12)

    # Format y-axis label
    ###x.yaxis.set_major_formatter(StrMethodFormatter('{x:,g}'))

plt.title("Speed Histogram : Ublox GPS Data", y=1.02, fontsize=22);

plt.show()

### Altitude (metres)
strlist = """
41.10
28.10
25.00
23.00
22.60
22.20
21.10
20.70
24.80
21.50
21.60
23.30
24.60
22.20
22.70
21.90
19.20
23.40
15.50
19.50
19.60
18.60
14.90
9.20
3.60
3.90
4.90
5.30
5.30
5.50
11.20
13.10
19.10
27.60
22.60
22.40
23.60
26.50
43.60
43.30
38.90
"""

lst = strlist.split("\n")

df = pd.DataFrame(lst)

df.columns = ['alt']

nan_value = float("NaN")
df.replace("", nan_value, inplace=True)
df = df.dropna()

df['alt'] = pd.to_numeric(df['alt'], downcast="float")

print df['alt']

df.plot(figsize=(12, 10), linewidth=2.5, color='#86bf91')

plt.xlabel("Click", labelpad=15)
plt.ylabel("Altitude (metres)", labelpad=15)
plt.title("Altitude : Ublox GPS Data", y=1.02, fontsize=22);

plt.show()
