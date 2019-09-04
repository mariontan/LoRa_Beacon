#!/bin/bash
#start in ~/
password="pi"

#dtnoutbox outboxSender outboxFolder/ dtn://raspidtn22/inboxReceiver &	
dtnoutbox sharebox outboxFolder dtn://raspidtn23/sharebox &
#dtninbox sharebox outboxFolder & #to receive data from android phones
#dtninbox inboxReceiver inboxFolder/ &
dtninbox sharebox inboxFolder/ &
#dtntrigger inboxReceiver python outbox_file_cmp.py &
dtntrigger sharebox python outbox_file_cmp.py &
#dtntrigger sharebox python kiosk_loader.py &
echo $password | sudo -S dtnd -c ibrdtnd2.conf


