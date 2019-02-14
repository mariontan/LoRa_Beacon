#get data from database every x minutes
import time
import datetime
from datetime import timedelta
import select
import sqlite3
import csv
import atexit

previousid = 0
conn = sqlite3.connect('/home/pi/beaconrx.db')
c = conn.cursor()

def get_max_id():
	c.execute("select max(id) from beacon;")
	id = c.fetchall()
	return id[0][0]

def get_current_time():
	currenttime = datetime.datetime.now()
	return currenttime.strftime("%Y-%m-%d %H:%M:%S")

def get_data(startid,endid):
	c.execute("select * from beacon where id between ? and ?;",[startid,endid])
	query = c.fetchall()
	return query

#only makes a csv data when sqldata is not empty
def write_data(sqldata):
	if sqldata:
		data = csv.writer(open("/home/pi/outboxFolder/query"+get_current_time()+".csv","wb"))
		for row in sqldata:
			data.writerow(row)

while True:
	result=get_data(previousid,get_max_id())
	write_data(result)
	previousid = get_max_id()+1
	time.sleep(120)

atexit.register(conn.close())

