#
# This file is a small demo for the planner/python integration
# Copy it under ~/.gnome2/planner/python/ and restart planner.
# You'll notice a new entry in the file->export menu :)
#

import pygtk
pygtk.require('2.0')
import gtk
import planner

print '***************'

ui_string = """
<ui>
  <menubar name="MenuBar">
    <menu action="File">
      <menu action="Import">
        <placeholder  name="Import placeholder">
          <menuitem action="Python import"/>
        </placeholder>
      </menu>
    </menu>
  </menubar>
</ui>
"""

def import_python(*args):
    action = args[0]
    print 'Activated the action:',action.get_name()
    project = window.get_project()
    t1 = planner.Task()
    t1.set_name('Python demo')
    t1.set_property('work', 8*60*60)
    project.insert_task(task=t1)


python_actions = [
    ('Python import', None, '_Python', None, 'Python demo', import_python),
    ]


group = gtk.ActionGroup('PythonImport')
group.add_actions(python_actions)

ui = window.get_ui_manager()
ui.insert_action_group(group,0)
ui.add_ui_from_string(ui_string)
ui.ensure_update()

print '***************'

