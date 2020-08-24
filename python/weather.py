#
# Weather Data Analytics & Stats
# Process Arduino Weather Station Data recorded to SD card in json format
# Includes sensor metrics: temperature (Celcius), humidity, light, air pressure
#
# Compute 3hour & daily metric stats: Average, Min, Max, STD Deviation, % Change
#
# Demonstrates some uses of Python - Pandas, Matplotlib
# References: https://www.dataquest.io/blog/tutorial-time-series-analysis-with-pandas/
#


import os
import json
import numpy as np

import matplotlib.pyplot as plt
import matplotlib.dates as mdates
import pandas as pd
from datetime import datetime, timedelta
from datetime import datetime as dt
import seaborn as sns

sns.set(rc={'figure.figsize':(11, 4)})


days_to_extract = 7;
path = "../data/weather/"
files = []

### Data file path and file format ../data/weather/20200823.json
for (path, dirs, files) in os.walk(path):
    files = [ fi for fi in files if fi.endswith(".json") ]

### DataFrame to store Daily Metrics - Average, Mix, Max, STD Deviation, Close
df_stat = pd.DataFrame(columns = ['data', 'date', 'mean' , 'std', 'min' , 'max', 'last'])
frames = []
stats = []

### Load JSON data, compute statistics dataframes
def load_json_data(filepath, frames, df_stat):
    with open(filepath) as f:
        d = json.load(f)
        df = pd.DataFrame(d)
        filename = os.path.basename(filepath)
        a = os.path.splitext(filename)
        date = a[0]
        ### Compute Daily Stats - Mean, Min, Max, STD Deviation (freq: all data points)
        ### Format {"ts":1597968061,"t":21.3,"h":87,"l":995,"p":99826,"t2":23.9,"a":-0.422764,"w":198,"el":-27.24617,"az":1.479747,"lat":50.7192,"lon":1.8808,"sr":"05:50:15","ss":"19:59:38","mn":2},
        row1 = pd.DataFrame({'data': 'pressure','date': date,'mean': df.p.mean() ,'std': df.p.std(),'min': df.p.min(),'max': df.p.max(),'last': df.p.iloc[-1]},index=[0])
        row2 = pd.DataFrame({'data': 't','date': date,'mean': df.t.mean() ,'std': df.t.std(),'min': df.t.min(),'max': df.t.max(),'last': df.t.iloc[-1]},index=[0])
        row3 = pd.DataFrame({'data': 'humidity','date': date,'mean': df.h.mean() ,'std': df.h.std(),'min': df.h.min(),'max': df.h.max(),'last': df.h.iloc[-1]},index=[0])
        row4 = pd.DataFrame({'data': 'light','date': date,'mean': df.l.mean() ,'std': df.l.std(),'min': df.l.min(),'max': df.l.max(),'last': df.l.iloc[-1]},index=[0])

        df_stat = df_stat.append(row1,ignore_index=True,sort=True)
        df_stat = df_stat.append(row2,ignore_index=True,sort=True)
        df_stat = df_stat.append(row3,ignore_index=True,sort=True)
        df_stat = df_stat.append(row4,ignore_index=True,sort=True)

        frames.append(df)
        return df_stat

### process n days datafiles
for f in files:
    filename = path+f
    bits = os.path.splitext(f)
    datestr = bits[0]
    dtm = datetime.strptime(datestr, '%Y%m%d')
    if dtm >= datetime.now()-timedelta(days=days_to_extract):
        print filename
        df_stat = load_json_data(filename,frames,df_stat)


df = pd.concat(frames)


### convert timestamp to python datetime, index & sort
df.sort_values(by='ts', ascending=True)
df['datetime'] = pd.to_datetime(df['ts'],unit='s')
df['dts'] = df['datetime'].dt.strftime('%d/%m/%Y %H:%M:%S')
df = df.set_index('datetime')
df.index

# add temporal column indexes
df['Year'] = df.index.year
df['Month'] = df.index.month
df['Day'] = df.index.day
df['Weekday Name'] = df.index.weekday_name
df['Hour'] = df.index.hour

### Examples of slicing DataFrame

# display a random sample of 5 rows
#print(df.sample(5, random_state=0))

# display a single day
#print(df.loc['2020-04-03'])

# display data for a datetime range
#print(df.loc['2020-04-02 05:00':'2020-04-03 22:00'])

# to locate a specific range
# df.loc['2020-04-02 05:00':'2020-04-03 17:00']['t']


### Begin charting


### Temperature, Humidity, Air Pressure - 7 Day Line Chart

# Create 3 charts
fig, ax = plt.subplots(3)

# resample: last 7 days, 60sec mean interval
end_dt = datetime.now()
start_dt  = datetime.now() - timedelta(days=7)

start_dt = start_dt.strftime("%Y-%m-%d %H:%M:%S")
end_dt = end_dt.strftime("%Y-%m-%d %H:%M:%S")

df_t = df.loc[start_dt:end_dt]['t'].resample('60S').mean()
df_h = df.loc[start_dt:end_dt]['h'].resample('60S').mean()
df_p = df.loc[start_dt:end_dt]['p'].resample('60S').mean()


ax[0].plot(df_t,linewidth=0.5, label='Temp (C)')
ax[0].set_title('Temp (C)')
ax[0].set_ylabel('')
ax[0].legend();

ax[1].plot(df_h,linewidth=0.5, label='Humidity')
ax[1].set_title('Humidity %')
ax[1].set_ylabel('')
ax[1].legend();

ax[2].plot(df_p,linewidth=0.5, label='Pressure (pascals)')
ax[2].set_title('Barometric Air Pressure')
ax[2].set_ylabel('')
ax[2].axhline(y=102268.9, color='r', linestyle='-', label="High Pressure")
ax[2].axhline(y=100914.4, color='b', linestyle='-', label="Low Pressure")
ax[2].legend();

### sensor is currently indoors, do not display light data
###ax[3].plot(df_l,linewidth=0.5, label='Light Level (l)')
###ax[3].set_title('Light Level')
###ax[3].set_ylabel('')
###ax[3].legend();


### Chart Hourly / Daily mean (average), min & max

# Resample to hourly / daily frequency, aggregating with mean, max, min

data_columns = ['p', 't', 'h', 'l']

df_hourly_mean = df[data_columns].resample('H').mean()
df_daily_mean = df[data_columns].resample('D').mean()

df_hourly_max = df[data_columns].resample('H').max()
df_daily_max = df[data_columns].resample('D').max()

df_hourly_min = df[data_columns].resample('H').min()
df_daily_min = df[data_columns].resample('D').min()

# compare rowcounts between base & resampled data frame to see difference in data point counts
#print((df.shape[0]))
#print((df_hourly_mean.shape[0]))

#  Temperature / Humidity chart hourly average and daily avg, min, max
fig, ax = plt.subplots(2)

ax[0].plot(df_hourly_mean['t'],marker='.', linestyle='-', linewidth=0.5, label='Avg Temp (C) (hourly)')
ax[0].plot(df_daily_mean['t'],marker='.', linestyle='-', linewidth=0.5, label='Avg Temp (C) (daily)')
ax[0].plot(df_daily_min['t'],marker='.', linestyle='-', linewidth=0.5, label='Min Temp (C) (daily)')
ax[0].plot(df_daily_max['t'],marker='.', linestyle='-', linewidth=0.5, label='Max Temp (C) (daily)')

ax[0].set_title('Temperature (Celcuis) Hourly / Daily ')
ax[0].set_ylabel('')
ax[0].legend();


ax[1].plot(df_hourly_mean['h'],marker='.', linestyle='-', linewidth=0.5, label='Avg Humidity (hourly)')
ax[1].plot(df_daily_mean['h'],marker='.', linestyle='-', linewidth=0.5, label='Avg Humidity (daily)')
ax[1].plot(df_daily_min['h'],marker='.', linestyle='-', linewidth=0.5, label='Min Humidity (daily)')
ax[1].plot(df_daily_max['h'],marker='.', linestyle='-', linewidth=0.5, label='Max Humidity (daily)')

ax[1].set_title('Humidity Hourly / Daily ')
ax[1].set_ylabel('')
ax[1].legend();


#  Air Pressure - hourly / daily average

fig, ax = plt.subplots(3)

ax[0].plot(df_hourly_mean['p'],marker='.', linestyle='-', linewidth=0.5, label='Avg Pressure (hourly)')
ax[0].plot(df_daily_mean['p'],marker='.', linestyle='-', linewidth=0.5, label='Avg Pressure (daily)')
ax[0].plot(df_daily_min['p'],marker='.', linestyle='-', linewidth=0.5, label='Min Pressure (daily)')
ax[0].plot(df_daily_max['p'],marker='.', linestyle='-', linewidth=0.5, label='Max Pressure (daily)')

ax[0].axhline(y=102268.9, color='r', linestyle='-', label="High Pressure")
ax[0].axhline(y=100914.4, color='b', linestyle='-', label="Low Pressure")
ax[0].set_title('Barometric Air Pressure - Hourly / Daily')
ax[0].set_ylabel('')
ax[0].legend(loc='upper right')
ax[0].xaxis.set_major_formatter(mdates.DateFormatter('%b %d %H:%M'))

#  Air Pressure - hourly air pressure % change

# show pct_change as directional (signed) to indicate pressure rising/falling
df_hourly_mean_pct = df_hourly_mean.pct_change()*np.sign(df_hourly_mean.shift(periods=1))
df_hourly_min_pct = df_hourly_min.pct_change()*np.sign(df_hourly_min.shift(periods=1))
df_hourly_max_pct = df_hourly_max.pct_change()*np.sign(df_hourly_max.shift(periods=1))

ax[1].plot(df_hourly_mean_pct['p'],marker='.', linestyle='-', linewidth=0.5, label='Avg Pressure (hourly % change)')
ax[1].set_title('Barometric Air Pressure - Hourly % Change ')
ax[1].set_ylabel('')
ax[1].legend(loc='upper right')
ax[1].xaxis.set_major_formatter(mdates.DateFormatter('%b %d %H:%M'))



### 3 Hour Average - Temperature, Humidity, Air Pressure

# resample last 48 hours, 3 hour intervals
end_dt = datetime.now()
start_dt  = datetime.now() - timedelta(hours=160)

start_dt = start_dt.strftime("%Y-%m-%d %H:%M:%S")
end_dt = end_dt.strftime("%Y-%m-%d %H:%M:%S")

df_temp_3h_mean = df.loc[start_dt:end_dt]['t'].resample('3H').mean()
df_h_3h_mean = df.loc[start_dt:end_dt]['h'].resample('3H').mean()

# Air Pressure - Magnitude & Rate of change

df_p_3h_mean = df.loc[start_dt:end_dt]['p'].resample('3H').mean()

# 1st derivative, magnitude (quantity) of change
df_p_3h_mean_pct = df_p_3h_mean.pct_change()*np.sign(df_p_3h_mean.shift(periods=1))
# 2nd derivative, rate of change
df_p_3h_mean_pct_2d = df_p_3h_mean_pct.pct_change()*np.sign(df_p_3h_mean.shift(periods=1))

### Compute sequences of consequetive increase (rising) or decrease (falling) air pressure

df_p_3h_mean_pct_f = df_p_3h_mean_pct.to_frame()

## 3h mean with diff()

df_p_3h_mean_f = df_p_3h_mean.to_frame()

df_p_3h_mean_f['diff'] = df_p_3h_mean_f.diff(axis = 0, periods = 1)

df_p_3h_mean_f['gt0'] = df_p_3h_mean_f['diff'].gt(0)
df_p_3h_mean_f['lt0'] = df_p_3h_mean_f['diff'].lt(0)

df_p_3h_mean_f['monotonic_rise']=df_p_3h_mean_f['gt0'].groupby(df_p_3h_mean_f['gt0'].ne(df_p_3h_mean_f['gt0'].shift()).cumsum()).cumcount().add(1).where(df_p_3h_mean_f['gt0'],0)
df_p_3h_mean_f['monotonic_fall']=df_p_3h_mean_f['lt0'].groupby(df_p_3h_mean_f['lt0'].ne(df_p_3h_mean_f['lt0'].shift()).cumsum()).cumcount().add(1).where(df_p_3h_mean_f['lt0'],0)

print(df_p_3h_mean_f)


### plot rise/fall sequences as bar chart
width = 0.10
ax[2].bar(df_p_3h_mean_pct_f.index,df_p_3h_mean_f['diff'],width,label='Diff +/- (hPa)',color=df_p_3h_mean_f['gt0'].map({True: 'steelblue', False: 'coral'}))
ax[2].legend(loc='upper right')
ax[2].set_ylabel('')
ax[2].set_title('Air Pressure 3h Change')
ax[2].xaxis.set_major_formatter(mdates.DateFormatter('%b %d %H:%M'))



### using 1st derivation % change (signed)

df_p_3h_mean_pct_f = df_p_3h_mean_pct.to_frame()

h = df_p_3h_mean_pct_f['p'].gt(0)

### monotonic (increasing)
##h = np.sign(df_p_3h_mean['p'])
##print h

df_p_3h_mean_pct_f['monotonic']=h.groupby(h.ne(h.shift()).cumsum()).cumcount().add(1).where(h,0)

###print(df_p_3h_mean_pct_f)
###print(df_p_3h_mean_pct_f['monotonic'].max())
###print(df_p_3h_mean_pct_f['monotonic'].sum())


### Monotonic (increasing / decreasing) seq count using group by / join

df_p_3h_mean_pct_f = df_p_3h_mean_pct_f.join( h.groupby(h.ne(h.shift()).cumsum()).cumcount().add(1)
               .to_frame('values')
               .assign(monotic = np.where(h,'monotic_greater_0',
                                          'monotic_not_greater_0'),
                       index = lambda x: x.index)
               .where(df_p_3h_mean_pct_f['p'].notna())
               .pivot_table(columns = 'monotic',
                            index = 'index',
                            values = 'values',
                            fill_value=0) )

### sum() quantifies amount of increase / decrease occuring in period
### max indicates duration (length) of consequetive increase / decrease

###print(df_p_3h_mean_pct_f['monotic_greater_0'].max())
###print(df_p_3h_mean_pct_f['monotic_greater_0'].sum())

###print(df_p_3h_mean_pct_f['monotic_not_greater_0'].max())
###print(df_p_3h_mean_pct_f['monotic_not_greater_0'].sum())

###width = 0.15
###ax[2].bar(df_p_3h_mean_pct_f.index,df_p_3h_mean_pct_f['monotic_greater_0'],width,label='Rising')
###ax[2].bar(df_p_3h_mean_pct_f.index,df_p_3h_mean_pct_f['monotic_not_greater_0'],width,label='Falling')
###ax[2].legend(loc='best')
###ax[2].set_ylabel('')
###ax[2].set_title('Air Pressure')


fig, ax = plt.subplots(3)

### Temperature (celcuis) - 3 hour mean
ax[0].plot(df_temp_3h_mean,marker='.', linestyle='-', linewidth=0.5, label='Temp (C) (3 hour mean)')
ax[0].set_title('Temperature (Celcuis) 3 hour mean')
ax[0].set_ylabel('')
ax[0].legend();
ax[0].xaxis.set_major_formatter(mdates.DateFormatter('%b %d %H:%M'))

### Humidity
ax[1].plot(df_h_3h_mean,marker='.', linestyle='-', linewidth=0.5, label='Humidity (3 hour mean)')
ax[1].set_title('Humidity 3 hour mean')
ax[1].set_ylabel('')
ax[1].legend();
ax[1].xaxis.set_major_formatter(mdates.DateFormatter('%b %d %H:%M'))


### Barometric Pressure
ax[2].plot(df_p_3h_mean,marker='.', linestyle='-', linewidth=0.5, label='Air Pressure (3 hour mean)')
ax[2].set_title('Air Pressure (Pascals) 3 hour mean')
ax[2].set_ylabel('')
ax[2].legend();
ax[2].axhline(y=102268.9, color='r', linestyle='-', label="High Pressure")
ax[2].axhline(y=100914.4, color='b', linestyle='-', label="Low Pressure")
ax[2].xaxis.set_major_formatter(mdates.DateFormatter('%b %d %H:%M'))

# Barometric Pressure 3 hour mean, % change, rate of change

fig, ax = plt.subplots(3)

ax[0].plot(df_p_3h_mean,marker='.', linestyle='-', linewidth=0.5, label='Air Pressure (3h mean)')
ax[0].set_title('Air Pressure (3h mean)')
ax[0].set_ylabel('')
ax[0].legend();
ax[0].axhline(y=102268.9, color='r', linestyle='-', label="High Pressure")
ax[0].axhline(y=100914.4, color='b', linestyle='-', label="Low Pressure")
ax[0].xaxis.set_major_formatter(mdates.DateFormatter('%b %d %H:%M'))


# 1st derivative, % change
ax[1].plot(df_p_3h_mean_pct,marker='.', linestyle='-', linewidth=0.5, label='% change')
ax[1].set_title('Air Pressure - % change')
ax[1].set_ylabel('')
ax[1].legend();
ax[1].xaxis.set_major_formatter(mdates.DateFormatter('%b %d %H:%M'))


# 2nd derivative, rate of change
ax[2].plot(df_p_3h_mean_pct_2d,marker='.', linestyle='-', linewidth=0.5, label='% change rate)')
ax[2].set_title('Air Pressure - % rate of change')
ax[2].set_ylabel('')
ax[2].legend();
ax[2].xaxis.set_major_formatter(mdates.DateFormatter('%b %d %H:%M'))


### display charts
plt.show()
