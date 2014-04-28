# searchClient
This is a hack of the BlueGiga code.

## Tools
You will need dbusxx-xml2cpp.  In Ubuntu, you will find this in the libdbus-c++-dev package.

## XML Files
I've dropped them in the xml directory.

## Generating the abstract class
In the xml directory, run:
```

dbusxx-xml2cpp interface.xml --proxy=outfile.h
```
Replace interface.xml with the file you want (e.g. sc.xml).  Name outfile.h with the class name you want.
