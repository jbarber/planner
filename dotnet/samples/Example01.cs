using System; 
using Planner;

using Gtk;

public class Test {
        static int Main() {
                Task task;

		Gtk.Application.Init ();
		GtkSharp.Libplanner.ObjectManager.Initialize ();

                task = new Task ();
                Console.WriteLine (task.Start);
                Console.WriteLine ("Planner 0.8 here");
                return 0;
        }
}
