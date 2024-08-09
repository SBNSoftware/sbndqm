import numpy as np
import psycopg2
import pandas as pd
import datetime as dt
import time

def query_channel(channel):
    database = psycopg2.connect(host='icarus-db01.fnal.gov',
                                database='icarus_online_prd',
                                user='mueller',
                                port=5434)
    cursor = database.cursor()

    today = dt.datetime.today()
    lower = (today - dt.timedelta(weeks=208))
    upper = today.strftime('%Y-%m-%d')
    lower = lower.strftime('%Y-%m-%d')

    table = "runcon_prd.tpc_channel_rms_mean_monitor"
    date = "smpl_time > \'" + lower + "\' AND smpl_time < \'" + upper +"\'"
    entries = list()
    chan = "channel_id = " + str(channel)
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
    df = query_channel(43518)
    df.to_csv('ch43518.csv', index=False)
    """
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
    """

if __name__ == '__main__':
    main()
