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

/*
// Skip comments
function skipComment() {
    while (true) {
        fe
    }
}
*/

// Skip white characters
function skipWhite() {
    $char;
    while (true) {
        if (feof(STDIN)) {
            return -1;
        }

        $char = fgetc(STDIN);

        if ($char == '#') {
            fgets(STDIN);
            continue;
        }
        else if ($char == '\n') {
            return '\n';
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
    $op_code = skipWhite();

    if ($op_code == -1) {
        return "eof";
    }

    //global $help_arr, $inst_0, $inst_1, $inst_2, $inst_3;
    global $all_inst;

    //$inst = new Instruction();
    $op_code = $op_code.strtolower(stream_get_line(STDIN, 20, " "));

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

// Read int constant
function getInt() {
    $c = fgetc(STDIN);

    if (is_numeric($c) || $c == '+' || $c == '-') {
        
    }
}

// Constant or variable
function getSymb($end, $str) {
    $str = skipWhite();

    if ($str == 'L' || $str == 'T' || $str == 'G') {
        $tmp = fgetc(STDIN);
        if ($tmp == 'F') {
            $str.=$tmp;
        }
        else {
            return "error";
        }

        if (($tmp = fgetc(STDIN)) != '@') {
            return "error";
        }
        else {
            $str.="@";
            $tmp = getVarLab($end, false);
            return $str.$tmp;         
        }
    }
    
    if (ctype_lower($str)) {
        $tmp;
        while ($tmp = fgetc(STDIN)) {
            if (ctype_lower($tmp)) {
                $str.$tmp;
            }
            else {
                break;
            }
        }
        if (strcmp($str, "int") == 0 && $tmp == '@') == 0) {
            getInt();
        }
        if (strcmp($str, "string") == 0 && $tmp == '@') {
            getString();
        }
        if (strcmp($str, "bool") == 0 && $tmp == '@')
            $str.='@';
            $tmp = "";
            while ($c = fgetc(STDIN)) {
                if (ctype_lower($c)) {
                    $tmp.$c;
                }
                else {
                    break;
                }
            }
            if (strcmp($tmp, "true") == 0 || strcmp($tmp, "false") == 0) {
                return $str.$tmp;
            }
        }
    }

    // @ check
    

}

// First part of variable name
function getFrame() {
    $str = skipWhite();

    // LF, TF or GF
    if ($str == 'L' || $str == 'T' || $str == 'G') {
        $tmp = fgetc(STDIN);
        if ($tmp == 'F') {
            $str.=$tmp;
        }
        else {
            return "error";
        }
    }
    else {
        return "error";
    }

    // @ check
    if (($tmp = fgetc(STDIN)) != '@') {
        return "error";
    }
    return $str."@";
}

// Variable or label
function getVarLab($end, $is_var) {
    
    if ($is_var == true)
        $str = getFrame();
    else 
        $str = "";

    if (strcmp($str, "error") == 0) {
        return "error";
    }

    // Must start with alpha or special
    $tmp = fgetc(STDIN);
    if (ctype_alpha($tmp) || $tmp == '_' || $tmp == '-' || $tmp == '$' || $tmp == '&' || $tmp == '%' || $tmp == '*') {
        $str.=$tmp;
    }
    else {
        return "error";
    }

    while (true) {
        $tmp = fgetc(STDIN);

        if (ctype_alnum($tmp) || $tmp == '_' || $tmp == '-' || $tmp == '$' || $tmp == '&' || $tmp == '%' || $tmp == '*') {
            $str.=$tmp;
            continue;
        }
        if ($c == ' ' || $c == '\t') {
            break;
        }
        if ($tmp == '\n' && $end == true) {
            break;
        }
        else {
            return "error";
        }
    }
    return $str;
}


function getType($end) {
    $str = skipWhite();
    while (true) {
        $c = fgetc(STDIN);

        if (ctype_alpha($c)) {
            $str.=$c;
            continue;    
        }
        if ($c == ' ' || $c == '\t') {
            break;
        }
        if ($c == '\n' && $end == true) {
            break;
        }
        else {
            return "error";
        }
    }
    if (strcmp($str, "int") == 0 || strcmp($str, "string") == 0 || strcmp($str, "bool") == 0) {
        return $str;
    }
    else {
        return "error";
    }
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
    $char = skipWhite();
    $first_line = $char.rtrim(strtolower(fgets(STDIN)));
    if (strcmp($first_line, ".ippcode18") != 0) {
        fwrite(STDERR, ".IPPcode18 is missing!\n");
        return 21;
    }
    /*
    $char = skipWhite();
    if ($char == -1) {
        return 0;
    }
    else if ($char != '\n') {
        print $char;
        fwrite(STDERR, ".IPPcode18 wrong!\n");
        return 21;
    }
    */
    // Repeat until error or EOF
    while (true) {
        $var = getInst();

        if (strcmp($var, "eof")) {
            break;
        }
        $end;
        foreach ({$all_inst[$var]} as $key => $value) {
            if count({$all_inst[$var]} == ($key + 1)) 
                $end = true;
            else
                $end = false;

            switch ($value) {
                case 1:
                    getVarLab($end, true);
                    break;
                case 2:
                    getVarLab($end, false);
                    break;
                case 3:
                    $c = skipWhite();
                    if ()
                    getSymb($end);
                    break;
                case 4:
                    getType($end);
                    break;
                default:
                    // WTF
                    break;
            }
        }

        //print $var;
        //print $var->name;
        //print $var->params;
    }
    

    return 0;
}

return main();

?>