import time
from datetime import datetime
from pytz import timezone
import select
import serial
import sqlite3
import requests

BEACON_TABLE = 'beacon'
BEACON_TABLE_FIELDS = '''   id      INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
        node    INTEGER NOT NULL,
            rssi    INTEGER NOT NULL,
        datetime    DATETIME NOT NULL,
            lat         TEXT NOT NULL,
            long        TEXT NOT NULL,
            alt         TEXT NOT NULL,
            hdop        TEXT NOT NULL,
        msg         TEXT    
        '''
BEACON_TABLE_DEFAULT_VALUES = "NULL,?,?,?,?,?,?,?,?"
BEACON_TABLE_NODE = 0;
BEACON_TABLE_RSSI = 1;
BEACON_TABLE_DATE = 2;
BEACON_TABLE_LATITUDE = 3;
BEACON_TABLE_LONGITUDE = 4;
BEACON_TABLE_ALTITUDE = 5
BEACON_TABLE_HDOP = 6;
BEACON_TABLE_MESSAGE = 7;

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

def reformat_date(date, from_timezone, to_timezone, from_format, to_format):
    mydt = datetime.strptime(date, from_format)
    mydt = timezone(from_timezone).localize(mydt)
    currentime = mydt.astimezone(timezone(to_timezone))   
    return currentime.strftime(to_format)

def sanitize_data(data_string):
    data_string[BEACON_TABLE_DATE] = reformat_date(data_string[BEACON_TABLE_DATE],'UTC', 'Asia/Manila', "%d%m%y,%H%M%S", "%Y-%m-%d %H:%M:%S")
    return data_string

def split_data(data):
    data_string = ''.join(data).split(';')
    return sanitize_data(data_string);

def create_table_if_not_exists(cursor, table, fields):
    cursor.execute('CREATE TABLE IF NOT EXISTS %s ( %s );' % (table, fields))

def insert_into_table(cursor, db_connection, table, values, data):
    cursor.execute("INSERT INTO %s VALUES(%s);" % (table, values) ,data)
    db_connection.commit()

def select_last_entry_from_table(cursor, table)
    cursor.execute("SELECT * FROM %s WHERE id = (SELECT MAX(id) from %s);" % (table, table))
    print(cursor.fetchall())

def save_data(data):
    conn = sqlite3.connect('beaconrx.db')
    c = conn.cursor()
    create_table_if_not_exists(c, BEACON_TABLE, BEACON_TABLE_FIELDS)
    insert_into_table(c, conn, BEACON_TABLE, BEACON_TABLE_DEFAULT_VALUES,data)
    select_last_entry_from_table(c, BEACON_TABLE)
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

