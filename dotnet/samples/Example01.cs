using System; 
using Planner;

using Gtk;

public class Test {
        static int Main() {
		Planner.Application app;
                Task task;

		Gtk.Application.Init ();
		GtkSharp.Libplanner.ObjectManager.Initialize ();

		app = new Planner.Application ();

                task = new Task ();
                Console.WriteLine (task.Start);
                Console.WriteLine ("Planner here");
                return 0;
        }
}
