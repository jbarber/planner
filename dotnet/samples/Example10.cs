using System; 
using Planner;
using Gtk;
using GLib;

public class Example10 {
        static int Main() {
                Planner.Application app;
                Project project;
                List  tasks, resources;

		// GtkSharp.Libplanner.ObjectManager.Initialize ();

		Gtk.Application.Init ();

                app = new Planner.Application();
                project = new Project (app);

		//project.Loaded += Project_Loaded;
		
                project.Load ("../../examples/project-x.mrproject");

                tasks = project.AllTasks;
                resources = project.Resources;

		foreach (Task task in project.AllTasks) {
		    Console.WriteLine ("Task name: "+ task.Name);
		}
		
                foreach (Resource resource in project.Resources) {
                  Console.WriteLine ("Resource name: "+ resource.Name);
                }
                
                return 0;
        }
    
        static void Project_Loaded (object o, EventArgs args) {
	        Console.WriteLine ("Project is loaded");
        }
}
