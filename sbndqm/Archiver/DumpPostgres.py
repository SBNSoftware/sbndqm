import psycopg2
import datetime as dt
import numpy as np
import pandas as pd
import time

def Plane(Channel):
    PChannel = Channel % 13824
    p = 0
    if PChannel > 2303 and PChannel < 8064: p = 1
    elif PChannel >= 8064: p = 2
    return p + 3*int(Channel / 13824)

def HalfCrate(Channel):
    return int(Channel / 288)

def main():
    Granularity = 24
    Database = psycopg2.connect(host='icarus-db01.fnal.gov', database='icarus_online_prd', user='mueller', port=5434)
    
    Today = dt.datetime.today()
    for day in range(0, 90):
        Lower = Today - dt.timedelta(days=day)
        Upper = Lower + dt.timedelta(days=1)
        LS = Lower.strftime('%Y-%m-%d')
        US = Upper.strftime('%Y-%m-%d')
        Cursor = Database.cursor()
        #Cursor.execute("SELECT * FROM runcon_prd.tpc_channel_rms_mean_monitor WHERE smpl_time > \'2021-03-" + "{:0>2}".format(day) + "\' AND smpl_time < \'2021-04-" + "{:0>2}".format(1) + "\';")
        Cursor.execute("SELECT * FROM runcon_prd.tpc_channel_rms_mean_monitor WHERE smpl_time > \'" + LS + "\' AND smpl_time < \'" + US + "\' AND channel_id > 4288 AND channel_id < 4319;")
        Entries = Cursor.fetchall()
        Cursor.close()
        print(len(Entries))
        Data = {'channel': [], 'date': [], 'rms': [], 'halfcrate': []} # 'plane': []}
        for i in range(len(Entries)):
            Data['channel'].append(Entries[i][0])
            Data['date'].append(Entries[i][1])
            Data['rms'].append(Entries[i][2])
            #Data['plane'].append(Plane(Entries[i][0]))
            Data['halfcrate'].append(HalfCrate(Entries[i][0]))
        dataframe = pd.DataFrame(Data)
        dataframe = FilterZeros(dataframe)
    
        CompressedDataFrame = pd.DataFrame()
        for Date in np.unique([dt.datetime(int(x.strftime('%Y')), int(x.strftime('%m')), int(x.strftime('%d'))) for x in dataframe.date.to_numpy()]):
            Time = time.mktime(Date.timetuple()) + 60*30*(24.0/Granularity)
            DateCut = SelectByDay(Date, dataframe)
            for Hour in range(Granularity):
                print('Working on hour: ' + str(Hour) + '...')
                BinCut = SelectByHour(Hour, DateCut, Difference=(24.0/Granularity))
                #TempDF = BinCut.groupby('channel').mean().reset_index()
                #TempDF = BinCut.groupby('plane').mean().reset_index()
                TempDF = BinCut.groupby('halfcrate').mean().reset_index()
                TempDF['date'] = Time + Hour*(60 * (24.0/Granularity))*60
                CompressedDataFrame = CompressedDataFrame.append(TempDF)

        CompressedDataFrame.to_csv('Run0_' + str(day) + '.csv', index=False)
    #dataframe.to_csv('dump.csv')

def SelectByDay(Date, Data):
    print(Date)
    AsArray = Data.date.to_numpy()
    Cut = Data.loc[[(x.strftime('%d') == Date.strftime('%d') and x.strftime('%m') == Date.strftime('%m') and x.strftime('%Y') == Date.strftime('%Y')) for x in AsArray]]
    return Cut


def SelectByHour(Hour, Data, Difference=1):
    LowHour = Hour*Difference
    HighHour = (Hour+1)*Difference
    AsArray = Data.date.to_numpy()
    Cut = Data.loc[[(int(x.strftime('%-H')) + float(x.strftime('%M'))/60 < HighHour and int(x.strftime('%-H')) + float(x.strftime('%M'))/60 >= LowHour) for x in AsArray]]
    return Cut

def FilterZeros(Data):
    Cut = Data.loc[Data.rms != 0]
    return Cut

if __name__ == '__main__':
    main()
