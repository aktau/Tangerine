===============================================================
 ``Tangerine`` -- next generation GUI for the ``Thera`` project
===============================================================

	Provide the ``Thera`` project with a GUI and other tools that enable
	of faster recognition and confirmation of fresco fragment matches

You can get ``Tangerine`` via ``git`` by saying::

    git clone git://github.com/Aktau/Tangerine.git

Setting up
==========

Requirements::

- a relatively recent checkout of the full Thera project (libs used: theratypes, theracore, theragui)
- Qt 4.6+ (QWeakRef<QObject> is used, which was only introduced in Qt 4.6, there are possibly other classes in use that are quite recent) 

Place the project in a subfolder of the ``/src`` directory, as such:

	thera/src/Tangerine/... 

Note that this project is developed as an ``Eclipse`` project with the ``MingW-GCC`` compiler

Have fun!