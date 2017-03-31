# import matplotlib.pyplot as plt

############################## Calculations of arm dataset #############################

def armCalculations():

	print "Arm Calculations"
	f_arm = open('dataset.txt','r')

	arm_array = []
	for i in xrange(1005):
		try:
			node = f_arm.readline().split()
			if i > 5:
				arm_array.append(int(node[1]))
		except:
			break

	lg = len(arm_array)
	time = []
	for j in xrange(int(lg/100)-1):

		arm_array_cropped = arm_array[100*j:100*(j+1)]
		# plt.plot(arm_array_cropped)
		# plt.show()
		l = len(arm_array_cropped)
		arm_array_sorted = sorted(arm_array_cropped)
		mid_point = (arm_array_sorted[0] + arm_array_sorted[l-1])/2

		for i in xrange(l):
			if arm_array_sorted[i] > mid_point:
				nearest_middle_value = arm_array_sorted[i]
				index = arm_array_cropped.index(nearest_middle_value)
				break
		print
		print "Values for sample "+str(j+1)
		print 
		print "array =",arm_array_sorted,l
		print "max value =",arm_array_sorted[l-1]
		print "min value =",arm_array_sorted[0]
		print "length of array =",l
		print "average value in dataset =",mid_point
		print "nearest value to average =",nearest_middle_value,"index =",index
	 	time.append(0.006*index)
	
	return time
############################## Calculations of fingure dataset #############################

def fingureCalculations(length):
	print "Fingure Calculations"
	f_fingure = open('dataset.txt','r')

	arm_array = []

	for i in xrange(length):
		try:
			node = f_fingure.readline().split()
			if i > (length - 1005):
				arm_array.append(int(node[1]))
		except:
			break
	
	lg = len(arm_array)
	time = []
	for j in xrange(int(lg/100)-1):

		arm_array_cropped = arm_array[100*j:100*(j+1)]
		# plt.plot(arm_array_cropped)
		# plt.show()
		l = len(arm_array_cropped)
		arm_array_sorted = sorted(arm_array_cropped)
		mid_point = (arm_array_sorted[0] + arm_array_sorted[l-1])/2


		for i in xrange(l):
			if arm_array_sorted[i] > mid_point:
				nearest_middle_value = arm_array_sorted[i]
				index = arm_array_cropped.index(nearest_middle_value)
				break
		print
		print "Values for sample "+str(j+1)
		print 
		print "array =",arm_array_sorted,l
		print "max value =",arm_array_sorted[l-1]
		print "min value =",arm_array_sorted[0]
		print "length of array =",l
		print "average value in dataset =",mid_point
		print "nearest value to average =",nearest_middle_value,"index =",index
	 	time.append(0.006*index)
	
	return time

def result():

	val1 = armCalculations()
	
	f = open("dataset.txt",'r')

	counter = 0
	while True:
		counter+=1
		node = f.readline().split()
		if f.readline().split() == []:
			break
	print 
	val2 = fingureCalculations(counter)

	c = 8.2007
	avg_dist = 0.508
	sum = 0
	for i in range(min(len(val1),len(val2))):

		if abs(val1[i]-val2[i]) <= 0.04 and abs(val1[i]-val2[i]) >= 0.02:
			sum = sum + c*(avg_dist/abs(val1[i]-val2[i]))
		else:
			sum = sum + 120
			
	print "Blood Pressure =", sum/max(len(val1),len(val2)) 

result()