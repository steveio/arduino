#
# Weather Daily Metric & Stats
# Process 1..n daily json format data file recording weather sensor metrics: temp, humidity, light, pressure
# Compute interval (daily) metric stats: Average, Min, Max, STD Deviation, % Change
# Chart / Plot results
#
# Demonstrates some uses of Python - Pandas, Matplotlib
#


import os
import json
import numpy as np

import matplotlib.pyplot as plt
import matplotlib.dates as mdates
import pandas as pd
from datetime import datetime as dt
import seaborn as sns
from datetime import datetime, timedelta

sns.set(rc={'figure.figsize':(11, 4)})


import pymongo
from bson import json_util

mongo_server = "mongodb://localhost:27017/"
mongo_db = "weather"
mongo_collection = "sensorData"

mongoClient = pymongo.MongoClient(mongo_server)
mydb = mongoClient[mongo_db]
sensorData = mydb[mongo_collection]

d = []


currts = sensorData.find_one(sort=[("ts", pymongo.DESCENDING)])
startts = currts['ts'] - 86400
endts = currts['ts'] - 18000

mydoc = sensorData.find({"ts":{'$gte': startts,'$lte': endts}}, {'_id': 0})

for doc in mydoc:
  s = json.dumps(doc, sort_keys=True,default=json_util.default)
  o = json.loads(s)
  d.append(o)


pd.set_option('display.max_rows', None)
pd.set_option('display.max_columns', None)
pd.set_option('display.width', None)
pd.set_option('display.max_colwidth', -1)


df_stat = pd.DataFrame(columns = ['data', 'date', 'mean' , 'std', 'min' , 'max', 'last'])
frames = []
stats = []


df = pd.DataFrame(d)
print(df.info(verbose=True))

### Compute Daily Stats - Mean, Min, Max, STD Deviation (freq: all data points)
row1 = pd.DataFrame({'data': 'pressure','date': df.ts,'mean': df.p.mean() ,'std': df.p.std(),'min': df.p.min(),'max': df.p.max(),'last': df.p.iloc[-1]},index=[0])
row2 = pd.DataFrame({'data': 'tempC','date': df.ts,'mean': df.tempC.mean() ,'std': df.tempC.std(),'min': df.tempC.min(),'max': df.tempC.max(),'last': df.tempC.iloc[-1]},index=[0])
row3 = pd.DataFrame({'data': 'humidity','date': df.ts,'mean': df.h.mean() ,'std': df.h.std(),'min': df.h.min(),'max': df.h.max(),'last': df.h.iloc[-1]},index=[0])
row4 = pd.DataFrame({'data': 'light','date': df.ts,'mean': df.LDR.mean() ,'std': df.LDR.std(),'min': df.LDR.min(),'max': df.LDR.max(),'last': df.LDR.iloc[-1]},index=[0])

df_stat = df_stat.append(row1,ignore_index=True,sort=True)
df_stat = df_stat.append(row2,ignore_index=True,sort=True)
df_stat = df_stat.append(row3,ignore_index=True,sort=True)
df_stat = df_stat.append(row4,ignore_index=True,sort=True)

frames.append(df)


print(df_stat)

df = pd.concat(frames)


df.sort_values(by='ts', ascending=True)
df['datetime'] = pd.to_datetime(df['ts'],unit='s')
df['dts'] = df['datetime'].dt.strftime('%d/%m/%Y %H:%M:%S')


# https://www.dataquest.io/blog/tutorial-time-series-analysis-with-pandas/

df = df.set_index('datetime')
df.index

# add temporal column indexes
df['Year'] = df.index.year
df['Month'] = df.index.month
df['Day'] = df.index.day
df['Weekday Name'] = df.index.weekday_name
df['Hour'] = df.index.hour

# display a random sample of 5 rows
#print(df.sample(5, random_state=0))

# display a single day
#print(df.loc['2020-04-03'])

# display data for a datetime range
#print(df.loc['2020-04-02 05:00':'2020-04-03 22:00'])


# Plot each metric as line chart
fig, ax = plt.subplots(4)

# to locate a specific range
# df.loc['2020-04-02 05:00':'2020-04-03 17:00']['tempC']
ax[0].plot(df['tempC'],linewidth=0.5, label='Temp (C)')
ax[0].set_ylabel('Temp (C)')
ax[0].legend();

ax[1].plot(df['h'],linewidth=0.5, label='Humidity')
ax[1].set_ylabel('Humidity %')
ax[1].legend();

ax[2].plot(df['p'],linewidth=0.5, label='Pressure (pascals)')
ax[2].set_ylabel('Air Pressure (pascals)')
ax[2].axhline(y=102268.9, color='r', linestyle='-', label="High Pressure")
ax[2].axhline(y=100914.4, color='b', linestyle='-', label="Low Pressure")
ax[2].legend();

ax[3].plot(df['LDR'],linewidth=0.5, label='Light Level (LDR)')
ax[3].set_ylabel('LDR Level')
ax[3].legend();


###plt.show()



data_columns = ['p', 'tempC', 'h', 'LDR']

# Resample to hourly / daily frequency, aggregating with mean, max, min

df_hourly_mean = df[data_columns].resample('H').mean()
df_daily_mean = df[data_columns].resample('D').mean()

df_hourly_max = df[data_columns].resample('H').max()
df_daily_max = df[data_columns].resample('D').max()

df_hourly_min = df[data_columns].resample('H').min()
df_daily_min = df[data_columns].resample('D').min()

print(df_hourly_mean.head(15))

# compare rowcounts to see difference in data point counts
#print((df.shape[0]))
#print((df_hourly_mean.shape[0]))

# chart hourly/daily avg, min, max for temperature and pressure
fig, ax = plt.subplots(3)

ax[0].plot(df_hourly_mean['tempC'],marker='.', linestyle='-', linewidth=0.5, label='Avg Temp (C) (hourly)')
ax[0].plot(df_daily_mean['tempC'],marker='.', linestyle='-', linewidth=0.5, label='Avg Temp (C) (daily)')

ax[0].plot(df_hourly_min['tempC'],marker='.', linestyle='-', linewidth=0.5, label='Min Temp (C) (hourly)')
ax[0].plot(df_daily_min['tempC'],marker='.', linestyle='-', linewidth=0.5, label='Min Temp (C) (daily)')

ax[0].plot(df_hourly_max['tempC'],marker='.', linestyle='-', linewidth=0.5, label='Max Temp (C) (hourly)')
ax[0].plot(df_daily_max['tempC'],marker='.', linestyle='-', linewidth=0.5, label='Max Temp (C) (daily)')

ax[0].set_title('Temperature (Celcuis) Hourly / Daily ')
ax[0].set_ylabel('')
ax[0].legend();

ax[1].plot(df_hourly_mean['p'],marker='.', linestyle='-', linewidth=0.5, label='Avg Pressure (hourly)')
ax[1].plot(df_daily_mean['p'],marker='.', linestyle='-', linewidth=0.5, label='Avg Pressure (daily)')

ax[1].plot(df_hourly_min['p'],marker='.', linestyle='-', linewidth=0.5, label='Min Pressure (hourly)')
ax[1].plot(df_daily_min['p'],marker='.', linestyle='-', linewidth=0.5, label='Min Pressure (daily)')

ax[1].plot(df_hourly_max['p'],marker='.', linestyle='-', linewidth=0.5, label='Max Pressure (hourly)')
ax[1].plot(df_daily_max['p'],marker='.', linestyle='-', linewidth=0.5, label='Max Pressure (daily)')

ax[1].axhline(y=102268.9, color='r', linestyle='-', label="High Pressure")
ax[1].axhline(y=100914.4, color='b', linestyle='-', label="Low Pressure")

ax[1].set_title('Barometric Air Pressure - Hourly / Daily')
ax[1].set_ylabel('')
ax[1].legend();

# hourly air pressure % change
# show pct_change as directional (signed) to indicate pressure rising/falling
df_hourly_mean_pct = df_hourly_mean.pct_change()*np.sign(df_hourly_mean.shift(periods=1))
df_hourly_min_pct = df_hourly_min.pct_change()*np.sign(df_hourly_min.shift(periods=1))
df_hourly_max_pct = df_hourly_max.pct_change()*np.sign(df_hourly_max.shift(periods=1))

ax[2].plot(df_hourly_mean_pct['p'],marker='.', linestyle='-', linewidth=0.5, label='Avg Pressure (hourly % change)')
ax[2].plot(df_hourly_min_pct['p'],marker='.', linestyle='-', linewidth=0.5, label='Min Pressure (hourly % change)')
ax[2].plot(df_hourly_max_pct['p'],marker='.', linestyle='-', linewidth=0.5, label='Max Pressure (hourly % change)')

# filter by date range (48 hour period)
#ax[2].plot(df_hourly_mean_pct.loc['2020-04-03 00:00':'2020-04-05 00:00']['p'],marker='.', linestyle='-', linewidth=0.5, label='Avg Pressure (hourly % change)')
#ax[2].plot(df_hourly_min_pct.loc['2020-04-03 00:00':'2020-04-05 00:00']['p'],marker='.', linestyle='-', linewidth=0.5, label='Min Pressure (hourly % change)')
#ax[2].plot(df_hourly_max_pct.loc['2020-04-03 00:00':'2020-04-05 00:00']['p'],marker='.', linestyle='-', linewidth=0.5, label='Max Pressure (hourly % change)')

ax[2].set_title('Barometric Air Pressure - Hourly % Change ')
ax[2].set_ylabel('')
ax[2].legend();


###plt.show()


# During last interval (48hours) for sample frequency f (3hourly mean) was increase/decrease observed, what was direction, magnitude & rate of change?


# resample last 48 hours, 3 hour intervals
end_dt = datetime.now()
start_dt  = datetime.now() - timedelta(hours=48)

start_dt = start_dt.strftime("%Y-%m-%d %H:%M:%S")
end_dt = end_dt.strftime("%Y-%m-%d %H:%M:%S")


df_temp_3h_mean = df.loc[start_dt:end_dt]['tempC'].resample('3H').mean()
df_p_3h_mean = df.loc[start_dt:end_dt]['p'].resample('3H').mean()

df_temp_3h_mean_pct = df_temp_3h_mean.pct_change()*np.sign(df_temp_3h_mean.shift(periods=1))
df_p_3h_mean_pct = df_p_3h_mean.pct_change()*np.sign(df_p_3h_mean.shift(periods=1))

df_p_3h_mean_pct_2d = df_p_3h_mean_pct.pct_change()*np.sign(df_p_3h_mean_pct.shift(periods=1))

print df_p_3h_mean_pct
print df_p_3h_mean_pct_2d


fig, ax = plt.subplots(4)

### Firstly, show temperature (celcuis) 3 hour mean
ax[0].plot(df_temp_3h_mean,marker='.', linestyle='-', linewidth=0.5, label='Temp (C) (3 hour mean)')
ax[0].set_title('Temperature (Celcuis) 3 hour mean')
ax[0].set_ylabel('')
ax[0].legend();

### Barometric Pressure 3 hour mean
ax[1].plot(df_p_3h_mean,marker='.', linestyle='-', linewidth=0.5, label='Air Pressure (3 hour mean)')
ax[1].set_title('Air Pressure (Pascals) 3 hour mean')
ax[1].set_ylabel('')
ax[1].legend();
ax[1].axhline(y=102268.9, color='r', linestyle='-', label="High Pressure")
ax[1].axhline(y=100914.4, color='b', linestyle='-', label="Low Pressure")

# Barometric Pressure 3 hour mean, % change
ax[2].plot(df_p_3h_mean_pct,marker='.', linestyle='-', linewidth=0.5, label='Pressure (3h mean % change)')
ax[2].set_title('Air Pressure (Pascals) 3 hour mean % change')
ax[2].set_ylabel('')
ax[2].legend();

# Barometric Pressure 3 hour mean, % change, 2nd derivative (rate of change)
ax[3].plot(df_p_3h_mean_pct_2d,marker='.', linestyle='-', linewidth=0.5, label='Pressure (3h mean % change rate)')
ax[3].set_title('Air Pressure (Pascals) 3 hour mean % change rate')
ax[3].set_ylabel('')
ax[3].legend();

plt.tight_layout()

plt.show()


"""

# chart level of light for hours of daylight only
# indicate dawn/dusk (sunrise/senset) based on level change of LDR sensor values


# Display metric chart for a time period
fig, ax = plt.subplots()

ax.plot(df.loc['2020-04-03 12:00':'2020-04-04 12:00', 'p'], linewidth=0.5)
ax.set_ylabel('Barometric Pressure (Pascals)')
ax.set_title('Barometer')
ax.xaxis.set_major_formatter(mdates.DateFormatter('%b %d %H:%M'));

plt.show()

ax.plot(df.loc['2020-04-03 12:00':'2020-04-04 12:00', 'tempC'], linewidth=0.5)
ax.set_ylabel('Temp (C)')
ax.set_title('Temperature')
ax.xaxis.set_major_formatter(mdates.DateFormatter('%b %d %H:%M'));


plt.show()


fig, axes = plt.subplots(3, 1, figsize=(11, 10), sharex=True)

for name, ax in zip(['p', 'tempC', 'h'], axes):
    sns.boxplot(data=df, x='Weekday Name', y=name, ax=ax)
    ax.set_ylabel('Value')
    ax.set_title(name)
"""

"""

print(df.describe())


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

"""
