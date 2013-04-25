#!/usr/bin/gnuplot -p

set datafile separator ","
set datafile missing '_'
set datafile commentschars "#"

set ylabel "Average relative overhead (lower is better)"
set ytics 1 border nomirror
set grid ytics
set format y "%.0f %%"

set style data histogram
set style histogram clustered gap 1
set bars fullwidth
set style fill solid border 0
set boxwidth 1

# set title "Incinerator overhead in Dacapo benchmarks"

set xtics axis nomirror rotate by 20 offset -2,graph -0.1
#set xtics axis nomirror rotate by 20 offset -2,graph -0.03
set border 3

#set term svg
#set term postscript clip 12
#set term latex
set term wxt 1
plot	\
	'dacapo-avg-overhead.csv' using 4:xtic(2) title "Powerful computer" linecolor rgb "orange"
#	'dacapo-tablet-avg-overhead.csv' using 4:xtic(2) title "Resource-constrained computer" linecolor rgb "grey"

#set xtics axis nomirror rotate by 20 offset 0,graph 0
