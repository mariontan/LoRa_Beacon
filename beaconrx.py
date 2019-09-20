import time
from datetime import datetime
from pytz import timezone
import select
import serial
import sqlite3
import requests

BEACON_TABLE = 'beacon'
BEACON_TABLE_FIELDS = '''id 
    INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
    node    INTEGER NOT NULL,
    rssi    INTEGER NOT NULL,
    datetime    DATETIME NOT NULL,
    lat     TEXT NOT NULL,
    long    TEXT NOT NULL,
    alt     TEXT NOT NULL,
    hdop    TEXT NOT NULL,
    msg     TEXT,
    fixq    INTEGER NOT NULL,
    fix     INTEGER NOT NULL'''

BEACON_TABLE_DEFAULT_VALUES = "NULL,?,?,?,?,?,?,?,?,?,?"
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

def forced_reformat_date(date):
    return "20" + date[4:6] + "-" + date[2:4] + "-" + date[0:2] +  " " + date[7:9] + ":" + date[9:11] + ":" + date[11:13]

def sanitize_data(data_string):
    #TODO: Catch invalid dates
    orig_date = data_string[BEACON_TABLE_DATE]
    try:
        data_string[BEACON_TABLE_DATE] = reformat_date(orig_date,'UTC', 'Asia/Manila', "%d%m%y,%H%M%S", "%Y-%m-%d %H:%M:%S")
    except:
        try:
            data_string[BEACON_TABLE_DATE] = forced_reformat_date(orig_date)
            print("Forced to reformat date: %s" % orig_date)
        except:
            data_string[BEACON_TABLE_DATE] = "00-00-00 00:00:00"
            print("Failed to reformat date: %s" % orig_date)

    return data_string

def split_data(data):
    raw_data = ''.join(data)
    print("Splitting: %s" % raw_data)
    data_string = raw_data.split(';')
    return sanitize_data(data_string)

def create_table_if_not_exists(cursor, table, fields):
    create_table_transaction = 'CREATE TABLE IF NOT EXISTS %s ( %s );' % (table, fields)
    cursor.execute(create_table_transaction)

def insert_into_table(cursor, db_connection, table, values, data):
    insert_transaction = "INSERT INTO %s VALUES(%s);" % (table, values);
    cursor.execute(insert_transaction,data)
    db_connection.commit()

def select_last_entry_from_table(cursor, table):
    select_transaction = "SELECT * FROM %s WHERE id = (SELECT MAX(id) from %s);" % (table, table)
    cursor.execute(select_transaction)
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

