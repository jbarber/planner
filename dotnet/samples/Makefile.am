local_paths = -L ../libplanner
all_assemblies = -r libplanner.dll -r gtk-sharp.dll -r glib-sharp.dll

EXTRA_DIST = \
	Example01.cs \
	Example10.cs

MCS = mcs

all: example01.exe example10.exe


example01.exe: Example01.cs
	$(MCS) -o example01.exe $(local_paths) $(all_assemblies) Example01.cs

example10.exe: Example10.cs
	$(MCS) -o example10.exe $(local_paths) $(all_assemblies) Example10.cs
