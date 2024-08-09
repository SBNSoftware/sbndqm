import numpy as np
import psycopg2
import pandas as pd
import datetime as dt
import time

def query_channel(channels, weeks=4):
    database = psycopg2.connect(host='icarus-db01.fnal.gov',
                                database='icarus_online_prd',
                                user='mueller',
                                port=5434)
    cursor = database.cursor()

    today = dt.datetime.today()
    lower = (today - dt.timedelta(weeks=weeks))
    upper = (lower + dt.timedelta(weeks=4)).strftime('%Y-%m-%d')
    lower = lower.strftime('%Y-%m-%d')

    table = "runcon_prd.tpc_channel_rms_mean_monitor"
    date = "smpl_time > \'" + lower + "\' AND smpl_time < \'" + upper +"\'"
    entries = list()
    for c in channels:
        chan = "channel_id = " + str(c)
        comm = "SELECT * FROM " + table + " WHERE " + date + " AND " + chan + ";"
        cursor.execute(comm)
        entries.extend(cursor.fetchall())
    cursor.close()
    entries = list(zip(*entries))
    data = {'channel': entries[0],
            'time': [int(time.mktime(d.timetuple())) for d in entries[1]],
            'rms': entries[2]}
    return pd.DataFrame(data)


def main():
    unstable_channels = [144,432,720,976,1296,1584,3312,3888,4464,4752,
                         5040,5328,5904,6192,7056,7920,9936,10224,10512,
                         10800,11088,11376,11664,13104,13392,13648,13968,
                         14256,16848,17712,18000,18288,18864,19152,20016,
                         20592,21168,21456,21712,21744,23152,24048,24624,
                         27504,27792,28080,28368,28624,28912,28944,29200,
                         329232,31248,31536,32112,32976,33264,34704,34992,
                         35280,36720,38448,38736,41040,41616,41904,43056,
                         44208,44496,44784,45360,45616,46224,46512,46800,
                         47664,47952,48816,49104,49360,49712,50256,51696,
                         54000,55152]
    stable_channels = [16,304,592,848,1168,1456,3184,3760,4336,4624,4912,
                       5200,5776,6064,6928,7792,9808,10096,10384,10672,
                       10960,11248,11536,12976,13264,13520,13840,14128,
                       16720,17584,17872,18160,18736,19024,19888,20464,
                       21040,21328,21584,21616,23024,23920,24496,27376,
                       27664,27952,28240,28496,28784,28816,29072,29104,
                       31120,31408,31984,32848,33136,34576,34864,35152,
                       36592,38320,38608,40912,41488,41776,42928,44080,
                       44368,44656,45232,45488,46096,46384,46672,47536,
                       47824,48688,48976,49232,49584,50128,51568,53872,
                       55024]
    for w in range(4, 104, 4):
        print('Querying month: ' + str(int(w/4)) + '.')
        df = query_channel(unstable_channels, weeks=w)
        print(len(df))
        if w != 4:
            pd.concat([pd.read_csv('unstable.csv'), df],
                      ignore_index=True).to_csv('unstable.csv', index=False)
        else: df.to_csv('unstable.csv', index=False)
        df = query_channel(stable_channels, weeks=w)
        print(len(df))
        if w != 4:
            pd.concat([pd.read_csv('stable.csv'), df],
                      ignore_index=True).to_csv('stable.csv', index=False)
        else: df.to_csv('stable.csv', index=False)

if __name__ == '__main__':
    main()
