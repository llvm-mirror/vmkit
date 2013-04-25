#!/usr/bin/gnuplot -p

set datafile separator ","
set datafile missing '-'
set datafile commentschars "#"

set ylabel "Duration (lower is better)"
set ytics border nomirror
set grid ytics
set yrange [0:*]
set ydata time
set timefmt "%S"
set format y "%M:%S"

set xtics axis nomirror rotate by -20
set border 3

set style data histogram
set style histogram errorbars gap 1 lw 1
set bars fullwidth
set style fill solid border 0
set boxwidth 1

set title "Incinerator overhead in Dacapo benchmarks"

set term wxt 0
plot	\
	'stats-j3-dacapo-2006-10-MR2.csv' using (column(4)/1000.0):(column(7)/1000.0):xtic(2) linecolor rgb "blue" title "J3",	\
	'stats-incinerator-dacapo-2006-10-MR2.csv' using (column(4)/1000.0):(column(7)/1000.0):xtic(2) linecolor rgb "red" title "Incinerator"

#set term latex
#set term postscript clip 12
#plot	\
#	'stats-j3-dacapo-2006-10-MR2.csv' using (column(4)/1000.0):(column(7)/1000.0):xtic(2) linecolor rgb "blue" title "J3",	\
#	'stats-incinerator-dacapo-2006-10-MR2.csv' using (column(4)/1000.0):(column(7)/1000.0):xtic(2) linecolor rgb "red" title "Incinerator"
