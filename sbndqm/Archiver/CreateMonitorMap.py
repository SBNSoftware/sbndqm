import psycopg2

Database = psycopg2.connect(host='icarus-db01.fnal.gov', database='icarus_online_prd', user='runcon_admin', password='xqr98CEm', port=5434)
Cursor = Database.cursor()


##########################################
## DROP And CREATE PMT RMS table
##########################################

#Cursor.execute('DELETE FROM runcon_prd.pmt_channel_rms_median_monitor;')
#Cursor.execute('CREATE TABLE runcon_prd.pmt_channel_rms_median_monitor (CHANNEL_ID INTEGER NOT NULL, SMPL_TIME TIMESTAMP WITH TIME ZONE NOT NULL, SMPL_VALUE DOUBLE PRECISION, PRIMARY KEY (CHANNEL_ID, SMPL_TIME));')
#Database.commit()

##########################################
## DROP And CREATE PMT baseline table
##########################################

#Cursor.execute('DELETE FROM runcon_prd.pmt_channel_baseline_mean_monitor;')
#Cursor.execute('CREATE TABLE runcon_prd.pmt_channel_baseline_mean_monitor (CHANNEL_ID INTEGER NOT NULL, SMPL_TIME TIMESTAMP WITH TIME ZONE NOT NULL, SMPL_VALUE DOUBLE PRECISION, PRIMARY KEY (CHANNEL_ID, SMPL_TIME));')
#Database.commit()


##########################################
## Clean (DELETE) runcon_prd.monitor_map
##########################################

#Cursor.execute('DELETE FROM runcon_prd.monitor_map;')
#Database.commit()

##########################################
## ADD TPC RMS Metrics to monitoring
##########################################

#for i in range(0,55296):
#    Cursor.execute('INSERT INTO runcon_prd.monitor_map (GROUP_NAME, METRIC, CHANNEL_ID, MONITOR_TYPE, POSTGRES_TABLE, TIME_INTERVAL) VALUES (\'tpc_channel\', \'rms\',' + str(i) + ', 0, \'tpc_channel_rms_mean_monitor\', 1200);')
#    Database.commit()

##########################################
## ADD PMT RMS Metrics to monitoring
##########################################

#for i in range(1,360):
#    Cursor.execute('INSERT INTO runcon_prd.monitor_map (GROUP_NAME, METRIC, CHANNEL_ID, MONITOR_TYPE, POSTGRES_TABLE, TIME_INTERVAL) VALUES (\'PMT\', \'rms\',' + str(i) + ', 1, \'pmt_channel_rms_median_monitor\', 3600);')
#    Database.commit()

##########################################
## ADD PMT baseline Metrics to monitoring
##########################################

#for i in range(0,360):
#    Cursor.execute('INSERT INTO runcon_prd.monitor_map (GROUP_NAME, METRIC, CHANNEL_ID, MONITOR_TYPE, POSTGRES_TABLE, TIME_INTERVAL) VALUES (\'PMT\', \'baseline\',' + str(i) + ', 0, \'pmt_channel_baseline_mean_monitor\', 3600);')
#    Database.commit()

##########################################
## Example fetch and print
##########################################

#Cursor.execute('SELECT * FROM monitor_map;')
#StreamConfig = Cursor.fetchall()
#for i in range(25):
#    print(StreamConfig[i])

##########################################
## DROP And CREATE TPC RMS table (OLD)
##########################################

#Cursor.execute('DROP TABLE tpc_channel_rms_mean_monitor')
#Cursor.execute('CREATE TABLE tpc_channel_rms_mean_monitor (CHANNEL_ID INTEGER NOT NULL, SMPL_TIME TIMESTAMP WITH TIME ZONE NOT NULL, SMPL_VALUE DOUBLE PRECISION, PRIMARY KEY (CHANNEL_ID, SMPL_TIME));')

##########################################
## Create Monitor Map (OLD)
##########################################

#Cursor.execute('DROP TABLE monitor_map;')
#Database.commit()

#Cursor.execute('CREATE TABLE monitor_map (GROUP_NAME CHARACTER VARYING(128) NOT NULL, METRIC CHARACTER VARYING(32) NOT NULL, CHANNEL_ID INTEGER NOT NULL, MONITOR_TYPE INTEGER NOT NULL, POSTGRES_TABLE CHARACTER VARYING(128) NOT NULL, TIME_INTERVAL DOUBLE PRECISION NOT NULL, CREATE_TIME TIMESTAMP WITH TIME ZONE DEFAULT NOW(), PRIMARY KEY (GROUP_NAME,METRIC,CHANNEL_ID,MONITOR_TYPE));')
#Database.commit()
