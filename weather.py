#
# Weather Daily Metric & Stats
# Process 1..n daily json format data file recording weather sensor metrics: temp, humidity, light, pressure
# Compute interval (daily) metric stats: Average, Min, Max, STD Deviation, % Change
# Chart / Plot results
#

import os
import json
import matplotlib.pyplot as plt
import matplotlib.dates as mdates
import numpy as np
import pandas as pd
from datetime import datetime as dt
import seaborn as sns

sns.set(rc={'figure.figsize':(11, 4)})

"""
Pre-process sensor SD Card data file;
grep "^\[" ./data/20200403.TXT | sed 's/\[//g' | sed 's/\]/,/g' > ./data/20200403.json

"""

### DataFrame to store Daily Metrics - Average, Mix, Max, STD Deviation, Close
path = "data/weather/"

for (path, dirs, files) in os.walk(path):
    files = [ fi for fi in files if fi.endswith(".json") ]


df_stat = pd.DataFrame(columns = ['data', 'date', 'mean' , 'std', 'min' , 'max', 'last'])
frames = []
stats = []


def load_json_data(filepath, frames, df_stat):
    with open(filepath) as f:
        d = json.load(f)
        df = pd.DataFrame(d)
        filename = os.path.basename(filepath)
        a = os.path.splitext(filename)
        date = a[0]
        ### Compute Daily Average
        row1 = pd.DataFrame({'data': 'pressure','date': date,'mean': df.p.mean() ,'std': df.p.std(),'min': df.p.min(),'max': df.p.max(),'last': df.p.iloc[-1]},index=[0])
        row2 = pd.DataFrame({'data': 'tempC','date': date,'mean': df.tempC.mean() ,'std': df.tempC.std(),'min': df.tempC.min(),'max': df.tempC.max(),'last': df.tempC.iloc[-1]},index=[0])
        row3 = pd.DataFrame({'data': 'humidity','date': date,'mean': df.h.mean() ,'std': df.h.std(),'min': df.h.min(),'max': df.h.max(),'last': df.h.iloc[-1]},index=[0])
        row4 = pd.DataFrame({'data': 'light','date': date,'mean': df.LDR.mean() ,'std': df.LDR.std(),'min': df.LDR.min(),'max': df.LDR.max(),'last': df.LDR.iloc[-1]},index=[0])

        df_stat = df_stat.append(row1,ignore_index=True,sort=True)
        df_stat = df_stat.append(row2,ignore_index=True,sort=True)
        df_stat = df_stat.append(row3,ignore_index=True,sort=True)
        df_stat = df_stat.append(row4,ignore_index=True,sort=True)

        frames.append(df)
        return df_stat


for f in files:
    filename = path+f
    print filename
    df_stat = load_json_data(filename,frames,df_stat)


print df_stat

df = pd.concat(frames)

df.sort_values(by='ts', ascending=True)

df['datetime'] = pd.to_datetime(df['ts'],unit='s')
df['dts'] = df['datetime'].dt.strftime('%d/%m/%Y %H:%M:%S')


print df.describe()


fig, ax = plt.subplots(nrows=2, ncols=2)

fig.suptitle('Weather Daily Stats')

df_stat.set_index('date', inplace=True)

df_stat[(df_stat.data == 'tempC')].plot(y=['min','max','mean','last'],legend=True, ax=ax[0,0]).set_title('temp(C)')
df_stat[(df_stat.data == 'humidity')].plot(y=['min','max','mean','last'],legend=True, ax=ax[0,1]).set_title('Humidity')
df_stat[(df_stat.data == 'pressure')].plot(y=['min','max','mean','last'],legend=True, ax=ax[1,0]).set_title('Pressure')
df_stat[(df_stat.data == 'light')].plot(y=['min','max','mean','last'],legend=True, ax=ax[1,1]).set_title('Light')

plt.subplots_adjust(bottom=0.1)
plt.show()


fig, ax = plt.subplots(4)

fig.suptitle('Weather Stats')

ax[0].set_title('Pressure (Pascals)')
ax[0].xaxis.set_major_formatter(mdates.DateFormatter('%b %d %H:%M'))
ax[0].plot(df['datetime'], df['p'])
ax[1].set_title('Temp (Celcius)')
ax[1].xaxis.set_major_formatter(mdates.DateFormatter('%b %d %H:%M'))
ax[1].plot(df['datetime'], df['tempC'])
ax[2].set_title('Humidity')
ax[2].xaxis.set_major_formatter(mdates.DateFormatter('%b %d %H:%M'))
ax[2].plot(df['datetime'], df['h'])
ax[3].set_title('Light (LDR)')
ax[3].xaxis.set_major_formatter(mdates.DateFormatter('%b %d %H:%M'))
ax[3].plot(df['datetime'], df['LDR'])

plt.show()
