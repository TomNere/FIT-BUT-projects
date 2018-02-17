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
        if ($char == '\n') {
            return '\n';
        }
        if (ctype_space($char)) {
            continue;
        }
        else {
            return $char;
        }
    }
}

// Read Escape sequence
function getEscape($end) {

    $number = fgets(STDIN, 3);
    // 000-999
    if (ctype_digit($number)) {
        if (strcmp($number, "060") == 0)
            return "&lt";
        else if (strcmp($number, "062") == 0)
            return "&gt"
        else if (strcmp($number, "038"))
            return "&amp";
        else 
            return $number;
    }
    else
        return "error";
}

// Read string
function getString($end) {
    $c;
    $str = "";
    while($c = fgetc(STDIN)) {
        if (($c == '\n' && $end) || $c == ' ' || $c == '\t') {
            break;
        }
        if ($c == 92) {
            $tmp = getEscape($end);

            if (strcmp($tmp, "error") != 0) {
                $str.$tmp;
                break;
            }
            else
                return "error";
        }
        // Comment
        if ($c == 35) {
            skipComment();
            break;
        }
        if (ctype_print($c)) {
            $str.=$c;
            continue;
        }
    }
}


// Read int constant
function getInt($end) {
    $c = fgetc(STDIN);
    $number;

    if (is_numeric($c) || $c == '+' || $c == '-') {
        $number = $c;
    }
    else 
        return "error";

    while ($c = fgetc(STDIN)) {
        if (is_numeric($c)) {
            $number.=$c;
        }
        else 
            break;
    }
    if (($c == '\n' && $end) || $c == '\t' || $c == ' ') {
        return $number;
    }
    else 
        return "error";
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
            $tmp = getInt($end);
            if (strcmp($tmp, "error") != 0) {
                $str.="@";
                return $str.$tmp;
            } 
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

function getInst() {
    $op_code = skipWhite();

    while ($op_code == '\n') {
        $op_code = skipWhite();        
    }

    if ($op_code == -1) {
        return -1;
    }

    //global $help_arr, $inst_0, $inst_1, $inst_2, $inst_3;
    global $all_inst;

    //$inst = new Instruction();
    $c;
    while ($c = fgetc(STDIN)) {
        if ($c == ' ' || $c == '\t')
            break;
        if (ctype_alnum($c))
            $op_code.$c;
        else 
            return -2;
    }
    // Case insentive
    $op_code = strtolower($op_code);

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

    return -2;
}

function main() {

    // Arguments check
    global $argc;
    global $argv;
    
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

    // Basic XML string
    $xmlstr = <<<XML
    <?xml version="1.0" encoding="UTF-8"?>
    <program language="IPPcode18">
    </program>
XML;

    // Instruction counter
    $inst_count = 0;

    // Repeat until error or EOF
    while (true) {
        $var = getInst();

        // End of file
        if ($var == -1 {
            return 0;
        }
        // Unknown instruction
        if ($var == -2) {
            return 21;
        }

        // $end signalize last parameter
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