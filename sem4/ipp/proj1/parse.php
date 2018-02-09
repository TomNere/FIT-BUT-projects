<?php 

// Arguments check
const ARG_ERR = 10;
$inst_counter = 0;

if ($argc > 1) {
    if ($argv[1] == '--help') {
        print "show passed\n";
    }
    else {
        fwrite(STDERR, "Unknown arguments!\n");
        return 10;
    }
}

// First line check
$first_line = rtrim(strtolower(fgets(STDIN)));
if (strcmp($first_line, ".ippcode18") != 0) {
    fwrite(STDERR, ".IPPcode18 is missing!\n");
    return 21;
}

//string getInst()

?>