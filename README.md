Particle / Adafruit Ultimate GPS Unit Logger

I started writing this in an effort to learn more about precision GPS tracking and have been tweaking and adding features as new use cases have come up.  I have a couple of major changes coming up so I decided to track my progress in git to keep motivated and make it available in the spirit of sharing.

I've attempted to over-document things for personal reference, so hopefully it might save someone some research later.


Current Features:
1) Module is configurable through the Particle Cloud
	- Enable / Disable / Clear module flash logging
	- Enable / Disable / Change Interval of reporting to the Particle Cloud
		- Location and Informational messages are split out, and either/or can be enabled
	- Raw NMEA sentences can be sent via Particle Function for maximum flexibility
2) DGPS settings are enabled by default (SBAS/WAAS).   For some reason they didn't mention this in the Adafruit guide when it can make a pretty big difference.
3) The reported variables are the ones I found to be actually accurate and significant.  If I didn't report something that looks like it should be:
	a) There are some features listed in the datasheet (and even a few in their tutorial) that are not available, as Adafruit had to choose between certain features from the factory.  I spent alot of time finding out the hard way.
			E.g.  The Flash logger has a number of configurable options, but these are hardcoded at the factory. 

Planned Enhancements:
1) Move from interrupt based handling of processing NMEA messages to allow for higher than 9600 baud communication with the GPS module. 
	-  At 9600, you are basically limited to 1HZ updates with RMC and GGA sentences.  Fine for almost any practical application, but I've never been very practical.
	-  Dumping the flash log at 9600 baud is PAINFULLY slow/buggy/inconsistent if you've logged more than an hour or two.
2) Setup Power Management/Sleep Modes of operation for when out of wifi access, battery conservation or periodic check-in
3) Implement TCP server streaming/batch sending of NMEA messages to allow GPSD clients to piggyback or for offline analysis of all collected data
