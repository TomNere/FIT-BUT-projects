<?php 
/*********************CLASS DEFS***************************/

/**
* This class represents instruction
*/
/*
class Instruction {
    public $name;
    public $params;
}
*/

const ARG_ERR = 10;

/********************ARRAYS FOR INSTRUCTIONS*****************/
/*
$help_arr = array(
    "inst_0" => 0,
    "inst_1" => 1,
    "inst_2" => 2,
    "inst_3" => 3
);
*/

/**
* All instructions 
* KEY - operation code
* VALUE - array of parameters
* 1 - variable
* 2 - label
* 3 - symb
* 4 - type
* 5 -
*/
$all_inst = array(
    "createframe" => array (), 
    "pushframe" => array (), 
    "popframe" => array (), 
    "return" => array (), 
    "break" => array (),

    "defvar" => array (1), 
    "call" => array (2), 
    "pushs" => array (3), 
    "pops" => array (1), 
    "write" => array (3), 
    "label" => array (2), 
    "jump" => array (2), 
    "dprint" => array (3),

    "move" => array (1, 3), 
    "int2char" => array (1, 3), 
    "read" => array (1, 4), 
    "strlen" => array (1, 3), 
    "type" => array (1, 3),

    "add" => array(1, 3, 3), 
    "sub" => array(1, 3, 3), 
    "mul" => array(1, 3, 3), 
    "idiv" => array(1, 3, 3), 
    "lt" => array(1, 3, 3), 
    "gt" => array(1, 3, 3), 
    "eq" => array(1, 3, 3), 
    "and" => array(1, 3, 3), 
    "or" => array(1, 3, 3), 
    "not" => array(1, 3, 3), 
    "stri2int" => array(1, 3, 3), 
    "concat" => array(1, 3, 3), 
    "getchar" => array(1, 3, 3), 
    "setchar" => array(1, 3, 3), 
    "jumpifeq" => array(2, 3, 3), 
    "jumpifneq" => array(2, 3, 3)
);

/*
// Zero parameter instructions
$inst_0 = array("createframe", "pushframe", "popframe", "return", "break");

// One parameter instructions
$inst_1 = array("defvar", "call", "pushs", "pops", "write", "label", "jump", "dprint");

// Two parameter instructions
$inst_2 = array("move", "int2char", "read", "strlen", "type");

// Three parameter instructions
$inst_3 = array("add", "sub", "mul", "idiv", "lt", "gt", "eq", "and", "or", "not", "stri2int", "concat", "getchar", "setchar", "jumpifeq", "jumpifneq");
*/

/*********************FUNCTION DEFS************************/

// Skip comments
function skipComment() {

}

// Skip white characters
function skipWhite() {
    $char;
    while (true) {
        $char = fgetc(STDIN);
        if ($char == '#') {
            skipComment();
            return '#';
        }
        else if ($char == EOF) {
            return EOF;
        }
        else if (ctype_space($char)) {
            continue;
        }
        else {
            return $char;
        }
    }
}

function getInst() {
    //global $help_arr, $inst_0, $inst_1, $inst_2, $inst_3;
    global $all_inst;

    //$inst = new Instruction();
    $op_code = strtolower(stream_get_line(STDIN, 20, " "));

    /*
    foreach ($help_arr as $arr_name => $number) {
        foreach ($$arr_name as $value) {
            if ($value == $inst->name) {
                $inst->params = $number;
                return $inst;
            }
        }
    }
    */

    foreach ($all_inst as $key => $value) {
        if (strcmp($key, $op_code) == 0) {
            return $op_code;
        }
    }

    return "unknown";
}

function main() {

    // Arguments check
    global $argc;
    global $argv;

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
    
    // Repeat until error or EOF
    while ( true ) {
        $var = getInst();

        print $var;
        break;
        //print $var->name;
        //print $var->params;
    }
    

    return 0;
}

return main();

?>