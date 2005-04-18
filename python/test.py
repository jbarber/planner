import planner

app = planner.Application()

project = planner.Project(app)

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


