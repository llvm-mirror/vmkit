#!/usr/bin/gnuplot -p

set datafile separator ","
set datafile missing '_'
set datafile commentschars "#"

set ylabel "Memory usage (lower is better)"
set ytics border nomirror
set grid ytics
set yrange [0:*]
set format y "%.1s %cB"

set xtics 15 axis nomirror
set border 3
set xdata time
set format x "%M:%S"
set timefmt "%M:%S"

# set title "Stale reference memory leaks in J3 and Incinerator"

#set term svg
#set term postscript clip 12
#set term latex
set term wxt 0
plot	\
	'j3-stale-ref.csv' using 1:(column(11)*1024) linecolor rgb "grey" linewidth "2pt" title "J3" with lines,	\
	'j3-stale-ref-corrected.csv' using 1:(column(11)*1024) linecolor rgb "orange" linewidth "2pt" title "Incinerator" with lines
