#!/usr/bin/gnuplot -p

set datafile separator ","
set border 3
set xtics border nomirror
set ytics border nomirror

set ylabel "Memory usage (lower is better)"
set xdata time
set timefmt "%M:%S"
set format x "%M:%S"
set format y "%.1s %cB"

set term wxt 0
plot \
	"j3-stale-ref.csv"	\
	using 2:3	\
	title "Presence of stale references"	\
	with lines linewidth "2pt" linecolor rgb "red",	\
	"j3-stale-ref-corrected.csv"	\
	using 2:3	\
	title "Stale references corrected"	\
	with lines linewidth "3pt" linetype "dotted" linecolor rgb "blue"

set term svg
plot \
	"j3-stale-ref.csv"	\
	using 2:3	\
	title "Presence of stale references"	\
	with lines linewidth "2pt" linecolor rgb "red",	\
	"j3-stale-ref-corrected.csv"	\
	using 2:3	\
	title "Stale references corrected"	\
	with lines linewidth "3pt" linetype "dotted" linecolor rgb "blue"
