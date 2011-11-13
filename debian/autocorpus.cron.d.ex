#
# Regular cron jobs for the autocorpus package
#
0 4	* * *	root	[ -x /usr/bin/autocorpus_maintenance ] && /usr/bin/autocorpus_maintenance
