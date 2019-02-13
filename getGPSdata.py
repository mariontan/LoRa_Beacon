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

def get_data(startid,endid,nde):
	c.execute("select lat,long from beacon where datetime between ? and ? and node = ?;",[startid,endid,nde])
	#c.execute("select * from beacon;")
	query = c.fetchall()
	return query

#only makes a csv data when sqldata is not empty
def write_data(sqldata,name):
	if sqldata:
		data = csv.writer(open("/home/pi/scenario"+name+".csv","wb"))
		for row in sqldata:
			latdeg = int(float(row[0])/100)
			latmin = (float(row[0])-latdeg*100)
			longdeg = int(float(row[1])/100)
			longmin = (float(row[1])-longdeg*100)
			coordinates = str(latdeg) +" " + str(latmin)+"," + str(longdeg) + " " + str(longmin)
			lst = list(row)
			lst[0] = str(latdeg) +" " + str(latmin)
			lst[1] =  str(longdeg) + " " + str(longmin)
			data.writerow(lst)

result=get_data("2018-11-17 14:40:00","2018-11-17 16:05:00",6)
write_data(result,"1_beacon6")
conn.close()


