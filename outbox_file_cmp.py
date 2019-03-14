import glob
import os
import shutil
import logging
import filecmp
import time

report_filename = 'report_outbox_%s.txt' % time.strftime('%Y_%m_%d_%H_%M_%S')
logging.basicConfig(
				filename=report_filename,
				level=logging.INFO,
				format='%(levelname)s:%(message)s')

logging.info('sample header\n')
logging.error('sample error\n')
logging.info('%d %s\n' % (5, 'sample'))

def file_cmp(source, dest):
	if not os.path.exists(dest):
		logging.info("%s does not exist in %s" % (source, dest))
		return False
	return filecmp.cmp(source, dest)

# path: path where to create directory
# create a directory if it does not already exist
def create_directory(path):
	logging.info("creating directory: %s" % path)
	if os.path.exists(path):
		logging.info("existing directory: %s" % path)
		return
	os.makedirs(path)

# source: source file path
# dest: destiation directory path
# file: name of the file
# move files at source path to dest folder with the name filename
def move_file(source, dest, file):
	# create directory if the destination doesn't exist yet
	create_directory(dest)
	# move to destination directory
	path = "%s/%s" % (dest,file)
	shutil.move(source, path)
	logging.info("Moving file from %s to %s" % (source, path))

# load all files in directory
# loop over all files currently in the folder
# move files to back up directory after processing
# ignore files that cannot be loaded
def load_directory(dir_path, dest_path):
	logging.info('load beacon data from path: %s' % dir_path)
	for file in os.listdir(dir_path):
		# after saving the csv data to db move the file to new folder
		file_path = "%s/%s" % (dir_path, file)
		dest_file_path = "%s/%s" % (dest_path, file)

		# try to load file
		# skip to the next file if it already exists and it's the same file
		if file_cmp(file_path, dest_file_path):
			logging.info("Ignoring %s because it is the same as %s." (file_path, dest_file_path))
			continue

		# move the successfully loaded file
		move_file(file_path, dest_path, file)

# dir_path: directory to load
# loads all file in the directory
def load():
	# get directory to read from
	dir_path = os.path.dirname(os.path.realpath(__file__))

	in_path = "%s/inboxFolder" % dir_path
	out_path = "%s/outboxFolder" % dir_path
	create_directory(in_path)
	create_directory(out_path)

	# read all the files in the directory
	load_directory(in_path,out_path)

# start loading files
load()