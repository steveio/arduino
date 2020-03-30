import json
import matplotlib.pyplot as plt
import matplotlib.dates as mdates
import numpy as np
import pandas as pd
from datetime import datetime as dt

## sample interval 15seconds

with open('./data/weather.json') as f:
   data = json.load(f)
print data

df = pd.DataFrame(data)

df['datetime'] = pd.to_datetime(df['ts'],unit='s')

df['dts'] = df['datetime'].dt.strftime('%d/%m/%Y %I:%M:%S')

print df.info()
###print df.dts

fig, ax = plt.subplots(3)

fig.suptitle('Weather Stats')

ax[0].set_title('Pressure (Pascals)')
ax[0].xaxis.set_major_formatter(mdates.DateFormatter('%b %d %I:%M'))
ax[0].plot(df['datetime'], df['p'])
ax[1].set_title('Temp (Celcius)')
ax[1].xaxis.set_major_formatter(mdates.DateFormatter('%b %d %I:%M'))
ax[1].plot(df['datetime'], df['tempC'])
ax[2].set_title('Humidity')
ax[2].xaxis.set_major_formatter(mdates.DateFormatter('%b %d %I:%M'))
ax[2].plot(df['datetime'], df['h'])



plt.show()


"""


#set ticks every week
ax.xaxis.set_major_locator(mdates.WeekdayLocator())
#set major ticks format
ax.xaxis.set_major_formatter(mdates.DateFormatter('%b %d'))


plt.show()


fig, ax = plt.subplots(figsize=(15,7))

plt.plot(df.datetime, df.tempC)

dt_fmt = mdates.DateFormatter('%d/%m/%Y %I:%M:%S')

plt.xaxis.set_major_formatter(mdates.DateFormatter('%b %d'))

plt.title('')
plt.xlabel('Time')
plt.ylabel('Value')
plt.grid(axis='y', alpha=0.75)
plt.legend()
plt.show()


df_split = np.array_split(df, 8)

###print(df_split[0]);

df_stat = pd.DataFrame(columns = ['mean' , 'std', 'min' , 'max', 'last'])

print(df_stat)

for d in df_split:
    #row = pd.Series([d.pascals.mean() , d.pascals.std(), d.pascals.min(), d.pascals.max()])
    row = pd.DataFrame({'mean': d.pascals.mean() ,'std': d.pascals.std(),'min': d.pascals.min(),'max': d.pascals.max(),'last': d.pascals.iloc[-1]},index=[0])
    df_stat = df_stat.append(row,ignore_index=True,sort=True)

df_pct = df_stat.pct_change();

print(df_pct)

plt.plot( 'last', data=df_pct)
plt.plot( 'mean', data=df_pct)
###plt.plot( 'std', data=df_pct)
plt.plot( 'min', data=df_pct)
plt.plot( 'max', data=df_pct)

plt.title('')
plt.xlabel('Period')
plt.ylabel('% Change')
plt.grid(axis='y', alpha=0.75)
plt.legend()
plt.show()


x = pd.Series(df.index)
y = pd.Series(df.pascals)

## compute linear regression
m, b = np.polyfit(x, y, 1)

print(m)
print(b)


plt.plot(x,y)
plt.plot(x, m*x + b)
plt.ylabel('Pascals (Pa)')
plt.savefig('bmp180_700pts.png')

plt.show()


    s = pd.Series(d.pascals)
    c = s.pct_change()

c.plot.hist(grid=True, bins=20, rwidth=0.9)
plt.title('Pressure % Change Histogram')
plt.xlabel('Pascals')
plt.ylabel('Count')
plt.grid(axis='y', alpha=0.75)
plt.show()

s = pd.Series(df.pascals)

print(s.pct_change())

###print(df.diff(periods=3))

###df.insert(0, 'index', range(0, len(df)))

"""
