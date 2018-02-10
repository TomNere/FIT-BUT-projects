<?php 
/*********************CLASS DEFS***************************/

/**
* This class represents instruction
*/
class Instruction {
    public $name;
    public $params;
}

/********************ARRAYS FOR INSTRUCTIONS*****************/

$help_arr = array(
    "inst_0" => 0,
    "inst_1" => 1,
    "inst_2" => 2,
    "inst_3" => 3
);

// Zero parameter instructions
$inst_0 = array("createframe", "pushframe", "popframe", "return", "break");

// One parameter instructions
$inst_1 = array("defvar", "call", "pushs", "pops", "write", "label", "jump", "dprint");

// Two parameter instructions
$inst_2 = array("move", "int2char", "read", "strlen", "type");

// Three parameter instructions
$inst_3 = array("add", "sub", "mul", "idiv", "lt", "gt", "eq", "and", "or", "not", "stri2int", "concat", "getchar", "setchar", "jumpifeq", "jumpifneq");

/*********************FUNCTION DEFS************************/

function getInst() {
    global $help_arr, $inst_0, $inst_1, $inst_2, $inst_3;

    $inst = new Instruction();
    $inst->name = strtolower(stream_get_line(STDIN, 20, " "));

    foreach ($help_arr as $arr_name => $number) {
        foreach ($$arr_name as $value) {
            if ($value == $inst->name) {
                $inst->params = $number;
                return $inst;
            }
        }
    }
    $inst->params = -1;
    return $inst;
}

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

$var = getInst();
print $var->name;
print $var->params;

?>