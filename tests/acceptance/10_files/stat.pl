printf "%o" . " %d" x 4, (stat("$ARGV[0]"))[2]&07777, (stat(_))[3..5,7]
