#!/usr/bin/env python

import pygtk
pygtk.require("2.0")
import mrproject

app = mrproject.Application()

project = mrproject.Project(app)
task = mrproject.Task()

task.set_property('name','test task')

root = project.get_root_task()

# FIXME: Need to make this handle None as parent
project.insert_task (root, 0, task)

#project.set_property('project_start', 200000)

task2 = mrproject.Task()
project.insert_task (task, 0, task2)

resource = mrproject.Resource()
resource.set_property('name', 'test resource')

resource.assign(task, 100)

print 'Assignments:'
for a in task.get_assignments():
    print '  Resource: ', a.get_property('resource').get_property('name'), \
          '\n  Task: ', a.get_property('task').get_property('name'), \
          '\n  Units: ', a.get_property('units')

print

print 'Assigned resources:'
for r in task.get_assigned_resources():
    print '  ', r.get_property('name')
    
print

print 'Predecessors: ', task.get_predecessor_relations()
print 'Successors: ', task.get_successor_relations()

#project.save_as('foo.mrproject', True)


