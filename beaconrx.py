import time
from datetime import datetime
from pytz import timezone
import select
import serial
import sqlite3
import requests


def read_data(node):
    data = []
    temp = None
    while temp != '#':
        temp = node.read().decode(encoding='UTF-8')

    while temp != '*':
        temp = node.read().decode(encoding='UTF-8')
        data.append(temp)

    # return data without the trailing (#) stop byte
    return data[:-1]


def split_data(data):
    data_string = ''.join(data)
    data_string = data_string.split(';')
    mydt = datetime.strptime(data_string[0], "%d%m%y,%H%M%S")
    mydt = timezone('UTC').localize(mydt)
    currentime = mydt.astimezone(timezone('Asia/Manila'))	
    data_string[0] = currentime.strftime("%Y-%m-%d %H:%M:%S")
    return data_string


def save_data(data):
    conn = sqlite3.connect('beaconrx.db')
    c = conn.cursor()
    c.execute('''CREATE TABLE IF NOT EXISTS beacon
        (   id		INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
	    datetime    DATETIME NOT NULL,
	    node	INTEGER NOT NULL,
            rssi   	INTEGER NOT NULL,
            lat       	TEXT NOT NULL,
	    nsd         TEXT NOT NULL,	
            long       	TEXT NOT NULL,
	    ewd         TEXT NOT NULL,
            alt         TEXT NOT NULL,
            hdop        TEXT NOT NULL,
	    msg         TEXT 	
        );
        ''')
    c.execute("INSERT INTO beacon VALUES(NULL,?,?,?,?,?,?,?,?,?,?);",data)
    conn.commit()
    c.execute("SELECT * FROM beacon WHERE id = (SELECT MAX(id) from beacon);")
    print(c.fetchall())
    conn.close()


node_1 = serial.Serial('/dev/ttyACM0', 9600)
select.select([],[],[],20.0)
while True:
    	data_returned = read_data(node_1)
    	try:
    		data_returned = split_data(data_returned)
    		save_data(data_returned)
	except Exception as e:
		print(e)

