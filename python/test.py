import pygtk
pygtk.require('2.0')
import gtk
import planner

#if project.is_empty():
#	print 'The project is empty.'
#else:
#	print 'The project is not empty.'

#
# Test case nr 1
#

t1 = planner.Task()
t1.set_name('T1')
t1.set_property('work', 8*60*60)
project.insert_task(task=t1)

r1 = planner.Resource()
r1.set_name('R1')
project.add_resource(r1)
r1.set_custom_property('cost',125.0)
r1.assign(t1,100)

if t1.get_cost() == 125*8:
	print 'Test1 .... OK'
else:
	print 'Test1 .... FAILED'

#
# Test case nr 2
#


