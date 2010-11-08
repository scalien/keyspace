import sys
import time
import sys

num = 1000
print_granularity = 1000

count = 0
first = True
start = 0
gran_start = 0
min = 0
max = 0
avg = 0
sum = 0

def set_print_granularity(p):
	global print_granularity
	print_granularity = p
	print("%s: print granularity = %s" % (sys.argv[0], print_granularity))

def loop_count():
	global count
	global first
	global start
	global sum
	global min, max, avg
	global gran_start
	now = round(time.time() * 1000)
	if not first:
		elapsed = now - start
		if elapsed < min: min = elapsed
		if elapsed > max: max = elapsed
		sum = sum + elapsed
	start = now
	count = count + 1
	if count % print_granularity == 0 and not first:
		gran_elapsed = now - gran_start
		gran_start = now
		avg = sum / print_granularity
		print("%s: last %s run stats in msec \t\t elapsed = %s \t min = %s \t max = %s \t avg = %s" % (sys.argv[0], print_granularity, sum, min, max, avg))
		# sys.stdout.write("-")
		# sys.stdout.flush()
	if first or count % print_granularity == 0:
		gran_start = now
		min = 10e10
		max = -10e10
		avg = 0
		sum = 0
		first = False
