#!/usr/bin/gnuplot -p

set datafile separator ","
set border 3
set xtics axis nomirror
set ytics border nomirror
#unset xtics

set ylabel "Duration (lower is better)"
set yrange [0:330]
set ydata time
set timefmt "%M:%S"
set format y "%M:%S"
set style data histogram
set style histogram cluster gap 1
set style fill solid border -1
set xtic rotate by -20

set term wxt 0
plot \
	"dacapo-incinerator-off-on.csv"	\
	using 2:xtic(1)	\
	title "J3 with Incinerator"	\
	with histogram linecolor rgb "red",	\
	"dacapo-incinerator-off-on.csv"	\
	using 3:xtic(1)	\
	title "J3 without Incinerator"	\
	with histogram linecolor rgb "blue"

set term svg
plot \
	"dacapo-incinerator-off-on.csv"	\
	using 2:xtic(1)	\
	title "J3 with Incinerator"	\
	with histogram linecolor rgb "red",	\
	"dacapo-incinerator-off-on.csv"	\
	using 3:xtic(1)	\
	title "J3 without Incinerator"	\
	with histogram linecolor rgb "blue"
