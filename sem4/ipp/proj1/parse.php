<?php 

const ARG_ERR = 10;
inst_counter = 0;

if ($argc > 1) {
    if ($argv[1] == '--help') {
        print "show passed\n";
    }
    else {
        fwrite(STDERR, "Neznamy argument.\n");
        return 10;
    }
}
else {
    print "no argument passed\n";
}

?>