<?php 
/*********************FUNCTION DEFS************************/

function getInst() {
    return strtolower(stream_get_line(STDIN, 20, " "));
}

/********************ARRAYS FOR INSTRUCTIONS*****************/

// Zero parameter instructions
$inst_0 = array("createframe", "pushframe", "popframe", "return", "break");

// One parameter instructions
$inst_1 = array("defvar", "call", "pushs", "pops", "write", "label", "jump", "dprint");

// Two parameter instructions
$inst_2 = array()

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

print getInst();
print "\n";
print fgets(STDIN);


?>