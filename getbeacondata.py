nao#get data from database every x minutes
import time
import datetime
from datetime import timedelta
import select
import sqlite3
import requests
import csv

def get_current_time():
	currenttime = datetime.datetime.now()
	return currenttime.strftime("%Y-%m-%d %H:%M-%S")

def get_previous_time():
	previoustime = datetime.datetime.now()-timedelta(minutes=2)
	return previoustime.strftime("%Y-%m-%d %H:%M-%S")

def get_data(starttime,endtime):
	conn = sqlite3.connect('beaconrx.db')
	c = conn.cursor()
	c.execute("select * from beacon where datetime between ? and ?;",[starttime,endtime])
	return c.fetchall()
	conn.close()

def write_data(sqldata):
	data = csv.writer(open("query"+get_current_time()+".csv","wb"))
	for row in sqldata:
		data.writerow(row)

while True:
	result=get_data(get_previous_time(),get_current_time())
	write_data(result)
	time.sleep(120)
